<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Development and Release Workflow

For workstation setup and day-one workflows in VSCodium, see [`DEVELOPER_README.md`](DEVELOPER_README.md).

## Coding standard

All project-owned source code, comments, identifiers, Doxygen documentation, and developer documentation use **US English**.

### Naming

| Element | Convention | Example |
| --- | --- | --- |
| Namespace | lowercase | `clockfw::engine` |
| Class / struct | PascalCase | `ClockEngine` |
| Enum class | PascalCase | `ChannelMode` |
| Enum value | PascalCase | `ChannelMode::Euclid` |
| Function / method | lowerCamelCase | `updateChannel()` |
| Local / parameter | lowerCamelCase | `channelIndex` |
| Constant | `k` + PascalCase | `kSchedulerFrequencyHz` |
| Private member | lowerCamelCase + `_` | `gateOutputs_` |
| Boolean | positive/question-like meaning where practical | `rescheduleChannel`, `gateHigh` |

Avoid abbreviations unless they are established domain/UI terms such as BPM, PPQN, GPIO, CLK, EUC, or SEQ.

### File headers

Every project-owned C++ source/header contains a Doxygen file header with at least:

```cpp
/**
 * @file example.cpp
 * @brief One-sentence responsibility of the file.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
```

Project-owned scripts, workflows, configuration files, and Markdown documentation carry equivalent author/license metadata in the comment syntax appropriate to that file type. `LICENSE.md` itself and binary assets are naturally exempt.

### Doxygen

Every method and function must have a useful Doxygen comment. Relevant constants, shared state, and non-obvious data structures must also be documented.

Good comments explain:

- why a method exists
- ownership and side effects
- timing/thread/ISR constraints
- units and ranges
- non-obvious safety behavior
- meaning of parameters that is not obvious from the type/name

Inline comments should explain **why**, not narrate obvious code. For example, documenting why output enable stays disabled through boot is useful; commenting `++channelIndex` as “increment channel” is not.


### Project configuration layout

Global project configuration is intentionally kept at the top of `src/`:

- `config.h` — compile-time behavior and transport policy
- `defaults.h` — editable factory user settings
- `pin_map.h` — physical GPIO/peripheral routing
- `ui_text.h` — static user-visible text / localization catalog
- `version.h` — prerelease version only

Headers belonging to an implementation module live beside its `.cpp` file. CI rejects a source implementation whose matching header has drifted back into a separate include tree.

See [`CONFIGURATION.md`](CONFIGURATION.md) and [`DEPENDENCIES.md`](DEPENDENCIES.md).

### Real-time restrictions

Code reachable from `ClockEngine::processSchedulerTick()` must remain:

- allocation-free
- bounded
- non-blocking
- independent from OLED/UI rendering
- independent from serial/logging

Do not introduce floating-point timeline accumulation. Rational timing uses integer/Q32 arithmetic with retained remainder.

### Architecture boundaries

Hardware/framework dependencies belong only in `src/hal` and `src/pin_map.h`.

Run:

```bash
python scripts/check_architecture.py
```

The check enforces file headers, HAL-only hardware dependencies, a small `main.cpp`, and source-size guardrails. See [`ARCHITECTURE.md`](ARCHITECTURE.md) for the full dependency policy.

## Local firmware build

```bash
pio run -e blackpill_f401cc
```

Compile-check the alternative SPI display transport with:

```bash
pio run -e blackpill_f401cc_spi_ssd1315
```

The STM32 target remains pinned to `ststm32@19.7.1` and the project explicitly builds its C++ sources as GNU C++17.

## Native production-core tests

```bash
pio test -e native
```

The native suite runs production code from `lib/clock_core` and currently contains **44 named test cases**, with additional exhaustive loops inside the invariant tests.

Covered areas include:

- rate normalization and local-meter conversion
- Q32 rational remainder accumulation and master timeline accumulation
- swing invariants and clamping
- phase math
- deterministic RNG and probability boundaries
- gate-length conversion / edge protection
- sequencer step wrapping and 64-bit pattern helpers
- exhaustive Euclidean hit-count invariants for 1–64 steps
- sequencer rotation and step 64
- GLOBAL/FREE reset policy

## Python tooling tests

```bash
python -m unittest discover -s scripts/tests -p 'test_*.py' -v
```

These cover:

- firmware version extraction
- exact tag/version matching
- changelog release-note extraction
- missing release-note versions
- deterministic release filenames
- firmware + license packaging
- SHA-256 generation
- missing artifact failure
- repository license-policy checks

## Coverage

The authoritative repository-wide gate is:

```bash
python scripts/run_host_tests.py --skip-sanitizers
python scripts/run_host_tests.py --sanitizers-only
```

