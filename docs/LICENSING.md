<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Licensing and Distribution

This document records the engineering-side licensing assessment for the current prerelease firmware. It is not legal advice and does not replace review of the upstream license texts.

## Project-owned firmware

Project-owned source code is licensed under **PolyForm Noncommercial License 1.0.0**. The Required Notice is:

```text
Required Notice: Copyright © 2026 Axel Napolitano.
```

PolyForm Noncommercial applies only to project-owned material. It does not relicense third-party framework code, HAL/CMSIS components, or font-derived raster data.

## Why all three bundled third-party license files are relevant

The current PlatformIO target uses `framework = arduino` with STM32duino. The resulting firmware therefore contains or links code under more than one upstream license.

| Bundled license | Current reason | Remove now? |
| --- | --- | --- |
| LGPL-2.1 | Arduino/STM32duino-derived core and library portions used by the build, including Wiring/Wire-style code | **No** |
| BSD-3-Clause | STM32F4 HAL/LL components | **No** |
| Apache-2.0 | CMSIS/CMSIS Device and the Roboto Condensed Bold-derived tempo raster glyphs | **No** |

Accordingly, none of the license files under `third_party/` is redundant in the present architecture.

## Coexistence with PolyForm Noncommercial

BSD-3-Clause and Apache-2.0 components can be distributed alongside PolyForm-licensed application code while retaining their own notices and license terms. They are not converted to PolyForm Noncommercial.

The LGPL-covered STM32duino/Arduino portions require more care. LGPL 2.1 permits a larger work to use the library under terms of the application's choice, but distribution of a statically linked executable creates additional obligations intended to let recipients modify/relink the LGPL-covered library and debug those modifications.

Therefore:

- the **source tree** can contain PolyForm-licensed application code alongside these third-party components;
- the third-party notices and license texts must remain visible;
- **shipping a statically linked firmware BIN/ELF requires a deliberate LGPL compliance method**; merely adding `LICENSE-LGPL-2.1.txt` is not by itself a complete binary-distribution strategy.

Before a stable binary release, the project must either document/package an appropriate LGPL relinking/source mechanism for the exact STM32duino version used, or remove the LGPL dependency by migrating the remaining framework boundary to components with suitable permissive licenses (for example direct STM32Cube HAL/LL where technically appropriate).

## Roboto Condensed raster glyphs

The firmware does not ship a font file. It embeds only the native-resolution 1-bit numeral/dash raster data needed by the OLED UI. Those raster glyphs are derived from Roboto Condensed Bold and retain the Apache-2.0 attribution in `THIRD_PARTY_NOTICES.md`; the Apache-2.0 text remains bundled.

## Release policy while STM32duino remains in the target

The repository keeps the third-party notices and complete referenced license texts in every source bundle. **GitHub releases are source-only while the firmware statically links LGPL-covered STM32duino code.** CI still compiles both I2C and SPI targets, but it deliberately does not upload or attach the resulting BIN/ELF files.

`scripts/package_release.py` is likewise fail-closed: static binary packaging requires the explicit `--acknowledge-lgpl-static-link` flag and is intended only for a separately compliance-reviewed distribution.

This removes accidental binary redistribution from the normal prerelease workflow. The preferred long-term engineering solution is to migrate the remaining runtime boundary from `framework = arduino` to direct STM32Cube HAL/LL + CMSIS. Once that migration is complete and no LGPL-covered runtime code is linked, the LGPL third-party license can be removed from the runtime distribution set and ordinary BIN/ELF publication can be re-enabled.

Until then, developers build and flash firmware locally from source.

## Native simulator

The optional desktop simulator links SDL3 under the zlib license. SDL is not linked into the STM32 firmware, does not alter the PolyForm Noncommercial licensing of project-owned simulator code, and does not add embedded-runtime obligations. The SDL notice is retained in `THIRD_PARTY_NOTICES.md` and `third_party/LICENSE-SDL-zlib.txt`.
