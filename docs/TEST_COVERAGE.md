<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Test Coverage — v0.19.0-beta.4

The prerelease test policy is repository-wide rather than limited to `clock_core`. `pio test -e native` exposes all 352 named Native cases, while `run_host_tests.py` remains the authoritative variant/sanitizer/coverage matrix.

## Hard CI gates

The following are mandatory:

- **Executable lines: >=95%**
- **Functions: >=95%**
- **Non-throw decision branches: >=90%**

The prerelease gates remain stringent without requiring synthetic tests for every defensive/error path. The coverage report still lists every uncovered production function and source decision so regressions remain visible during review.

Current repository-wide validated baseline (`0.19.0-beta.4`):

```text
Executable lines:   6269/6517 (96.19%)
Functions:           533/542  (98.34%)
Decision branches:  3741/4122 (90.76%)
Compiler branches:  3742/4796 (78.02%, informational only)
```

## Full host matrix

`scripts/run_host_tests.py` builds and executes seventeen coverage inputs:

1. exhaustive deterministic `clock_core` tests
2. focused realtime timing/SYNC/RST tests
3. realtime tests through configured external GPIO IRQ pins
4. atomic external-SYNC/input-front-end behavioral tests
5. atomic swing and observed engine-edge tests
6. deterministic One Clock Humanize tests
7. Tap Tempo estimator edge-case tests
8. rotary encoder and button HAL behavior tests
9. atomic Settings editor and boundary tests
10. screensaver renderer/state tests
11. Easter-egg launch/reset/control-state tests
12. complete firmware with I2C OLED auto-probe
13. complete firmware with SPI SSD1306 transport
14. complete firmware with SPI SSD1315 transport
15. complete firmware with custom SPI wiring
16. complete firmware with fixed I2C address and configured OLED reset pin
17. the PlatformIO native-smoke entry point

The complete firmware variants compile the real production `.cpp` files against deterministic host fakes for Arduino GPIO/time, Wire, SPI and HardwareTimer. This tests firmware logic and HAL contracts without pretending to measure real electrical timing.

## Areas executed by host tests

- application boot lifecycle, safe display-failure path, scheduler callback and Arduino `setup()`/`loop()` glue
- all `ClockEngine` public methods and OFF/CLOCK/EUC/SEQ behavior
- pause/stop, external-loss STOP/FREE behavior, bar progression/wrap and GLOBAL/FREE reset handling
- all settings mutation pages and sequencer commands, including invalid-index boundary handling; a dedicated 52-case Settings suite also locks down min/max coupling, enum boundaries and cross-setting invariants
- tap tempo and all factory templates
- all menu pages, labels, compact formatters and localization fallback
- performance, pictographic 2×4 channel overview, six-function 2×3 mode palette, settings, Clock/Plug/Heartbeat/Acid/Spectrum/Field/Fractal/Orbit screensaver effects, preset overwrite/name-band, templates and sequencer render paths
- I2C auto-probe success/fallback/failure, fixed-address mode, deferred 24-byte-chunk refresh, NACK retry/latest-frame-wins behavior, SPI transport and optional reset-pin path
- framebuffer text/primitives, all font roles, pixel clipping, dirty-page updates, unchanged-frame suppression and black/white drawing
- interrupt-driven encoder quadrature accumulation across foreground/display stalls and button debounce edges
- gate-output enable/disable and channel bounds
- periodic timer, interrupt lock and system clock wrappers
- complete deterministic timing/pattern core invariants
- sixteenth-note base-rate semantics for Euclid/Sequencer (`x1`: 16 steps per 4/4 bar)
- complete CURRENT-state persistence plus CRC/schema validation and 8 named user preset slots, including screensaver mode/start/dim/off preferences
- schema-v3/v4/v5 to schema-v6 CURRENT/preset migration, corrupt-record isolation, preset-name validation, and preservation of unrelated NVM bytes
- external-sync regression cases for 20/120/999 BPM, 1/2/4/24 PPQN, jitter, adaptive lock timeout, queue overflow/reacquisition, timestamp wraparound, effective-tempo gate release, monotonic scheduler time, phase alignment, ISR-safe pulse entry, and PPQN interpretation
- atomic external-SYNC behavior at 1/20/30/60/120/240/300/600/900/999 BPM, strong deterministic jitter, alternating/chaotic periods, abrupt tempo changes, duplicate timestamps, glitch storms, runtime PPQN/edge changes, lock-loss policies and continuity recovery
- continuous external-tempo movement including 60->180 and 180->60 ramps, slow drift, repeated tempo steps, ramp-plus-jitter and 24-PPQN acceleration; permanently asserted SYNC is checked for one-edge behavior and natural lock timeout rather than a synthetic pulse train
- dedicated Swing edge-train tests, One Clock Humanize bounds/repeatability/mode isolation, Tap Tempo rolling-estimator boundaries, fast-turn encoder backlog/saturation behavior, and atomic button debounce/queue behavior
- nominal LM393-front-end design-model checks for the documented resistor/hysteresis network, including end-to-end modeled 2.5 V, 3 V and 4 V clocks at low/nominal/maximum supported tempo; this remains explicitly separate from physical comparator HIL
- ISR-safe external reset with configurable rising-edge TRIGGER or level-sensitive GATE semantics, startup-HIGH handling, queue saturation, deterministic RST-over-SYNC priority, and PLAY-transport preservation
- fastest legal scheduler-rate/swing behavior and immediate full-state OFF/MUTE gate release
- channel overview selection, TAP-turn six-function palette, TAP+encoder Settings chord, removed long-press behavior, OFF/Unified/Divider modes, deferred Flash commit, shared C/E/S epoch rescheduling, and preset overwrite/save/load/name-band workflow
- 1–999 BPM master accumulation, 60-second Tap Tempo input, all screensaver renderers/state sweeps, Pixel Raid gameplay model, Formula 1, Breakout, Egg Journey and Beatknecht launch/reset/control paths, game-only exit gestures, shared game-specific retro intro/initials/scrollable Top-100 flow, independent persistent Pixel Raid/Formula 1/Breakout/Egg Journey leaderboards with legacy single-score fallback, and gate-buffer-disabled LED life-loss effects

