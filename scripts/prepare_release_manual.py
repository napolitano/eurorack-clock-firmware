#!/usr/bin/env python3
"""Stamp and freeze the CLOCK user manual for one firmware version.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import io
import re
import shutil
import subprocess
import tempfile
import zipfile
from pathlib import Path
from xml.etree import ElementTree as ET

try:
    from PIL import Image, ImageDraw, ImageFont, PngImagePlugin
except ImportError as exc:  # pragma: no cover - exercised by CLI diagnostics
    raise SystemExit("Pillow is required: python -m pip install pillow") from exc

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_SOURCE = ROOT / "docs" / "manual-source" / "clock-user-manual.odt"
VERSION_HEADER = ROOT / "src" / "version.h"
STABLE_VERSION_RE = re.compile(r"^[0-9]+\.[0-9]+\.[0-9]+$")
VERSION_DEFINE_RE = re.compile(r'^#define\s+CLOCK_FIRMWARE_VERSION\s+"([^"]+)"\s*$', re.MULTILINE)
COVERAGE_VERSION_RE = re.compile(
    r"firmware\s+([0-9]+\.[0-9]+\.[0-9]+(?:-[0-9A-Za-z]+(?:[.-][0-9A-Za-z]+)*)?)",
    re.IGNORECASE,
)
ODT_MIMETYPE = b"application/vnd.oasis.opendocument.text"
BLUE = (11, 79, 192, 255)
WHITE = (255, 255, 255, 255)
PNG_VERSION_KEY = "clock_firmware_version"


def current_version() -> str:
    text = VERSION_HEADER.read_text(encoding="utf-8")
    match = VERSION_DEFINE_RE.search(text)
    if match is None:
        raise RuntimeError("CLOCK_FIRMWARE_VERSION is missing from src/version.h")
    return match.group(1)


def source_version(meta_xml: str, content_xml: str) -> str:
    """Return the firmware version embedded in the manual source.

    License versions are independent metadata and must never participate in firmware
    version detection. Prefer the dedicated FirmwareVersion field and only fall back
    to explicit ``firmware <semver>`` prose in older manual sources.
    """
    ns = {
        "meta": "urn:oasis:names:tc:opendocument:xmlns:meta:1.0",
    }
    try:
        root = ET.fromstring(meta_xml)
    except ET.ParseError:
        root = None
    if root is not None:
        for element in root.findall(".//meta:user-defined", ns):
            if element.attrib.get(f"{{{ns['meta']}}}name") == "FirmwareVersion":
                value = (element.text or "").strip()
                if value:
                    return value

    for text in (meta_xml, content_xml):
        match = COVERAGE_VERSION_RE.search(text)
        if match is not None:
            return match.group(1)
    raise RuntimeError("Unable to identify the firmware version embedded in the manual source")


def resolve_font(preferred_style: str, *, allow_substitution: bool) -> str:
    query = f"Ubuntu:style={preferred_style}"
    result = subprocess.run(
        ["fc-match", "-f", "%{family}\n%{file}\n", query],
        text=True,
        capture_output=True,
        check=True,
    )
    lines = [line.strip() for line in result.stdout.splitlines() if line.strip()]
    if len(lines) < 2:
        raise RuntimeError(f"Unable to resolve font for {query}")
    family, path = lines[0], lines[1]
    if "Ubuntu" not in family and not allow_substitution:
        raise RuntimeError(
            f"Ubuntu font required for manual stamping; resolved {family!r}. "
            "Install fonts-ubuntu or pass --allow-font-substitution for a non-release preview."
        )
    return path


def page_images(content_xml: str, archive: zipfile.ZipFile) -> tuple[str, str]:
    hrefs = re.findall(r'xlink:href="(Pictures/[^"]+\.png)"', content_xml)
    candidates: list[str] = []
    for href in hrefs:
        try:
            with Image.open(io.BytesIO(archive.read(href))) as image:
                if image.size == (1010, 2144):
                    candidates.append(href)
        except KeyError:
            continue
    unique = list(dict.fromkeys(candidates))
    if len(unique) != 2:
        raise RuntimeError(
            "Manual cover contract changed: expected exactly two 1010x2144 full-page PNGs, "
            f"found {len(unique)}"
        )
    return unique[0], unique[-1]


def release_stage_label(version: str) -> str:
    """Return the publication label used on the manual cover for one SemVer stage."""
    lowered = version.lower()
    if "-alpha" in lowered:
        return "Prototype"
    if "-beta" in lowered:
        return "Beta"
    if "-rc" in lowered:
        return "Release candidate"
    return "Release"


def stamp_cover(image_bytes: bytes, version: str, font_path: str) -> bytes:
    image = Image.open(io.BytesIO(image_bytes)).convert("RGBA")
    draw = ImageDraw.Draw(image)
    draw.rectangle((45, 1905, 780, 1965), fill=BLUE)
    font = ImageFont.truetype(font_path, 33)
    draw.text((58, 1909), f"{release_stage_label(version)} · firmware {version}", font=font, fill=WHITE)
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text(PNG_VERSION_KEY, version)
    out = io.BytesIO()
    image.save(out, format="PNG", pnginfo=metadata, optimize=True)
    return out.getvalue()


def stamp_back(image_bytes: bytes, version: str, font_path: str) -> bytes:
    image = Image.open(io.BytesIO(image_bytes)).convert("RGBA")
    draw = ImageDraw.Draw(image)
    draw.rectangle((45, 1406, 720, 1468), fill=BLUE)
    font = ImageFont.truetype(font_path, 38)
    draw.text((58, 1409), f"{version} ({release_stage_label(version).lower()})", font=font, fill=WHITE)
    metadata = PngImagePlugin.PngInfo()
    metadata.add_text(PNG_VERSION_KEY, version)
    out = io.BytesIO()
    image.save(out, format="PNG", pnginfo=metadata, optimize=True)
    return out.getvalue()


def _replace_explicit_firmware_reference(text: str, old_version: str, version: str) -> str:
    """Replace a firmware version only when the surrounding text identifies it as firmware."""
    escaped = re.escape(old_version)
    patterns = (
        re.compile(
            rf"(?i)(\bfirmware(?:\s+version)?\s*(?:[:·—–-]\s*)?){escaped}\b"
        ),
        re.compile(rf"(?i)(\bCLOCK\s+){escaped}\b"),
    )
    updated = text
    for pattern in patterns:
        updated = pattern.sub(lambda match: f"{match.group(1)}{version}", updated)
    return updated


def replace_firmware_version_in_content(content_xml: str, old_version: str, version: str) -> str:
    """Stamp only firmware-version occurrences in ODT content.xml.

    In particular, a firmware release such as 1.1.0 must never rewrite an
    unrelated license identifier such as PolyForm Noncommercial License 1.0.0.
    """
    updated = _replace_explicit_firmware_reference(content_xml, old_version, version)

    # The technical-status table stores the value in a separate cell next to an
    # exact ``Firmware`` label, so there is no textual ``firmware <version>``
    # phrase for the generic replacement above to match. Restrict the fallback
    # to only that labeled table row.
    row_pattern = re.compile(r"<table:table-row\b.*?</table:table-row>", re.DOTALL)
    exact_value = re.compile(rf"(?<![0-9A-Za-z.-]){re.escape(old_version)}(?![0-9A-Za-z.-])")

    def replace_firmware_row(match: re.Match[str]) -> str:
        row = match.group(0)
        if re.search(r">\s*Firmware\s*<", row, re.IGNORECASE) is None:
            return row
        return exact_value.sub(version, row)

    return row_pattern.sub(replace_firmware_row, updated)


def update_meta_xml(meta_xml: str, old_version: str, version: str) -> str:
    ns = {
        "office": "urn:oasis:names:tc:opendocument:xmlns:office:1.0",
        "meta": "urn:oasis:names:tc:opendocument:xmlns:meta:1.0",
        "dc": "http://purl.org/dc/elements/1.1/",
    }
    ET.register_namespace("office", ns["office"])
    ET.register_namespace("meta", ns["meta"])
    ET.register_namespace("dc", ns["dc"])
    root = ET.fromstring(meta_xml)
    office_meta = root.find("office:meta", ns)
    if office_meta is None:
        raise RuntimeError("ODT meta.xml does not contain office:meta")

    description = office_meta.find("dc:description", ns)
    if description is not None and description.text:
        description.text = _replace_explicit_firmware_reference(
            description.text, old_version, version
        )

    existing = None
    for element in office_meta.findall("meta:user-defined", ns):
        name = element.attrib.get(f"{{{ns['meta']}}}name")
        if name == "Coverage" and element.text:
            element.text = _replace_explicit_firmware_reference(
                element.text, old_version, version
            )
        elif name == "FirmwareVersion":
            existing = element
    if existing is None:
        existing = ET.SubElement(
            office_meta,
            f"{{{ns['meta']}}}user-defined",
            {f"{{{ns['meta']}}}name": "FirmwareVersion"},
        )
    existing.text = version
    return ET.tostring(root, encoding="unicode", xml_declaration=True)


def stamp_odt(source: Path, destination: Path, version: str, *, allow_font_substitution: bool) -> None:
    if not source.is_file():
        raise FileNotFoundError(f"Manual source not found: {source}")
    destination.parent.mkdir(parents=True, exist_ok=True)

    regular_font = resolve_font("Regular", allow_substitution=allow_font_substitution)
    bold_font = resolve_font("Bold", allow_substitution=allow_font_substitution)

    with zipfile.ZipFile(source, "r") as archive:
        if archive.read("mimetype") != ODT_MIMETYPE:
            raise RuntimeError(f"Not an ODT document: {source}")
        content_xml = archive.read("content.xml").decode("utf-8")
        meta_xml = archive.read("meta.xml").decode("utf-8")
        old_version = source_version(meta_xml, content_xml)
        cover_name, back_name = page_images(content_xml, archive)

        replacements: dict[str, bytes] = {
            "content.xml": replace_firmware_version_in_content(content_xml, old_version, version).encode("utf-8"),
            "meta.xml": update_meta_xml(meta_xml, old_version, version).encode("utf-8"),
            cover_name: stamp_cover(archive.read(cover_name), version, regular_font),
            back_name: stamp_back(archive.read(back_name), version, bold_font),
        }

        with tempfile.NamedTemporaryFile(suffix=".odt", delete=False, dir=destination.parent) as temp_file:
            temp_path = Path(temp_file.name)
        try:
            with zipfile.ZipFile(temp_path, "w") as output:
                mimetype_info = archive.getinfo("mimetype")
                output.writestr(mimetype_info, archive.read("mimetype"), compress_type=zipfile.ZIP_STORED)
                for info in archive.infolist():
                    if info.filename == "mimetype":
                        continue
                    payload = replacements.get(info.filename, archive.read(info.filename))
                    output.writestr(info, payload)
            temp_path.replace(destination)
        finally:
            if temp_path.exists():
                temp_path.unlink()


def is_stable_release(version: str) -> bool:
    """Return whether *version* is a final X.Y.Z release rather than a prerelease."""
    return STABLE_VERSION_RE.fullmatch(version) is not None


def default_output(version: str) -> Path:
    """Return the default frozen-manual path without polluting the stable archive."""
    if is_stable_release(version):
        return ROOT / "docs" / "manual" / f"clock-user-manual.{version}.odt"
    return ROOT / "build" / "manual" / f"clock-user-manual.{version}.odt"


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", default=current_version())
    parser.add_argument("--source", type=Path, default=DEFAULT_SOURCE)
    parser.add_argument("--output", type=Path)
    parser.add_argument("--allow-font-substitution", action="store_true")
    args = parser.parse_args()

    output = args.output or default_output(args.version)
    stamp_odt(
        args.source.resolve(),
        output.resolve(),
        args.version,
        allow_font_substitution=args.allow_font_substitution,
    )
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
