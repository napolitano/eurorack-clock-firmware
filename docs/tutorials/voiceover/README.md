<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK tutorial voice-over scripts

This directory contains the authored US-English narration for the twelve editable Storybook examples. Narration is now a first-class **local publication input** rather than a manually aligned afterthought: the local renderer creates the speech first, measures its real duration, resolves the Story presentation holds from those measurements, renders the picture, and finally muxes the speech onto Storybook's own narration cue timeline.

The human-readable `.txt` files remain the spoken source of truth. [`segments.json`](segments.json) maps stable Storybook narration IDs such as `s01` and `s02` to one or more script paragraphs. The Story YAML contains the same IDs on bounded non-tutorial scenes or explicit tutorial `beat` / `beat_end` containers. This keeps prose, visible actions and media timing explicitly related without storing generated audio in the repository.

The scripts use purposeful Eleven expressive Audio Tags in square brackets together with punctuation and paragraph structure. Tags are performance direction rather than decoration: `[warmly]` for the opening, `[conversational]` for normal explanation, `[thoughtful]` or `[slowly]` where a distinction needs space, `[confidently]` or `[with emphasis]` for an important rule, and `[pause]` only where an actual pause belongs. The configured ElevenLabs TTS model must support the authored tags; the local pipeline does not silently rewrite them.

## Opening contract

Every tutorial is designed to work as a standalone video. The opening therefore does three things before detailed operation begins:

1. identifies South Signal Lab and Axel;
2. states clearly what this video is going to demonstrate;
3. gives enough product context to understand what CLOCK is and why the demonstrated function matters.

The canonical opening form begins:

> Hi, this is South Signal Lab. My name is Axel, and today I’ll show you ...

Light subject-specific variation after that sentence is expected. The identity and orientation must not be shortened away.

## Closing contract

Every tutorial keeps the full sign-off intent rather than collapsing it into a short promotional line. The closing must:

- mark the end of the tutorial;
- thank the viewer for their interest and time;
- mention staying up to date and optional support;
- name South Signal Lab / Ko-fi appropriately;
- refer to behind-the-scenes development insight;
- end with a natural goodbye.

Wording may vary slightly from video to video, but those elements stay present.

## Timing contract

The user's measured reference delivery remains useful while authoring: roughly **15 seconds for the opening**, **16 seconds for the sign-off**, and approximately **195 spoken words/minute** as a planning pace. These values are no longer the final publication clock.

For an actual local render, `scripts/render_tutorials.py` calls ElevenLabs segment by segment, measures each encoded audio file with `ffprobe`, and makes that real duration authoritative. The resolved Story adds a small visual headroom to every segment, never shortens a checked-in beat below its authored visual/action minimum, and keeps minimum opening/closing budgets of 18/20 seconds unless local configuration raises them. In tutorial scenes the resolved duration belongs to the whole beat: narration starts before its actions, those actions consume part of the same budget, and `beat_end` holds only the remaining spoken time. The checked-in Story YAML remains editable source; resolved timing lives only under ignored `tutorial-output/`.

Generated narration audio and its local cache are intentionally ignored by Git and must never be added to source packages.

See [`TIMING.md`](TIMING.md) for the timing model and the Eight Independent Clocks Walkthrough / Groove Editor Walkthrough checklists. See [`../VIDEO_GENERATION.md`](../VIDEO_GENERATION.md) for `.env`, ElevenLabs and local rendering setup.

## Script map

- **Power and First Clock** — `01-power-and-first-clock.txt`
- **Transport Basics** — `02-transport-basics.txt`
- **Encoder Tempo** — `03-encoder-tempo.txt`
- **Tap Tempo** — `04-tap-tempo.txt`
- **Topology and Channel** — `05-topology-and-channel.txt`
- **Clock Mode** — `06-clock-mode.txt`
- **Euclidean Rhythm** — `07-euclidean-rhythm.txt`
- **Sequencer Basics** — `08-sequencer-basics.txt`
- **Divider Bank** — `09-divider-bank.txt`
- **External SYNC and RST** — `10-external-sync.txt`
- **Eight Independent Clocks Walkthrough** — `11-eight-independent-clocks-walkthrough.txt`
- **Groove Editor Walkthrough** — `12-groove-editor-walkthrough.txt`

<h6 align="center">From Munich with &#9829;</h6>
