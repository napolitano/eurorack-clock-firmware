<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Roadmap

This roadmap separates the first stable DIY release from later musical expansion. Version 1.0 is not defined by exhausting every feature idea. It is defined by a hardware/firmware/documentation combination that can be built, measured, supported, and used without known release-blocking defects.

## Product boundary

CLOCK is deliberately a **digital timing, gate, and trigger instrument**. Hardware revision 1 does not add analog CV/modulation outputs or a general-purpose CV modulation matrix, and it does not add general parameter-CV inputs.

This is a deliberate DIY-quality decision, not a missing 1.0 requirement. A quality eight-channel analog modulation-output path would move the design into a different cost and assembly class: the project would require an eight-channel 16-bit DAC-class solution plus two quad output op-amp stages. A 12-bit MCP-class compromise is not considered adequate for this product direction. The present project estimate is roughly **EUR 30-40 additional BOM cost** before the wider consequences for fine-pitch SMD assembly, PCB complexity, calibration, testing, and documentation.

General CV parameter inputs would likewise add analog front ends, routing, jacks, and UI/validation scope. They are therefore outside hardware Rev 1. Post-1.0 channel interaction may still become deep, but it should initially remain **internal and event-based**: clocks, gates, resets, logic, probability, fills, and other deterministic rhythm relationships rather than an analog modulation subsystem.

## Path to 1.0

| Stage | Version | Scope | Exit criterion |
| --- | --- | --- | --- |
| Current baseline | `0.19.0-alpha.63` | Implemented V1 feature set before freeze | Superseded by beta freeze |
| **V1 feature freeze** | **`0.19.0-beta.1`** | No new musical features. Freeze product scope; audit persistence and event architecture for later 1.x migration; align docs and release semantics. | Existing behavior unchanged; forward-compatibility risks documented and guarded by tests |
| V1 qualification | `0.19.0-beta.x` | PCB bring-up, electrical measurements, SYNC/RST comparator validation, SPI/I2C stress, timing HIL, defect correction, build/manual refinement | Physical qualification plan complete; no known V1 blockers |
| Release candidate | `1.0.0-rc.x` | Externally buildable release candidate; release blockers only | Independent build/use feedback and all mandatory gates pass |
| Stable | **`1.0.0`** | First fully supported CLOCK DIY release | Hardware, firmware, tests, build docs, and user manual agree and are supportable |

### V1 feature freeze rule

From `0.19.0-beta.1` until `1.0.0`, a change is accepted only when it fixes a defect, closes a qualification/documentation gap, improves reproducibility, or is required to preserve a safe migration path. Swing/Groove expansion, Sequencer expansion, Euclid Auto-Fill, Ratchets, structured random, channel logic, scenes, and other new musical behavior are intentionally deferred.

## Post-1.0 musical roadmap

| Firmware line | Theme | Planned direction |
| --- | --- | --- |
| `1.1.x` | Swing 2.0 + Groove | grid-aware classic swing, substantial factory groove library, editable custom grooves, amount/rotation; Humanize remains a separate layer |
| `1.2.x` | Sequencer 2.0 | per-step probability, per-step trigger/gate duration, duty-cycle behavior, tie |
| `1.3.x` | Euclid + Conditions | interval/probability-driven Euclid Auto-Fill, fill amount/region, trigger conditions and shared Fill semantics |
| `1.4.x` | Performance rhythm | Ratchet/Burst with bounded EVEN/ACCEL/DECEL/BOUNCE timing models |
| `1.5.x` | Structured random | looped random, evolving/mutating patterns, deterministic LFSR modes |
| `1.6.x` | Channel interaction | AND/OR/XOR/A-minus-B, event-based cross-channel relationships, utility outputs such as RUN/BAR/DOWNBEAT |
| `1.7.x` | Performance + phase | quantized scene changes, Phase Spread, group rotation |
| Later experiments | Timing research | Groove Learn, Elastic Sync, Clock Nudge, Phase Drift, Correlated Random, Euclid Morph; promote only if musically useful |

## VCV Rack track

A VCV Rack version is planned as a separate post-1.0 track rather than being tied mechanically to the firmware version. The goal is to reuse the same musical timing core and make identical state/timing scenarios produce equivalent event streams on hardware, the SDL/headless simulator, and VCV Rack.

The initial VCV module should model the physical CLOCK rather than gain virtual-only CV sockets simply because software makes them cheap. A later extended virtual instrument can be considered separately if that becomes musically justified.

## Versioning rules

- Firmware follows Semantic Versioning.
- Hardware revision and firmware version are separate identities.
- `PATCH` fixes compatible behavior after 1.0.
- `MINOR` adds backward-compatible musical functionality after 1.0.
- `MAJOR` is reserved for a genuine public-contract break, such as an incompatible hardware generation or non-migratable persistent-state contract.
- Development builds may use SemVer prerelease/build metadata; ordinary commits do not require a product-version bump.
