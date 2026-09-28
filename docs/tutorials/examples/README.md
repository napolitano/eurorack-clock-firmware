<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook example stories

These ten files are a didactic, copy-and-edit Storybook starter set. They are deliberately separate from the canonical CI reference catalog in `../stories/`.

1. `01-power-and-first-clock.yaml` — power-up, PLAY, tempo and STOP.
2. `02-transport-basics.yaml` — PLAY, PAUSE, resume and STOP.
3. `03-encoder-tempo.yaml` — master BPM with the encoder.
4. `04-tap-tempo.yaml` — Tap Tempo without changing CLOCK SOURCE.
5. `05-topology-and-channel.yaml` — select Independent topology and one output.
6. `06-clock-mode.yaml` — Independent Clock with visible gate telemetry.
7. `07-euclidean-rhythm.yaml` — select Euclid and edit a useful pattern.
8. `08-sequencer-basics.yaml` — select Sequencer and toggle a step.
9. `09-divider-bank.yaml` — Divider Bank and divider-family selection.
10. `10-external-sync.yaml` — external SYNC lock plus independent RST phase reset.

All examples change CLOCK only through recorded physical controls, patch actions or external stimuli. `setup:` establishes only pre-recording factory/power conditions.

Validate one example before rendering:

```bash
./build/simulator-headless/clock-storybook validate docs/tutorials/examples/01-power-and-first-clock.yaml
```

Generate a final MP4 only when you actually want the media output:

```bash
./build/simulator-headless/clock-storybook video docs/tutorials/examples/01-power-and-first-clock.yaml \
  --output docs/tutorials/generated/example-01 \
  --format mp4
```

See [`../VIDEO_GENERATION.md`](../VIDEO_GENERATION.md) for the complete workflow.

<h6 align="center">From Munich with &#9829;</h6>
