#!/usr/bin/env python3
"""Locate and launch the native simulator from a CMake build directory.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
BUILD_DIR = ROOT / "build" / "simulator"


def candidate_paths() -> list[Path]:
    """Return common single- and multi-config CMake executable locations."""
    executable = "clock-simulator.exe" if os.name == "nt" else "clock-simulator"
    return [
        BUILD_DIR / executable,
        BUILD_DIR / "Debug" / executable,
        BUILD_DIR / "Release" / executable,
        BUILD_DIR / "RelWithDebInfo" / executable,
    ]


def main() -> int:
    """Launch the first simulator executable found and forward extra arguments."""
    forwarded = list(sys.argv[1:])
    if "--layout" not in forwarded:
        forwarded = ["--layout", str(ROOT / "sim" / "panel_layout.ini"), *forwarded]

    for candidate in candidate_paths():
        if candidate.is_file():
            return subprocess.call([str(candidate), *forwarded], cwd=ROOT)

    print(
        "Simulator executable not found. Build it first with "
        "`cmake --preset simulator` and `cmake --build --preset simulator`.",
        file=sys.stderr,
    )
    return 1


if __name__ == "__main__":
    raise SystemExit(main())
