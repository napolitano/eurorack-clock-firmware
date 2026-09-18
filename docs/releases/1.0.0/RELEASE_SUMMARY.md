<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK 1.0.0

CLOCK 1.0.0 is the first stable firmware release for the eight-output STM32F401 Eurorack clock. It freezes the V1 feature set and incorporates the final hardware-bring-up corrections, with particular emphasis on making external SYNC behavior match what the panel reports.

## Highlights

- **External SYNC is now end-to-end coherent:** the first pulse starts acquisition, the second valid pulse establishes BPM and lock, and a newly acquired external clock can start transport automatically without overriding an explicit user STOP or PAUSE.
- **Loss policies now do what they say:** STOP stops transport, FREE continues at the last measured external tempo, and INTERNAL continues at the configured fallback BPM. A sync-loss STOP can restart on a valid reacquisition; a manual stop cannot.
- **The Performance display shows the real external BPM while locked** in AUTO or EXTERNAL instead of continuing to display the internal fallback value.
- **Factory behavior remains performance-safe:** SOURCE defaults to AUTO, Tap Tempo changes only the fallback BPM, RST remains phase-only, and external SYNC/RST activity wakes the display.
- **Final hardware documentation is aligned with the build:** the production pin map, SPI OLED contract, TIM4 encoder path, and 74HCT541-class output buffer are reflected in current documentation.
- **407 named Native tests** now include 90 focused external-SYNC cases and 29 complete host-firmware cases; the sanitizer matrix and both headless simulator tests pass, with repository coverage at 96.39% lines / 98.36% functions / 90.03% source decision branches.

## Firmware updates

Routine USB updates use CLOCK's persistence-preserving DfuSe release images and leave the A/B persistence sectors untouched. Disconnect Eurorack power before connecting USB. ST-LINK/SWD remains the recommended first-install and recovery path.

## Compatibility

The 1.0.0 release keeps the established persistence schema, eight named presets, Top-100 arcade data, final production GPIO map, and persistence-safe DFU layout. Existing stored clock-source settings are intentionally preserved across firmware updates; a unit that previously stored `INTERNAL` remains internal until changed by the user or restored through Factory Reset.
