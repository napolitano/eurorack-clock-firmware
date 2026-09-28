<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK tutorial narration timing contract

The narration is a post-production layer. Storybook owns deterministic picture timing; ElevenLabs owns generated speech; the NLE performs the final mix.

## Fixed bumper timing

- tracked intro: **4.5 s**;
- spoken opening window immediately after the intro: **18.0 s**;
- tracked outro: **5.5 s**;
- spoken closing window immediately before the outro: **20.0 s**.

Axel's measured delivery is approximately **15 s** for the standard introduction and **16 s** for the sign-off. The Storybook windows therefore retain roughly three and four seconds of editorial margin respectively. Do not consume that margin by starting the outro early.

## Eleven v3 authoring

Scripts are US English and intended for Eleven v3. Audio Tags are deliberately sparse. Use bracketed delivery cues such as `[warmly]`, `[thoughtful]`, `[slower]`, and `[short pause]` only where they improve the cloned voice; normal punctuation and paragraph boundaries do most of the pacing work. Eleven v3 does not use SSML `<break>` tags.

For tight edits, generate paragraph by paragraph. Keep the paragraph boundaries in the repository script because they correspond to the major visual beats of the Story.

## Standard video rhythm

1. intro bumper — 4.5 s;
2. title/opening card — 18 s, spoken introduction;
3. tutorial content — narration follows actions and explanatory focus holds;
4. final summary / thanks card — 20 s, spoken sign-off;
5. outro bumper — 5.5 s.

Short subtitles are not a transcript. They identify the current action. The voice-over explains intent, context and consequences and may span several actions inside the same tutorial scene.

## Detailed walkthrough — Example 11

`11-eight-independent-clocks-walkthrough.yaml` is intentionally slower than the short examples. Its important narration beats are:

| Beat | Visual contract | Narration purpose |
| --- | --- | --- |
| Opening | 18 s title card | Introduce eight Independent Clocks, channel selection and long-press settings. |
| Factory state | 14 s text card | Explain One Clock versus the eight stored channel configurations. |
| Topology change | physical TAP + encoder sequence | Show that selecting Clock enters Independent topology through the real UI. |
| Eight-channel overview | 6.5 s focus hold | Establish that all eight channels are Clock after the factory-state topology change. |
| Channel 4 selection | slow encoder + short push | Explain the short-push channel-selection grammar. |
| Long press | 800 ms real hold + 7 s focus | Explain the second encoder gesture and the MODE / TIMING / CLOCK / OUTPUT hierarchy. |
| TIMING | 8 s group hold + 4.5 s result hold | Explain rate, rational ratio, Swing and Groove; edit Channel 4 from ×1 to ×2. |
| CLOCK | 6.5 s hold | Explain meter beats and meter unit as mode-specific Clock settings. |
| OUTPUT | 9 s group hold + 4.5 s result hold | Explain Probability, Gate, Phase, Reset and Mute; edit Gate from 10 to 20 ms. |
| Result | 4.5 s live scope | Show the selected channel running with the edited configuration. |
| Summary | 9 s callout | Reinforce independent configuration on one shared musical reference. |
| Closing | 20 s thanks card | Deliver the varied South Signal Lab sign-off before the outro. |

The long focus holds are deliberate. They freeze firmware time while the explanatory overlay is visible, giving the voice-over enough room without allowing hidden musical state to advance behind the explanation.

<h6 align="center">From Munich with &#9829;</h6>
