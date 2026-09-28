<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK tutorial voice-over and picture timing

Voice-over is recorded after the Storybook picture and mixed in the editor. The scripts in this directory are the authoritative spoken copy. Story YAML contains real control actions plus presentation-only holds at the states being explained. Subtitles remain short action labels rather than transcripts.

## Reference pace and framing

- The tracked intro and outro clips measure **4.5 s** and **5.5 s** respectively (`ffprobe` on the bundled MP4 assets).
- The provided spoken opening and sign-off references take roughly **15 s** and **16 s**.
- Planning therefore uses approximately **195 spoken words/minute** (3.25 words/s), but this is only a baseline. Audio Tags, punctuation, breathing and visual explanation add time.
- Every example reserves **18 s** for the opening card and **20 s** for the closing card.
- Body holds are sized from the actual script rather than from a fixed tutorial-length target. The narration dictates the edit; it is not squeezed into an arbitrary picture duration.
- Eleven v3 uses inline Audio Tags, punctuation and text structure for performance direction. SSML `<break>` markup is not part of the v3 contract.
- Final publication timing must still be checked against the actual generated or recorded take. If a take differs materially, retime the corresponding Story hold and regenerate the video/subtitle sidecars together.

## Current planning values

`Speech` is the plain spoken word count at 195 wpm and therefore excludes additional expressive pauses. `Explicit Story time` is the exact sum of authored scene/focus durations and explicit waits in the YAML. It does **not** include the additional deterministic time consumed by encoder turns, pushes, button actions, patch motions or `wait_until`, so the final StoryRunner duration is longer. `+ bumpers` adds the fixed 10 seconds from the 4.5-s intro and 5.5-s outro.

| Example | Spoken words | Speech | Explicit Story time | + bumpers |
| --- | ---: | ---: | ---: | ---: |
| 01 · First Clock | 301 | 1:33 | 1:47 | 1:57 |
| 02 · Transport | 299 | 1:32 | 1:48 | 1:58 |
| 03 · Encoder tempo | 253 | 1:18 | 1:32 | 1:42 |
| 04 · Tap Tempo | 274 | 1:24 | 1:38 | 1:48 |
| 05 · Topology / channel | 308 | 1:35 | 1:51 | 2:01 |
| 06 · Clock mode | 302 | 1:33 | 1:46 | 1:56 |
| 07 · Euclid | 339 | 1:44 | 2:01 | 2:11 |
| 08 · Sequencer | 318 | 1:38 | 1:52 | 2:02 |
| 09 · Divider Bank | 300 | 1:32 | 1:48 | 1:58 |
| 10 · External SYNC / RST | 322 | 1:39 | 1:54 | 2:04 |
| 11 · Eight Clock channels | 1295 | 6:38 | 7:12 | 7:22 |

These are edit budgets, not measured ElevenLabs output lengths.

## Example 11: narrative structure

Example 11 is intentionally a full walkthrough rather than a fast feature demo. It now starts with a proper orientation before the first control action:

- what South Signal Lab is showing;
- what CLOCK is;
- why Independent still uses one shared timing reference;
- what will be changed on channel 4;
- what will remain unchanged and only be explained.

The tutorial then shows the real topology switch and confirmation, the eight-channel overview, channel-four selection, the short encoder push versus long encoder press, and the four channel-setting groups `MODE · TIMING · CLOCK · OUTPUT`.

Every setting that is actually shown on screen receives its own explanation and visual hold:

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

`GROOVE` is explained as the entry to its own editor but that editor is not opened in Example 11. A separate editor would add settings that are not otherwise shown in this walkthrough and would turn the example into a second tutorial.

The final scope view demonstrates the two actual edits: channel 4 at ×2 with a 20-ms gate while the other seven Clock channels retain their original settings.

<h6 align="center">From Munich with &#9829;</h6>
