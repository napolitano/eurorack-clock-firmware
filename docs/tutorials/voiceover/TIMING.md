<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK tutorial voice-over and picture timing

Voice-over is recorded after the Storybook picture and mixed in the editor. The scripts in this directory are the authoritative spoken copy. Story YAML contains real control actions and presentation-only holds at the states being described. Subtitles remain short action labels, not transcripts.

## Reference pace and fixed framing

- The tracked intro and outro clips measure **4.5 s** and **5.5 s** respectively (`ffprobe` on the bundled MP4s).
- The provided spoken opening and sign-off examples took roughly **15 s** and **16 s**. For rough planning, use **195 spoken words/minute** (about 3.25 words/s), then allow for natural pauses and watching a control action.
- Every story has an **18 s** opening card after the intro. All revised scripts fit that card at the reference pace.
- Every story now has a **24 s** closing card before the outro. The current closings are longer than the reference recording, so the previous universal 20 s assumption was too tight.
- Eleven v3 delivery tags are cues, not words to speak. Record a paragraph at a time when a control action needs a more precise edit. Speech duration must ultimately be measured from the recorded take or generated audio; the numbers below are planning values, not audio measurements.

## Revised planning lengths

`Picture` includes title, story actions, explanation holds, summary and closing card, but excludes the two bumper clips. The action duration estimate follows `interaction_profiles.yaml`; `wait_until` in example 10 is variable, so its picture time is less certain. `Speech` counts script words at 195 wpm. Both columns are rounded planning values, not rendered-video measurements.

| Example | Spoken words | Speech | Picture | With bumpers |
| --- | ---: | ---: | ---: | ---: |
| 01 · First Clock | 250 | 1:17 | ~1:25 | ~1:35 |
| 02 · Transport | 228 | 1:10 | ~1:16 | ~1:26 |
| 03 · Encoder tempo | 212 | 1:05 | ~1:09 | ~1:19 |
| 04 · Tap Tempo | 213 | 1:06 | ~1:08 | ~1:18 |
| 05 · Topology / channel | 280 | 1:26 | ~1:28 | ~1:38 |
| 06 · Clock mode | 219 | 1:07 | ~1:14 | ~1:24 |
| 07 · Euclid | 341 | 1:45 | ~1:52 | ~2:02 |
| 08 · Sequencer | 314 | 1:37 | ~1:39 | ~1:49 |
| 09 · Divider Bank | 290 | 1:29 | ~1:32 | ~1:42 |
| 10 · External SYNC / RST | 279 | 1:26 | ~1:27* | ~1:37* |
| 11 · Eight Clock channels | 1,042 | 5:21 | ~6:00 | ~6:10 |

\* Includes an assumed ~1 s acquisition of external lock. The actual picture duration depends on the accepted incoming edges.

## Example 11: coverage and editing order

The chapter introduces the goal; the factory-state card explains why eight Clock channels are already present behind One Clock. The tutorial then shows the topology switch and confirmation, eight-tile overview, channel-four selection and the long encoder press. It highlights MODE, then physically moves through **every visible Clock-channel setting**. The VO follows those exact selections:

| Selected menu row | What the narration explains | Picture state |
| --- | --- | --- |
| MODE | Clock, Euclid, Sequencer and Off change this channel's function | MODE selected; no mode change |
| TIMING → DIV/MULT | integer divide/multiply relative to shared timing | edit ×1 → ×2 and confirm |
| TIMING → NUMERATOR | upper part of the rational rate | selected, left at 1 |
| TIMING → DENOMINATOR | lower part of the rational rate | selected, left at 1 |
| TIMING → SWING | alternating interval displacement | selected, unchanged |
| TIMING → GROOVE | repeatable pattern timing and its separate page | selected, unchanged |
| CLOCK → METER BEATS | beats per local measure | selected, unchanged |
| CLOCK → METER UNIT | note value counted as one beat | selected, unchanged |
| OUTPUT → PROBABILITY | chance an eligible event reaches the jack | selected, unchanged |
| OUTPUT → GATE | pulse HIGH time | edit 10 → 20 ms and confirm |
| OUTPUT → PHASE | channel offset relative to common reference | selected, unchanged |
| OUTPUT → RESET | Global vs Sync Free phase behavior | selected, unchanged |
| OUTPUT → MUTE | silence without clearing configuration | selected, unchanged |

Each selection gets a separately timed focus on the corresponding OLED row. The focus freezes firmware time while it is presented; physical encoder and button actions still run under `HUMAN_NORMAL`. The final scope view runs with firmware time active so the changed rate can be seen. A one-row focus on GROOVE explains the entry point, not the eight subsettings inside its separate groove editor; that editor is outside this Clock-channel walkthrough.

At the reference pace the 1,042-word script needs roughly **5:21 of speech**, before pauses. The revised story plans roughly **6:00 of picture plus 10 seconds of bumper clips**. This is a working edit budget, not a promise that a particular ElevenLabs take will fit without an NLE adjustment. If a take differs, retime its corresponding hold or action beat and regenerate the video and subtitle sidecars together. Preserve the 18 s opening, 24 s closing and bumper boundaries unless the recorded take requires a documented change.

<h6 align="center">From Munich with &#9829;</h6>
