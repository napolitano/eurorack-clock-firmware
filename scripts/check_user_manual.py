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
ROOT = Path(__file__).resolve().parents[1]
REPOSITORY_URL = "https://github.com/napolitano/eurorack-clock-firmware"
REPOSITORY_QR = ROOT / "docs" / "manual-source" / "assets" / "repository-qr.png"
UPDATES_QR = ROOT / "docs" / "manual-source" / "assets" / "updates-qr.png"

PRERELEASE_VERSION_RE = re.compile(
    r"\b[0-9]+\.[0-9]+\.[0-9]+-(?:alpha|beta|rc)\.[0-9]+\b",
    re.IGNORECASE,
)



def requires_release_license_section(version: str) -> bool:
    """Return whether this manual version is covered by the beta.14+ release-license contract."""
    match = re.fullmatch(r"(\d+)\.(\d+)\.(\d+)(?:-(alpha|beta|rc)\.(\d+))?", version, re.IGNORECASE)
    if match is None:
        return True
    base = tuple(int(match.group(i)) for i in range(1, 4))
    if base != (0, 19, 0):
        return base > (0, 19, 0)
    stage = (match.group(4) or "stable").lower()
    number = int(match.group(5) or 0)
    if stage == "alpha":
        return False
    if stage == "beta":
        return number >= 14
    return True


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



def requires_layout_v2(path: Path, version: str) -> bool:
    """Return whether the post-1.1.0 publication-layout contract applies."""
    if path.name == "clock-user-manual.odt":
        return True
    base = version.split("-", 1)[0]
    try:
        parts = tuple(int(part) for part in base.split("."))
    except ValueError:
        return False
    return parts > (1, 1, 0)


