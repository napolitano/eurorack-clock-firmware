<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK tutorial voice-over and picture timing

The authored scripts are the spoken source of truth; **measured generated audio is the publication timing source of truth**. Checked-in Story YAML keeps readable editorial budgets, while the local publication runner creates a resolved copy whose narration-linked scene or beat duration is calculated from the actual ElevenLabs audio files.

## Timing pipeline

```text
voice-over paragraphs
        ↓ segments.json
ElevenLabs TTS segments
        ↓ ffprobe actual duration
resolved Story YAML
        ↓ Story Runner narration cues
sparse/change-driven picture render
        ↓
intro + tutorial + outro
        ↓ cue-aligned narration mix
final local master
```

For each narration segment the local runner uses:

```text
story budget = max(authored minimum, measured audio duration + visual headroom)
```

and rounds upward to a deterministic timing quantum. The first and last narration segments additionally retain configurable minimum budgets for the established South Signal Lab opening and sign-off.

Default local policy from `.env.example`:

- body headroom: **1200 ms**;
- opening minimum: **18000 ms**;
- closing minimum: **20000 ms**;
- rounding quantum: **100 ms**.

These defaults can be changed locally without modifying source. The generated resolved stories and timing plans live under `tutorial-output/` and remain untracked.

## Authoring estimate only

The measured reference delivery — roughly 15 seconds for the opening, 16 seconds for the sign-off and about **195 spoken words/minute** overall — remains useful before TTS exists. It is not allowed to override a real rendered duration. Audio Tags, punctuation, breathing and the selected voice/model can all materially change the result.

## Stable narration IDs

[`segments.json`](segments.json) groups one or more adjacent script paragraphs under stable IDs (`s01`, `s02`, ...). Each ID occurs exactly once in the matching Story YAML: directly on a bounded non-tutorial scene or on one explicit tutorial `beat`, closed by the matching `beat_end`.

For tutorial beats, `NarrationBegin` occurs before the first enclosed action. Buttons, encoder moves, patches, waits and intermediate focus holds then consume presentation time while the narration is already running. At `beat_end`, Storybook holds only the remaining resolved audio budget. A zero-duration final focus may remain visible through that tail. If the actions themselves exceed the resolved budget, execution fails rather than silently shifting speech after the operation.

Storybook records the resulting begin/end presentation timestamps and publishes a `<story-id>.narration.json` sidecar after shifting the cue positions by the measured intro duration. That sidecar, not frame counting or YAML guesswork, positions the audio in the final master.

## Example 11: narrative structure

Example 11 is intentionally a full walkthrough rather than a fast feature demo. It starts with a proper orientation before the first control action:

- what South Signal Lab is showing;
- what CLOCK is;
- why Independent still uses one shared timing reference;
- what will be changed on channel 4;
- what will remain unchanged and only be explained.

The tutorial then shows the real topology switch and confirmation, the eight-channel overview, channel-four selection, the short encoder push versus long encoder press, and the four channel-setting groups `MODE · TIMING · CLOCK · OUTPUT`.

Every setting actually shown on screen receives its own explanation and narration anchor:

| Selected menu row | Narration purpose | Picture state |
| --- | --- | --- |
| MODE | Clock, Euclid, Sequencer and Off; scope is the selected channel only | MODE selected; unchanged |
| TIMING → DIV/MULT | integer divide/multiply relative to the shared reference | edit ×1 → ×2 and confirm |
| TIMING → NUMERATOR | upper half of a rational rate relationship | selected; unchanged |
| TIMING → DENOMINATOR | lower half of that rational relationship | selected; unchanged |
| TIMING → SWING | long/short event-spacing displacement without changing average tempo | selected; unchanged |
| TIMING → GROOVE | deterministic pattern-based timing; entry point only | selected; groove editor not opened |
| CLOCK → METER BEATS | beats in this channel's local measure | selected; unchanged |
| CLOCK → METER UNIT | note value that counts as one beat | selected; unchanged |
| OUTPUT → PROBABILITY | chance that an eligible event reaches the output | selected; unchanged |
| OUTPUT → GATE | physical HIGH time of the pulse | edit 10 → 20 ms and confirm |
| OUTPUT → PHASE | offset relative to the shared timing reference | selected; unchanged |
| OUTPUT → RESET | Global versus Sync Free reset behavior | selected; unchanged |
| OUTPUT → MUTE | silence the output without deleting its configuration | selected; unchanged |

`GROOVE` is explained as the entry to its own editor but that editor is not opened in Example 11. A separate editor would introduce settings not otherwise shown in this walkthrough.

The final scope view demonstrates the two actual edits: channel 4 at ×2 with a 20-ms gate while the other seven Clock channels retain their original settings.

<h6 align="center">From Munich with &#9829;</h6>
