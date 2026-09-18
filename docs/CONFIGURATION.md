<!-- Author: Axel Napolitano -->
<!-- License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Firmware Configuration

The stable firmware deliberately keeps the most frequently edited compile-time configuration at the top level of `src/`.

## `src/config.h`

Use this file for firmware policy and hardware-independent compile-time behavior:

- active UI language
- display transport (final hardware: `Spi`; legacy I2C path retained for host regression)
- display geometry, address, normal/dim contrast, and bus speeds
- boot duration and refresh interval
- scheduler frequency
- UI refresh, encoder long-push threshold, and screensaver frame timings
- compile-time boot Easter egg (`CLOCK_EASTER_EGG`)
- hard technical BPM range (currently 1–999; distinct from user factory limits)

The final CLOCK hardware supports **SSD1306** and **SSD1315** 128×64 controllers over 4-wire SPI. The legacy I2C transport remains in the HAL only for regression/reference builds; PB6/PB7 are now dedicated to the encoder.

Current final-hardware/default build:

```text
controller  SSD1306
transport   4-wire SPI
CLK         PA5
DIN         PA7  (SPI MOSI; labeled SDA on the earlier prototype module)
CS          PA4
DC          PB9
RES         PB15
SPI clock   1 MHz
```

The display-side labels are authoritative for wiring the target module: **GND, VCC, CLK, DIN, RES, DC, CS**. The earlier prototype OLED used **SCL/SDA/RES/DC/CS** silk-screen labels even in SPI mode; on that module SCL was the SPI clock and SDA was the SPI data input. This does not indicate I2C operation.

PlatformIO provides the two production controller profiles:

```bash
pio run -e blackpill_f401cc_spi_ssd1315
pio run -e blackpill_f401cc_spi_ssd1306
```

`blackpill_f401cc_spi` remains a compatibility alias for the SSD1306 SPI profile. `default_envs` selects the SPI SSD1306 reference build. The controller can also be selected directly with `CLOCK_DISPLAY_CONTROLLER=1306` or `1315`.

### Boot Easter egg

`CLOCK_EASTER_EGG` is a compile-time selector and is deliberately not exposed in the normal UI. The shipped/default build uses **BEATKNECHT (`5`)**:

| Value | Game | Controls |
| --- | --- | --- |
| `1` | Pixel Raid | encoder move, TAP fire; score + independent Top 100 |
| `2` | Formula 1 | encoder steer; automatic speed/crash recovery; score + independent Top 100 |
| `3` | Breakout | encoder paddle, TAP launch; variable angles/modifiers, three lives; score + independent Top 100 |
| `4` | Egg Journey | auto-scrolling parallax terrain; encoder forward/backward, TAP jump/retry; three lives/progression; score + independent Top 100 |
| `5` **(default)** | BEATKNECHT | PLAY/PAUSE controls rhythm transport; STOP resets to step 1; TAP cycles styles; encoder changes BPM; long encoder push opens a guarded NO/YES exit confirmation; eight curated 16-step gate patterns drive OUT 1–8; no leaderboard |

The boot chord is unchanged: hold encoder push throughout the boot screen. Every selection opens with an individual retro intro and there is no automatic timeout into gameplay. The four ranked games start with TAP or encoder PUSH; BEATKNECHT starts with PLAY so TAP remains exclusively its style control. Ranked arcade games route final scores through the shared initials/scrollable Top-100 flow; BACK from the ranking starts a new run and a long encoder hold exits. Existing single-score prerelease records are retained as migration fallbacks. All modes run before the normal clock scheduler starts. The arcade games keep all gate source GPIOs muted; BEATKNECHT intentionally allows gate output only after PLAY leaves its intro. PLAY/PAUSE then controls transport, STOP resets phase and mutes all gate sources, and a long encoder push opens a NO/YES confirmation. The prompt silences active gates; NO restores the previous transport state, while YES exits with every output LOW and gate output muted.

## `src/defaults.h`

