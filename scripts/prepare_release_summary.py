#!/usr/bin/env python3
"""Create a review-required user-facing release-summary scaffold for CLOCK.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
VERSION_RE = re.compile(r'^#define\s+CLOCK_FIRMWARE_VERSION\s+"([^"]+)"\s*$', re.MULTILINE)


def current_version() -> str:
    match = VERSION_RE.search((ROOT / "src/version.h").read_text(encoding="utf-8"))
    if match is None:
        raise RuntimeError("CLOCK_FIRMWARE_VERSION is missing")
    return match.group(1)


def main() -> int:
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument("--version", default=current_version())
    parser.add_argument("--force", action="store_true")
    parser.add_argument("--output", type=Path)
    args = parser.parse_args()
    out = args.output.resolve() if args.output else ROOT / "docs" / "releases" / args.version / "RELEASE_SUMMARY.md"
    if out.exists() and not args.force:
        print(out)
        return 0
    out.parent.mkdir(parents=True, exist_ok=True)
    out.write_text(
        "<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->\n\n"
        f"# CLOCK {args.version}\n\n"
        "CLOCK is an eight-output Eurorack clock, divider, Euclidean rhythm source, and gate sequencer. "
        "TODO: replace this sentence with the release-specific user-facing context.\n\n"
        "## Highlights\n\n"
        "- TODO: describe the most important user-visible change.\n"
        "- TODO: describe the next user-visible change.\n\n"
        "## Compatibility\n\n"
        "TODO: state anything users need to know about presets, hardware, updates, or behavior.\n",
        encoding="utf-8",
    )
    print(out)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
