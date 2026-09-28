<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook

CLOCK Storybook is the project-local, host-only documentation-video system for deterministic how-to material. It drives the production CLOCK application through existing simulator boundaries and composes documentation frames from real firmware output rather than reproducing firmware behaviour.

Current implementation status: **Phase 1 SB-0 through SB-8 complete; post-Phase-1 authoring/visual polish active.**

- [`STORYBOOK_ARCHITECTURE.md`](STORYBOOK_ARCHITECTURE.md) defines ownership, dependency, timing, stream, and publication boundaries.
- [`STORY_SCHEMA_1.md`](STORY_SCHEMA_1.md) defines the initial authoring interface and stable action spellings.
- [`stories/external-sync.yaml`](stories/external-sync.yaml) is the first executable end-to-end reference story and proves the SYNC/cable/SOURCE/manual-STOP boundary through the real simulator.
- [`stories/examples/`](stories/examples/) contains contract examples used during implementation and validation.
- [`interaction_profiles.yaml`](interaction_profiles.yaml) defines the deterministic `HUMAN_SLOW`, `HUMAN_NORMAL`, and `HUMAN_FAST` timing profiles consumed by the runner.
- [`assets/video/`](assets/video/) is the canonical location for optional pre-produced intro/outro publication assets.
- `sim/tutorial/panel_presentation.*` records one-way physical presentation events and combines them with the real `SimulatorRuntime`/`PanelLayout` state.
- `sim/tutorial/panel_dynamic_layer.*` provides a deterministic headless RGBA interaction/LED layer for regression and later video composition.

Generated videos, frames, subtitles, manifests, coverage output, and local publication artifacts belong under ignored build/output locations. They are not source artifacts and must not be included in source bundles.



## Publication visual defaults

The default publication theme now follows the manual more closely: Ubuntu/Ubuntu Mono typography, CLOCK Blue `#0B4FC0` chapter cards, black tutorial backgrounds, dark framed content surfaces, and a dedicated subtitle bar. Typography, font weights/sizes, colours, and the optional tutorial background image are theme data. Normal tutorial pacing uses the deliberately slower `HUMAN_NORMAL` profile; `HUMAN_FAST` remains available for timing-sensitive gestures such as Tap Tempo.

Final publication system-font rendering currently uses the native Windows host font rasterizer and refuses silent face substitution. The separate `south-signal-lab-ci` theme keeps the dependency-free project bitmap font for deterministic CI renderer tests.

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


## SB-6 deterministic frame and subtitle pipeline

`sim/tutorial/frame_pipeline.*` executes each validated Story exactly once. A read-only execution observer samples that same live run at the configured rational frame timestamps (`frame_index × 1,000,000 / fps`) and immediately renders the current production framebuffer, simulator telemetry, physical presentation state, active scene, and active subtitle.

Generated output is staged and published atomically:

```text
<output>/
  frames/
    frame-000000.png
    frame-000001.png
    ...
  <story-id>.srt
  <story-id>.vtt
  manifest.json
```

PNG frames are deterministic RGBA8 lossless images produced by project-local host tooling. The writer uses PNG Sub filtering plus a deterministic fixed-Huffman DEFLATE stream, so frame generation has no image-codec package dependency.

SRT, WebVTT, and burned-in subtitles all derive from the same Story Runner subtitle events. Sidecar cue boundaries therefore use the same presentation timeline as the rendered frames. Publication-time intro offsets are intentionally deferred to SB-7 because intro/outro media are publication assets, not Story execution.

The manifest records the Story/schema identity, firmware version, caller-supplied simulator source revision, theme, interaction profile, output profile, presentation duration, frame count, subtitle count, and an FNV-1a-64 digest of each raw RGBA8 frame. The digest is a deterministic regression identity, not a security checksum.

Generation writes to a sibling staging directory first. A parser, runner, renderer, subtitle, PNG, or filesystem failure removes that staging output and does not replace a previously successful output directory.

## SB-7 publication pipeline

`sim/tutorial/publication_pipeline.*` is the host-only media stage. It consumes one already successful SB-6 frame-pipeline result and never executes CLOCK again. `ffprobe` validates referenced pre-produced media and obtains the duration that is actually used for subtitle offsetting; `ffmpeg` performs normalization and encoding.

Publication order is fixed:

```text
intro clip? -> generated tutorial frames -> outro clip? -> final media
```

The default result is `<story-id>.mp4` encoded as H.264/yuv420p. WebM is optional and uses VP9; when publication audio exists it uses Opus. Intro/outro video is scaled down preserving aspect ratio and padded to the Story output geometry, then converted to the Story frame rate. Source clip duration is explicitly preserved during normalization so audio padding cannot extend the visible clip.

If any intro/outro contains audio, all composed segments receive a normalized 48 kHz stereo audio stream. Existing intro/outro audio is preserved; an otherwise silent tutorial or silent clip receives deterministic silence only for composition compatibility. If no publication clip contains audio, the final tutorial stays audio-free.

Final SRT/WebVTT sidecars are regenerated from the SB-6 cue list with the measured normalized intro duration added to every tutorial cue. Outro duration never offsets tutorial subtitles. No intro/outro content is added to the Story Runner trace.

Publication uses a sibling staging directory and replaces the requested output only after all requested encodes and sidecars succeed. If an older successful output exists, it is first renamed to a sibling backup and restored if the final staged-directory swap fails. Missing media, probe failure, normalization failure, codec failure or final composition failure therefore leaves a previous successful publication untouched. Missing `ffmpeg`/`ffprobe` affects only this media-publication stage; firmware and non-publication simulator/Storybook targets do not depend on them.

## Canonical reference catalog

Phase 1 ships 13 didactic stories under [`stories/`](stories/README.md). The implementation evidence for the frozen 40-point Phase-1 gate is maintained in [`PHASE1_ACCEPTANCE.md`](PHASE1_ACCEPTANCE.md). The catalog is executable documentation: CI parses, validates and runs every story twice through the real simulator boundary and requires deterministic logical traces. Story files remain documentation assets under `docs/tutorials/`; generated frames/videos remain ignored publication output.
## Authoring examples and command-line generation

- [`examples/`](examples/README.md) contains ten copy-and-edit teaching Stories covering the main CLOCK workflows. They are examples, not a second canonical CI catalog.
- [`VIDEO_GENERATION.md`](VIDEO_GENERATION.md) documents the `clock-storybook` host CLI for validation, frame generation and final MP4/WebM publication.
- The CLI is built from the headless CMake preset and does not require SDL3. FFmpeg/ffprobe are required only for the final `video` command.

### Readability and focus overlays

The publication renderer treats the large OLED as the primary readable CLOCK UI. The right side is split into a magnified **CONTROL DETAIL** crop and a smaller **LOCATION** view of the complete module. Real encoder/button/patch actions automatically highlight the relevant hardware. Stories may add presentation-only `focus` actions for explanatory arrows, including arbitrary regions of the canonical 128x64 OLED framebuffer. Burned-in subtitles use a configurable lower safe margin so common video-player controls do not cover them.

<h6 align="center">From Munich with &#9829;</h6>
