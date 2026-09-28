<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Generating CLOCK Storybook videos

CLOCK Storybook is a host-only documentation tool. It runs the production firmware through the native simulator, renders deterministic tutorial frames, writes SRT/WebVTT from the same subtitle timeline, and optionally publishes MP4/H.264 and WebM/VP9.

The normal workflow is:

```text
Story YAML -> validate -> one deterministic CLOCK run -> PNG/SRT/VTT -> MP4/WebM
```

The video encoder never changes CLOCK behaviour and is not part of the embedded build.

## 1. Prerequisites

For validation and frame generation:

- CMake 3.20 or newer;
- Ninja;
- a C++17 host compiler.

For final video publication also install:

- `ffmpeg`;
- `ffprobe`.

SDL3 is **not** required. Use the headless simulator preset.

Verify the media tools when you want final video output:

```bash
ffmpeg -version
ffprobe -version
```

## 2. Build the Storybook CLI

From the repository root:

```bash
cmake --preset simulator-headless
cmake --build --preset simulator-headless --target clock-storybook
```

The executable is then:

```text
build/simulator-headless/clock-storybook
```

On Windows the file has the usual `.exe` suffix.

## 3. Start with the examples

Ten editable teaching examples live in [`examples/`](examples/README.md). They are intentionally separate from the canonical CI reference catalog under [`stories/`](stories/README.md).

Validate one Story before doing any expensive rendering:

```bash
./build/simulator-headless/clock-storybook validate \
  docs/tutorials/examples/01-power-and-first-clock.yaml
```

Windows PowerShell:

```powershell
.\build\simulator-headless\clock-storybook.exe validate `
  docs\tutorials\examples\01-power-and-first-clock.yaml
```

A successful validation prints the Story ID and title and exits with status 0.

## 4. Generate frames only

Use this while authoring text, timings or presentation layout. It does **not** encode a video:

```bash
./build/simulator-headless/clock-storybook frames \
  docs/tutorials/examples/07-euclidean-rhythm.yaml \
  --output docs/tutorials/generated/example-07-frames
```

The output contains:

```text
frames/frame-000000.png
frames/frame-000001.png
...
<story-id>.srt
<story-id>.vtt
manifest.json
```

`docs/tutorials/generated/` is intentionally ignored by Git and must not be included in source bundles.

## 5. Generate an MP4

For the normal publication case:

```bash
./build/simulator-headless/clock-storybook video \
  docs/tutorials/examples/01-power-and-first-clock.yaml \
  --output docs/tutorials/generated/example-01 \
  --format mp4
```

The final directory contains the MP4 plus the publication SRT/WebVTT and manifest. Intermediate PNG frames are deleted after a successful publication.

To keep the intermediate frames for inspection:

```bash
./build/simulator-headless/clock-storybook video \
  docs/tutorials/examples/01-power-and-first-clock.yaml \
  --output docs/tutorials/generated/example-01 \
  --format mp4 \
  --keep-frames
```

The retained intermediate directory is a sibling named `<output>.frames`.

## 6. Generate WebM or both formats

WebM only:

```bash
./build/simulator-headless/clock-storybook video \
  docs/tutorials/examples/01-power-and-first-clock.yaml \
  --output docs/tutorials/generated/example-01-webm \
  --format webm
```

MP4 and WebM in one publication run:

```bash
./build/simulator-headless/clock-storybook video \
  docs/tutorials/examples/01-power-and-first-clock.yaml \
  --output docs/tutorials/generated/example-01-both \
  --format both
```

MP4/H.264 is the default publication format and should normally be used for the documentation videos.

## 7. Add a pre-produced intro or outro

Intro and outro are optional **pre-produced videos**, not Storybook scenes. Put reusable clips under `docs/tutorials/assets/video/` or another stable project location and reference them from the Story YAML:

```yaml
publication:
  intro_video: "../assets/video/south-signal-lab-intro.mp4"
  outro_video: "../assets/video/south-signal-lab-outro.mp4"
```

Paths are resolved relative to the Story YAML file.

The publication pipeline then performs:

```text
intro? -> generated CLOCK tutorial -> outro? -> final MP4/WebM
```

The clips are probed before use and normalized to the Story resolution and FPS while preserving aspect ratio. Existing intro/outro audio is retained. If any publication clip contains audio, silent tutorial segments receive silence only so the streams can be concatenated correctly.

Final SRT/WebVTT cues are shifted by the measured normalized intro duration. Outro duration never shifts tutorial cues.

## 8. Use explicit ffmpeg/ffprobe paths when needed

Normally the CLI finds both programs on `PATH`. To select specific binaries:

```bash
./build/simulator-headless/clock-storybook video \
  docs/tutorials/examples/01-power-and-first-clock.yaml \
  --output docs/tutorials/generated/example-01 \
  --ffmpeg /path/to/ffmpeg \
  --ffprobe /path/to/ffprobe
