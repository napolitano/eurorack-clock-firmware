<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Licensing and Distribution

This document records the engineering-side licensing assessment for the current firmware. It is not legal advice.

## Project-owned firmware

Project-owned source code is licensed under **PolyForm Noncommercial License 1.0.0**. Required Notice:

```text
Required Notice: Copyright © 2026 Axel Napolitano.
```

## Embedded runtime dependency set

The STM32F401 target now uses PlatformIO `framework = stm32cube`; STM32duino/Arduino Core is not part of the embedded runtime graph.

| Component | License | Runtime role |
| --- | --- | --- |
| Project-owned CLOCK firmware | PolyForm Noncommercial 1.0.0 | Application, engine, UI, HAL wrappers |
| STM32CubeF4 HAL / device support | BSD-3-Clause | MCU startup and peripheral drivers |
| CMSIS / CMSIS Device | Apache-2.0 | Cortex-M4 and device definitions |
| Roboto Condensed Bold raster derivatives | Apache-2.0 | Tempo numeral bitmap data |
| GNU Arm Embedded Toolchain runtime objects | GCC Runtime Library Exception and component-specific Newlib notices | Runtime support actually pulled into the linked image |

The former LGPL-2.1 STM32duino dependency has been removed. `third_party/LICENSE-LGPL-2.1.txt` is therefore no longer part of the repository or release distribution.

## Binary releases

There is no longer an LGPL static-link blocker for CLOCK firmware images. Tagged binary releases are allowed only through the release workflow, which treats licensing material as part of the artifact contract. Every release must contain the project license and Required Notice, manual license, third-party notices, BSD-3-Clause and Apache-2.0 texts, the exact license capture from the pinned GNU Arm Embedded Toolchain 7.2.1 package, and linker-map-derived `BUILD-INFO.txt`.

The release workflow builds every supported Easter-egg flavor independently and creates both SHA-256 and MD5 manifests over the complete payload. Packaging fails if any required binary, manual, notice, license file, toolchain license archive, or build-provenance file is missing.

CI must fail if the embedded PlatformIO environment returns to `framework = arduino`, introduces `framework-arduinoststm32`, or otherwise adds an unreviewed runtime dependency.

## Roboto Condensed raster glyphs

The firmware embeds only the native-resolution 1-bit numeral/dash raster data required by the OLED UI. The repository does not distribute a font file. Apache-2.0 attribution remains in `THIRD_PARTY_NOTICES.md`.

## Native simulator

The optional desktop simulator links SDL3 under the zlib license. SDL is not linked into STM32 firmware and does not alter the embedded runtime license set.
