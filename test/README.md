<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Test automation

## Native PlatformIO suite

```bash
pio test -e native
```

This is the developer-facing Native entry point. The suite deliberately makes timing, musical behavior, persistence, UI, and physical-control contracts visible instead of presenting only the 44-case mathematical core. PlatformIO executes eleven conventionally named suites:

| Suite | Named cases | What it proves |
| --- | ---: | --- |
| `test_clock_core` | 44 | hardware-independent clock mathematics and exhaustive invariants |
| `test_realtime` | 28 | deterministic scheduler/GPIO/SYNC/RST integration and stress behavior |
| `test_sync_behavior` | 116 | exact/changing external clocks plus configurable INPUT 1/2 roles (SYNC/RESET/RUN/START/STOP/RESTART/TAP), role reassignment, RUN baselining, jitter/glitches/loss, PPQN/edge semantics and nominal analogue-front-end behavior |
| `test_swing` | 39 | Swing/Groove mathematics, exhaustive Custom-Groove sweeps, TAP Recorder capture/Count-In/One-Shot/Endless behavior, and observable engine timing invariants |
| `test_humanize` | 12 | deterministic One Clock timing displacement, bounds, repeatability and isolation |
| `test_tap_tempo` | 24 | tap estimator acquisition, rolling average, clamps, invalid intervals, jitter and timestamp wrap |
| `test_controls` | 51 | TIM4 encoder quadrature, detent-phase recovery after missed/coalesced transitions and encoder-push phase shifts, first-detent and immediate direction-reversal behavior, NORMAL/REVERSED direction mapping, fast-turn backlog/drain and saturation, debounce, bounce, hold and simultaneous button behavior |
| `test_settings` | 88 | settings boundaries, configurable inputs, grouped channel settings, Groove Editor/Recorder menus, Custom Groove slot management, Info overflow/popover, device preferences and invalid-input invariants |
| `test_screensavers` | 19 | all screensaver renderers, deterministic/rewind behavior and long frame sweeps |
| `test_easter_eggs` | 36 | intro launch gating, reset state, output safety and game-specific controls |
| `test_host_firmware` | 39 | complete firmware/UI/HAL/persistence behavior, Groove Editor/Record workflows, schema migrations and framebuffer/navigation contracts using deterministic host fakes |
| **Total** | **496** | public Native inventory |

The default Native configuration executes more than **433,000 assertions**. `scripts/check_test_inventory.py` enforces the visible-suite inventory contract so a broad test-count increase cannot hide a regression in one focused suite.

### Swing, Groove, Recorder, and Humanize behavior

`test_swing` verifies classic Swing plus the deterministic Groove layer used by Clock, Euclid, Sequencer and One Clock. It sweeps Custom Groove lengths 1-64 and Amount 0-100%, exercises representative BPM/rate/grid combinations, signed early/late offsets, rotation, bounds and Swing interaction, and covers Groove Record Count-In, One-Shot/Endless wrap, targeted overwrite and shared draft behavior. Event order and gate-off safety remain bounded by the same scheduler invariants as straight timing.

`test_humanize` treats Humanize as a separate timing contract. OFF must keep all eight One Clock outputs coincident; 250/500/1000/2000 us settings must remain inside their configured scheduler-quantized windows; a fresh run must reproduce the same offsets; outputs must exhibit channel spread; and a stored One Clock Humanize value must have no timing effect in Independent mode. Humanize combined with maximum current Swing is also exercised for ordering safety.

### Tap Tempo behavior

`test_tap_tempo` exposes the estimator independently of the broad UI integration scenarios. It verifies 30/60/120/240 BPM-class intervals, the 999 BPM ceiling, the MIN-BPM-derived sequence timeout, exact slowest-valid boundaries, timestamp zero, reset behavior, too-fast/duplicate intervals, rolling four-interval averaging, small jitter, alternating fast/slow taps and unsigned timestamp wrap. The host-firmware suite separately verifies that the first TAP is visually silent, the second and following TAPs restart the four-frame 8×8 shrinking indicator, and a timeout returns the UI to first-tap state.

### Encoder and buttons

`test_controls` drives the production `ControlPanel` against GPIO fakes. It verifies both Gray-code directions, device-level NORMAL/REVERSED semantic mapping, partial cycles, contact bounce, six detents accumulated while foreground polling is absent, 1000-detent fast turns in either direction, repeated burst/drain cycles, deliberate signed backlog saturation without wrap, the +/-127 report bound with remainder preservation, idle sampling without gratuitous global interrupt masking, active-low button press/release debounce, no hold-repeat, bounce restart, simultaneous buttons and encoder activity during button debounce. The broader host-firmware suite continues to test how these samples drive the UI state machine.

### External-SYNC behavior suite

`test_sync_behavior` intentionally targets the failure modes most likely to matter musically:

