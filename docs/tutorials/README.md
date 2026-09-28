<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook

CLOCK Storybook is the project-local, host-only documentation-video system for deterministic how-to material. It drives the production CLOCK application through existing simulator boundaries and composes documentation frames from real firmware output rather than reproducing firmware behaviour.

Current implementation status: **SB-0/SB-1/SB-2/SB-3/SB-4 complete; SB-5 tutorial renderer next**.

- [`STORYBOOK_ARCHITECTURE.md`](STORYBOOK_ARCHITECTURE.md) defines ownership, dependency, timing, stream, and publication boundaries.
- [`STORY_SCHEMA_1.md`](STORY_SCHEMA_1.md) defines the initial authoring interface and stable action spellings.
- [`stories/external-sync.yaml`](stories/external-sync.yaml) is the first executable end-to-end reference story and proves the SYNC/cable/SOURCE/manual-STOP boundary through the real simulator.
- [`stories/examples/`](stories/examples/) contains contract examples used during implementation and validation.
- [`interaction_profiles.yaml`](interaction_profiles.yaml) defines the deterministic `HUMAN_SLOW`, `HUMAN_NORMAL`, and `HUMAN_FAST` timing profiles consumed by the runner.
- `sim/tutorial/panel_presentation.*` records one-way physical presentation events and combines them with the real `SimulatorRuntime`/`PanelLayout` state.
- `sim/tutorial/panel_dynamic_layer.*` provides a deterministic headless RGBA interaction/LED layer for regression and later video composition.

Generated videos, frames, subtitles, manifests, coverage output, and local publication artifacts belong under ignored build/output locations. They are not source artifacts and must not be included in source bundles.

<h6 align="center">From Munich with &#9829;</h6>
