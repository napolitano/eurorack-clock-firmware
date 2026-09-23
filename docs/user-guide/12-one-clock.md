<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 12 One Clock

One Clock is the factory topology and the quickest way to use CLOCK as an eight-way master clock multiple.

![One Clock performance screen while playing, showing the shared master tempo and Humanize indicator while all eight physical outputs are driven from one common clock configuration.](../manual-source/assets/performance-one-clock-play.png)

One shared configuration controls all eight outputs:

- rate and rational ratio;
- Swing;
- gate length;
- Phase;
- **Humanize**.

Humanize exists only in One Clock. Available amplitudes are:

```text
OFF / 250 / 500 / 1000 / 2000 µs
```

It applies small deterministic per-output timing displacement while keeping the common restart/downbeat exact. It is intended to remove perfect simultaneity between eight copies without turning the outputs into unrelated clocks.

### Shared timing layers



All three change **when** an output edge occurs, but they solve different musical problems and remain separate controls.

| Timing tool | Character | Repeats how? | Scope | Main controls |
| --- | --- | --- | --- | --- |
| **Swing** | Regular long/short feel | Alternates every second subdivision | One Clock globally or per Independent channel | `SWING 0–50%` |
| **Groove** | Deterministic rhythmic microtiming pattern | Repeats over the selected 1–64-step Groove pattern | One Clock globally or per Independent channel; not Divider Bank | preset/custom pattern, `AMOUNT`, `ROTATE` |
| **Humanize** | Small deterministic per-output displacement around the shaped grid | Deterministic event/channel sequence rather than a stored Groove pattern | **One Clock only** | `OFF / 250 / 500 / 1000 / 2000 µs` |

Swing is the simplest choice when the desired feel is a regular alternating shuffle. Groove is for a repeatable timing fingerprint that can span more than two events, including early as well as late Custom-Groove steps. Humanize is different again: it slightly separates otherwise coincident One Clock outputs without becoming part of the Groove pattern. The common restart/downbeat remains exact.

The layers can be combined. CLOCK first shapes the nominal event with Swing and Groove, then applies the bounded One Clock Humanize displacement. Safety limits reduce effective displacement when necessary so adjacent events remain ordered and at least one scheduler quantum apart.

<h6 align="center">From Munich with &#9829;</h6>
