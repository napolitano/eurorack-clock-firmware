#!/usr/bin/env python3
"""Validate CLOCK ODT/PDF user-manual release artifacts.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import io
import re
import subprocess
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

try:
    from PIL import Image
except ImportError as exc:
    raise SystemExit("Pillow is required: python -m pip install pillow") from exc

PNG_VERSION_KEY = "clock_firmware_version"
ODT_MIMETYPE = b"application/vnd.oasis.opendocument.text"
VERSION_TEXT_RE = re.compile(r"\b[0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z.-]+)?\b")
ALPHA_VERSION_RE = re.compile(r"\b[0-9]+\.[0-9]+\.[0-9]+-alpha\.[0-9]+\b", re.IGNORECASE)


def run_text(command: list[str]) -> str:
    result = subprocess.run(command, text=True, capture_output=True, check=True)
    return result.stdout


def page_images(content_xml: str, archive: zipfile.ZipFile) -> list[str]:
    hrefs = re.findall(r'xlink:href="(Pictures/[^"]+\.png)"', content_xml)
    result: list[str] = []
    for href in hrefs:
        if href in result:
            continue
        with Image.open(io.BytesIO(archive.read(href))) as image:
            if image.size == (1010, 2144):
                result.append(href)
    return result


def validate_odt(path: Path, version: str) -> None:
    if path.name != f"clock-user-manual.{version}.odt" and path.name != "clock-user-manual.odt":
        raise RuntimeError(f"Unexpected manual ODT filename: {path.name}")
    with zipfile.ZipFile(path, "r") as archive:
        if archive.read("mimetype") != ODT_MIMETYPE:
            raise RuntimeError("ODT mimetype is invalid")
        content_xml = archive.read("content.xml").decode("utf-8")
        meta_xml = archive.read("meta.xml").decode("utf-8")
        styles_xml = archive.read("styles.xml").decode("utf-8")
        if "Ubuntu Light" not in styles_xml or "Ubuntu" not in styles_xml:
            raise RuntimeError("Manual style contract must request Ubuntu and Ubuntu Light")
        if version not in content_xml:
            raise RuntimeError(f"Manual body does not identify firmware {version}")
        if version not in meta_xml:
            raise RuntimeError(f"Manual metadata does not identify firmware {version}")
        embedded_versions = {match.group(0).lower() for match in ALPHA_VERSION_RE.finditer(content_xml + meta_xml)}
        if embedded_versions != {version.lower()}:
            raise RuntimeError(
                f"Manual contains stale or mixed firmware versions: {sorted(embedded_versions)!r}"
            )

        ns = {
            "office": "urn:oasis:names:tc:opendocument:xmlns:office:1.0",
            "meta": "urn:oasis:names:tc:opendocument:xmlns:meta:1.0",
        }
        root = ET.fromstring(meta_xml)
        values = [
            element.text or ""
            for element in root.findall(".//meta:user-defined", ns)
            if element.attrib.get(f"{{{ns['meta']}}}name") == "FirmwareVersion"
        ]
        if values != [version]:
            raise RuntimeError(f"Manual FirmwareVersion metadata mismatch: {values!r}")

        full_pages = page_images(content_xml, archive)
        if len(full_pages) != 2:
            raise RuntimeError(f"Expected two full-page cover images, found {len(full_pages)}")
        for image_name in full_pages:
            with Image.open(io.BytesIO(archive.read(image_name))) as image:
                embedded = image.info.get(PNG_VERSION_KEY)
            if embedded != version:
                raise RuntimeError(
                    f"Cover image {image_name} version metadata mismatch: {embedded!r} != {version!r}"
                )


def validate_ubuntu_font_environment() -> None:
    """Require the exact Ubuntu family faces used by the release manual."""
    for style in ("Regular", "Light", "Bold"):
        result = subprocess.run(
            ["fc-match", "-f", "%{family}\n%{style}\n%{file}\n", f"Ubuntu:style={style}"],
            text=True,
            capture_output=True,
            check=True,
        )
        lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
        if len(lines) < 3:
            raise RuntimeError(f"Unable to resolve Ubuntu {style} with fontconfig")
        family, resolved_style, font_file = lines[0], lines[1], lines[2]
        if "Ubuntu" not in family or style.lower() not in resolved_style.lower():
            raise RuntimeError(
                f"Ubuntu {style} required for release PDF; fontconfig resolved "
                f"{family!r} / {resolved_style!r} ({font_file})"
            )


def validate_ubuntu_pdf_fonts(fonts: str) -> None:
    """Validate that the PDF contains an embedded Ubuntu-family Unicode subset.

    Poppler reports PDF BaseFont names, which do not reliably preserve the
    source face/style name.  In particular, LibreOffice may emit an Ubuntu
    Light face as a subset named simply ``AAAAAA+Ubuntu``.  Exact style
    availability is therefore checked with fontconfig before conversion; the
    PDF check only verifies that the Ubuntu family made it into the artifact
    and is embedded.
    """
    ubuntu_rows = [
        line for line in fonts.splitlines()
        if re.search(r"\bUbuntu\b", line, re.IGNORECASE)
    ]
    if not ubuntu_rows:
        raise RuntimeError("Release PDF does not contain Ubuntu-family fonts")
    if not any(re.search(r"\byes\s+yes\s+yes\s+", line, re.IGNORECASE) for line in ubuntu_rows):
        raise RuntimeError(
            "Release PDF contains Ubuntu-family font references but no embedded/subset Unicode face"
        )


def validate_pdf(path: Path, version: str, *, require_ubuntu_fonts: bool) -> None:
    if path.name != f"clock-user-manual.{version}.pdf":
        raise RuntimeError(f"Unexpected manual PDF filename: {path.name}")
    info = run_text(["pdfinfo", str(path)])
    match = re.search(r"^Pages:\s+(\d+)\s*$", info, re.MULTILINE)
    if match is None or int(match.group(1)) < 1:
        raise RuntimeError("PDF page count is missing or zero")
    text = run_text(["pdftotext", str(path), "-"])
    if version not in text:
        raise RuntimeError(f"PDF body does not contain firmware version {version}")
    pdf_versions = {match.group(0).lower() for match in ALPHA_VERSION_RE.finditer(text)}
    if pdf_versions != {version.lower()}:
        raise RuntimeError(f"PDF contains stale or mixed firmware versions: {sorted(pdf_versions)!r}")
    if require_ubuntu_fonts:
        validate_ubuntu_pdf_fonts(run_text(["pdffonts", str(path)]))


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("artifact", type=Path)
    parser.add_argument("--source", type=Path)
    parser.add_argument("--version", required=True)
    parser.add_argument("--require-ubuntu-fonts", action="store_true")
    args = parser.parse_args()

    artifact = args.artifact.resolve()
    if artifact.suffix.lower() == ".odt":
        validate_odt(artifact, args.version)
    elif artifact.suffix.lower() == ".pdf":
        if args.source is None:
            raise SystemExit("--source is required when validating a PDF")
        validate_odt(args.source.resolve(), args.version)
        validate_pdf(artifact, args.version, require_ubuntu_fonts=args.require_ubuntu_fonts)
    else:
        raise SystemExit(f"Unsupported manual artifact: {artifact}")
    print(f"Manual artifact check passed: {artifact.name}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
