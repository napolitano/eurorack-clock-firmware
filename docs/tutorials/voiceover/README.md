<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK tutorial voice-over scripts

This directory contains the US-English narration scripts for the eleven editable Storybook examples. Voice-over is deliberately a **post-production layer**: Storybook renders the deterministic tutorial and bumper composition first; narration generated from these scripts is added afterwards in the video editor.

The `.txt` files are written for **Eleven v3**. They sound like Axel explaining what he does at the front panel, rather than reciting UI labels. Audio Tags are limited to moments when the delivery actually changes: `[warmly]` at the opening, occasional `[slowly]` for a distinction, and a single `[pause]` before the reset result. Natural punctuation and paragraph breaks carry most of the rhythm. See ElevenLabs’ [Audio Tags 101](https://elevenlabs.io/blog/v3-audiotags) for v3 usage. Do not convert these scripts to SSML `<break>` markup. Generate paragraph by paragraph when a line needs a different take, and listen for any tag that gets read aloud instead of performed.

The opening identifies Axel and the one thing this video demonstrates. The closing refers to what was just shown, usually points to the next topic, and mentions South Signal Lab and Ko-fi once. The wording changes with the subject; it is not a fixed promo read.

After the 4.5-second pre-produced intro, examples 01–10 reserve 18 seconds for the opening and 20 seconds for the sign-off. The detailed example 11 reserves 16 seconds for the opening and 22 seconds for its longer sign-off. The measured reference is a pacing guide, not the duration of these unrecorded scripts.

Generated narration audio (`.wav`, `.mp3`, `.m4a`, `.aac`, `.flac`) is intentionally ignored by Git and must not be added to source packages.

See [`TIMING.md`](TIMING.md) for the narration-window contract and the detailed walkthrough timing notes.

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
