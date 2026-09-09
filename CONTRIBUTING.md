<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Contributing to CLOCK

CLOCK is an embedded real-time project. Changes are welcome when they preserve the module's timing, safety, DIY maintainability, and documentation standards.

## Before changing code

Read:

- [`docs/ARCHITECTURE.md`](docs/ARCHITECTURE.md)
- [`docs/DEVELOPMENT.md`](docs/DEVELOPMENT.md)
- [`docs/TEST_COVERAGE.md`](docs/TEST_COVERAGE.md)
- [`docs/HIL_TEST_PLAN.md`](docs/HIL_TEST_PLAN.md) for hardware-facing changes

## Non-negotiable engineering rules

- Hardware/framework APIs stay behind HAL or the declarative pin map.
- UI code never generates musical timing.
- ISR/scheduler paths stay bounded, non-blocking, allocation-free, and free of display/logging/persistence work.
- User-visible static firmware strings belong in `src/ui_text.h`.
- Public C++ declarations require concise Doxygen API documentation.
- New behavior requires regression tests at the lowest meaningful level and integration coverage when subsystem boundaries are involved.
- Real electrical claims require HIL evidence; host mocks are not timing instruments.
- Do not weaken coverage, warning, sanitizer, architecture, persistence, or boot-safety gates to make a change pass.

## Local verification

```bash
python scripts/check_architecture.py
python scripts/check_documentation.py
python -m unittest discover -s scripts/tests -p 'test_*.py' -v
pio test -e native
python scripts/run_host_tests.py --skip-sanitizers
python scripts/run_host_tests.py --sanitizers-only
cmake --preset simulator-headless
cmake --build --preset simulator-headless
ctest --preset simulator-headless
```

Target builds:

```bash
pio run -e blackpill_f401cc
pio run -e blackpill_f401cc_spi
```

## Documentation changes

Update the documentation in the same change when behavior, wiring, settings, persistence schema, simulator controls, architecture, or build procedure changes. Mermaid diagrams belong in Markdown; reusable publication artwork belongs in `docs/manual/assets/` as plain SVG.

## Commit hygiene

Do not commit build directories, `.pio`, coverage output, simulator state, generated compile databases with local paths, or editor/user-specific state. The checked-in `.gitignore` is the minimum rule, not permission to commit other local artifacts.
