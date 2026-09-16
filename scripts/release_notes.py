#!/usr/bin/env python3
"""Build GitHub release notes from the user summary plus the matching changelog section.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def changelog_section(version: str, changelog: Path) -> str:
    text = changelog.read_text(encoding="utf-8")
    heading = re.compile(rf"^##\s+(?:\[)?{re.escape(version)}(?:\])?.*$", re.MULTILINE)
    match = heading.search(text)
    if not match:
        raise RuntimeError(f"No CHANGELOG section found for {version}")
    next_heading = re.compile(r"^##\s+", re.MULTILINE).search(text, match.end())
    end = next_heading.start() if next_heading else len(text)
    return text[match.start():end].strip()


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

    output = args.output
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(
        summary.read_text(encoding="utf-8").strip()
        + "\n\n---\n\n## Detailed changelog\n\n"
        + details
        + "\n",
        encoding="utf-8",
    )
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
