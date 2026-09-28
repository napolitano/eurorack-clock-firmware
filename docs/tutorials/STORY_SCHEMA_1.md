<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook Schema 1

Status: **PROPOSED / SB-0 frozen interface**. Parser implementation follows in SB-2.

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
- encoder_push:
    state: press
- button:
    name: PLAY
- power:
    state: off
- power:
    state: on
```

Stable control spellings are `encoder`, `encoder_push`, `PLAY`, `TAP`, `STOP_BACK`, and `power`.

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

Exact durations are defined centrally by Storybook configuration, not duplicated in every story.

## Publication result

The generated tutorial is composed first. Optional pre-produced intro/outro clips are concatenated only in the publication stage. Default final media is MP4/H.264; WebM is optional.
