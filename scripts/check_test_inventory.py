#!/usr/bin/env python3
# Author: Axel Napolitano
# License: PolyForm-Noncommercial-1.0.0
"""Validate that the public PlatformIO native command exposes the full native test inventory."""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
PIO = ROOT / "platformio.ini"
SUITES = {
    "test_clock_core": ROOT / "test" / "test_clock_core" / "test_main.cpp",
    "realtime": ROOT / "test" / "realtime" / "test_main.cpp",
    "host_firmware": ROOT / "test" / "host_firmware" / "test_main.cpp",
}
MIN_CASES = 90


def main() -> int:
    config = PIO.read_text(encoding="utf-8")
    native_match = re.search(r"(?ms)^\[env:native\]\n(.*?)(?=^\[|\Z)", config)
    if native_match is None:
        raise RuntimeError("platformio.ini is missing [env:native]")
    native = native_match.group(1)
    if not re.search(r"(?m)^test_build_src\s*=\s*yes\s*$", native):
        raise RuntimeError("env:native must build production src for integration tests")
    for suite in SUITES:
        if not re.search(rf"(?m)^\s+{re.escape(suite)}\s*$", native):
            raise RuntimeError(f"env:native does not expose suite: {suite}")

    total = 0
    counts: dict[str, int] = {}
    for suite, path in SUITES.items():
        text = path.read_text(encoding="utf-8")
        names = re.findall(r"RUN_TEST\((test[A-Za-z0-9_]+)\)", text)
        if len(names) != len(set(names)):
            raise RuntimeError(f"duplicate RUN_TEST registration in {suite}")
        counts[suite] = len(names)
        total += len(names)

    if total < MIN_CASES:
        raise RuntimeError(f"native test inventory regressed to {total}; minimum is {MIN_CASES}")
    print("Native test inventory: " + ", ".join(f"{k}={v}" for k, v in counts.items()) + f", total={total}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
