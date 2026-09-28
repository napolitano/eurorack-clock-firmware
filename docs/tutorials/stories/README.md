<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook reference stories

This directory is the canonical authored Storybook source. Story YAML belongs here; executable Storybook implementation remains under `sim/tutorial/`.

## Phase-1 reference catalog

The Phase-1 teaching sequence is intentionally didactic rather than a feature dump:

1. [`getting-started.yaml`](getting-started.yaml) — POWER, boot, PLAY, tempo, STOP.
2. [`play-stop.yaml`](play-stop.yaml) — PLAY, PAUSE, resume and authoritative STOP.
3. [`changing-tempo.yaml`](changing-tempo.yaml) — master-BPM encoder editing.
4. [`tap-tempo.yaml`](tap-tempo.yaml) — multi-tap internal tempo measurement without SOURCE mutation.
5. [`selecting-operating-topology.yaml`](selecting-operating-topology.yaml) — real mode-carousel selection and confirmation.
6. [`selecting-independent-channel.yaml`](selecting-independent-channel.yaml) — enter Independent topology and select one of eight channels.
7. [`clock-mode.yaml`](clock-mode.yaml) — Independent Clock with real gate telemetry.
8. [`euclidean-mode.yaml`](euclidean-mode.yaml) — select Euclid and edit STEPS/HITS through the real Settings UI.
9. [`sequencer-mode.yaml`](sequencer-mode.yaml) — select Sequencer and toggle a real step in the production editor.
10. [`divider-bank.yaml`](divider-bank.yaml) — select Divider Bank and edit the bank family.
11. [`saving-loading-preset.yaml`](saving-loading-preset.yaml) — save slot 1, modify the working state, then restore the preset.
12. [`external-sync.yaml`](external-sync.yaml) — cable/generator independence, two-edge lock, AUTO and manual STOP precedence.
13. [`external-rst.yaml`](external-rst.yaml) — patched reset, explicit pulse and transport-independent reset semantics.

Every reference starts from an explicit pre-recording setup and performs product changes only through recorded module controls, patch actions or external stimuli. `setup:` may establish factory persistence/power preconditions but must not set BPM, topology, channel mode, transport, SOURCE or presets directly.

## Physical modifier gestures

CLOCK uses real simultaneous front-panel gestures. Schema 1 therefore supports explicit button levels:

```yaml
- button:
    name: TAP
    state: down
- encoder_push: {}
- button:
    name: TAP
    state: up
```

Omitting `state` remains one normal press/release click. Explicit holds must be balanced before the story ends. Existing encoder long-push workflows use an actual hold duration:

```yaml
- encoder_push:
    hold_ms: 800
```

These operations still drive the production control path; they are not direct navigation setters.

## CI contract

`storybook_reference_stories_tests` parses and semantically validates all 13 files, rejects duplicate IDs, executes every story twice on independent `SimulatorRuntime` instances, compares the complete logical traces and verifies the intended final production state. The normal `simulator-headless` CTest run therefore validates the full authored catalog on all native-simulator CI operating systems.

The `examples/` directory contains schema fixtures and deliberate failure cases. It is not part of the canonical publication sequence.

<h6 align="center">From Munich with &#9829;</h6>
