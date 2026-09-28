<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook

CLOCK Storybook is the project-local, host-only documentation-video system for deterministic how-to material. It drives the production CLOCK application through existing simulator boundaries and composes documentation frames from real firmware output rather than reproducing firmware behaviour.

Current implementation status: **PROPOSED / SB-0 architecture and schema freeze**.

- [`STORYBOOK_ARCHITECTURE.md`](STORYBOOK_ARCHITECTURE.md) defines ownership, dependency, timing, stream, and publication boundaries.
- [`STORY_SCHEMA_1.md`](STORY_SCHEMA_1.md) defines the initial authoring interface and stable action spellings.
- [`stories/examples/`](stories/examples/) contains contract examples used during implementation and validation.

Generated videos, frames, subtitles, manifests, coverage output, and local publication artifacts belong under ignored build/output locations. They are not source artifacts and must not be included in source bundles.

<h6 align="center">From Munich with &#9829;</h6>