```

This is useful on Windows systems with multiple FFmpeg installations or in controlled CI environments.


## 9. Windows process-launch troubleshooting

If `clock-storybook video` exits immediately on Windows with process status `0xC0000139` / `-1073741511`, use a source revision containing the Storybook Windows process-launch fix (r48j or later) and rebuild `clock-storybook`. The fixed implementation invokes `ffmpeg`/`ffprobe` through `CreateProcessW` rather than the CRT `_wspawnv` path and preserves arguments containing spaces, embedded quotes and trailing backslashes.

After replacing/updating the source, remove the old Storybook executable by rebuilding the target:

```powershell
cmake --build --preset simulator-headless --target clock-storybook
```

Then retry the same `video` command. The CLI prints the frame-render stage, frame count and resolved FFmpeg/FFprobe executable paths before publication starts.

## 10. Publication typography, colours and backgrounds

The publication theme is [`themes/south-signal-lab-default.yaml`](themes/south-signal-lab-default.yaml). Its defaults follow the CLOCK manual visual language:

- **Ubuntu** for body/headings/subtitles;
- **Ubuntu Mono** for technical labels;
- **CLOCK Blue `#0B4FC0`** for chapter/interstitial screens;
- **black** for the normal tutorial background.

Font families, weights and pixel sizes are theme data rather than renderer constants:

```yaml
fonts:
  body:
    backend: system
    family: "Ubuntu"
    weight: 400
    size_px: 30
  heading:
    backend: system
    family: "Ubuntu"
    weight: 700
    size_px: 46
```

The current publication system-font raster backend uses native Windows GDI and rejects substitution: if the requested family is not installed, rendering fails with an explicit message. Install Ubuntu/Ubuntu Mono on the publication workstation or change the configured family to another installed face. Font files themselves are workstation assets and are not bundled with CLOCK.

The dependency-free CI renderer continues to use `south-signal-lab-ci`, which deliberately keeps the project-owned bitmap faces. Do not use that theme for final publication unless the pixel-font look is intentional.

A normal tutorial may optionally replace the black background with an image:

```yaml
presentation:
  background_image: "assets/backgrounds/tutorial-background.bmp"
  background_image_mode: cover
```

The path is relative to `docs/tutorials/`. Supported image mode values are `cover`, `contain`, and `stretch`. The current SDL-free image loader accepts uncompressed 24-bit or 32-bit Windows BMP so background images do not add an image-codec dependency to the headless renderer. Leaving `background_image` empty uses pure black.

## 11. Tutorial pacing

Interaction speed is controlled centrally by [`interaction_profiles.yaml`](interaction_profiles.yaml), not by video playback speed. `HUMAN_NORMAL` and `HUMAN_SLOW` are intentionally paced for viewers to follow the OLED and physical control movement. `HUMAN_FAST` remains reserved for interactions whose timing is semantically meaningful, especially Tap Tempo.

For ordinary instructional material use:

```yaml
interaction_profile: HUMAN_NORMAL
```

For especially dense menu/navigation explanations use:

```yaml
interaction_profile: HUMAN_SLOW
```

Do not slow Tap Tempo by changing its profile unless the demonstrated tap interval is updated accordingly; the taps are real CLOCK input and therefore change the measured BPM.

## 12. Record the source revision in the manifest

The default manifest revision is `working-tree`. For an archived or release-oriented render, pass the Git revision explicitly:

```bash
./build/simulator-headless/clock-storybook video \
  docs/tutorials/examples/01-power-and-first-clock.yaml \
  --output docs/tutorials/generated/example-01 \
  --source-revision "$(git rev-parse HEAD)"
```

PowerShell equivalent:

```powershell
$revision = git rev-parse HEAD
.\build\simulator-headless\clock-storybook.exe video `
  docs\tutorials\examples\01-power-and-first-clock.yaml `
  --output docs\tutorials\generated\example-01 `
  --source-revision $revision
```

## 13. Recommended authoring loop

Use this order while developing a Story:

1. Copy the closest YAML from `docs/tutorials/examples/`.
2. Change the `id`, `title`, explanatory text and recorded actions.
3. Run `clock-storybook validate` until the schema and semantics are clean.
4. Run the Storybook reference/host tests if the Story exercises a new interaction pattern.
5. Use `frames` only when visual inspection is needed.
6. Run `video` only for the final publication candidate.
7. Keep generated output under `docs/tutorials/generated/`; never add it to a source bundle.

Do not replace physical UI actions with direct product-state setters. If a workflow cannot be expressed through the existing module controls, patch actions and external stimuli, extend the Storybook contract explicitly and regression-test that addition first.

## 14. Useful verification commands

Validate the full native Storybook/test matrix:

```bash
cmake --preset simulator-headless
cmake --build --preset simulator-headless
ctest --preset simulator-headless
```

Validate the Storybook repository policy:

```bash
python scripts/tests/test_storybook_tooling.py
```

The publication integration test automatically skips only when `ffmpeg`/`ffprobe` are unavailable; all non-publication Storybook and firmware/simulator targets remain independent of those tools.

<h6 align="center">From Munich with &#9829;</h6>

## Focus overlays and subtitle safe area

The default publication theme keeps burned-in subtitles above a 150-pixel lower player-control safe area at 1080p. Adjust `presentation.subtitle_safe_bottom_px` in the selected theme if the target player requires more or less clearance.

Physical actions are highlighted automatically. For explanatory pauses, add a presentation-only focus action. For example:

```yaml
- focus:
    target: oled_top_bar
    placement: right
    label: "Top bar: status, musical context, transport"
    duration_ms: 2800
```

For a specific UI element use `target: oled_region` with `x`, `y`, `width`, and `height` in the real 128x64 OLED coordinate system. `placement: auto` is the default; use `left`, `right`, `above`, or `below` when a particular teaching frame needs a fixed callout direction. Arrow leaders terminate outside the target and use a black/white contrast outline around the CLOCK-blue core. Focus actions do not advance CLOCK firmware time.
