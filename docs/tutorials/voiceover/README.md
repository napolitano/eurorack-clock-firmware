<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK tutorial voice-over scripts

This directory contains the US-English narration scripts for the eleven editable Storybook examples. Voice-over is deliberately a **post-production layer**: Storybook renders the deterministic tutorial and bumper composition first; narration generated from these scripts is added afterwards in the video editor.

The `.txt` files are written for **Eleven v3**. They use sparse natural-language Audio Tags in square brackets and punctuation/paragraph structure for pacing. Do not convert those scripts to SSML `<break>` markup when using Eleven v3. Generate paragraph-by-paragraph when tighter editorial timing is required; this also makes it easier to regenerate one sentence without changing the rest of the narration.

Every script follows the same South Signal Lab framing, but the closing wording is intentionally varied slightly from video to video:

- opening: `Hi, this is South Signal Lab. My name is Axel, and today I’ll show you ...`
- closing: the same thank-you / follow / Ko-fi message, lightly varied so repeated videos do not sound templated.

After the 4.5-second pre-produced intro, every example reserves an 18-second opening card. The current closing scripts need up to roughly 20 seconds at the reference pace, so the final Storybook scene now holds for 24 seconds before the 5.5-second outro. The measured 16-second sign-off is a pacing reference, not the measured duration of these newly written scripts.

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
