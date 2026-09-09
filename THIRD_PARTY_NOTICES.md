<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Third-Party Notices

The project firmware itself is licensed under the PolyForm Noncommercial
License 1.0.0. The firmware build also incorporates or derives data from the
following third-party open-source components. Their licenses apply to those
components independently of the project license.

## STM32duino / Arduino Core for STM32

The PlatformIO build uses `framework = arduino` on the ST STM32 platform and
therefore links against the STM32duino Arduino Core. Arduino-derived core and
library files used by the framework (including Wiring and Wire/SPI-style core
facilities) are distributed under the GNU Lesser General Public License,
version 2.1 or, where stated by the upstream source file, any later version.
STM32-specific portions additionally contain code under BSD-3-Clause and other
component licenses described by the upstream STM32duino/STM32Cube packages.

Upstream project: `stm32duino/Arduino_Core_STM32`

Relevant license text bundled here:

- `third_party/LICENSE-LGPL-2.1.txt`
- `third_party/LICENSE-BSD-3-Clause.txt`
- `third_party/LICENSE-Apache-2.0.txt`

## STM32CubeF4 HAL / LL

The STM32F4 HAL/LL components supplied through the STM32 framework are
copyright STMicroelectronics and licensed under BSD-3-Clause.

Upstream project: `STMicroelectronics/STM32CubeF4`

License text: `third_party/LICENSE-BSD-3-Clause.txt`

## CMSIS / CMSIS Device

CMSIS components used by STM32CubeF4 are copyright Arm Limited and, for device
content, Arm Limited and STMicroelectronics. The STM32CubeF4 license manifest
identifies these components as Apache License 2.0.

License text: `third_party/LICENSE-Apache-2.0.txt`

## Roboto Condensed Bold tempo numerals

The native-resolution 1-bit tempo glyphs in `src/hal/display_font.cpp` are
raster derivatives of **Roboto Condensed Bold**, designed by Christian
Robertson for Google. Roboto is licensed under the Apache License 2.0.
Only the rasterized numeral/dash glyph data required by the OLED UI is embedded
in the firmware; no font file is distributed by this repository.

Upstream project: `googlefonts/roboto-2`

License text: `third_party/LICENSE-Apache-2.0.txt`

## SDL3 (native simulator only)

The optional desktop simulator uses **SDL3** for its native Windows, macOS,
and Linux window/input/rendering frontend. SDL3 is distributed under the
zlib license. The firmware target itself does not link SDL3.

Upstream project: `libsdl-org/SDL`

License text: `third_party/LICENSE-SDL-zlib.txt`

## Development and build tooling

PlatformIO, GCC, OpenOCD, Python, GitHub Actions and editor extensions are
development/build tools rather than project-owned firmware source. They retain
their own upstream licenses. They are not relicensed by this project.

## Notes

This notice is intended to keep runtime and source-code attributions visible in
both source and release distributions. It does not replace the license headers
or notices shipped by the upstream projects.

## Static-link distribution note

The current STM32 target is built against STM32duino as a statically linked embedded firmware image. LGPL-covered core/library portions therefore carry distribution obligations beyond retaining this notice and the LGPL text. In particular, a final binary distribution needs a deliberate mechanism that satisfies the LGPL requirements applicable to modifying/relinking the covered library portions. The project-owned PolyForm Noncommercial license does not replace or narrow those third-party rights.

The normal prerelease workflow therefore publishes source-only GitHub releases. CI verifies the target builds but does not distribute BIN/ELF artifacts. The preferred long-term resolution is migration of the runtime HAL boundary to permissively licensed STM32Cube HAL/LL + CMSIS. The prerelease engineering assessment is maintained in `docs/LICENSING.md`.
