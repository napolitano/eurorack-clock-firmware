<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook Phase-1 acceptance evidence

Status: **CURRENT implementation evidence for the 40 frozen Phase-1 requirements**.

This matrix is an evidence index. The normative behavior remains the Storybook architecture/schema contracts and production CLOCK source; this document does not create alternate simulator or firmware semantics.

| # | Phase-1 requirement | Implementation evidence | Regression / gate |
| ---: | --- | --- | --- |
| 1 | Versioned Story format | `schema: 1`, typed `Story` model | `storybook_parser_tests` |
| 2 | Executable YAML, not presentation-only prose | strict local YAML subset parser + `StoryRunner` | parser + runner tests |
| 3 | Production CLOCK remains behavioral source of truth | `StorySimulatorPort` wraps `SimulatorRuntime`/real `ClockApplication` | architecture + simulator-port tests |
| 4 | Module Controls use real simulator inputs | encoder, push, PLAY, TAP, STOP/BACK, POWER through port | runner/reference tests |
| 5 | Patch Actions are separate semantic actions | SYNC/RST connect/disconnect | parser/validator + port tests |
| 6 | External Stimulus is separate from module controls/patching | generator, BPM/PPQN/waveform, RST pulse | parser/validator + port tests |
| 7 | SYNC cable connect invokes existing connect semantics | `setPatchConnected(SyncCable, true)` delegates to runtime | simulator-port test |
| 8 | Generator hold/run preserves cable state | strict port rejects implicit connect and keeps patch unchanged | simulator-port + external-sync story |
| 9 | Environment operations never mutate CLOCK SOURCE | port has no SOURCE setter | architecture/tooling policy + external-sync story |
| 10 | User-visible SOURCE changes must use CLOCK UI | no setup/state mutation API for SOURCE | schema/architecture policy |
| 11 | OLED video uses the real production framebuffer | renderer reads `SimulatorRuntime::framebuffer()` | renderer tests |
| 12 | OLED scaling is integer nearest-neighbour | composition enforces integer raster scale | renderer tests |
| 13 | Panel geometry has one source | Storybook presentation consumes `PanelLayout` | panel-presentation tests |
| 14 | Encoder detents are visibly represented | one-way presentation sink + runtime visual position | panel-presentation tests |
| 15 | Encoder push is visibly represented | control down/up presentation state | panel-presentation tests |
| 16 | PLAY/TAP/STOP presses are visibly represented | control down/up presentation state | panel-presentation tests |
| 17 | Patching is visibly represented | deterministic plug/cable motion around real patch action | panel-presentation tests |
| 18 | LEDs/gates are never synthesized by Storybook | dynamic layer reads real gate telemetry/LED hold helper | panel-presentation tests |
| 19 | POWER ON/OFF is scriptable through real lifecycle | `StorySimulatorPort::setPower()` | simulator-port + getting-started story |
| 20 | Scope is optional and limited initially to visible channel | presentation `scope show/hide`, `channel: visible` | renderer/reference stories |
| 21 | Human interaction timing is deterministic | HUMAN_SLOW/NORMAL/FAST resource file | runner tests |
| 22 | Firmware, interaction and presentation time remain separate | `StoryRunner` owns presentation timing, port owns firmware advancement | runner tests |
| 23 | Pure chapter/text/callout time freezes firmware | presentation-only scene advancement | runner tests |
| 24 | Theme values are data-driven | `docs/tutorials/themes/` + strict loader | renderer tests |
| 25 | Fonts are reproducible with no silent system fallback | CLOCK UI / CLOCK Mono deterministic faces | renderer tests |
| 26 | Text clipping/overflow is a hard failure | measured wrapping and overflow exceptions | renderer/frame-pipeline tests |
| 27 | Phase-1 scene types are chapter/tutorial/text/callout with TIP/WARNING/RECIPE | typed scene/callout model | parser + renderer tests |
| 28 | Story subtitle events are the single subtitle source | runner timed Subtitle trace | frame-pipeline tests |
| 29 | Burn-in, SRT and WebVTT share timing/content | renderer + sidecars consume same timed subtitle events | frame-pipeline tests |
| 30 | `wait`, `wait_until` and `assert` are executable flow controls | deterministic 1-ms polling + read-only assertions | runner tests |
| 31 | Unknown/invalid syntax and semantics fail explicitly | strict parser, validator, contextual `StoryIssue` | parser tests |
| 32 | Phase 1 supports CUT only | validator rejects FADE | parser tests |
| 33 | Logical frames are deterministic | rational frame scheduler + raw RGBA FNV-1a hashes | frame-pipeline tests |
| 34 | Frame generation is desktop-capture-free | headless RGBA renderer/PNG writer | CMake headless tests |
| 35 | Default publication format is MP4/H.264 | host-only FFmpeg compositor, `libx264`, yuv420p | publication integration test |
| 36 | WebM is an optional publication format | VP9 + Opus when audio exists | publication integration test |
| 37 | Intro/outro are optional pre-produced videos | `publication.intro_video` / `outro_video`, ffprobe normalization | publication integration test |
| 38 | Mandatory generation/publication failure is atomic | staging + previous-output rollback | frame/publication failure regressions |
| 39 | Embedded build and existing simulator workflows remain isolated | no Storybook deps in PlatformIO/`src`/`lib/clock_core`; normal headless CTest retained | architecture/tooling/CI gates |
| 40 | Canonical teaching set and External-SYNC end-to-end semantics are continuously verified | 13 reference stories under `docs/tutorials/stories/`; catalog test executes each twice | `storybook_reference_stories_tests` + CI |

## Reference-story closure

The canonical Phase-1 catalog contains exactly 13 stories. `storybook_reference_stories_tests` parses and validates the complete catalog, validates unique IDs, executes every story twice against independent real simulator runtimes, compares deterministic traces, and verifies intended final production state. The External-SYNC story explicitly proves AUTO remains selected, two-edge lock/reacquisition uses the real firmware path, and manual STOP remains authoritative while incoming SYNC remains connected/running/locked.

## Publication and generated-output policy

Generated frames, subtitle sidecars, manifests, probes, MP4/WebM files and internal verification output remain build/publication artifacts. They are excluded from delivered source bundles.

<h6 align="center">From Munich with &#9829;</h6>
