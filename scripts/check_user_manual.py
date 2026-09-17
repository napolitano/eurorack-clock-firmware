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
REPOSITORY_QR = ROOT / "docs" / "manual" / "assets" / "repository-qr.png"

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
