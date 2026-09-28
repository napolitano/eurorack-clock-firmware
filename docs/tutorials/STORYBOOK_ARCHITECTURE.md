<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook Architecture — Schema 1 / Phase 1

Status: **PROPOSED / implementation contract frozen by SB-0**.

## Ownership

CLOCK production firmware remains authoritative for UI state, navigation, tempo, transport, modes, SOURCE selection, SYNC/RST interpretation, gates, persistence-visible behaviour, screensavers, and all product UI.

The simulator remains authoritative for physical/environment emulation: encoder/button input, virtual POWER, SYNC/RST cable/source semantics, virtual time, framebuffer access, gate telemetry, scope telemetry, and simulator persistence.

Storybook owns orchestration and presentation only. It must not contain an alternate CLOCK state machine.

## Dependency direction

```text
Story YAML
   ↓
Host-only Storybook parser/model
   ↓
Story runner
   ↓
StorySimulatorPort
   ↓
existing SimulatorRuntime boundaries
   ↓
production ClockApplication / ClockEngine / UI
```

Rendering is one-way:

```text
production OLED framebuffer ─┐
PanelLayout + physical state ├─→ tutorial compositor → lossless frames
real gate/LED telemetry ─────┤
optional real scope telemetry ┤
story presentation events ───┘
                                      ↓
                                publication encoder
                                      ↓
                                  MP4 / WebM
```

Forbidden reverse dependencies are enforced by `scripts/check_architecture.py`: embedded `src/` and `lib/` code must not include or depend on `sim/tutorial` Storybook code.

## Interaction classes

The type split is normative:

- **Module Controls:** encoder, encoder push, PLAY, TAP, STOP/BACK, POWER.
- **Patch Actions:** SYNC cable connect/disconnect, RST cable connect/disconnect.
- **External Stimulus:** SYNC generator/source parameters, RST generator/pulse.
- **Presentation Actions:** subtitles and optional scope visibility.
- **Flow/Validation:** wait, wait_until, assert.

Cable state, external source activity, and CLOCK `SOURCE` configuration are independent concepts.

## POWER

Recorded POWER ON/OFF operations use the existing simulator power boundary. Storybook never fabricates a powered-off framebuffer, transport reset, boot screen, or safe gate state. POWER presentation and simulator input must occur together.

## Scope

The developer scope is hidden by default. A story may explicitly request it as a presentation stream. Phase 1 supports the currently visible/selected CLOCK channel (`channel: visible`). Scope samples come from existing simulator telemetry; Storybook must not infer waveforms from BPM, sequencer steps, or semantic state.

## Timing domains

Three clocks remain independent:

1. **Firmware time** advances production scheduling, gates, sync/reset, and screensavers.
2. **Interaction time** controls deterministic human pacing for physical actions.
3. **Presentation time** controls cards, subtitles, scope visibility, and frame composition.

Phase 1 freezes firmware time during pure `chapter`, `text`, and `callout` scenes. Tutorial actions and explicit `wait`/`wait_until` operations advance firmware time. Storybook never slows or stretches the CLOCK scheduler to improve readability.

## Deterministic output profile

Schema 1 defaults to **1920×1080 at 30 fps**. Stories may override the output profile explicitly. OLED pixels are scaled only by integer nearest-neighbour factors inside the composition.

The deterministic contract covers logical event timing, lossless frame pixels, and subtitle timing. Byte-identical MP4/WebM output is not required across different encoder versions.

Lossless intermediate frames are PNG/RGBA8 unless a later schema explicitly changes that contract.

## Story parser

Schema 1 uses a **project-local strict YAML subset parser** in `sim/tutorial/story_yaml.*`. The accepted authoring surface is deliberately narrower than generic YAML: block mappings/sequences, quoted/plain scalars, literal `|` text, and empty `{}` / `[]` collections. Tabs, anchors, aliases, tags and non-empty flow collections fail explicitly.

This removes an otherwise unnecessary host package dependency while retaining YAML authoring. The parser remains host-only and must never enter PlatformIO, embedded firmware, `src/`, or `lib/clock_core`. Normal firmware and simulator builds require no external YAML library.

## Fonts

Tutorial text rendering is host-only. Themes must resolve requested fonts explicitly; silent system-family substitution is prohibited. For deterministic publication, an externally supplied font file may be paired with an expected SHA-256. Font binaries are not source-bundle artifacts.

## Setup boundary

Schema 1 setup is intentionally narrow. It may establish deterministic preconditions such as factory/persistence state, initial power state, and external-source parameterisation before recorded frames begin. It must not become a general `setClockState(...)` facility.

Recorded product changes happen through real controls, patch operations, and external stimulus only.

## Stream composition

A tutorial frame is composed from synchronized logical streams:

- production OLED framebuffer;
- panel/physical interaction/patch state from `PanelLayout`;
- real gate/LED telemetry;
- optional scope telemetry;
- Storybook presentation/subtitle events.

The Storybook controls visibility and timing but does not manufacture product state.

## Publication

