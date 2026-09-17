<!-- Author: Axel Napolitano -->
<!-- License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Dependency Analysis

## Current result

The firmware has **no third-party PlatformIO `lib_deps`** and no STM32duino/Arduino runtime dependency.

The embedded target uses:

| Component | Use | License |
| --- | --- | --- |
| STM32CubeF4 HAL/device support | GPIO, SYNC/RST EXTI, TIM4 encoder mode, TIM3/TIM5, SPI1, flash/startup, SYSCFG/PWR | BSD-3-Clause |
| CMSIS / CMSIS Device | Cortex-M4 and STM32F401 definitions | Apache-2.0 |
| GNU Arm Embedded Toolchain 7.2.1 runtime objects | `libgcc`, Newlib/Newlib-nano/libnosys as actually referenced by the link | GCC Runtime Library Exception and component-specific Newlib notices |

Project-owned HAL wrappers isolate those facilities from the timing engine, UI, domain model and persistence services. The exact runtime archives included in a tagged binary are recorded from each linker map in `BUILD-INFO.txt`; the corresponding license files are captured directly from the pinned PlatformIO toolchain package.

## Project-owned peripheral layer

`src/hal/platform_io.*` owns MCU initialization, VTOR/MSP setup, the 84 MHz system clock, GPIO, SYNC/RST EXTI, TIM4 hardware encoder mode, critical sections, the millisecond timebase and the free-running 1 MHz TIM5 microsecond counter.

`PeriodicTimer` configures TIM3 directly through STM32Cube for the scheduler. The production `OledDisplay` profile uses STM32Cube SPI1 directly; the older I2C1 transport remains compiled only in host regression variants. The application therefore no longer depends on Arduino GPIO, `Wire`, `SPI`, or `HardwareTimer`.

Host tests and the simulator retain small **project-owned compatibility shims** so production HAL behavior can be exercised deterministically on a desktop compiler. Those shims are test infrastructure and are not linked into STM32 firmware.

## Display

The display stack is project-owned: 128×64 framebuffer, drawing primitives, compact glyphs, SSD1306/SSD1315 protocol, SPI transport plus legacy I2C regression transport, and simulator controller model. Tempo numeral raster data remains attributed to Roboto Condensed Bold under Apache-2.0.

## Native simulator

SDL3 3.4.16 is an optional simulator-only dependency under the zlib license. It is not present in the embedded runtime graph.

## Architectural policy

CI rejects new `lib_deps`, framework hardware access outside HAL/pin mapping, and any return of the Arduino framework to the embedded environment. New runtime dependencies require an explicit architecture and licensing review.
