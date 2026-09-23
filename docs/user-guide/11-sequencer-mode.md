<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 11 Sequencer mode

Each Sequencer channel stores a binary gate pattern of up to 64 steps. The active sequence length is 1–64 steps.

![Sequencer editor showing one 16-step page of the 64-step binary gate pattern with a movable cursor and gate/rest states.](../manual-source/assets/sequencer-editor.png)

The editor is divided into four possible pages:

```text
1–16     17–32     33–48     49–64
```

Inside the editor:

- encoder turn moves the step cursor;
- encoder press toggles the selected gate;
- PLAY/PAUSE moves to the next 16-step page;
- TAP moves to the previous 16-step page;
- STOP/BACK returns to Sequencer parameters.

Sequencer tools include **Length, Rotate, Invert, Clear, Fill Alternate, Copy, and Paste**. Rate, Swing and Groove remain shared channel-timing controls. As with Euclid, `×1` is a sixteenth-note grid, so a 16-step sequence occupies one 4/4 bar.

<h6 align="center">From Munich with &#9829;</h6>
