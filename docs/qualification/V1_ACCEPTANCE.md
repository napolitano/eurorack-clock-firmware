<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK V1 Hardware Acceptance Contract

This document converts the HIL plan from qualitative language into reviewable release criteria. It does **not** claim that the current hardware has passed them.

## Firmware-derived timing budget

The V1 scheduler runs at 20 kHz, so one service quantum is exactly **50 µs**. For events whose intended time is produced by the scheduler itself, one scheduler quantum is the V1 maximum software-timing error budget unless a test below states a tighter invariant. This is a release requirement derived from the firmware architecture, not a measurement result.

The following criteria can therefore be fixed before physical measurement:

| Measurement | V1 criterion |
| --- | --- |
| Configured gate HIGH time | absolute jack-level error ≤ 50 µs for widths that are not intentionally shortened to protect the next edge |
| Identically configured simultaneous outputs | maximum rising-edge skew across OUT1…OUT8 ≤ 50 µs |
| Configured channel phase | absolute edge-position error ≤ 50 µs relative to the expected scheduler-quantized phase |
| Display stress | zero missing/extra musical edges; additional maximum period deviation versus idle display ≤ 50 µs |
| SPI vs I²C event sequence | identical event count/order for the same musical workload |
| Boot/reset safety | zero unintended positive gate pulses |

These limits are intentionally tied to the current 20-kHz architecture. A later output-compare implementation may improve measured performance without changing the V1 functional contract.

## Criteria that remain hardware-dependent

Some pass limits cannot be invented from firmware constants and remain unresolved until the final electrical implementation or component specification is frozen:

- **absolute internal-clock ppm accuracy:** the scheduler can prove absence of software accumulation error, but absolute drift depends on the oscillator actually fitted to the selected Black Pill / hardware revision;
- **SYNC/RST analogue threshold tolerance:** nominal thresholds are derived in the hardware documentation, but final min/max acceptance requires the final resistor tolerances, comparator behavior, clamp behavior and measured output levels;
- **analogue input amplitude/rate envelope:** must be stated from the final comparator/protection circuit and verified with square, sine and triangle sources;
- **electrical output load specification:** static 0/5-V acceptance must be checked under the representative Eurorack input load defined by the final schematic/BOM;
- **external-SYNC IRQ latency envelope:** EXTI is the V1 baseline, but its measured capture-to-output distribution is the evidence that determines whether timer Input Capture is necessary.

A release candidate must not convert these open items into guessed numbers. The relevant HIL ledger entry remains `BLOCKED` until the final hardware premise exists, then `PENDING` until measured.

## Statistical evidence policy

Timing checks should use at least 100 complete events for ordinary gate/period/skew measurements and at least 1,000 events for display-stress comparisons. Long-duration drift uses the duration stated by the HIL test plan rather than a small event count. Any discarded sample must be explained in the evidence record.

For the 50-µs criteria above the repository analyzer uses worst observed absolute error/skew, not only mean or standard deviation. Display stress compares event counts and the maximum deviation from the median period so a clean average cannot hide isolated timing excursions.

## Release semantics

Beta builds validate the ledger structure but are allowed to contain `PENDING` and `BLOCKED` physical tests. `1.0.0-rc.*` and `1.0.0` require all normative HIL tests to be `PASS` with evidence. This is enforced by `scripts/check_hil_qualification.py --require-pass-if-rc` in CI/release tooling.

<h6 align="center">From Munich with &#9829;</h6>
