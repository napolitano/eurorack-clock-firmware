<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Third-Party Notices

CLOCK project-owned firmware is licensed under the PolyForm Noncommercial License 1.0.0. Third-party components retain their own licenses.

## STM32CubeF4 HAL / LL

The embedded target uses PlatformIO `framework = stm32cube` and links STM32CubeF4 HAL/device support from STMicroelectronics. The STM32F4 HAL is licensed under BSD-3-Clause.

Upstream: `STMicroelectronics/STM32CubeF4`

License text: `third_party/LICENSE-BSD-3-Clause.txt`

## CMSIS / CMSIS Device

CMSIS core and STM32F4 CMSIS Device support used by STM32CubeF4 are licensed under Apache License 2.0.

License text: `third_party/LICENSE-Apache-2.0.txt`

## Roboto Condensed Bold tempo numerals

The native-resolution 1-bit tempo glyphs in `src/hal/display_font.cpp` are raster derivatives of Roboto Condensed Bold. Roboto is licensed under Apache License 2.0. No font file is distributed by this repository.

License text: `third_party/LICENSE-Apache-2.0.txt`

## SDL3 (native simulator only)

The optional desktop simulator uses SDL3 under the zlib license. SDL3 is not linked into STM32 firmware.

License text: `third_party/LICENSE-SDL-zlib.txt`

## Development and build tooling

PlatformIO, GCC, dfu-util/OpenOCD, Python, GitHub Actions and editor extensions are development tools and retain their own upstream licenses.

## Runtime distribution status

The embedded runtime no longer links STM32duino/Arduino Core, Wire, SPI, or HardwareTimer. The previous LGPL-2.1 runtime dependency and its binary-distribution blocker have therefore been removed. Firmware binaries must continue to ship the applicable BSD-3-Clause and Apache-2.0 notices above.
