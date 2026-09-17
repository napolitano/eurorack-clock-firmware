#!/usr/bin/env python3
"""Build curated GitHub release notes from summary, changelog, artifacts and source diff.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
REPOSITORY_URL = "https://github.com/napolitano/eurorack-clock-firmware"
FIRMWARE_FLAVORS = (
    ("default", "BEATKNECHT (default)"),
    ("pixel-raid", "Pixel Raid"),
    ("formula-1", "Formula 1"),
    ("breakout", "Breakout"),
    ("egg-journey", "Egg Journey"),
)


def changelog_section(version: str, changelog: Path) -> str:
    text = changelog.read_text(encoding="utf-8")
    heading = re.compile(rf"^##\s+(?:\[)?{re.escape(version)}(?:\])?.*$", re.MULTILINE)
    match = heading.search(text)
    if not match:
        raise RuntimeError(f"No CHANGELOG section found for {version}")
    next_heading = re.compile(r"^##\s+", re.MULTILINE).search(text, match.end())
    end = next_heading.start() if next_heading else len(text)
    return text[match.start():end].strip()


def previous_version(version: str, changelog: Path) -> str | None:
    text = changelog.read_text(encoding="utf-8")
    headings = re.findall(r"^##\s+\[([^\]]+)\]", text, re.MULTILINE)
    try:
        index = headings.index(version)
    except ValueError:
        return None
    return headings[index + 1] if index + 1 < len(headings) else None


def firmware_table(version: str) -> str:
    rows = [
        "## Firmware images",
        "",
        "All firmware images are persistence-safe DfuSe files for the STM32F401CC Black Pill.",
        "The only difference between flavors is the compiled boot Easter egg.",
        "",
        "| Flavor | Release asset |",
        "| --- | --- |",
    ]
    for flavor, label in FIRMWARE_FLAVORS:
        rows.append(f"| {label} | `eurorack-clock-firmware-{flavor}-{version}.dfu` |")
    return "\n".join(rows)


def licensing_section(version: str) -> str:
    return "\n".join([
        "## Documentation, licenses and integrity",
        "",
        f"The release includes the versioned user manual as both `clock-user-manual.{version}.pdf` and `clock-user-manual.{version}.odt`, the full `CHANGELOG.md`, project and manual licenses, third-party notices, the exact GNU Arm Embedded Toolchain 7.2.1 license capture used for the build, and `BUILD-INFO.txt` derived from the linker maps.",
        "",
        "`SHA256SUMS.txt` and `MD5SUMS.txt` cover the complete published payload.",
    ])


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("version")
    parser.add_argument("--changelog", type=Path, default=Path("CHANGELOG.md"))
    parser.add_argument("--summary", type=Path)
    parser.add_argument("--output", type=Path, default=Path("dist/RELEASE_NOTES.md"))
    args = parser.parse_args()

    summary = args.summary or ROOT / "docs" / "releases" / args.version / "RELEASE_SUMMARY.md"
    if not summary.is_file():
        raise SystemExit(f"Release summary missing: {summary}")
    try:
        details = changelog_section(args.version, args.changelog)
    except RuntimeError as exc:
        raise SystemExit(str(exc)) from exc

    parts = [
        summary.read_text(encoding="utf-8").strip(),
        firmware_table(args.version),
        licensing_section(args.version),
        "## Detailed changelog\n\n" + details,
    ]
    previous = previous_version(args.version, args.changelog)
    if previous:
        parts.append(
            "## Source comparison\n\n"
            f"[`v{previous}...v{args.version}`]({REPOSITORY_URL}/compare/v{previous}...v{args.version})"
        )
    else:
        parts.append(f"## Source\n\n[{REPOSITORY_URL}]({REPOSITORY_URL})")

    output = args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text("\n\n---\n\n".join(parts) + "\n", encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
