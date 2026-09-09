#!/usr/bin/env python3
"""Extract the matching version section from CHANGELOG.md for a GitHub Release.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("version")
    parser.add_argument("--changelog", default="CHANGELOG.md")
    parser.add_argument("--output", default="dist/RELEASE_NOTES.md")
    args = parser.parse_args()

    text = Path(args.changelog).read_text(encoding="utf-8")
    # Accept headings such as ## [1.2.3] or ## 1.2.3.
    heading = re.compile(rf"^##\s+(?:\[)?{re.escape(args.version)}(?:\])?.*$", re.MULTILINE)
    match = heading.search(text)
    if not match:
        raise SystemExit(f"No CHANGELOG section found for {args.version}")

    next_heading = re.compile(r"^##\s+", re.MULTILINE).search(text, match.end())
    end = next_heading.start() if next_heading else len(text)
    body = text[match.start():end].strip() + "\n"

    output = Path(args.output)
    output.parent.mkdir(parents=True, exist_ok=True)
    output.write_text(body, encoding="utf-8")
    print(output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
