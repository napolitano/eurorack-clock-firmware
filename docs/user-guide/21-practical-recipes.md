<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 21 Practical recipes

### Eight exact copies of one master clock

1. Select **One Clock**.
2. Set `RATE = ×1` and `HUMANIZE = OFF`.
3. Choose the shared gate length.
4. Press **PLAY**.

### A conventional clock tree

1. Switch to **Independent**.
2. Load the **CLOCK TREE** template or set channel rates manually.
3. Use `RESET = GLOBAL` on channels that must return to the same downbeat.

### A Euclidean percussion line

1. Choose **Euclid**.
2. Set `STEPS = 16`.
3. Choose **HITS**.
4. Use **ROTATE** to place the hits.
5. Add **PROBABILITY** only if controlled omissions are wanted.

### A 64-step gate phrase

1. Choose **Sequencer**.
2. Set `LENGTH = 64`.
3. Open **EDITOR**.
4. Use PLAY/TAP to move forward/back through the four 16-step pages.
5. Toggle gates with encoder presses.
6. BACK returns to Sequencer settings.

### Record a Groove by feel — step by step

Use this short workflow when the musical feel matters more than drawing offsets by hand. The detailed Recorder/Editor behavior is described in [Chapter 13](13-grooves-and-record.md).

1. Open `TIMING → GROOVE → RECORD`.
2. Long-press the encoder and set **LENGTH**, recorder **COUNT IN**, and `ONE SHOT` or `ENDLESS`.
3. Press **PLAY**; capture starts after the optional Recorder Count-In.
4. Tap the feel with **TAP** against the moving playhead.
5. Press PLAY again to stop and rewind the recorder.
6. Open `EDITOR >` for precise marker correction and **SAVE** to a named Custom Groove slot.

Recorder Count-In is local to Groove Record and is independent from the global transport Pre-Count. In `ENDLESS`, later passes overwrite only steps that receive a new TAP.

<h6 align="center">From Munich with &#9829;</h6>
