<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 22 Troubleshooting

| Symptom | Check first |
| --- | --- |
| No gates after power-up | Normal until PLAY is pressed. Then check topology, `OFF`/Mute, Probability and routing. |
| All eight outputs are identical | Factory topology is One Clock. Switch to an Independent channel function if outputs should differ. |
| Tempo stops at a value | Check `GENERAL SETTINGS → CLOCK → MIN BPM / MAX BPM`. |
| Tap Tempo did not become authoritative | Tap changes the internal/fallback BPM but preserves SOURCE. In AUTO/EXTERNAL, inspect external lock and LOSS behavior. |
| RESET did not restart one channel | That channel may use `RESET = FREE`. |
| Screensaver does not start | Display-protection timers advance only in STOP, not PLAY or PAUSE. |
| Preset save asks for confirmation | The slot is occupied; NO is the safe default. |
| External lock is unavailable/unstable | Check `DIAGNOSTICS → INPUTS`, input role assignment, then SOURCE, PPQN, EDGE, FILTER, SMOOTHING and TIMEOUT. Physical comparator behavior remains HIL evidence. |

### Useful diagnostic order

Work from the outside inward: **topology → transport → selected channel/mode → MUTE/OFF/PROBABILITY → rate → clock source**. This prevents chasing timing parameters when the real issue is simply that no event is eligible to reach the output.

For external timing, separate the questions: first verify that a conditioned edge arrives, then verify that the correct input owns the SYNC role, then verify SOURCE/PPQN/EDGE, and only then tune FILTER, SMOOTHING or TIMEOUT.

<h6 align="center">From Munich with &#9829;</h6>
