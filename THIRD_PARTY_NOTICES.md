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

## GNU Arm Embedded Toolchain runtime

Release firmware is built with the pinned PlatformIO GNU Arm Embedded Toolchain 7.2.1 package (`platformio/toolchain-gccarmnoneeabi@1.70201.0`). Depending on the symbols used by a particular flavor, the final linked image may contain runtime code from GCC support libraries and Newlib/Newlib-nano/libnosys. Those components retain their upstream licenses. GCC runtime components are distributed subject to the applicable GNU licenses and GCC Runtime Library Exception; Newlib components carry their permissive upstream notices.

Every tagged firmware release captures the license material from the exact installed 7.2.1 toolchain package into `GNU-ARM-EMBEDDED-7.2.1-LICENSES.zip`. `BUILD-INFO.txt` is generated from the linker maps and records the static archives referenced by each firmware flavor. This avoids relying on a generic or newer toolchain license bundle when distributing binaries.


## VCV Rack / Rack SDK (experimental VCV target only)

The experimental `vcv/` plugin is built against the VCV Rack SDK. The SDK is an external build dependency, is not vendored into this repository, and is not bundled in the CLOCK `.vcvplugin`. Rack supplies its own runtime/API when loading the plugin.

VCV Rack is GPL-3.0-or-later with the VCV Rack Non-Commercial Plugin License Exception. CLOCK relies on that exception for the free-of-charge Trial-First plugin's Rack API/link boundary. No Rack/Core source or VCV visual assets are copied into CLOCK.

Upstream license: <https://github.com/VCVRack/Rack/blob/v2/LICENSE.md>

## Development and build tooling

PlatformIO, dfu-util/OpenOCD, Python, GitHub Actions and editor extensions are development tools and retain their own upstream licenses. The GNU Arm compiler is also a build tool, while any runtime code actually linked from its support libraries is covered by the runtime notice above.

## Runtime distribution status

The embedded runtime no longer links STM32duino/Arduino Core, Wire, SPI, or HardwareTimer. The previous LGPL-2.1 runtime dependency and its binary-distribution blocker have therefore been removed. Firmware binary releases ship the project license and Required Notice, the applicable BSD-3-Clause and Apache-2.0 texts, this notice file, and the license capture from the exact pinned GNU Arm toolchain.
