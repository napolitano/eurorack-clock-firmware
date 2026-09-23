<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 8 Independent channels

Independent topology gives each of the eight outputs its own generator and timing/output state. Each channel can be `CLOCK`, `EUCLID`, `SEQ`, or `OFF`; channels continue to share the same deterministic master timeline rather than becoming eight unrelated free-running clocks.

Use the channel overview to select an output, then open its settings. Generator pages define the pattern itself; the shared **TIMING** and **OUTPUT** pages define when eligible events occur and what the resulting gate does.

| Shared control | Meaning |
| --- | --- |
| `RATE / NUM / DEN` | Integer or exact rational relation to the master timeline. |
| `SWING` | 0–50% alternating timing displacement. |
| `GROOVE` | Factory/Custom deterministic microtiming with Amount and Rotate. |
| `PROBABILITY` | Chance that an eligible event emits a physical gate. |
| `GATE` | Fixed 1 / 2 / 5 / 10 / 20 / 50 / 100 ms HIGH time. |
| `PHASE` | 0–99% offset against the common timeline. |
| `RESET` | `GLOBAL` re-anchors on global reset; `FREE` keeps the local cycle relationship. |
| `MUTE` | Suppresses output without discarding the generator state. |

The dedicated [Clock](09-clock-mode.md), [Euclid](10-euclid-mode.md), and [Sequencer](11-sequencer-mode.md) chapters describe the generator-specific pages.

<h6 align="center">From Munich with &#9829;</h6>
