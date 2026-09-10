<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Hardware-in-the-Loop Test Plan

This document defines the electrical and timing checks that cannot be proven by host-side code coverage.

### SPI OLED electrical bring-up

For the current SPI prototype, verify the physical pins before functional UI testing:

- PA5 -> OLED SCK/D0
- PA7 -> OLED MOSI/D1
- PA4 -> OLED CS
- PB9 -> OLED D/C
- PB15 -> OLED RESET/RES (active low)
- 3.3 V logic/power and common GND as required by the selected module

On every firmware boot, PA4 must be driven HIGH before PB15 is pulsed high -> low -> high. After RESET returns HIGH, the firmware waits before the selected SSD1306/SSD1315 command stream. PA4 must then pulse LOW only for command/data transactions. The default prototype SPI clock is held to 1 MHz during bring-up; remapped builds must verify the configured pins instead of assuming these defaults. A panel that only works with RESET hard-wired high or CS hard-wired low fails this bring-up check and indicates firmware-target, GPIO-routing, or wiring mismatch.


## 1. Purpose

Host verification currently executes every project-owned production function, executable line, and non-throw decision branch. That proves software-path execution, but it cannot prove analog thresholds, electrical output levels, physical jitter, bus integrity, or boot behavior at the Eurorack jacks.

HIL therefore complements host coverage rather than replacing it. The machine-readable qualification status lives in [`qualification/v1_qualification.json`](qualification/v1_qualification.json), and objective timing limits are defined in [`qualification/V1_ACCEPTANCE.md`](qualification/V1_ACCEPTANCE.md). Raw timing captures should be normalized to the repository CSV format in [`qualification/HIL_CAPTURE_FORMAT.md`](qualification/HIL_CAPTURE_FORMAT.md).

## 2. Required test equipment

Minimum useful bench setup:

- oscilloscope or logic analyzer with sub-microsecond timing resolution
- frequency/period counter where available
- stable external clock source
- adjustable signal generator for External Sync threshold testing
- DMM for static output-level verification
- final or electrically representative output-buffer and comparator hardware

## 3. Boot and reset safety

### HIL-BOOT-001 — Outputs remain inactive during power-up

**Procedure**

1. Monitor all eight physical gate jacks during power-up.
2. Repeat power cycling at least 100 times.
3. Repeat with the OLED connected and disconnected.
4. Repeat with USB connected and disconnected where electrically supported.

**Pass criteria**

- no positive pulse exceeding the receiving-system trigger threshold
- output buffer remains disabled until firmware has explicitly initialized all channel lines LOW

### HIL-BOOT-002 — Reset does not create false gates

Monitor all outputs while repeatedly asserting MCU reset.

**Pass criteria**

- no unintended trigger edge at any output

## 4. Gate-output timing

### HIL-GATE-001 — Static output levels

Measure LOW and HIGH voltage at each jack.

**Pass criteria**

- LOW is close to 0 V and HIGH meets the nominal +5 V output specification on every channel
- verify both unloaded outputs and the representative Eurorack input load defined by the final schematic/BOM
- no +10 V output level is expected from the current 74HCT244 architecture

### HIL-GATE-002 — Pulse width

For the complete configured set **1, 2, 5, 10, 20, 50, and 100 ms**, measure actual high time at all outputs. The factory setting is **10 ms**.

**Pass criteria**

- for widths that are not intentionally shortened to protect the next edge, jack-level absolute error is **<= 50 us** (one V1 scheduler quantum)
- pulses never swallow the next scheduled rising edge; at high event rates the scheduler may shorten the requested width to fit the effective interval
- capture at least 100 complete pulses per tested width; use `scripts/analyze_hil_capture.py pulse-width` for reviewable evidence

### HIL-GATE-003 — Inter-channel skew

Configure all channels to `CLK x1`, zero phase and zero swing.

**Pass criteria**

- maximum channel-to-channel rising-edge skew across OUT1...OUT8 is **<= 50 us** (one V1 scheduler quantum)
- capture at least 100 complete simultaneous edge groups; use `scripts/analyze_hil_capture.py skew`

### HIL-GATE-004 — Divider/multiplier accuracy

Measure representative values including `/32`, `/3`, `x3`, `x32`, `2:3`, `3:2`, `5:4`, and `7:8`.

**Pass criteria**

- no cumulative phase drift beyond the defined scheduler quantization bound

## 5. Swing and phase

### HIL-TIME-001 — Swing pair duration

Measure long/short intervals at several BPM values and Swing settings.

**Pass criteria**

- each long/short pair preserves the unswung two-step duration
- no cumulative phase walk relative to the master timeline

### HIL-TIME-002 — Phase offset

Measure phase offsets on two otherwise identical channels.

**Pass criteria**

- measured offset agrees with the expected scheduler-quantized phase within **<= 50 us**

## 6. External SYNC / RST inputs

The firmware already accepts conditioned SYNC/RST levels through interrupt-driven capture queues. These HIL tests become authoritative once the final comparator circuit and PCB pin routing exist. The simulator's ideal comparator is not evidence for any electrical pass criterion below.

### HIL-SYNC-001 — Comparator threshold

Sweep the input amplitude slowly through the switching region.

**Pass criteria**

- rising and falling thresholds are inside the final specification
- hysteresis prevents chatter near threshold

### HIL-SYNC-002 — Glitch rejection

Inject pulses shorter than, equal to, and longer than the configured filter threshold.

**Pass criteria**

- rejected pulses do not advance the clock
- valid pulses are captured once

### HIL-SYNC-003 — Lock acquisition

Apply stable clocks at the minimum, nominal, and maximum supported rates.

**Pass criteria**

