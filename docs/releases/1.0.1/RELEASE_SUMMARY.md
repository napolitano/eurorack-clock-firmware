<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK 1.0.1

CLOCK 1.0.1 is the first maintenance release for the stable V1 firmware line. It fixes the remaining mismatch between external FREEWHEEL timing and the BPM shown on the Performance screen, and it makes manual screenshots reproducible release-build artifacts rather than trusted static images.

## Highlights

- **FREEWHEEL display now matches the engine:** when external SYNC stops with `LOSS = FREE`, the Performance screen keeps showing the last measured external BPM instead of jumping to the internal fallback value.
- **Fallback semantics remain explicit:** `LOSS = INTERNAL` still returns the display and engine to the configured internal BPM, while locked AUTO/EXTERNAL operation continues to show the measured external tempo.
- **Manual screenshots are rebuilt for every release:** the tagged source regenerates the deterministic OLED screenshot catalog and refreshes all generated figures and Screensaver/Easter-egg galleries before PDF export.
- **Screenshot publication is guarded against stale UI states:** the generator rejects degenerate captures, renders Independent-mode examples with the correct topology, and keeps the SYNC settings figure focused on `FILTER`, `SMOOTHING`, and `TIMEOUT`.

## Compatibility

CLOCK 1.0.1 is a drop-in maintenance update for 1.0.0. It does not change the hardware pin map, persistence schema v8, preset layout, saved CURRENT state, score data, or DFU persistence contract. Routine release DFU updates continue to preserve user settings and stored data.