Use this file for the user-visible factory state:

- master BPM, user MIN/MAX BPM limits, and meter
- initial transport and clock source
- factory operating mode (`ONE CLOCK`) and channel-mode palette order
- default channel mode/rate
- swing, probability, gate length, phase, reset mode, mute, and One Clock Humanize default
- CLOCK meter
- EUCLID steps/hits/rotation
- SEQ length/rotation/pattern
- external-sync defaults
- screensaver mode and STOP inactivity thresholds

No factory-setting value should be duplicated in renderer or engine code.

Factory mode is **ONE CLOCK**. The UI palette order is **ONE CLOCK, DIVIDER, CLOCK, EUCLID, SEQUENCER, OFF**. Gate-length choices are `1/2/5/10/20/50/100 ms`; the factory value is `10 ms`. External clock source defaults to `AUTO`; SYNC period smoothing defaults to `LOW` (75% new period / 25% previous), with `OFF`, `MEDIUM`, and `FULL` also available. The microsecond `FILTER` remains a separate glitch-rejection setting.

## Gate-output electrical contract

The current hardware target is **0 V LOW / nominal +5 V HIGH** at OUT 1–8. The gate buffer translates the MCU logic domain to the 5 V output domain. The final MCU pin map has no dedicated `/OE` control line, so firmware safety is enforced by forcing every source GPIO LOW whenever logical gate output is disabled. +10 V is intentionally not a supported output level in this architecture; adding it would require a separate higher-voltage driver/level-shifter stage plus a renewed protection and load-current review.

## Analog/CV product boundary

Hardware Rev 1 deliberately stops at digital timing I/O: eight 0/+5 V gate/trigger outputs plus dedicated conditioned SYNC and RST inputs. It does **not** add general parameter-CV inputs, analog CV/modulation outputs, or a general modulation matrix.

For analog modulation outputs, the project quality target would require an eight-channel 16-bit DAC-class path and two quad output-op-amp stages. A 12-bit MCP-class DAC is not accepted as a quality shortcut for this use. The current project estimate is roughly EUR 30-40 extra BOM cost before the additional fine-pitch SMD assembly, PCB routing, calibration, and validation effort. General CV inputs would add separate analog input-conditioning/routing and panel-I/O costs.

This boundary is intentional for the first hardware generation. Firmware-side cross-channel interaction may be expanded later using internal digital events without changing the analog BOM. See [`ROADMAP.md`](ROADMAP.md) and [`V1_FORWARD_COMPATIBILITY.md`](V1_FORWARD_COMPATIBILITY.md).

## `src/pin_map.h`

Use this file when hardware wiring changes. Names intentionally describe the connected circuit/front-panel function rather than the MCU peripheral.

Current mappings include. Gate/LED channels are also exposed as individually named constants (`kChannel1GateLedPin` through `kChannel8GateLedPin`) before being assembled into the ordered driver array:

| Function | STM32 pin |
| --- | --- |
| PLAY/PAUSE button | PA1 |
| RESET/BACK button | PA2 |
| TAP TEMPO button | PA3 |
| OLED SPI CS | PA4 |
| OLED clock (`CLK`; `SCL` on the prototype module) | PA5 |
| OLED data (`DIN`; SPI MOSI; `SDA` on the prototype module) | PA7 |
| SYNC comparator input | PA8 |
| RST comparator input | PA9 |
| Gate/LED channel 1 | PB0 |
| Gate/LED channel 2 | PB1 |
| Gate/LED channel 3 | PB2 |
| Gate/LED channel 4 | PB5 |
| Encoder phase A / CLK | PB6 (TIM4_CH1) |
| Encoder phase B / DT | PB7 (TIM4_CH2) |
| Gate/LED channel 5 | PB8 |
| OLED data/command (`DC`) | PB9 |
| Gate/LED channel 6 | PB10 |
| Gate/LED channel 7 | PB12 |
| Gate/LED channel 8 | PB13 |
| Encoder push | PB14 |
| OLED reset (`RES`) | PB15 |
| Gate-buffer `/OE` | not MCU-controlled in the final pin map |

