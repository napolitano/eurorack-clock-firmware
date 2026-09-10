<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# V1 Qualification Evidence

This directory is the evidence boundary for the CLOCK V1 hardware qualification. Host coverage, simulator tests and static analysis remain in CI; physical measurements live here or are referenced from the machine-readable qualification ledger.

The normative procedure is [`../HIL_TEST_PLAN.md`](../HIL_TEST_PLAN.md). Objective timing limits and the distinction between firmware-derived and hardware-dependent criteria are defined in [`V1_ACCEPTANCE.md`](V1_ACCEPTANCE.md). Captures used by the repository analyzer must follow [`HIL_CAPTURE_FORMAT.md`](HIL_CAPTURE_FORMAT.md). Exact repeatable device configurations are collected in [`V1_BENCH_MATRIX.md`](V1_BENCH_MATRIX.md). Use [`HIL_EVIDENCE_TEMPLATE.md`](HIL_EVIDENCE_TEMPLATE.md) when a measurement needs explanatory metadata beyond CSV/analyzer output.

`v1_qualification.json` is deliberately **not** a success report. A test stays `PENDING` or `BLOCKED` until representative hardware has actually been measured. `PASS` is valid only with at least one repository-relative evidence reference. CI and release workflows always validate ledger structure/evidence semantics, but incomplete HIL status is **advisory through firmware 1.4.x**. `scripts/check_hil_qualification.py --require-pass-from 1.5.0` activates the hard all-PASS gate for release candidates and stable releases from `1.5.0` onward.

Recommended evidence naming:

```text
qualification/evidence/
  HIL-GATE-002-10ms-spi.csv
  HIL-GATE-002-10ms-spi.analysis.json
  HIL-DISP-003-i2c-stress.csv
  HIL-DISP-003-spi-stress.csv
  HIL-SYNC-001-thresholds.md
```

Do not commit captures containing unrelated personal or machine-identifying data. Raw oscilloscope screenshots are useful supporting evidence, but edge CSV plus analyzer output should be preferred for timing claims because it is reviewable and reproducible.

<h6 align="center">From Munich with &#9829;</h6>
