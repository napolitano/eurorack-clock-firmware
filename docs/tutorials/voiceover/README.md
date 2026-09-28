<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK tutorial voice-over scripts

This directory contains the US-English narration scripts for the eleven editable Storybook examples. Voice-over remains a **post-production layer**: Storybook renders the deterministic tutorial and bumper composition first; narration generated from these scripts is added afterwards in the video editor.

The scripts target **Eleven v3** and follow the official Audio Tags guidance. Tags are used as performance direction rather than decoration: `[warmly]` for the opening, `[conversational]` for normal explanation, `[thoughtful]` or `[slowly]` where a distinction needs space, `[confidently]` or `[with emphasis]` for an important rule, and `[pause]` only where an actual pause belongs. Punctuation, ellipses, paragraph boundaries and normal sentence rhythm remain part of the direction. Do not convert the scripts to SSML `<break>` markup; Eleven v3 does not use SSML break tags.

The intended delivery is friendly, technically precise and human rather than announcer-like. Audio Tags must support that delivery, not turn the tutorials into a dramatic performance. Generate paragraph by paragraph when a section needs a different take or tighter synchronization, and always listen for a tag that the chosen voice interprets badly or reads aloud.

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

Wording may vary slightly from video to video, but those elements stay present. The closing is deliberately given enough screen time to be spoken naturally.

## Timing contract

The user's measured reference delivery is roughly **15 seconds for the opening** and **16 seconds for the closing**. Storybook therefore reserves **18 seconds for the opening card** and **20 seconds for the closing card** in all eleven examples. The body timing is derived from the actual narration length at roughly **195 spoken words per minute**, then padded for Audio Tags, natural pauses, control actions and visual comprehension.

Generated narration audio (`.wav`, `.mp3`, `.m4a`, `.aac`, `.flac`) is intentionally ignored by Git and must not be added to source packages.

See [`TIMING.md`](TIMING.md) for the per-example planning values and the detailed Example 11 setting checklist.

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
