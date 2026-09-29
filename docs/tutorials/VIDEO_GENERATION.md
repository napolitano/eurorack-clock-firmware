<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Generating CLOCK Storybook videos

CLOCK Storybook is a host-only documentation tool. It runs the production firmware through the native simulator, renders deterministic tutorial frames, writes SRT/WebVTT from the same subtitle timeline, and optionally publishes MP4/H.264 and WebM/VP9.

For narrated teaching examples the preferred local workflow is narration-first:

```text
authored Story + voice-over
        -> ElevenLabs segments
        -> measured audio durations
        -> resolved Story timing
        -> sparse/change-driven CLOCK render
        -> intro/tutorial/outro
        -> cue-aligned narration mix
        -> local publication master
```

The video encoder and ElevenLabs client never change CLOCK behaviour and are not part of the embedded build.

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

## 2. Configure local ElevenLabs access

Generated narration is intentionally a workstation concern. Secrets never belong in Story YAML, `segments.json`, render manifests, source bundles or CI configuration for this local workflow.

Copy the tracked template once:

```bash
cp .env.example .env
```

Windows PowerShell:

```powershell
Copy-Item .env.example .env
```

Then edit only `.env` locally:

```dotenv
ELEVENLABS_API_KEY=your-local-secret
ELEVENLABS_VOICE_ID=your-cloned-voice-id
ELEVENLABS_MODEL_ID=auto-v4
```

`.gitignore` rejects `.env` and `.env.*` while explicitly allowing only `.env.example`. The renderer reads repository-root `.env` automatically; already exported environment variables override values from the file. The API key is never copied to cache metadata or render reports.

Treat `.env` as a local secret container, not as project configuration:

- never rename `.env.example` itself and put secrets into it; copy it to `.env`;
- never paste the API key into YAML, JSON, command lines, screenshots, issues or CI logs;
- keep `.env` in the repository root so the standard local runner finds it;
- if a different secret file is needed, keep it outside the repository and pass it with `--env-file`;
- revoke/rotate the ElevenLabs key immediately if it is ever committed or otherwise exposed.

The Voice ID and selected model are also kept in `.env` for one-place local configuration. They are not authentication secrets, but generated render output remains ignored and should not be treated as source.

You can verify the ignore rule without exposing the file contents:

```bash
git check-ignore -v .env
```

`ELEVENLABS_MODEL_ID=auto-v4` asks the ElevenLabs `/v1/models` endpoint at runtime and selects the one accessible text-to-speech model whose ID or display name contains `v4`. If the account exposes zero or more than one matching model, the job stops before synthesis and asks for the exact model ID in `.env`; it never guesses a provider-specific identifier.

The tracked `.env.example` also contains local timing policy. These values are publication defaults, not firmware state:

```dotenv
CLOCK_TUTORIAL_BODY_HEADROOM_MS=1200
CLOCK_TUTORIAL_OPENING_MIN_MS=18000
CLOCK_TUTORIAL_CLOSING_MIN_MS=20000
CLOCK_TUTORIAL_ROUNDING_MS=100
```

### Preferred commands

Render one complete narrated master:

```bash
python scripts/render_tutorials.py "Eight Independent Clocks Walkthrough"
```

Render all twelve examples as a local evening/nightly job:

```bash
python scripts/render_tutorials.py --all
```

Useful partial passes:

```bash
python scripts/render_tutorials.py "Eight Independent Clocks Walkthrough" --audio-only
python scripts/render_tutorials.py "Eight Independent Clocks Walkthrough" --picture-only
python scripts/render_tutorials.py "Eight Independent Clocks Walkthrough" --force-tts
```

The runner builds the optimized `simulator-headless-release` Storybook target unless `--skip-build` is specified. All generated audio, resolved YAML, timing plans, intermediate publication media and final masters live under ignored `tutorial-output/`.

## 3. Build the Storybook CLI

From the repository root:

```bash
cmake --preset simulator-headless
cmake --build --preset simulator-headless --target clock-storybook
```

For full-length videos, build the optimized headless executable instead:

```bash
cmake --preset simulator-headless-release
cmake --build --preset simulator-headless-release --target clock-storybook
```

Use `build/simulator-headless-release/clock-storybook` in the commands below when rendering with this build (or append `.exe` on Windows). The default `simulator-headless` preset builds in Debug mode and is not the publication-performance path. `clock-storybook video` uses sparse frame storage: unchanged logical frames share one PNG, and publication writes an FFconcat run list so a long frozen explanation does not require thousands of duplicate PNG encodes. Dynamic tutorial intervals still retain the requested constant-frame-rate output semantics.

The executable is then:

```text
build/simulator-headless/clock-storybook
```

On Windows the file has the usual `.exe` suffix.

## 4. Start with the examples

Twelve editable teaching examples live in [`examples/`](examples/README.md). They are intentionally separate from the canonical CI reference catalog under [`stories/`](stories/README.md).

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

