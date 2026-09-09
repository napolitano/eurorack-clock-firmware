#!/usr/bin/env python3
"""Read and validate firmware version metadata.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import re
import sys
from pathlib import Path

VERSION_RE = re.compile(r'^#define\s+CLOCK_FIRMWARE_VERSION\s+"([^"]+)"\s*$')


def read_define(path: Path, pattern: re.Pattern[str]) -> str:
    for line in path.read_text(encoding="utf-8").splitlines():
        match = pattern.match(line.strip())
        if match:
            return match.group(1)
    raise SystemExit(f"Required version define not found in {path}")


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--file", default="src/version.h")
    parser.add_argument("--check-tag", help="Validate a Git tag, e.g. v1.2.3")
    args = parser.parse_args()

    path = Path(args.file)
    value = read_define(path, VERSION_RE)

    if args.check_tag:
        expected = f"v{value}"
        if args.check_tag != expected:
            print(f"Version mismatch: tag={args.check_tag!r}, firmware={expected!r}", file=sys.stderr)
            return 2

    print(value)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
