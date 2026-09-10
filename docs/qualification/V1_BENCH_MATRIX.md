<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK V1 Bench Configuration Matrix

This matrix turns the normative HIL test plan into repeatable device configurations. It does not replace the individual procedures or their pass criteria. Unless a row says otherwise, use INTERNAL source, 4/4 meter, transport PLAY, 100% probability, `RESET=GLOBAL`, mute off, and Humanize off.

## QP-01 — simultaneous-output reference

Purpose: HIL-GATE-003, phase-zero baseline, display A/B reference.

| Parameter | Value |
| --- | --- |
| Operating mode | ONE CLOCK |
| BPM | 120 |
| Rate | ×1, rational 1:1 |
| Swing | 0% |
| Gate | 10 ms |
| Phase | 0% |
| Humanize | OFF |
| Outputs | all eight active |

Capture OUT1…OUT8 for at least 100 simultaneous rising-edge groups. This is the canonical skew baseline.

## QP-02 — pulse-width matrix

Purpose: HIL-GATE-002.

Use INDEPENDENT mode, all eight channels in CLOCK at ×1 / 1:1 / 0% Swing / 0% Phase. Set gate lengths:

| Output | Gate length |
| --- | ---: |
| OUT1 | 1 ms |
| OUT2 | 2 ms |
| OUT3 | 5 ms |
| OUT4 | 10 ms |
| OUT5 | 20 ms |
| OUT6 | 50 ms |
| OUT7 | 100 ms |
| OUT8 | 10 ms reference |

Use 120 BPM for the ordinary width check. Repeat the long-pulse channels at a deliberately high effective event rate to verify the documented pulse-shortening protection rather than applying the ordinary 50-us width criterion to an intentionally shortened pulse.

## QP-03 — integer/rational rate matrix

Purpose: HIL-GATE-004 and long-term no-accumulation checks.

Use INDEPENDENT CLOCK channels at 120 BPM, 0% Swing and 10-ms gate:

| Output | Integer rate | Rational rate | Effective relationship |
| --- | --- | --- | --- |
| OUT1 | ÷32 | 1:1 | 1/32 |
| OUT2 | ÷3 | 1:1 | 1/3 |
| OUT3 | ×3 | 1:1 | 3 |
| OUT4 | ×32 | 1:1 | 32 |
| OUT5 | ×1 | 2:3 | 2/3 |
| OUT6 | ×1 | 3:2 | 3/2 |
| OUT7 | ×1 | 5:4 | 5/4 |
| OUT8 | ×1 | 7:8 | 7/8 |

Capture enough edges to include multiple complete periods of OUT1. Verify observed edge counts/periods against the exact rational relationship rather than rounded BPM labels.

## QP-04 — swing preservation matrix

Purpose: HIL-TIME-001.

Use one CLOCK channel with phase 0 and 10-ms gate. Run at 60, 120 and 240 BPM. At each tempo capture Swing 0%, 10%, 25% and 50%. For each non-zero setting verify both alternating intervals and verify that each long/short pair sums to the corresponding two-step straight duration within scheduler quantization.

The current V1 UI uses the existing 0…50% displacement convention. Post-1.0 Swing/Groove work must not be mixed into this V1 qualification.

## QP-05 — phase matrix

Purpose: HIL-TIME-002.

Use four otherwise identical CLOCK channels at 120 BPM, ×1 and 0% Swing:

| Output | Phase |
| --- | ---: |
| OUT1 | 0% |
| OUT2 | 25% |
| OUT3 | 50% |
| OUT4 | 75% |

OUT5…OUT8 may duplicate OUT1 as zero-phase references. Measure phase against the scheduler-quantized expected interval; absolute timing error target is defined in `V1_ACCEPTANCE.md`.

## QP-06 — mixed-mode endurance

Purpose: HIL-LONG-002 and display/runtime interaction.

Use INDEPENDENT mode with all major V1 generators represented:

| Output | Mode | Representative configuration |
| --- | --- | --- |
| OUT1 | CLOCK | ×1, 10-ms gate |
| OUT2 | CLOCK | ×3, 5-ms gate, 10% Swing |
| OUT3 | EUCLID | 16 steps / 5 hits / rotate 0 |
| OUT4 | EUCLID | 13 steps / 7 hits / rotate 3 |
| OUT5 | SEQUENCER | 16 steps, factory A pattern |
| OUT6 | SEQUENCER | 16 steps, factory B pattern |
| OUT7 | CLOCK | 3:2 rational rate |
| OUT8 | OFF | explicit inactive-channel check |

Run for several hours while exercising menus and transport periodically. A stall, spurious edge, counter failure or output from OUT8 is a failure.

## QP-07 — display-stress timing

Purpose: HIL-DISP-001, HIL-DISP-002 and HIL-DISP-003.

The production UI only redraws when visible state changes, so the stress workload must use a screen that actually changes at the 30-ms display service ceiling. Use INDEPENDENT mode with the selected visible channel in EUCLID or SEQUENCER so the live step strip causes continuous invalidation. Use an active dense pattern and keep PLAY running.

Run the identical musical state twice for each display transport:

1. **idle-reference capture** — leave the display on a stable screen with no playback-step visualization;
2. **maximum-production-refresh capture** — show the active Euclid/Sequencer Performance screen so step changes continuously request redraws.

At minimum perform 120 BPM and 999 BPM cases. For the high-rate case use a multiplier that keeps the scheduler busy without relying on any post-1.0 functionality. Capture at least 1,000 OUT1 events and the display bus/marker if available. Use `analyze_hil_capture.py compare` for the event-count and added-deviation check.

The test intentionally measures the real production refresh behavior. Do not add an artificial framebuffer torture loop and then present it as normal-product timing evidence.

## QP-08 — External Sync reference

Purpose: HIL-SYNC-002 through HIL-SYNC-006.

Start with 120 BPM, 1 PPQN, rising edge, documented glitch filter and FREE loss policy. Repeat PPQN 1/2/4/24, then minimum/nominal/maximum supported external rates. Apply deterministic jitter only after the stable-reference captures pass. Repeat loss policy as STOP/FREE/INT.

SYNC comparator threshold and amplitude-envelope work remains blocked until the final analogue tolerance premise is frozen; do not infer it from simulator behavior.

## QP-09 — RST semantics

Purpose: HIL-RST-002.

Use at least two active channels: one `RESET=GLOBAL`, one `RESET=FREE`. While PLAYING, exercise both RST TRIGGER and RST GATE modes. Capture RST together with both outputs. The RST transition itself must not create an unintended positive gate edge.

## QP-10 — boot/persistence safety

Purpose: HIL-BOOT-001 and HIL-BOOT-002 plus persisted-PLAY safety.

Repeat at least 100 complete power cycles while monitoring outputs. Include OLED connected/disconnected and the supported USB-power combinations defined by the hardware setup. Repeat after PLAY, PAUSE and STOP were the last user-visible transport states. The expected boot state is always STOP and all outputs LOW until explicit PLAY.

<h6 align="center">From Munich with &#9829;</h6>
