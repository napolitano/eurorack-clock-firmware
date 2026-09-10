<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# HIL Evidence Record Template

Copy this file for measurements that need more context than a normalized edge CSV. Do not mark the qualification ledger PASS until every procedure and pass criterion for the referenced HIL ID has been completed.

## Identity

- **HIL ID:** `HIL-...`
- **Firmware:** `0.19.0-beta.x`
- **Hardware revision / prototype:**
- **Black Pill board/source:**
- **Display/controller/transport:**
- **Date:**
- **Operator:**

## Bench equipment

| Instrument | Model | Relevant setting / calibration note |
| --- | --- | --- |
| Oscilloscope / logic analyzer |  |  |
| Signal generator / clock source |  |  |
| DMM / counter |  |  |
| Eurorack PSU |  |  |

## Device configuration

Reference the applicable `QP-*` entry from `V1_BENCH_MATRIX.md` and list every deliberate deviation.

## Procedure

Record the executed steps. For automated timing checks, include the exact `scripts/analyze_hil_capture.py` command.

## Results

Record measured values, event/sample count, analyzer status and any discarded sample with justification.

## Evidence files

- normalized capture CSV:
- analyzer JSON/text output:
- oscilloscope screenshot(s), if useful:
- additional notes:

## Conclusion

- **Result:** `PASS` / `FAIL` / `BLOCKED`
- **Reason:**
- **Follow-up:**

<h6 align="center">From Munich with &#9829;</h6>
