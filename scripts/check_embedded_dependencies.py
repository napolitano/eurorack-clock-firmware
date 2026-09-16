#!/usr/bin/env python3
"""Fail when the embedded runtime regains an unreviewed Arduino/LGPL dependency.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import re
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]


def main() -> int:
    errors: list[str] = []
    platformio = (ROOT / "platformio.ini").read_text(encoding="utf-8")
    base = re.search(r"\[env:blackpill_f401cc\](.*?)(?=\n\[|\Z)", platformio, re.S)
    if base is None:
        errors.append("missing [env:blackpill_f401cc]")
    else:
        body = base.group(1)
        if not re.search(r"^framework\s*=\s*stm32cube\s*$", body, re.M):
            errors.append("embedded base environment must use framework = stm32cube")
        if re.search(r"^lib_deps\s*=", body, re.M):
            errors.append("embedded base environment must not add lib_deps without review")
        if "platformio/toolchain-gccarmnoneeabi@1.70201.0" not in body:
            errors.append("embedded base environment must pin ARM GCC 7.2.1")
        if "-std=gnu++1z" not in body:
            errors.append("embedded base environment must compile project sources as GCC-7 C++17")
    if "framework = arduino" in platformio:
        errors.append("Arduino framework declaration found in platformio.ini")

    notice = (ROOT / "THIRD_PARTY_NOTICES.md").read_text(encoding="utf-8")
    if "## STM32duino / Arduino Core" in notice:
        errors.append("runtime third-party notice still declares the retired STM32duino dependency")
    if (ROOT / "third_party" / "LICENSE-LGPL-2.1.txt").exists():
        errors.append("retired LGPL license file is still bundled")

    linker = (ROOT / "ld" / "stm32f401cc_clock.ld").read_text(encoding="utf-8")
    if "ORIGIN = 0x20000000, LENGTH = 64K" not in linker:
        errors.append("custom STM32Cube linker script must declare the F401CC 64K RAM explicitly")
    if "LD_MAX_DATA_SIZE" in linker:
        errors.append("custom STM32Cube linker script must not depend on Arduino-only LD_MAX_DATA_SIZE")
    if "(READONLY)" in linker:
        errors.append("custom linker script uses READONLY output-section syntax unsupported by the pinned GCC 7.2.1 toolchain")

    production = "\n".join(
        path.read_text(encoding="utf-8")
        for root in (ROOT / "src", ROOT / "lib")
        for path in sorted(root.rglob("*"))
        if path.suffix in {".h", ".hpp", ".cpp"}
    )
    # Host/simulator adapters may mention these names behind compile-time guards,
    # but the embedded environment itself must not expose framework selection or
    # library dependencies. The real graph is verified again by PlatformIO CI.
    if "Adafruit_" in production:
        errors.append("Adafruit runtime dependency marker found in production source")

    if errors:
        print("Embedded dependency policy failed:")
        for error in errors:
            print(f"- {error}")
        return 1
    print("Embedded dependency policy passed: STM32CubeF4/CMSIS only; no LGPL runtime dependency declared.")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