def validate_layout_v2(content_xml: str, styles_xml: str) -> None:
    """Validate the maintained manual's post-1.1.0 publication-layout contract."""
    ns = {
        "office": "urn:oasis:names:tc:opendocument:xmlns:office:1.0",
        "text": "urn:oasis:names:tc:opendocument:xmlns:text:1.0",
        "table": "urn:oasis:names:tc:opendocument:xmlns:table:1.0",
        "style": "urn:oasis:names:tc:opendocument:xmlns:style:1.0",
        "fo": "urn:oasis:names:tc:opendocument:xmlns:xsl-fo-compatible:1.0",
        "draw": "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0",
    }
    content_root = ET.fromstring(content_xml)
    styles_root = ET.fromstring(styles_xml)

    def flat_text(element: ET.Element) -> str:
        return " ".join("".join(element.itertext()).split())

    def inches(value: str | None) -> float:
        if value is None or not value.endswith("in"):
            raise RuntimeError(f"Expected inch-valued layout property, got {value!r}")
        return float(value[:-2])

    # LibreOffice's outline label alignment must not shift manually numbered
    # semantic text:h headings away from the common left edge.
    alignments = styles_root.findall(
        ".//text:outline-style/text:outline-level-style/style:list-level-properties/style:list-level-label-alignment",
        ns,
    )
    if not alignments:
        raise RuntimeError("Manual outline alignment contract is missing")
    for alignment in alignments:
        if alignment.attrib.get(f"{{{ns['text']}}}label-followed-by") != "nothing":
            raise RuntimeError("Manual chapter headings must use left-aligned outline labels")
        if alignment.attrib.get(f"{{{ns['fo']}}}margin-left") != "0in":
            raise RuntimeError("Manual outline headings must not carry a left margin")
        if alignment.attrib.get(f"{{{ns['fo']}}}text-indent") != "0in":
            raise RuntimeError("Manual outline headings must not carry a text indent")

    body = content_root.find(".//office:body/office:text", ns)
    if body is None:
        raise RuntimeError("Manual body is missing")
    children = list(body)

    # Contents must use the normal framed page master and remain inside the
    # 3.125-inch publication text frame rather than inheriting the full-bleed cover.
    contents_index = next(
        (i for i, element in enumerate(children) if flat_text(element) == "Contents"),
        None,
    )
    chapter_one_index = next(
        (i for i, element in enumerate(children) if flat_text(element) == "1 Start here"),
        None,
    )
    if contents_index is None or chapter_one_index is None or contents_index >= chapter_one_index:
        raise RuntimeError("Manual Contents page must precede chapter 1")
    contents_heading = children[contents_index]
    if contents_heading.attrib.get(f"{{{ns['text']}}}style-name") != "ManualContentsHeading":
        raise RuntimeError("Manual Contents heading must use the framed-page publication style")
    contents_style = content_root.find(
        './/style:style[@style:name="ManualContentsHeading"]', ns
    )
    if contents_style is None or contents_style.attrib.get(f"{{{ns['style']}}}master-page-name") != "Converted1":
        raise RuntimeError("Manual Contents must use the normal framed content-page master")

    toc = next(
        (
            element
            for element in content_root.findall(".//table:table", ns)
            if element.attrib.get(f"{{{ns['table']}}}name") == "ManualContents"
        ),
        None,
    )
    if toc is None:
        raise RuntimeError("Manual Contents table is missing")
    columns = toc.findall("table:table-column", ns)
    if len(columns) != 2:
        raise RuntimeError(f"Manual Contents must use one title/page pair in two columns; found {len(columns)}")
    toc_style = content_root.find('.//style:style[@style:name="ManualContentsWide"]', ns)
    if toc_style is None:
        raise RuntimeError("Manual Contents table style is missing")
    toc_props = toc_style.find("style:table-properties", ns)
    if toc_props is None or inches(toc_props.attrib.get(f"{{{ns['style']}}}width")) > 3.125:
        raise RuntimeError("Manual Contents table exceeds the publication text frame")

    page_values = []
    for paragraph in toc.findall(".//text:p", ns):
        if paragraph.attrib.get(f"{{{ns['text']}}}style-name") == "ManualContentsPageP":
            value = flat_text(paragraph)
            if value:
                page_values.append(value)
    if len(page_values) != 24 or any(not value.isdigit() or value == "00" for value in page_values):
        raise RuntimeError("Manual Contents must contain 24 resolved numeric chapter page values")

    # Every top-level chapter uses the same semantic + visual contract: one
    # outline-level-1 heading, blue numeric span T1 and black title span T3.
    chapters = [
        element for element in content_root.findall(".//text:h", ns)
        if element.attrib.get(f"{{{ns['text']}}}outline-level") == "1"
    ]
    if len(chapters) != 24:
        raise RuntimeError(f"Manual must expose exactly 24 semantic chapter headings; found {len(chapters)}")
    for chapter in chapters:
        spans = chapter.findall("text:span", ns)
        styles = [span.attrib.get(f"{{{ns['text']}}}style-name") for span in spans]
        if styles != ["T1", "T3"]:
            raise RuntimeError(f"Manual chapter heading is not normalized: {flat_text(chapter)!r}")
        if chapter.attrib.get(f"{{{ns['text']}}}style-name") not in {"P15", "P19"}:
            raise RuntimeError(f"Manual chapter heading uses unexpected paragraph style: {flat_text(chapter)!r}")

    # P17 is the maintained intermediate-heading style. Every such heading uses
    # the blue/bold T2 text style and no inherited body-text formatting.
    for element in content_root.iter():
        if element.attrib.get(f"{{{ns['text']}}}style-name") != "P17":
            continue
        spans = element.findall("text:span", ns)
        if not spans or any(
            span.attrib.get(f"{{{ns['text']}}}style-name") != "T2" for span in spans
        ):
            raise RuntimeError(f"Manual intermediate heading is not normalized: {flat_text(element)!r}")

    # Direct screenshots and gallery images use separate explicit paragraph
    # spacing contracts so figures cannot accidentally inherit zero-margin P8.
    direct_count = gallery_count = 0
    for paragraph in content_root.findall(".//text:p", ns):
        frames = paragraph.findall(".//draw:frame", ns)
        for frame in frames:
            name = frame.attrib.get(f"{{{ns['draw']}}}name", "")
            if name.startswith("ManualImage"):
                direct_count += 1
                if paragraph.attrib.get(f"{{{ns['text']}}}style-name") != "ManualScreenshotP":
                    raise RuntimeError(f"Direct screenshot does not use ManualScreenshotP: {name}")
            elif name.startswith("ManualGallery"):
                gallery_count += 1
                if paragraph.attrib.get(f"{{{ns['text']}}}style-name") != "ManualGalleryScreenshotP":
                    raise RuntimeError(f"Gallery screenshot does not use ManualGalleryScreenshotP: {name}")
    if direct_count < 25 or gallery_count != 22:
        raise RuntimeError(
            f"Unexpected manual screenshot inventory: direct={direct_count}, gallery={gallery_count}"
        )

    # All ordinary data tables share the same blue header / dark body typography.
    # Callouts, galleries and the custom Contents table are intentionally exempt.
    for table in content_root.findall(".//table:table", ns):
        name = table.attrib.get(f"{{{ns['table']}}}name", "")
        if name == "ManualContents" or name.startswith("ManualCallout") or name.startswith("ManualGallery"):
            continue
        for paragraph in table.findall("./table:table-header-rows//text:p", ns):
            spans = paragraph.findall("text:span", ns)
            if paragraph.attrib.get(f"{{{ns['text']}}}style-name") != "P20" or not spans or any(
                span.attrib.get(f"{{{ns['text']}}}style-name") != "T12" for span in spans
            ):
                raise RuntimeError(f"Manual table header typography drifted in {name}")
        for paragraph in table.findall("./table:table-row/table:table-cell/text:p", ns):
            spans = paragraph.findall("text:span", ns)
            if paragraph.attrib.get(f"{{{ns['text']}}}style-name") != "P2" or not spans or any(
                span.attrib.get(f"{{{ns['text']}}}style-name") != "T8" for span in spans
            ):
                raise RuntimeError(f"Manual table body typography drifted in {name}")

    required_text = (
        "Swing, Groove and Humanize — what is the difference?",
        "Timing layer",
        "Record a Groove by feel",
        "How the meter changes musical time",
        "Gate length — fixed pulse time, not duty cycle",
        "Roles, SOURCE and LOSS are separate decisions",
        "AUTO — lock in four steps",
        "RESET — TRIGGER versus GATE",
        "CORE LIC",
        "Source repository",
    )
    for required in required_text:
        if required not in content_xml:
            raise RuntimeError(f"Manual post-1.1 didactic/layout contract is missing: {required}")
    for forbidden in (
        "The AUTHOR row is the primary current example",
        "settings-info-author-popover",
    ):
        if forbidden in content_xml:
            raise RuntimeError(f"Manual contains obsolete author-overflow presentation: {forbidden}")

    source_heading = next(
        (element for element in children if flat_text(element) == "Source repository"), None
    )
    if source_heading is None or source_heading.attrib.get(f"{{{ns['text']}}}style-name") != "ManualSubsectionPageHeading":
        raise RuntimeError("Source repository must begin on its own page")

    if not UPDATES_QR.is_file():
        raise RuntimeError(f"Canonical firmware-update QR asset missing: {UPDATES_QR}")
    with Image.open(UPDATES_QR) as image:
        if image.width < 100 or image.height < 100:
            raise RuntimeError("Firmware-update QR asset is unexpectedly small")

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
        if requires_layout_v2(path, version):
            validate_layout_v2(content_xml, styles_xml)
        if version not in content_xml:
            raise RuntimeError(f"Manual body does not identify firmware {version}")
        if version not in meta_xml:
            raise RuntimeError(f"Manual metadata does not identify firmware {version}")
        prerelease_versions = {
            match.group(0).lower()
            for match in PRERELEASE_VERSION_RE.finditer(content_xml + meta_xml)
        }
        expected_prerelease_versions = {version.lower()} if "-" in version else set()
        if prerelease_versions != expected_prerelease_versions:
            raise RuntimeError(
                "Manual contains stale or mixed firmware prerelease versions: "
                f"{sorted(prerelease_versions)!r}"
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

        if requires_release_license_section(version):
            required_release_text = (
                "LICENSES AND SOURCE",
                "Firmware license",
                "Third-party license notices",
                "PolyForm Noncommercial License 1.0.0",
                "GNU Arm Embedded Toolchain 7.2.1",
                REPOSITORY_URL,
            )
            for required in required_release_text:
                if required not in content_xml:
                    raise RuntimeError(f"Manual release-license section is missing: {required}")
            license_pos = content_xml.find("LICENSES AND SOURCE")
            colophon_pos = content_xml.upper().find("COLOPHON")
            if license_pos < 0 or colophon_pos < 0 or license_pos > colophon_pos:
                raise RuntimeError("Manual release-license section must appear before the Colophon")
            qr_match = re.search(
                r'draw:name="RepositoryQR"[^>]*>\s*<draw:image[^>]*xlink:href="([^"]+)"',
                content_xml,
            )
            if qr_match is None:
                raise RuntimeError("Manual repository QR image is missing")
            qr_href = qr_match.group(1)
            if qr_href not in archive.namelist():
                raise RuntimeError(f"Manual repository QR payload missing: {qr_href}")
            if not REPOSITORY_QR.is_file():
                raise RuntimeError(f"Canonical repository QR asset missing: {REPOSITORY_QR}")
            if archive.read(qr_href) != REPOSITORY_QR.read_bytes():
                raise RuntimeError("Embedded repository QR does not match the canonical asset")

        if "Grooves and Custom Groove Record" in content_xml:
            content_ns = {
                "text": "urn:oasis:names:tc:opendocument:xmlns:text:1.0",
                "draw": "urn:oasis:names:tc:opendocument:xmlns:drawing:1.0",
            }
            content_root = ET.fromstring(content_xml)
            chapter_headings = [
                element
                for element in content_root.findall(".//text:h", content_ns)
                if element.attrib.get(f"{{{content_ns['text']}}}outline-level") == "1"
            ]
            if len(chapter_headings) != 24:
                raise RuntimeError(
                    f"Manual must expose exactly 24 semantic chapter headings for PDF navigation; "
                    f"found {len(chapter_headings)}"
                )
            for required in (
                "Swing, Groove and Humanize",
                "Pre-Count versus Recorder Count-In",
                "Record a Groove by feel",
                "10-pin (2×5) Eurorack header",
            ):
                if required not in content_xml:
                    raise RuntimeError(f"Manual didactic contract is missing: {required}")
            for forbidden in (
                "standard Eurorack 2×8 power cable",
                "The gate buffer remains disabled while the firmware initializes",
            ):
                if forbidden in content_xml:
                    raise RuntimeError(f"Manual contains obsolete hardware wording: {forbidden}")
            for frame_name in ("ManualImage20", "ManualImage21", "ManualImage22", "ManualImage23"):
                frame = next(
                    (
                        element for element in content_root.findall(".//draw:frame", content_ns)
                        if element.attrib.get(f"{{{content_ns['draw']}}}name") == frame_name
                    ),
                    None,
                )
                if frame is None:
                    raise RuntimeError(f"Manual direct screenshot frame is missing: {frame_name}")
                if frame.attrib.get(f"{{{content_ns['text']}}}anchor-type") != "as-char":
                    raise RuntimeError(
                        f"Manual direct screenshot frame must be inline/as-char to avoid text overlap: {frame_name}"
                    )

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
    pdf_versions = {
        match.group(0).lower()
        for match in PRERELEASE_VERSION_RE.finditer(text)
    }
    expected_pdf_versions = {version.lower()} if "-" in version else set()
    if pdf_versions != expected_pdf_versions:
        raise RuntimeError(
            f"PDF contains stale or mixed firmware prerelease versions: {sorted(pdf_versions)!r}"
        )
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
