<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 13 Grooves and Custom Groove Record

Groove is deterministic microtiming applied to the same master timeline as Clock, Euclid and Sequencer. It is separate from One Clock **Humanize**, which is a bounded deterministic per-output displacement. Divider Bank deliberately remains Groove-free.

### Factory Grooves

`TIMING → GROOVE` offers `OFF`, `SWING 54`, `SWING 58`, `SWING 62`, `SWING 66`, and `POCKET A / B / C`. **AMOUNT** scales the selected pattern from straight (`0%`) through its stored shape (`100%`), while **ROTATE** changes where that pattern starts against the channel timeline. One Clock owns one global Groove; Independent stores Groove per channel and applies it equally to Clock, Euclid and Sequencer event timing.

### Custom Groove Editor

`GROOVE → EDITOR` opens the graphical 128×64 Custom Groove editor. A Groove can contain **1–64 steps**. Solid beat guides and dotted step guides show the nominal grid; diamond markers show the actual timing position of each step. The selected diamond is filled. Marker positions may move early or late but are bounded so events remain monotonic and cannot overtake neighboring steps.

<p align="center"><img src="../manual-source/assets/groove-editor-custom.png" alt="Custom Groove editor with beat and step guides plus diamond microtiming markers." width="360"><br><sub>Custom Groove editor — the grid is nominal time; diamonds are the stored microtiming positions.</sub></p>

Editor controls:

- **TAP** — select the next marker;
- **encoder turn** — move the selected marker finely;
- **TAP + turn** — coarse marker movement;
- **PLAY + turn** — quick zoom through `FIT / 32 / 16 / 8 / 4` without issuing a transport command;
- **encoder long press** — open the Groove editor menu;
- **BACK** — leave immediately when clean, or open `DISCARD CHANGES?` when the draft has changed.

Changes are previewed live while transport runs but remain temporary until saved. BACK/DISCARD restores the pre-entry Groove state. New Custom Grooves receive an editable generated two-word default name so the save workflow never starts from a blank label.

### TAP Record

`GROOVE → RECORD` is a second input mode over the **same Custom Groove draft**. It uses the same grid and recorded diamond markers, plus a moving playhead driven by the real engine phase. A recorded Groove can therefore be opened immediately in `EDITOR` for manual cleanup and is stored in exactly the same Custom Groove format.

<p align="center"><img src="../manual-source/assets/groove-record-live.png" alt="Custom Groove recorder showing recorded diamond markers and the live playhead." width="360"><br><sub>Groove Record — TAP events become signed microtiming offsets on the running grid.</sub></p>

Recorder controls:

- **PLAY** — start/stop capture; stopping rewinds the recorder to the beginning;
- **TAP** — record one timing event; TAP does not run Tap Tempo while the recorder is active;
- **PLAY + turn** — quick zoom;
- **encoder long press** — open the Record menu;
- **BACK** — leave/confirm discard using the same shared-draft rules as the editor.

The Record menu provides **MODE** (`ONE SHOT / ENDLESS`), recorder **COUNT IN** (`OFF / 1–64`, default `4`), **LENGTH** (`1–64`), **ZOOM**, **SAVE**, `EDITOR >`, and **CLEAR**. `ONE SHOT` stops at the end of one pattern pass. `ENDLESS` wraps and continues; only steps that receive a new TAP are overwritten, so an existing Groove can be refined over repeated passes.

Front-panel TAP capture preserves the high-resolution physical press timestamp through button debounce before converting it to a signed per-step offset. Host tests verify that software contract; absolute physical switch latency/jitter remains a HIL measurement.

### Record a Groove by feel — step by step

1. Open `TIMING → GROOVE → RECORD`. RECORD and EDITOR work on the same temporary Custom Groove draft.
2. Long-press the encoder and set **LENGTH** (`1–64`), recorder **COUNT IN** (`OFF / 1–64`, default `4`) and **MODE** (`ONE SHOT / ENDLESS`). Choose a useful **ZOOM** if needed.
3. Press **PLAY**. If recorder Count-In is enabled, it runs first; capture begins only after that local count reaches zero.
4. Tap the rhythm on **TAP**. Every press records one high-resolution timing event against the live Q32 engine position. Tap Tempo is disabled while the recorder is active.
5. Press **PLAY** again to stop. The recorder rewinds to the beginning and the recorded draft stays available.
6. Choose `EDITOR >` for precise marker cleanup, then **SAVE** to a named Custom Groove slot when the result is ready.

`ENDLESS` is deliberately non-destructive to untouched steps: later passes replace only steps that receive a new TAP. `ONE SHOT` stops automatically after one pattern cycle.

### Custom Groove library

The frozen 1.1.0 storage scope is **10 named Custom Groove slots**. The normal Groove page exposes `LOAD >`; saved Grooves can also be overwritten, renamed, or deleted with explicit safe confirmations. The Performance screen uses the stored name rather than a generic `CUSTOM` label. The earlier 99-slot idea remains a later storage target and is not part of the 1.1.0 release contract.

<h6 align="center">From Munich with &#9829;</h6>