- lock is achieved within the defined acquisition time
- displayed BPM converges without affecting scheduler timing

### HIL-SYNC-004 — Jitter tracking

Apply deterministic timing jitter to the external clock.

**Pass criteria**

- lock remains stable inside the supported jitter envelope
- measured jitter diagnostics track the injected condition within their specified accuracy

### HIL-SYNC-005 — Signal loss policies

Verify `STOP`, `FREE`, and `INT` loss modes.

**Pass criteria**

- transition behavior matches the selected policy without false output edges

### HIL-SYNC-006 — Jack Detect

Insert and remove a patch cable with and without clock activity.

**Pass criteria**

- JACK, SIGNAL, and LOCK states remain independently correct

### HIL-RST-001 — Reset threshold and edge integrity

Drive RST IN with square, sine, and triangle sources across the specified amplitude/rate range.

**Pass criteria**

- each valid threshold crossing produces exactly one conditioned reset edge
- slow analogue crossings do not chatter into multiple resets
- out-of-spec/short glitches follow the final documented rejection policy

### HIL-RST-002 — Global phase semantics

Run channels using both `RESET=GLOBAL` and `RESET=FREE`, then inject RST while transport is PLAYING.

**Pass criteria**

- transport remains PLAYING
- GLOBAL channels restart from the shared phase epoch
- FREE channels retain their independent phase
- no unintended gate pulse is emitted by the reset edge itself

## 7. Controls

### HIL-CTRL-001 — Encoder

Exercise slow and fast rotation in both directions plus push operation.

**Pass criteria**

- no missing or duplicate user-visible detents outside the accepted debounce policy
- the game-only encoder long-press does not create a short-press action on release

### HIL-CTRL-002 — Buttons

Exercise every D6R button with rapid and deliberately noisy presses.

**Pass criteria**

- exactly one intended logical action per valid press

## 8. Display and timing isolation

Display qualification is a timing test, not only a visual test. Run the same musical workload with the display idle and under worst-case refresh load, then compare the captured gate traces.

### HIL-DISP-001 — I2C bounded-refresh stress

Build an SSD1306 or SSD1315 I2C profile at the configured 400 kHz rate. Exercise the densest changing screens/screensavers while all eight outputs run, including representative 20, 120, and 999 BPM cases plus mixed multipliers/dividers.

**Pass criteria**

- no visible persistent corruption; stale UI frames may be skipped under load
- no missed gate edge, SYNC edge, RST event, or encoder detent attributable to OLED traffic
- musical edge timing shows no additional display-correlated maximum period deviation greater than **50 us** versus the equivalent idle-display capture
- logic-analyzer inspection confirms that runtime framebuffer traffic is split into bounded I2C transactions rather than one monolithic 1 KiB transfer

### HIL-DISP-002 — SPI reference stress

Repeat the identical workload with the reference SPI build.

**Pass criteria**

- UI semantics match the I2C build
- no missed musical/control events
- no display-correlated timing regression

### HIL-DISP-003 — A/B transport comparison

Capture the same output channel simultaneously with a display-bus trace or display-service marker for both firmware variants. Compare idle-display and maximum-refresh runs.

**Pass criteria**

- SPI and I2C produce the same musical event count and sequence
- same musical event count and order are mandatory; added maximum period deviation versus the equivalent idle baseline must remain **<= 50 us**
- if I2C exceeds the 50-us V1 timing budget, the release is blocked and the next implementation step is interrupt/DMA display transport rather than relaxing the musical timing requirement

### HIL-DISP-004 — Faulted I2C bus

Temporarily disconnect the OLED or hold the bus in an error condition that exercises the configured transfer timeout.

**Pass criteria**

- gate scheduling continues without missing events
- SYNC/RST capture remains functional
- encoder edges continue to accumulate even if visual response is delayed
- recovery behavior is documented; a display failure must never enable or corrupt gate outputs

## 9. Long-duration timing

### HIL-LONG-001 — Internal-clock drift

Run a stable internal clock for an extended measurement period.

**Pass criteria**

- measured drift remains within the MCU oscillator and release specification
- no software-induced cumulative ratio drift is observed

### HIL-LONG-002 — Mixed-mode endurance

Run all eight channels simultaneously with mixed OFF/CLK/EUC/SEQ configurations for several hours.

**Pass criteria**

- no stalls, spurious gates, counter failures, or UI/timing coupling

## 10. Evidence and release policy

For ordinary gate/period/skew tests, record at least 100 complete events. Display-stress comparisons use at least 1,000 complete events. Every PASS entry in the V1 qualification ledger must reference at least one evidence artifact. Timing evidence should include normalized CSV plus analyzer JSON where practical. Hardware-dependent criteria that cannot yet be stated from the frozen schematic/component specification remain BLOCKED rather than receiving guessed tolerances.


A stable release must not be declared from host coverage alone. The release checklist should record, at minimum:

- host coverage gate: PASS
- ASan: PASS
- UBSan: PASS
- I2C firmware build: PASS
- SPI firmware build: PASS
- I2C/SPI display-stress timing: PASS
- boot/reset output safety: PASS
- gate timing/jitter: PASS
- External Sync and RST HIL suites: PASS

## Persisted PLAY boot-safety

1. Run the module, enter PLAY, and leave enough time for the transport-state persistence delay to expire.
2. Power-cycle the module with an oscilloscope or logic analyzer attached to all eight gate outputs (or repeat per output if channel count is limited).
3. Verify that no output rises during boot and that all outputs remain LOW after the boot screen until PLAY is explicitly pressed.
4. Repeat after PAUSE and STOP were the last persisted user state.

Pass criterion: **no boot pulse and no automatic resume under any persisted transport value**.
