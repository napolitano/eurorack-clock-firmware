#!/usr/bin/env python3
"""Validate the frozen, user-facing release summary for one CLOCK version.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path


def validate(path: Path, version: str) -> None:
    if not path.is_file():
        raise RuntimeError(f"Release summary missing: {path}")
    text = path.read_text(encoding="utf-8")
    if version not in text:
        raise RuntimeError(f"Release summary does not mention firmware {version}")
    if "## Highlights" not in text:
        raise RuntimeError("Release summary requires a '## Highlights' section")
    if re.search(r"\b(?:TODO|TBD|FIXME)\b", text, re.IGNORECASE):
        raise RuntimeError("Release summary still contains an unfinished placeholder")
    bullets = re.findall(r"(?m)^-\s+.+$", text)
    if not 2 <= len(bullets) <= 10:
        raise RuntimeError("Release summary must contain 2-10 concise user-facing bullets")
    if len(text.strip()) < 350:
        raise RuntimeError("Release summary is too short to explain the release")


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("summary", type=Path)
    parser.add_argument("--version", required=True)
    args = parser.parse_args()
    try:
        validate(args.summary.resolve(), args.version)
    except RuntimeError as exc:
        raise SystemExit(str(exc)) from exc
    print(f"Release summary check passed: {args.summary}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
