<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 1 Start here

CLOCK is a 10 HP, eight-output Eurorack master clock, rhythm generator, and gate sequencer. All outputs share one deterministic master timeline, but you can use that timeline in three different ways:

- **One Clock** — one shared clock configuration drives all eight outputs. This is the factory topology.
- **Independent** — each output independently runs Clock, Euclid, Sequencer, or Off.
- **Divider Bank** — one shared source produces eight fixed divisions from a selectable family.

![Diagram showing the three CLOCK output topologies: One Clock, Independent, and Divider Bank.](../manual-source/assets/operating-modes.svg)

The important design choice is that Independent does **not** mean eight unrelated free-running clocks. Clock, Euclid, and Sequencer channels remain anchored to the same master timeline, so rate changes, resets, and mixed rhythmic functions can stay musically related.

### Product boundary

CLOCK is intentionally an eight-channel **digital timing, gate, and trigger instrument**. Hardware Rev 1 provides two LM393-conditioned digital comparator inputs on the physical SYNC/PA8 and RST/PA9 nets. Current 1.1 firmware can assign their musical roles globally, but this does not turn them into general parameter-CV inputs; analog CV/modulation outputs and a general modulation matrix remain outside scope.

That boundary protects both signal quality and DIY buildability. A quality eight-channel analog modulation-output path would require an eight-channel 16-bit DAC-class solution plus two quad output-op-amp stages; a 12-bit MCP-class implementation is not considered an acceptable quality compromise for this product direction. The project currently estimates roughly EUR 30-40 of additional BOM cost before the added fine-pitch SMD assembly, PCB, calibration, validation, and documentation burden. General parameter-CV inputs would also require additional analog front ends, routing, and panel I/O.

This is therefore **not a missing V1 feature**. CLOCK concentrates on what eight digital event outputs can do well: coherent clocks, Euclidean rhythms, gate sequences, ratios, phase, probability, reset semantics, and robust synchronization. Later firmware may add deeper internal event relationships without requiring an analog modulation subsystem.

For the frozen V1/post-1.0 boundary, see [`ROADMAP.md`](../ROADMAP.md).

<h6 align="center">From Munich with &#9829;</h6>
