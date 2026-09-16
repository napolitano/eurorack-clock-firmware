<!-- Author: Axel Napolitano -->
<!-- License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Dependency Analysis

## Current result

The firmware has **no third-party PlatformIO `lib_deps`** and no STM32duino/Arduino runtime dependency.

The embedded target uses:

| Component | Use | License |
| --- | --- | --- |
| STM32CubeF4 HAL/device support | GPIO, EXTI, TIM3/TIM5, I2C1, SPI1, flash/startup | BSD-3-Clause |
| CMSIS / CMSIS Device | Cortex-M4 and STM32F401 definitions | Apache-2.0 |

Project-owned HAL wrappers isolate those facilities from the timing engine, UI, domain model and persistence services.

## Project-owned peripheral layer

`src/hal/platform_io.*` owns MCU initialization, the 84 MHz system clock, GPIO, EXTI, critical sections, the millisecond timebase and the free-running 1 MHz TIM5 microsecond counter.

`PeriodicTimer` configures TIM3 directly through STM32Cube for the scheduler. `OledDisplay` uses STM32Cube I2C1 or SPI1 directly. The application therefore no longer depends on Arduino GPIO, `Wire`, `SPI`, or `HardwareTimer`.

Host tests and the simulator retain small **project-owned compatibility shims** so production HAL behavior can be exercised deterministically on a desktop compiler. Those shims are test infrastructure and are not linked into STM32 firmware.

## Display

The display stack is project-owned: 128×64 framebuffer, drawing primitives, compact glyphs, SSD1306/SSD1315 protocol, I2C/SPI transport scheduling, and simulator controller model. Tempo numeral raster data remains attributed to Roboto Condensed Bold under Apache-2.0.

## Native simulator

SDL3 3.4.16 is an optional simulator-only dependency under the zlib license. It is not present in the embedded runtime graph.

## Architectural policy

CI rejects new `lib_deps`, framework hardware access outside HAL/pin mapping, and any return of the Arduino framework to the embedded environment. New runtime dependencies require an explicit architecture and licensing review.