It compiles and executes the real project-owned firmware across I2C, SPI, fixed-I2C/reset-pin, deterministic-core, and native-smoke variants using GCC coverage instrumentation.

CI requires:

- executable-line coverage >= **95%**
- function coverage >= **95%**
- non-throw decision-branch coverage >= **90%**
- AddressSanitizer = **clean**
- UndefinedBehaviorSanitizer = **clean**

The last complete repository-wide coverage baseline remains alpha.31 at 97.18% executable lines, 100.00% functions, and 90.12% source decision branches. Alpha.35 changes product branding and boot rendering; its default and SPI headless builds/tests are green, but the full serial gcov matrix was not completed in the current validation environment, so no new alpha.35 coverage percentage is claimed. The report lists every uncovered production function and decision branch explicitly so the prerelease thresholds do not hide newly introduced blind spots.

Raw GCC compiler-branch coverage is reported separately because GCC also emits synthetic throw/exception edges. These are informational only; source decision branches are gated at 90%.

Coverage artifacts are written to `coverage/full_coverage.txt` and `coverage/full_coverage.json`.

Production headers must not contain executable inline functions. Keeping executable logic in `.cpp` files makes every production function visible to coverage and is enforced by `scripts/check_architecture.py`.

## Native simulator build

The native simulator is a second hardware platform for the real firmware and uses CMake independently of PlatformIO:

```bash
cmake --preset simulator-headless
cmake --build --preset simulator-headless
ctest --preset simulator-headless
```

The interactive frontend uses SDL3:

```bash
cmake --preset simulator
cmake --build --preset simulator
python scripts/run_simulator.py
```

See [`SIMULATOR.md`](SIMULATOR.md) for platform setup, controls, persistence, and developer instrumentation.

## GitHub Actions

### CI

`.github/workflows/ci.yml` runs on pushes, pull requests, and manual dispatch.

Order:

1. architecture policy check
2. Python release/tooling tests
3. native production-core tests
4. coverage-instrumented firmware tests and sanitizer gates
5. headless full-firmware simulator build/tests on Windows, macOS, and Linux
6. SDL3 simulator frontend compile/build gate on Ubuntu
7. STM32F401 I2C firmware build
8. STM32F401 SPI display compile-check
9. verify both STM32 target builds and their memory budgets
10. upload host coverage only; firmware BIN/ELF files are deliberately not distributed while STM32duino LGPL code remains statically linked

The STM32 firmware build only runs after host coverage and both native-simulator CI gates pass.

### Releases

`.github/workflows/release.yml` runs for Git tags and can also be manually dispatched for release-candidate builds.

A release tag must exactly match `src/version.h`:

```text
firmware: 0.19.0-alpha.62
Git tag:   v0.19.0-alpha.62
```

A mismatch fails before publication.

Before tagging, freeze the maintained user manual for the current firmware version:

```bash
python scripts/prepare_release_manual.py
```

This creates `docs/manual/releases/<version>/clock-user-manual.<version>.odt`. The tag workflow requires that exact versioned manual source; a missing or stale manual blocks publication.

For a tag build the workflow:

1. validates architecture policy
2. validates version/tag consistency
3. runs Python tooling tests
4. runs native production-core tests
5. enforces coverage thresholds
6. builds the STM32 firmware
7. validates the frozen versioned ODT manual
8. converts that ODT to `clock-user-manual.<version>.pdf` with LibreOffice and validates the PDF/font contract
9. extracts release notes from the matching `CHANGELOG.md` section
10. publishes the GitHub Release with the versioned **ODT and PDF manual artifacts** attached

GitHub supplies the source archive for the tagged commit. Firmware BIN/ELF attachment is intentionally disabled until either the STM32duino LGPL static-link obligations are handled by a compliance-reviewed relinking package or the runtime is migrated to direct STM32Cube HAL/LL + CMSIS. The manual ODT/PDF are documentation artifacts and are published independently of that firmware-binary restriction. See `docs/LICENSING.md`.

## Versioning

The release source of truth is:

```cpp
src/version.h
```

Do not hard-code a second firmware version in workflows or production code. The manual preparation tool reads the same value, stamps it into body text, metadata, and cover/back-cover artwork, and freezes the corresponding release ODT before tagging.

## Doxygen

A repository `Doxyfile` is provided for generated API documentation. The documentation-quality checker validates repository-local links, README footers, fenced Markdown blocks, SVG accessibility metadata, and current-version identity before Doxygen generation.

```bash
python scripts/check_documentation.py
doxygen Doxyfile
```

Generated output goes to `build/docs/doxygen/` and is ignored by Git.

## License

Project software is licensed under `PolyForm-Noncommercial-1.0.0`. See `LICENSE.md` and `NOTICE.txt`. Third-party components retain their upstream licenses; see `THIRD_PARTY_NOTICES.md` and `third_party/`.
