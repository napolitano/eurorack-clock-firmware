<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook Schema 1

Status: **CURRENT development interface / SB-5 renderer implemented**.

## Required story fields

Every story declares:

```yaml
schema: 1
id: getting-started
title: "Getting Started"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
```

Unknown mandatory fields, unsupported schema versions, unknown scene/action kinds, or semantically invalid operations must fail validation. Silent fallback is prohibited when meaning could change.

## Output defaults

```yaml
output:
  width: 1920
  height: 1080
  fps: 30
```

Omitting `output` uses the same 1920×1080/30-fps contract.

## Publication assets

Pre-produced intro/outro clips are optional and are resolved relative to the story file unless an absolute publication-tool path is explicitly allowed by the runner configuration:

```yaml
publication:
  intro_video: "../assets/video/south-signal-lab-intro.mp4"
  outro_video: "../assets/video/south-signal-lab-outro.mp4"
```

They are not Storybook scenes and do not run firmware time.

## Setup

Schema 1 setup establishes preconditions before recorded frames begin. It is not recorded user interaction.

```yaml
setup:
  factory_reset: true
  power: on
```

Direct mutation of tempo, topology, menus, transport, CLOCK SOURCE, presets, or other product behaviour is not part of the recorded-story contract.

## Scenes

Initial scene kinds are `chapter`, `tutorial`, `text`, and `callout`.

Phase 1 supports only:

```yaml
transition:
  type: cut
```

`fade` is a known future transition but must fail Phase-1 validation.

## Module Controls

Recorded module controls represent visible physical operation plus real simulator input.

```yaml
- encoder:
    direction: clockwise
    detents: 2
- encoder_push: {}
- encoder_push:
    hold_ms: 800
- button:
    name: PLAY
# Optional explicit levels support real modifier gestures. Omitting state remains a normal click.
- button:
    name: TAP
    state: down
- encoder_push: {}
- button:
    name: TAP
    state: up
- power:
    state: off
- power:
    state: on
```

Stable control spellings are `encoder`, `encoder_push`, `PLAY`, `TAP`, `STOP_BACK`, and `power`. `encoder_push.hold_ms` records the real press duration and therefore supports existing long-push workflows without bypassing CLOCK navigation. `button.state` is optional; `down`/`up` keeps the physical button held across following actions so real CLOCK modifier chords can be recorded. Every explicit `down` must be balanced by `up` before the story ends.

## Patch Actions

```yaml
- sync_cable:
    state: connect
- sync_cable:
    state: disconnect
- rst_cable:
    state: connect
- rst_cable:
    state: disconnect
```

Connect uses the simulator's existing cable semantics, including starting the configured generator. Disconnect removes the virtual cable. Neither operation changes CLOCK `SOURCE`.

## External Stimulus

```yaml
- sync_generator:
    state: hold
- sync_generator:
    state: run
- sync_source:
    bpm: 120
    ppqn: 4
    waveform: square
- rst_generator:
    state: hold
- rst_pulse: {}
```

Generator run/hold requires the corresponding cable to be connected. Storybook validation must reject generator operations against an unpatched input.

## Presentation Actions

Subtitles and scope visibility are presentation-only:

```yaml
- subtitle:
    text: "Watch channel 3 follow the external clock."
- scope:
    state: show
    channel: visible
- scope:
    state: hide
```

Phase 1 accepts only `channel: visible` when scope is shown. Scope visibility never changes simulator state or firmware time by itself.

## Flow and validation

```yaml
- wait_ms: 1000
- wait_until:
    external_sync: locked
    timeout_ms: 3000
- assert:
    clock_source: auto
```

Timeouts and failed assertions abort generation.

## Pure presentation timing

`chapter`, `text`, and `callout` scene duration advances presentation time but not firmware time in Phase 1. This prevents invisible product evolution behind full-frame explanatory material.

## Interaction profiles

Schema 1 reserves the deterministic profile names:

- `HUMAN_SLOW`
- `HUMAN_NORMAL`
- `HUMAN_FAST`

