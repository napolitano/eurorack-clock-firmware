<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Test automation

## Native PlatformIO suite

```bash
pio test -e native
```

This is the developer-facing native test entry point and intentionally runs all native suites, not only the extracted core:

- 44 deterministic clock-core cases, including exhaustive ratio, accumulator, swing, probability, gate-width, Euclidean and sequencer invariants;
- 24 focused real-time/SYNC/RST integration and stress cases;
- 21 complete host-firmware/UI/HAL scenarios against deterministic Arduino/display/storage fakes.

That is **89 named native test cases**. In the current default host configuration they execute more than **218,000 assertions** (the clock-core invariant matrix alone executes about 215,000). The case count therefore describes independently reported scenarios, not the amount of boundary/input coverage. The separate `run_host_tests.py` matrix remains authoritative for display variants, sanitizers and aggregate coverage.

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
