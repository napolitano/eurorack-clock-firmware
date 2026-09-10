<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Test automation

## Native PlatformIO suite

```bash
pio test -e native
```

This is the developer-facing Native entry point. Beta.4 deliberately makes the test inventory visible instead of presenting only the 44-case mathematical core. PlatformIO executes four conventionally named suites:

| Suite | Named cases | What it proves |
| --- | ---: | --- |
| `test_clock_core` | 44 | hardware-independent clock mathematics and exhaustive invariants |
| `test_realtime` | 24 | deterministic scheduler/GPIO/SYNC/RST integration and stress behavior |
| `test_sync_behavior` | 74 | atomic external-clock edge cases, strongly varying input timing, glitch/loss behavior, runtime PPQN/edge changes, and nominal analogue-front-end behavior |
| `test_host_firmware` | 22 | complete firmware/UI/HAL/persistence behavior using deterministic host fakes |
| **Total** | **164** | public Native inventory |

The default Native configuration executes more than **221,000 assertions**. The core suite still contains exhaustive matrices, but beta.4 adds independently reported behavioral cases so that important contracts are reviewable by name in PlatformIO output. `scripts/check_test_inventory.py` now rejects both a total regression below 150 cases and a regression below the minimum assigned to any individual suite.

### External-SYNC behavior suite

`test_sync_behavior` intentionally targets the failure modes most likely to matter musically:

- exact 1, 20, 30, 60, 120, 240, 300, 600, 900 and 999 BPM boundaries;
- 1/2/4/24 PPQN interpretation and rising/falling-edge selection;
- duplicate timestamps, sub-filter glitches, glitch storms and opposite-polarity noise;
- deterministic jitter from +/-50 us through +/-20%, alternating 400/600 ms and 250/750 ms periods, chaotic-but-valid periods, and abrupt 60<->180 BPM changes;
- adaptive timeout boundaries and STOP/FREEWHEEL/INTERNAL/AUTO loss behavior;
- timestamp wrap, queue-overflow continuity and explicit lock reacquisition;
- runtime PPQN and selected-edge changes, including the beta.4 regression that previously allowed an old period filter to contaminate a newly interpreted clock.

The suite also contains a **host-only nominal electrical model** of the documented Rev-1 LM393 input resistor network. It calculates the nominal ~1.79 V rising and ~1.46 V falling thresholds and drives the production digital SYNC estimator from modelled 2.5 V, 3 V and 4 V inputs at 20, 120 and 999 BPM. This validates the intended nominal design behavior and hysteresis logic; resistor tolerance, clamp-diode behavior, LM393 common-mode/propagation effects, noise, PCB parasitics and actual switching thresholds remain physical HIL requirements.

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