The default final format is **MP4/H.264**. WebM is additionally supported by the planned publication stage.

Optional pre-produced intro/outro clips are publication assets, not Storybook-rendered scenes:

```text
intro video? → generated CLOCK tutorial → outro video? → final MP4/WebM
```

The publication compositor probes and normalizes referenced clips to the selected output profile. Intro/outro may carry their own audio. The generated tutorial body is initially audio-free. Final SRT/WebVTT timestamps are offset by the actual intro duration.

A missing/unreadable referenced media asset is a publication failure, not a reason to publish a partial result.

## Generated-artifact policy

The following are generated/local artifacts and must stay outside source bundles:

- rendered frames;
- MP4/WebM output;
- generated SRT/VTT;
- publication manifests;
- local encoder/probe output;
- coverage/sanitizer/verification reports;
- temporary media-normalisation files.

The repository `.gitignore` defines the local generated paths.

## SB-1 strict simulator port

`sim/tutorial/story_simulator_port.*` is the only Storybook semantic adapter implemented against `SimulatorRuntime` in SB-1. It deliberately tightens interactive-simulator convenience semantics where recording requires an unambiguous physical model:

- `sync_generator` and `rst_generator` run/hold fail unless their cable is already connected;
- one-shot external RST fails without an RST cable;
- environment operations never write `ClockState::source`;
- POWER uses `SimulatorRuntime::setPower()`;
- controls other than POWER are rejected while the module is off;
- one encoder action is exactly one `-1` or `+1` detent;
- exact source parameters are reached only through existing simulator source controls.

This adapter does not own firmware behaviour. It rejects invalid orchestration and delegates accepted operations to the existing simulator boundary.


## SB-3 deterministic Story Runner

`sim/tutorial/story_runner.*` now executes validated schema-1 stories against `StorySimulatorPort`. It owns orchestration only and emits a deterministic logical trace for later presentation/render synchronization.

Runner timing obeys the three-domain contract:

- pure `chapter`, `text`, and `callout` duration advances presentation time only;
- physical controls, patch actions, external stimulus pacing, explicit `wait_ms`, and `wait_until` advance presentation and the real simulator clock together;
- POWER OFF naturally stops firmware-time advancement because `SimulatorRuntime` is powered down, while presentation timing continues;
- `wait_until` uses a fixed 1 ms observation cadence and fails with story/scene/action context on timeout;
- assertions read production/simulator telemetry only and never repair state to make a story pass.

The runner emits stable trace events for scene/action boundaries, physical control down/up, encoder detents, patch state, generator state, external-source configuration, RST pulses, subtitles, scope visibility, successful waits, and successful assertions. The trace is not an alternate firmware state model; it is an orchestration/presentation synchronization record.

The schema-1 setup boundary now has an explicit host implementation: `factory_reset: true` clears the simulator persistence image to the erased/factory precondition and reboots through the existing simulator lifecycle. It does not write `ClockState` directly.

`docs/tutorials/stories/external-sync.yaml` is the SB-3 reference story. Automated integration runs it twice from independent fresh simulator instances and requires identical logical traces while proving AUTO acquisition/lock, real firmware auto-start, generator hold/reacquisition, unchanged CLOCK `SOURCE`, and manual STOP precedence while incoming SYNC remains locked.

## SB-4 front-panel presentation layer

SB-4 introduces a one-way physical-presentation sink and deterministic panel layer without adding any reverse dependency into CLOCK. `StoryRunner` may notify a `StoryPresentationSink` at the exact presentation timestamp of an accepted simulator action; the sink cannot mutate `SimulatorRuntime` or `ClockState`.

`PanelPresentationTimeline` records only transient presentation facts that the simulator does not expose directly: button/encoder-push depression, plug insertion/removal motion, scope visibility, and the visible record of POWER actions. Stable product/environment facts remain authoritative elsewhere:

- encoder position comes from `SimulatorRuntime::encoderVisualPosition()`;
- POWER comes from `SimulatorRuntime::poweredOn()`;
- SYNC/RST cable and signal state come from simulator input telemetry;
- all eight LED states come from real gate telemetry through the shared `panelLedVisuallyLit()` rule;
- every control/jack/LED coordinate and size comes from `PanelLayout`.

Recorded patch actions now invoke the real simulator cable operation at the **start** of the visible insertion/removal motion. The deterministic `patch_action_ms` interval then represents physical motion only. This satisfies the physical interaction contract: the video never shows a patch operation that has not reached the simulator, and the simulator never receives a recorded patch operation without a matching visible motion.

`panel_dynamic_layer.*` rasterizes only dynamic host presentation into a transparent RGBA8 layer: encoder indicator/push feedback, depressed buttons, visible SYNC/RST plugs/cables, and the eight actual activity LEDs. Static panel art, OLED composition, typography, chapters and subtitles remain SB-5 responsibilities. The layer is deliberately headless and uses no SDL or desktop capture.

The interactive SDL panel and Storybook now share the same LED visual-persistence helper so short real gate pulses are presented consistently without either renderer synthesizing gate activity.