- exact 1, 20, 30, 60, 120, 240, 300, 600, 900 and 999 BPM boundaries;
- 1/2/4/24 PPQN interpretation and rising/falling-edge selection;
- duplicate timestamps, sub-filter glitches, glitch storms and opposite-polarity noise;
- deterministic jitter from +/-50 us through +/-20%, alternating 400/600 ms and 250/750 ms periods, and chaotic-but-valid periods;
- **continuous tempo movement**: linear 60->180 and 180->60 BPM ramps, slow drift around 120 BPM, repeated tempo steps, acceleration with alternating jitter and a 24-PPQN tempo ramp;
- abrupt 60<->180 BPM changes, adaptive timeout boundaries and STOP/FREEWHEEL/INTERNAL/AUTO loss behavior;
- timestamp wrap, queue-overflow continuity and explicit lock reacquisition;
- a permanently HIGH SYNC input: one selected edge only, natural timeout after a previous lock, and no manufactured falling-edge pulse train;
- runtime PPQN and selected-edge changes, including the beta.4 regression that previously allowed an old period filter to contaminate a newly interpreted clock.

The suite also contains a **host-only nominal electrical model** of the documented Rev-1 LM393 input resistor network. It calculates the nominal ~1.79 V rising and ~1.46 V falling thresholds and drives the production digital SYNC estimator from modelled 2.5 V, 3 V and 4 V inputs at 20, 120 and 999 BPM. This validates intended nominal design behavior and hysteresis logic; resistor tolerance, clamp-diode behavior, LM393 common-mode/propagation effects, noise, PCB parasitics and actual switching thresholds remain physical HIL requirements.

### Settings, screensavers and Easter eggs

`test_settings` turns the Settings editor into an explicit behavioral contract rather than relying only on broad UI scenarios. It covers master tempo/min/max coupling, meter and Pre-Count limits, configurable INPUT 1/2 roles and exclusivity, external timing settings, persistent encoder direction/OLED orientation, screensaver timing invariants, grouped channel settings, rate/Swing/Groove/output limits, Euclid and Sequencer bounds, Custom Groove Editor/Recorder menus, 10-slot load/rename/delete flows, long read-only Info truncation/popover behavior, One Clock Humanize choices and Divider Bank settings.

`test_screensavers` executes every renderer, checks expected framebuffer activity, deterministic/rewind behavior where applicable, and sweeps frames long enough to exercise stateful animations. `test_easter_eggs` verifies the shipped BEATKNECHT compile-time default, that each boot game waits for explicit launch input, starts in a defined reset state, keeps Eurorack outputs safe in the host path, and obeys its game-specific controls, including BEATKNECHT PLAY/PAUSE/STOP transport, encoder tempo, and TAP style selection. These are state-machine tests; graphical appearance is still covered separately by framebuffer/simulator regression assets.

## Complete firmware host suite

```bash
python scripts/run_host_tests.py
```

This is the authoritative coverage gate. It compiles the complete project-owned firmware against deterministic framework fakes in I2C, SPI and fixed-address/reset-pin configurations.

Current validated 1.1 development baseline (r26 production source):

```text
Executable lines:   9258 / 9665   95.79%
Functions:            805 / 819    98.29%
Decision branches:   5639 / 6246   90.28%
Compiler branches:   5640 / 7322   77.03% (informational)
```

The hard gates remain >=95% executable lines, >=95% functions and >=90% non-throw decision branches.

The script writes `coverage/full_coverage.txt` and `coverage/full_coverage.json`. The configured coverage thresholds are hard gates; uncovered production functions, lines, and non-throw decisions remain listed in the report.

Sanitizers run separately with:

```bash
python scripts/run_host_tests.py --sanitizers-only
```

Both AddressSanitizer and UndefinedBehaviorSanitizer are hard CI gates.

## Test-support fakes

`test/support/fake_framework/` contains intentionally small host implementations of the Arduino, Wire, SPI, HardwareTimer and Unity API surfaces used by this project. They exist to execute HAL logic deterministically; they are not substitutes for hardware-in-the-loop timing tests.

## Native simulator integration

The simulator integration suite is maintained under `sim/` and uses CMake/CTest rather than PlatformIO:

```bash
cmake --preset simulator-headless
cmake --build --preset simulator-headless
ctest --preset simulator-headless
```

It executes the complete firmware above the simulator HAL and validates boot, real debounced controls, scheduler/gate activity, OLED framebuffer output, and file-backed persistence. SDL is deliberately not required for these integration tests.

## Hardware-in-the-loop

Electrical gate timing, comparator behavior, real bus signaling, boot pulse safety, External Sync capture and physical Groove-Record TAP latency/jitter remain a separate HIL layer because those properties cannot be proven by host mocks.

<h6 align="center">From Munich with &#9829;</h6>