## Decision branches versus compiler branches

The source-decision branch gate is **90% minimum**. Compiler-generated throw/exception edges remain informational; source-level decisions are the quality metric used for the hard gate.

GCC also emits synthetic exception/throw control-flow edges around ordinary C++ calls. Those edges do not correspond to source-level decisions and are therefore reported separately, not hidden; the current raw compiler-branch baseline is shown above.

This distinction keeps the metric meaningful: source decisions are measured separately from compiler-generated exception plumbing. Function coverage has a 95% hard floor. Uncovered functions are still emitted explicitly by the test report, so deliberate defensive code does not disappear behind the aggregate percentage.

## Sanitizer gate

The host matrix is also compiled and executed with:

```text
-fsanitize=address,undefined
-fno-omit-frame-pointer
-Werror
```

ASan/UBSan runs cover the deterministic core and complete I2C, SPI, and fixed-I2C/reset-pin firmware variants. Any memory-safety or undefined-behavior finding fails CI.


## Native simulator integration

The desktop simulator has a separate CMake/CTest integration layer in `sim/`. It is intentionally not folded into the firmware gcov percentages above because it is a second hardware-platform implementation rather than STM32 production firmware.

Run the SDL-free simulator integration tests with:

```bash
cmake --preset simulator-headless
cmake --build --preset simulator-headless
ctest --preset simulator-headless
```

These tests boot the complete firmware above the simulator HAL, exercise real debounced controls and scheduler callbacks, verify OLED output and gate edges, verify host-file persistence, power-cycle the application, enter the configured boot Easter egg through the real boot chord, check framebuffer-visible projectiles, drive ideal SYNC/RST comparator sources, and verify scope reset/re-arm semantics. CI executes this layer on Windows, macOS, and Linux.

## Hardware-in-the-loop remains mandatory

Host execution cannot establish real-world timing quality. Planned HIL/electrical tests still include:

- gate jitter and pulse width at the actual jacks
- no false output pulses during boot/reset/preset changes
- HCT244 output-enable behavior
- encoder/button electrical bounce characteristics
- SYNC/RST comparator thresholds, hysteresis, slow-crossing chatter and glitch rejection
- external-SYNC capture latency/jitter and comparator behavior; move to timer Input Capture only if the measured EXTI path misses the V1 requirement
- External Reset capture/IRQ timing and GLOBAL/FREE phase behavior at the physical jacks
- I2C/SPI signal integrity and display-stress timing on the final PCB

Coverage proves that code paths execute; HIL proves that the hardware behaves correctly. The concrete bench procedure is maintained in [`HIL_TEST_PLAN.md`](HIL_TEST_PLAN.md).
