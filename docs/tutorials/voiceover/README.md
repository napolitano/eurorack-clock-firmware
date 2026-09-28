<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK tutorial voice-over scripts

This directory contains the authored US-English narration for the eleven editable Storybook examples. Narration is now a first-class **local publication input** rather than a manually aligned afterthought: the local renderer creates the speech first, measures its real duration, resolves the Story presentation holds from those measurements, renders the picture, and finally muxes the speech onto Storybook's own narration cue timeline.

The human-readable `.txt` files remain the spoken source of truth. [`segments.json`](segments.json) maps stable Storybook narration IDs such as `s01` and `s02` to one or more script paragraphs. The Story YAML contains the same IDs on bounded scenes or timed tutorial actions. This keeps prose, visible state and media timing explicitly related without storing generated audio in the repository.

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

For an actual local render, `scripts/render_tutorials.py` calls ElevenLabs segment by segment, measures each encoded audio file with `ffprobe`, and makes that real duration authoritative. The resolved Story adds a small visual headroom to every segment and keeps minimum opening/closing budgets of 18/20 seconds unless local configuration raises them. The checked-in Story YAML remains editable source; resolved timing lives only under ignored `tutorial-output/`.

Generated narration audio and its local cache are intentionally ignored by Git and must never be added to source packages.

See [`TIMING.md`](TIMING.md) for the timing model and Example 11 setting checklist. See [`../VIDEO_GENERATION.md`](../VIDEO_GENERATION.md) for `.env`, ElevenLabs and local rendering setup.

## Script map

1. `01-power-and-first-clock.txt`
2. `02-transport-basics.txt`
3. `03-encoder-tempo.txt`
4. `04-tap-tempo.txt`
5. `05-topology-and-channel.txt`
6. `06-clock-mode.txt`
7. `07-euclidean-rhythm.txt`
8. `08-sequencer-basics.txt`
9. `09-divider-bank.txt`
10. `10-external-sync.txt`
11. `11-eight-independent-clocks-walkthrough.txt`

<h6 align="center">From Munich with &#9829;</h6>