Exact durations are defined centrally in [`interaction_profiles.yaml`](interaction_profiles.yaml), not duplicated in every story. All three reserved Phase-1 profiles are currently defined and deterministic; no random jitter is permitted.

## Publication result

The generated tutorial is composed first. Optional pre-produced intro/outro clips are concatenated only in the publication stage. Default final media is MP4/H.264; WebM/VP9 is optional. Relative publication-asset paths resolve from the Story YAML file.

SB-7 probes every referenced clip before composition. Video is aspect-preserving scaled/padded to the Story output geometry and normalized to the Story FPS while preserving the probed source duration. If any intro/outro has audio, publication normalizes to 48 kHz stereo and inserts silence only into otherwise silent segments; otherwise final media has no audio stream. Final SRT/WebVTT cue times are offset by the normalized intro duration only.


## Physical panel presentation

SB-4 binds recorded physical actions to a one-way presentation timeline. Button/encoder-push down/up state is visible for the exact deterministic interaction interval. Encoder rotation uses the simulator's accepted detent position. SYNC/RST patch actions use `patch_action_ms` as visible insertion/removal time while invoking the real cable operation at the start of that motion.

The dynamic panel layer never owns CLOCK behaviour. POWER, cable connection, signal level, encoder position and all eight LEDs are read from `SimulatorRuntime`; geometry is read exclusively from `PanelLayout`. Scope visibility remains a presentation state and does not alter firmware time or simulator state.

## Execution contract

SB-3 executes validated stories through `StoryRunner` and `StorySimulatorPort`. The runner produces a deterministic logical trace; the same story, firmware/simulator revision, resource files, and starting simulator state must produce the same trace timestamps and events.

Physical actions use the selected interaction profile. `wait_ms` advances the real simulator clock for exactly the requested duration. `wait_until` polls the relevant read-only telemetry at a deterministic 1 ms cadence until the expected state is observed or `timeout_ms` expires. A timeout or failed assertion aborts execution with story, scene, action, and source-line context.

`factory_reset: true` is a pre-recording simulator-persistence precondition. It does not represent visible user interaction and does not directly write CLOCK application state. Recorded product changes still occur only through the module-control, patch, and external-stimulus action classes.

## Theme and text-layout contract

Phase-1 themes are stored under `docs/tutorials/themes/`. Font entries are explicit mappings:

```yaml
fonts:
  body:
    family: "CLOCK UI"
    size_px: 28
  monospace:
    family: "CLOCK Mono"
    size_px: 21
line_spacing_px: 12
```

Supported deterministic Phase-1 faces are `CLOCK UI` and `CLOCK Mono`. Sizes must be positive multiples of 7 pixels because they scale the project-owned 5×7 glyph source by exact integer factors. Unsupported requested fonts fail; there is no implicit system-font substitution.

Chapter, text, callout and subtitle strings are measured and wrapped before rasterization. Horizontal or vertical overflow is a generation error.

## Phase-1 frame composition

Tutorial frames use the story output profile (default 1920×1080). OLED pixels always come from the real production framebuffer and are enlarged only by an integer nearest-neighbour factor. The panel side reuses `PanelLayout` geometry and shows the same production OLED framebuffer inside the simulated module.

When `scope: {state: show, channel: visible}` is active, only the current production-UI selected channel is shown. Scope traces come from real gate-transition telemetry and the existing transport-referenced scope timeline. Storybook does not infer a waveform from BPM, steps, or story content.

The subtitle strip is outside the tutorial content area and therefore never obscures CLOCK.

## Generated Phase-1 artifacts

Schema 1 authoring does not name individual output frames or subtitle sidecars. The host generation pipeline derives them deterministically from the Story ID and output profile:

```text
frames/frame-000000.png
frames/frame-000001.png
...
<story-id>.srt
<story-id>.vtt
manifest.json
```

Frame timestamps are exact rational presentation timestamps (`floor(frame_index * 1,000,000 / fps)`). Burned-in subtitles, SRT, and WebVTT are generated from the same timed Story subtitle events. Intro/outro offsets are not applied at this stage; the SB-7 publication compositor applies those after probing the actual pre-produced intro duration.
