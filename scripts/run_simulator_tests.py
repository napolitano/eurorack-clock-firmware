#!/usr/bin/env python3
"""Configure, build, and execute the SDL-free native simulator integration tests.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import subprocess
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def run(command: list[str]) -> None:
    """Run one simulator build/test command from the repository root."""
    print("+", " ".join(command))
    subprocess.run(command, cwd=ROOT, check=True)


def main() -> int:
    """Build and execute the portable CMake/CTest simulator integration suite."""
    run(["cmake", "--preset", "simulator-headless"])
    run(["cmake", "--build", "--preset", "simulator-headless"])
    run(["ctest", "--preset", "simulator-headless"])
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
