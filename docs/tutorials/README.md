<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook

CLOCK Storybook is the project-local, host-only documentation-video system for deterministic how-to material. It drives the production CLOCK application through existing simulator boundaries and composes documentation frames from real firmware output rather than reproducing firmware behaviour.

Current implementation status: **SB-0/SB-1/SB-2/SB-3/SB-4/SB-5 complete; SB-6 frame/subtitle pipeline next**.

- [`STORYBOOK_ARCHITECTURE.md`](STORYBOOK_ARCHITECTURE.md) defines ownership, dependency, timing, stream, and publication boundaries.
- [`STORY_SCHEMA_1.md`](STORY_SCHEMA_1.md) defines the initial authoring interface and stable action spellings.
- [`stories/external-sync.yaml`](stories/external-sync.yaml) is the first executable end-to-end reference story and proves the SYNC/cable/SOURCE/manual-STOP boundary through the real simulator.
- [`stories/examples/`](stories/examples/) contains contract examples used during implementation and validation.
- [`interaction_profiles.yaml`](interaction_profiles.yaml) defines the deterministic `HUMAN_SLOW`, `HUMAN_NORMAL`, and `HUMAN_FAST` timing profiles consumed by the runner.
- `sim/tutorial/panel_presentation.*` records one-way physical presentation events and combines them with the real `SimulatorRuntime`/`PanelLayout` state.
- `sim/tutorial/panel_dynamic_layer.*` provides a deterministic headless RGBA interaction/LED layer for regression and later video composition.

Generated videos, frames, subtitles, manifests, coverage output, and local publication artifacts belong under ignored build/output locations. They are not source artifacts and must not be included in source bundles.


## SB-5 deterministic tutorial renderer

`sim/tutorial/tutorial_renderer.*` composes complete Phase-1 1920×1080 RGBA8 frames in memory. It does not capture a desktop window and does not reconstruct CLOCK screens from semantic state.

For `tutorial` scenes it combines:

- the real 128×64 `SimulatorRuntime::framebuffer()` at one exact integer nearest-neighbour scale;
- a front-panel presentation derived from `PanelLayout`, including the same production OLED image inside the panel representation;
- the SB-4 physical interaction layer and actual gate/LED telemetry;
- an optional single-channel scope driven by the production UI's currently selected channel and real gate-transition history;
- a dedicated subtitle strip below the tutorial content.

`chapter`, `text`, `TIP`, `WARNING`, and `RECIPE` scenes use the same data-driven theme and measured text layout. Horizontal or vertical overflow is a hard render failure; Storybook never silently clips instructional text.

Phase-1 typography is deliberately deterministic and independent of system font installation. Themes select project-local `CLOCK UI` or `CLOCK Mono` faces and explicit integer-scaled pixel sizes. Unknown requested faces fail validation/rendering rather than being silently substituted.

<h6 align="center">From Munich with &#9829;</h6>
