<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK 0.19.0-beta.14

CLOCK is an eight-output Eurorack master clock, divider, Euclidean rhythm source, and gate sequencer for the STM32F401 Black Pill. Beta.14 concentrates on hardware compatibility after the STM32Cube migration, safer firmware updating, deterministic 180-degree display mounting, stronger encoder regression coverage, and making the default BEATKNECHT rhythm Easter egg behave like a real performance tool.

## Highlights

- **Proper BEATKNECHT transport:** PLAY starts the rhythm and toggles PLAY/PAUSE; STOP/BACK stops immediately, resets to step 1, forces all gates LOW, and disables the output stage until PLAY is pressed again.
- **Guarded BEATKNECHT exit:** a long encoder push opens `EXIT GAME? / NO / YES`. The prompt silences active gates; NO restores the previous transport state and YES exits safely with the output stage disabled.
- **Reliable upside-down mounting:** `ORIENTATION = 180 DEG` now rotates the outgoing framebuffer in firmware while the OLED controller stays in its proven scan orientation. This removes the mirrored-text behavior seen with controller remapping.
- **STM32Cube hardware compatibility:** startup order, MSP/SYSCFG/PWR setup, SPI1 configuration, vector-table handling, linker memory layout, pinned GCC 7.2.1/C++17, and persistence-preserving DFU sequencing were tightened after real ARM/hardware testing.
- **Encoder hardening:** PA0/PA1 now use STM32 TIM2 encoder mode, with detent-phase resynchronization and expanded regression cases for first-detent, reversal, missing/extra transitions, counter wraparound, reversed direction, and fast-turn behavior. Physical confirmation of the remaining first-detent hardware observation is still part of beta qualification and is not claimed by host tests alone.
- **392 named Native tests:** the public suite now includes 49 control tests and 36 Easter-egg tests, plus sanitizer, simulator, documentation, architecture, and dependency gates.

## Firmware updates

Routine USB updates use CLOCK's persistence-preserving PlatformIO upload path. Disconnect Eurorack power - preferably unplug the Eurorack ribbon cable - before connecting USB. ST-LINK/SWD remains the recommended first-install/recovery path. See [`../../FIRMWARE_UPDATE.md`](../../FIRMWARE_UPDATE.md).

## Compatibility

The persistence schema, preset format, panel wiring, and persistence-safe release-image format remain compatible. The display orientation implementation changed internally but keeps the same `0 DEG / 180 DEG` user setting. The encoder backend changed from GPIO-edge delivery to TIM2 hardware quadrature counting while preserving the same physical PA0/PA1 wiring and `NORMAL / REVERSED` user option.
