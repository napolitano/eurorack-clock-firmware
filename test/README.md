<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Test automation

## Native PlatformIO suite

```bash
pio test -e native
```

This is the developer-facing Native entry point. Beta.4 deliberately makes timing, musical behavior and physical-control contracts visible instead of presenting only the 44-case mathematical core. PlatformIO executes eight conventionally named suites:

| Suite | Named cases | What it proves |
| --- | ---: | --- |
| `test_clock_core` | 44 | hardware-independent clock mathematics and exhaustive invariants |
| `test_realtime` | 24 | deterministic scheduler/GPIO/SYNC/RST integration and stress behavior |
| `test_sync_behavior` | 80 | exact and changing external clocks, jitter/glitches/loss, PPQN/edge semantics and nominal analogue-front-end behavior |
| `test_swing` | 22 | swing pair mathematics and observable scheduler edge spacing |
| `test_humanize` | 12 | deterministic One Clock timing displacement, bounds, repeatability and isolation |
| `test_tap_tempo` | 22 | tap estimator acquisition, rolling average, clamps, invalid intervals, jitter and timestamp wrap |
| `test_controls` | 28 | encoder quadrature, accumulated detents, debounce, bounce, hold and simultaneous button behavior |
| `test_host_firmware` | 22 | complete firmware/UI/HAL/persistence behavior using deterministic host fakes |
| **Total** | **254** | public Native inventory |

The default Native configuration executes more than **223,000 assertions**. `scripts/check_test_inventory.py` rejects a total regression below 240 cases and also enforces minimum sizes for every visible suite.

### Swing and Humanize behavior

`test_swing` verifies the low-level swing transform and the actual edge train produced by `ClockEngine`. Straight, 10%, 25% and 50% swing are observed at the gate-driver boundary; long/short pairs must preserve total musical duration, remain non-zero and repeat deterministically within scheduler quantization.

`test_humanize` treats Humanize as a separate timing contract. OFF must keep all eight One Clock outputs coincident; 250/500/1000/2000 us settings must remain inside their configured scheduler-quantized windows; a fresh run must reproduce the same offsets; outputs must exhibit channel spread; and a stored One Clock Humanize value must have no timing effect in Independent mode. Humanize combined with maximum current Swing is also exercised for ordering safety.

### Tap Tempo behavior

`test_tap_tempo` exposes the estimator independently of the broad UI integration scenarios. It verifies 30/60/120/240 BPM-class intervals, the 999 BPM clamp, user MIN/MAX clamps, timestamp zero, reset and sequence timeout, too-fast/duplicate intervals, rolling four-interval averaging, small jitter, alternating fast/slow taps and unsigned timestamp wrap.

### Encoder and buttons

`test_controls` drives the production `ControlPanel` against GPIO fakes. It verifies both Gray-code directions, partial cycles, contact bounce, six detents accumulated while foreground polling is absent, the +/-127 report bound with remainder preservation, idle sampling without gratuitous global interrupt masking, active-low button press/release debounce, no hold-repeat, bounce restart, simultaneous buttons and encoder activity during button debounce. The broader host-firmware suite continues to test how these samples drive the UI state machine.

### External-SYNC behavior suite

`test_sync_behavior` intentionally targets the failure modes most likely to matter musically:

- exact 1, 20, 30, 60, 120, 240, 300, 600, 900 and 999 BPM boundaries;
- 1/2/4/24 PPQN interpretation and rising/falling-edge selection;
- duplicate timestamps, sub-filter glitches, glitch storms and opposite-polarity noise;
- deterministic jitter from +/-50 us through +/-20%, alternating 400/600 ms and 250/750 ms periods, and chaotic-but-valid periods;
- **continuous tempo movement**: linear 60->180 and 180->60 BPM ramps, slow drift around 120 BPM, repeated tempo steps, acceleration with alternating jitter and a 24-PPQN tempo ramp;
- abrupt 60<->180 BPM changes, adaptive timeout boundaries and STOP/FREEWHEEL/INTERNAL/AUTO loss behavior;
- timestamp wrap, queue-overflow continuity and explicit lock reacquisition;
- runtime PPQN and selected-edge changes, including the beta.4 regression that previously allowed an old period filter to contaminate a newly interpreted clock.

The suite also contains a **host-only nominal electrical model** of the documented Rev-1 LM393 input resistor network. It calculates the nominal ~1.79 V rising and ~1.46 V falling thresholds and drives the production digital SYNC estimator from modelled 2.5 V, 3 V and 4 V inputs at 20, 120 and 999 BPM. This validates intended nominal design behavior and hysteresis logic; resistor tolerance, clamp-diode behavior, LM393 common-mode/propagation effects, noise, PCB parasitics and actual switching thresholds remain physical HIL requirements.

## Complete firmware host suite

```bash
python scripts/run_host_tests.py
```

This is the authoritative coverage gate. It compiles the complete project-owned firmware against deterministic framework fakes in I2C, SPI and fixed-address/reset-pin configurations.

Current baseline:

```text
Executable lines:   >=95%
Functions:          >=95%
Decision branches:  >=90%
Compiler branches:  informational
```

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

Electrical gate timing, comparator behavior, real bus signaling, boot pulse safety and External Sync capture remain a separate HIL layer because those properties cannot be proven by host mocks.

<h6 align="center">From Munich with &#9829;</h6>
