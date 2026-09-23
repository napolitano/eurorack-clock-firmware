#!/usr/bin/env python3
"""Update the CLOCK ODT contents page from an exported PDF.

The publication ODT uses a deliberately styled static chapter index instead of a
LibreOffice-generated TOC.  This helper resolves the real page of each semantic
level-1 chapter heading in an exported PDF and writes those page numbers back to
the maintained ODT without touching any other content.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import re
import shutil
import subprocess
import zipfile
from pathlib import Path
from lxml import etree

NS = {
    "office": "urn:oasis:names:tc:opendocument:xmlns:office:1.0",
    "text": "urn:oasis:names:tc:opendocument:xmlns:text:1.0",
    "table": "urn:oasis:names:tc:opendocument:xmlns:table:1.0",
}
Q = lambda prefix, local: f"{{{NS[prefix]}}}{local}"


def normalize(text: str) -> str:
    return " ".join(text.split())


def page_count(pdf: Path) -> int:
    result = subprocess.run(["pdfinfo", str(pdf)], text=True, capture_output=True, check=True)
    match = re.search(r"^Pages:\s+(\d+)\s*$", result.stdout, re.MULTILINE)
    if match is None:
        raise RuntimeError("Unable to read PDF page count")
    return int(match.group(1))


def pdf_page_text(pdf: Path, page: int) -> str:
    result = subprocess.run(
        ["pdftotext", "-f", str(page), "-l", str(page), "-layout", str(pdf), "-"],
        text=True,
        capture_output=True,
        check=True,
    )
    return normalize(result.stdout)


def chapter_headings(content_root: etree._Element) -> list[str]:
    headings: list[str] = []
    for heading in content_root.xpath('//text:h[@text:outline-level="1"]', namespaces=NS):
        value = normalize("".join(heading.itertext()))
        if re.match(r"^\d+\s+\S", value):
            headings.append(value)
    if len(headings) != 24:
        raise RuntimeError(f"Expected 24 semantic chapter headings, found {len(headings)}")
    return headings


def resolve_pages(pdf: Path, headings: list[str]) -> list[int]:
    texts = [""] + [pdf_page_text(pdf, p) for p in range(1, page_count(pdf) + 1)]
    pages: list[int] = []
    cursor = 3  # cover + contents are not chapter pages
    for heading in headings:
        found = None
        for page in range(cursor, len(texts)):
            if heading in texts[page]:
                found = page
                break
        if found is None:
            raise RuntimeError(f"Unable to resolve chapter heading in PDF: {heading}")
        pages.append(found)
        cursor = found + 1
    return pages


def update(odt: Path, pdf: Path) -> list[int]:
    with zipfile.ZipFile(odt, "r") as archive:
        entries = [(info, archive.read(info.filename)) for info in archive.infolist()]
    content_info, content = next((i, d) for i, d in entries if i.filename == "content.xml")
    root = etree.fromstring(content)
    headings = chapter_headings(root)
    pages = resolve_pages(pdf, headings)

    tables = root.xpath('//table:table[@table:name="ManualContents"]', namespaces=NS)
    if len(tables) != 1:
        raise RuntimeError(f"Expected one ManualContents table, found {len(tables)}")
    page_paragraphs = tables[0].xpath(
        './/text:p[@text:style-name="ManualContentsPageP"]', namespaces=NS
    )
    if len(page_paragraphs) != 24:
        raise RuntimeError(f"Expected 24 contents page cells, found {len(page_paragraphs)}")

    for paragraph, page in zip(page_paragraphs, pages):
        spans = paragraph.xpath('./text:span[@text:style-name="ManualContentsPageText"]', namespaces=NS)
        if len(spans) != 1:
            raise RuntimeError("Contents page cell does not contain exactly one ManualContentsPageText span")
        spans[0].text = str(page)

    payload = etree.tostring(root, xml_declaration=True, encoding="UTF-8")
    temporary = odt.with_suffix(".toc.tmp.odt")
    with zipfile.ZipFile(temporary, "w") as output:
        for info, data in entries:
            if info.filename == "content.xml":
                data = payload
            compression = zipfile.ZIP_STORED if info.filename == "mimetype" else info.compress_type
            output.writestr(info, data, compress_type=compression)
    shutil.move(temporary, odt)
    return pages


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("odt", type=Path)
    parser.add_argument("pdf", type=Path)
    args = parser.parse_args()
    pages = update(args.odt.resolve(), args.pdf.resolve())
    print("Updated manual contents pages: " + ", ".join(str(p) for p in pages))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