SPI display signals remain independently overridable from PlatformIO build flags:

| Build macro | Default |
| --- | --- |
| `CLOCK_DISPLAY_SPI_SCK_PIN` | `PA5` |
| `CLOCK_DISPLAY_SPI_MOSI_PIN` | `PA7` |
| `CLOCK_DISPLAY_SPI_CS_PIN` | `PA4` |
| `CLOCK_DISPLAY_SPI_DC_PIN` | `PB9` |
| `CLOCK_DISPLAY_RESET_PIN` | `PB15` |
| `CLOCK_DISPLAY_SPI_FREQUENCY_HZ` | `1000000` |

A board revision can therefore remap the OLED without editing source code, for example:

```ini
[env:my_clock_spi]
extends = env:blackpill_f401cc_spi_ssd1315
build_flags =
    ${env:blackpill_f401cc_spi_ssd1315.build_flags}
    -DCLOCK_DISPLAY_SPI_CS_PIN=clockfw::mcu::PB0
    -DCLOCK_DISPLAY_SPI_DC_PIN=clockfw::mcu::PB1
    -DCLOCK_DISPLAY_RESET_PIN=clockfw::mcu::PB5
```

Use CLOCK's framework-independent encoded pin constants (`clockfw::mcu::PA7`, `clockfw::mcu::PB9`, etc.) for these macros. The STM32Cube HAL maps those identifiers to the selected GPIO/alternate-function configuration.

The SPI bus is deliberately limited to **1 MHz** for prototype/first-PCB bring-up margin. Both supported controller families permit substantially faster serial operation, but the prototype wiring is not treated as a controlled-impedance PCB interconnect.

The final comparator outputs are fixed at PA8 for SYNC and PA9 for RST. Jack-detect contacts remain unassigned in firmware unless a later PCB revision routes dedicated detect signals.

## `src/ui_text.h`

All static user-visible firmware strings are stored here.

UI and domain code use `TextId` rather than embedding labels. The current catalog is English (US). The language selector already distinguishes `EnglishUs` and `GermanDe`; until a complete German catalog exists, unsupported catalogs fall back to English rather than mixing partial translations.

This arrangement means adding another language does not require changes to screen layout/control logic unless translated text exceeds the established pixel budget.


## Persistent configuration

Persistent user state is not configured by scattered constants. `PersistentStateService` stores one autosaved `CURRENT` state plus eight named user preset slots. Each state includes master settings, sync configuration (including the selected smoothing level), display/screensaver preferences, every channel's common settings, CLOCK meter data, Euclid parameters, and the complete 64-bit Sequencer patterns. The stable write format is schema v8; schema v7 and earlier supported formats are migrated losslessly, with pre-v8 records receiving the `LOW` smoothing default.

The current logical persistence image is 8192 bytes. The lower 4096 bytes preserve the established settings/preset and legacy-score layout; the upper half stores the four independent arcade Top-100 tables. Valid 4096-byte A/B generations from earlier prereleases remain readable and are promoted to 8192 bytes on the first subsequent write. On STM32F401 it is committed into two independent 16 KiB A/B slots in Flash sectors 1 and 2. A complete image is written to the inactive slot, CRC-verified, and made valid by programming its commit marker last. The previous generation remains untouched until the new one is durable, so a power loss during save cannot destroy the last valid state.

The persistent image has a hard architectural ceiling of 12 KiB, leaving at least 4 KiB physical headroom inside either slot. Firmware owns sector 0 plus sectors 3..5, for 224 KiB total Flash. `scripts/platformio_memory_gate.py` enforces the additional 90% Flash/RAM headroom policy after linking.

Preset names are limited to 16 display-supported characters. Storage records are schema-versioned and CRC-32 protected; do not serialize raw C++ structs into Flash.
