<!-- Author: Axel Napolitano -->
<!-- License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Dependency Analysis

## Current result

The firmware has **no third-party PlatformIO `lib_deps`**.

The former dependencies on Adafruit GFX and Adafruit SSD1306 have been removed. Display rendering and the SSD1306/SSD1315 protocol are now project-owned code in `src/hal/oled_display.*` and `src/hal/display_font.h`.

## Remaining framework dependencies

The firmware still intentionally uses facilities shipped as part of the STM32duino framework:

| Facility | Where used | Reason to keep for now |
| --- | --- | --- |
| Arduino GPIO API | HAL only | Small, readable pin/control/output glue |
| `Wire` | OLED HAL, I2C build | Built-in STM32duino I2C transport |
| `SPI` | OLED HAL, SPI build | Built-in STM32duino SPI transport |
| `HardwareTimer` | timer HAL only | Current prerelease scheduler timebase |
| CMSIS / STM32 HAL underneath STM32duino | framework | MCU startup and peripheral support |

These are framework components, not separately versioned application libraries. Their upstream licenses still apply and are documented in `../THIRD_PARTY_NOTICES.md`.

## Display

### Removed

- Adafruit GFX
- Adafruit SSD1306
- external runtime font library dependency

### Project-owned replacement

The display HAL now provides:

- 128×64 1-bit framebuffer
- pixel, line, rectangle, and fill primitives
- project-owned 5×7 pixel glyphs
- native-resolution tempo raster glyphs derived from Roboto Condensed Bold (Apache-2.0)
- text measurement
- SSD1306/SSD1315 initialization and framebuffer transfer
- selectable I2C or SPI transport

The rest of the application does not know which bus is selected.

The compact 5×7 UI glyphs are project-owned. Tempo numeral raster data is derived from Roboto Condensed Bold and therefore remains subject to Apache-2.0. The repository does not distribute the source font file; it ships the Apache-2.0 text and attribution notice instead.

## Encoder and buttons

There is no external encoder/button library.

`ControlPanel` owns:

- active-low input handling
- internal pull-ups
- button debouncing
- quadrature Gray-code transition decoding
- debounced push-button states and encoder detents consumed by the UI controller

A dedicated encoder library would add another dependency without currently replacing meaningful complexity.

## Timer

`HardwareTimer` remains encapsulated by `PeriodicTimer` in HAL. It can therefore be replaced later by direct STM32 timer HAL/LL or register-level compare scheduling without changing the timing engine interface.

This is the dependency most likely to be reconsidered when the prerelease 20 kHz service timer is replaced by the final event/compare scheduler. Removing it now would provide little benefit and would mix the scheduler redesign with unrelated UI/configuration work.

## Could `Wire` and `SPI` also be removed?

Technically yes. The OLED HAL could call STM32Cube HAL/LL directly. Doing so now is not recommended:

- it would make bus initialization more MCU-specific
- it would duplicate stable STM32duino transport code
- it would not improve the upper-layer architecture
- display timing is not in the real-time gate path

If direct STM32 peripheral access becomes useful later, the existing HAL boundary allows that migration without touching renderers or application logic.

## Native simulator dependency

The optional desktop simulator uses **SDL3 3.4.16** for window creation, mouse/keyboard input, and 2D rendering. SDL is **not** linked into the STM32 firmware and therefore does not change the embedded runtime dependency graph.

CMake can either discover a system SDL3 package (`find_package(SDL3)`) or fetch the pinned `release-3.4.16` source for the simulator build. SDL3 is distributed under the zlib license; the upstream notice is retained in `third_party/LICENSE-SDL-zlib.txt`.

The simulator intentionally does not use SDL_ttf or another font library. Panel/developer labels use SDL3's built-in debug text, while the virtual OLED displays the firmware's actual 1-bit framebuffer.

## Architectural policy

CI rejects:

- new `lib_deps` without an explicit architecture change
- Adafruit dependencies
- framework hardware APIs outside HAL/pin mapping
- static UI labels outside `src/ui_text.h`

The goal is not dependency elimination at any cost. The goal is **small, deliberate dependencies behind replaceable boundaries**.

## License attribution

The project firmware is PolyForm Noncommercial 1.0.0, but framework/components are not relicensed under it. `THIRD_PARTY_NOTICES.md` identifies STM32duino/Arduino Core (LGPL-2.1-or-later where stated upstream), STM32CubeF4 HAL/LL (BSD-3-Clause), CMSIS (Apache-2.0), and Roboto Condensed Bold raster derivatives (Apache-2.0). Complete referenced license texts are bundled under `third_party/` and retained in source releases. None of those three license files is currently redundant. Tagged releases are source-only while LGPL-covered Arduino/STM32duino code remains statically linked; see `LICENSING.md` for the migration and binary-distribution policy.