A successful validation prints the Story ID and title and exits with status 0. The most complete teaching example is `11-eight-independent-clocks-walkthrough.yaml`, which demonstrates Independent topology, channel selection, long-press context settings, and Clock-channel configuration.

## 5. Generate frames only

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

## 6. Generate an MP4

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

For `video`, each render uses a run-scoped sibling directory named `<output>.frames.run-<token>`. This avoids replacing a stale or temporarily locked frame tree on Windows. With `--keep-frames`, the CLI prints the exact retained path. Without it, the run-scoped frame tree is removed after publication. The standalone `frames` command still writes to the exact `--output` directory requested by the author.

## 7. Generate WebM or both formats

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

## 8. Add a pre-produced intro or outro

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

## 9. Narration-first publication details

The editable examples have matching US-English narration under [`voiceover/`](voiceover/README.md). The checked-in `.txt` scripts remain human-authored spoken copy. [`voiceover/segments.json`](voiceover/segments.json) maps their paragraphs to stable narration IDs embedded in the matching Story. Bounded non-tutorial scenes may carry `narration:` directly; tutorial narration uses explicit `beat` / `beat_end` containers so the spoken explanation begins before the actions it describes.

The local runner performs two passes before expensive video work:

1. synthesize or reuse each narration segment and measure its encoded duration with `ffprobe`;
2. create an ignored resolved Story copy whose matching scene or `beat.duration_ms` budget is the larger of the measured duration plus local visual headroom and the authored minimum budget.

The authored Story is never rewritten by the local job. After validation, Storybook executes the resolved copy once and records exact `NarrationBegin`/`NarrationEnd` presentation timestamps. Actions inside a tutorial beat execute while narration is active. `beat_end` holds only any remaining narration budget; it does not replay or delay the actions. Publication shifts the resulting cues by the probed intro duration and writes `<story-id>.narration.json`. The Python runner uses that sidecar to place the cached audio segments and copies the already encoded video stream into the final narrated master.

This means the actual generated voice — including Audio Tags, punctuation and pauses — controls timing. The historical ~195 wpm planning value is only an authoring estimate and is not used to overrule measured audio.

Audio cache identity includes the segment text hash, voice ID, resolved model ID and output format. Unchanged segments are therefore reused; `--force-tts` explicitly invalidates that convenience. No API key is stored in the cache.

## 10. Use explicit ffmpeg/ffprobe paths when needed

Normally the CLI finds both programs on `PATH`. To select specific binaries:

```bash
./build/simulator-headless/clock-storybook video \
  docs/tutorials/examples/01-power-and-first-clock.yaml \
  --output docs/tutorials/generated/example-01 \
  --ffmpeg /path/to/ffmpeg \
  --ffprobe /path/to/ffprobe
```

This is useful on Windows systems with multiple FFmpeg installations or in controlled CI environments.


## 11. Windows process-launch troubleshooting

If `clock-storybook video` exits immediately on Windows with process status `0xC0000139` / `-1073741511`, use a source revision containing the Storybook Windows process-launch fix (r48j or later) and rebuild `clock-storybook`. The fixed implementation invokes `ffmpeg`/`ffprobe` through `CreateProcessW` rather than the CRT `_wspawnv` path and preserves arguments containing spaces, embedded quotes and trailing backslashes.

After replacing/updating the source, remove the old Storybook executable by rebuilding the target:

```powershell
cmake --build --preset simulator-headless --target clock-storybook
```

Then retry the same `video` command. The CLI prints the frame-render stage, frame count and resolved FFmpeg/FFprobe executable paths before publication starts.

## 12. Publication typography, colours and backgrounds

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

## 13. Tutorial pacing

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

## 14. Record the source revision in the manifest

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

## 15. Recommended authoring loop

Use this order while developing a narrated teaching Story:

1. Copy the closest YAML from `docs/tutorials/examples/` and its voice-over script.
2. Change the `id`, `title`, explanatory text and recorded physical actions.
3. Split narration into editorially stable beats in `voiceover/segments.json`. Attach non-tutorial IDs to bounded scenes; wrap tutorial actions in `beat` / `beat_end` so narration begins before the operation it explains.
4. Use `focus.duration_ms: 0` for the final explanatory focus of a tutorial beat when it should remain visible through the remaining narration time.
5. Run `clock-storybook validate` until schema and semantics are clean.
6. Run `python scripts/render_tutorials.py "<tutorial name>" --audio-only` to hear the real takes and create measured timing.
7. Adjust prose/tags if necessary; unchanged segment audio remains cached.
8. Run `python scripts/render_tutorials.py "<tutorial name>"` for the sparse picture render and final cue-aligned master.
9. Use the low-level `frames` command only when individual frame inspection is needed.
10. Keep all generated output under ignored `tutorial-output/`; never add it to a source bundle.

Do not replace physical UI actions with direct product-state setters. If a workflow cannot be expressed through the existing module controls, patch actions and external stimuli, extend the Storybook contract explicitly and regression-test that addition first.

## 16. Useful verification commands

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

## 17. Focus overlays and subtitle safe area

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

<h6 align="center">From Munich with &#9829;</h6>
