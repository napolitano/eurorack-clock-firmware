<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook example stories

These twelve files are a didactic, copy-and-edit Storybook starter set. They are deliberately separate from the canonical CI reference catalog in `../stories/`.

- **Power and First Clock** — `01-power-and-first-clock.yaml`: power-up, PLAY, tempo and STOP.
- **Transport Basics** — `02-transport-basics.yaml`: PLAY, PAUSE, resume and STOP.
- **Encoder Tempo** — `03-encoder-tempo.yaml`: master BPM with the encoder.
- **Tap Tempo** — `04-tap-tempo.yaml`: Tap Tempo without changing CLOCK SOURCE.
- **Topology and Channel** — `05-topology-and-channel.yaml`: select Independent topology and one output.
- **Clock Mode** — `06-clock-mode.yaml`: Independent Clock with visible gate telemetry.
- **Euclidean Rhythm** — `07-euclidean-rhythm.yaml`: select Euclid and edit a useful pattern.
- **Sequencer Basics** — `08-sequencer-basics.yaml`: select Sequencer and toggle a step.
- **Divider Bank** — `09-divider-bank.yaml`: Divider Bank and divider-family selection.
- **External SYNC and RST** — `10-external-sync.yaml`: external SYNC lock plus independent RST phase reset.
- **Eight Independent Clocks Walkthrough** — `11-eight-independent-clocks-walkthrough.yaml`: full One Clock → eight Independent Clocks workflow, channel selection, long-press settings, and a separate on-screen explanation for MODE and each TIMING, CLOCK, and OUTPUT row.
- **Groove Editor Walkthrough** — `12-groove-editor-walkthrough.yaml`: standalone Custom Groove Editor walkthrough with a minimal hand-off from Eight Independent Clocks Walkthrough, editor entry, fine/coarse microtiming gestures, step navigation, zoom, context menu, pattern length, save, Amount/Rotate and discard protection.

All examples change CLOCK only through recorded physical controls, patch actions or external stimuli. `setup:` establishes only pre-recording factory/power conditions.

Every example also:

- prepends the tracked South Signal Lab intro and appends the tracked South Signal Lab outro from `../assets/video/`;
- has a matching US-English expressive voice-over script under [`../voiceover/`](../voiceover/README.md);
- carries stable `narration:` IDs that map those script paragraphs to concrete visible Story states via `../voiceover/segments.json`;
- retains readable authored timing, while the local publication job derives the final holds from measured narration audio.

The preferred publication path is local:

```bash
python scripts/render_tutorials.py "Eight Independent Clocks Walkthrough"
```

For all twelve videos:

```bash
python scripts/render_tutorials.py --all
```

The local runner creates ElevenLabs speech first, measures the actual segment duration, writes ignored resolved Story copies, uses tutorial narration beats so each voice-over starts before the actions it explains, renders through the optimized sparse Storybook path, then aligns narration to the publication cue sidecar. Generated audio/video, resolved stories, frame data, sidecars and manifests stay under ignored `tutorial-output/` and are not repository source.

For local `.env`/ElevenLabs configuration and low-level Storybook commands, see [`../VIDEO_GENERATION.md`](../VIDEO_GENERATION.md).

<h6 align="center">From Munich with &#9829;</h6>
