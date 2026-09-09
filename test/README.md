<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Test automation

## Fast deterministic core

```bash
pio test -e native
```

The 44 deterministic tests exercise the extracted timing/pattern core, including exhaustive ratio and Euclidean invariants.

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
