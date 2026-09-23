<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 10 Euclid mode

A Euclid channel distributes a selected number of hits as evenly as possible across a cycle.

- **STEPS** — 1–64
- **HITS** — 0–STEPS
- **ROTATE** — circular rotation from 0 to STEPS−1

Common channel timing — rate, Swing, Groove, Probability, gate length, Phase, reset policy, and Mute — remains outside the Euclid algorithm page.

At `×1`, Euclid advances on a **sixteenth-note grid**. In 4/4, 16 steps therefore span exactly one bar. The channel rate scales that step grid; `×1` does not mean one Euclid step per quarter note.

A simple 16-step / 4-hit pattern gives four evenly distributed hits. Rotation moves the pattern against the shared timeline without changing the hit count.

<h6 align="center">From Munich with &#9829;</h6>
