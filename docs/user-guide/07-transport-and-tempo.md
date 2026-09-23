<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 7 Transport and tempo

Transport has three states: **PLAY, PAUSE, STOP**.

- PLAY/PAUSE toggles running and paused states without an implicit phase reset.
- STOP/BACK from Performance enters STOP and applies the global reset policy.
- Tap Tempo adjusts the master tempo from a sequence of TAP presses within the supported 1–999 BPM technical range and the configured user MIN/MAX boundaries.
- The first TAP starts a new measurement sequence without an indicator. The second and every following TAP in that sequence triggers a four-frame **8×8 shrinking-dot animation** in a fixed right-aligned slot at the display edge, vertically centered on the tempo display. The BPM numerals remain centered and do not move.
- The Tap Tempo sequence expires after one beat at the configured **MIN BPM**. At the factory minimum of 20 BPM this is 3000 ms; after a longer pause the next TAP is again the silent first tap and the following TAP resumes visual feedback.

CLOCK persists the working configuration but never restores PLAY on power-up. Flash commits are also deferred while transport is PLAYING so erase/program work cannot block the live gate path.

### Pre-Count

`SETTINGS → MODE & CLOCK → PRE COUNT` controls an optional silent count-in. `OFF` preserves the immediate-start behavior; active values run from **1 through 64 beats**. A fresh STOP→PLAY starts the count at the configured master tempo. During the count-in CLOCK advances only the count-in timing reference: **all eight gate outputs remain LOW and the musical pattern position remains at phase zero**. PAUSE freezes an active count and PLAY resumes it; STOP cancels it, so the next PLAY starts the full configured count again.

While Pre-Count is active, the Performance screen keeps the normal context visible around a centered square popover. The popover is black with a one-pixel white border and shows the remaining beat count without any phase animation. Along its lower edge, one cell per master-meter beat visualizes the current step: exactly the active beat is filled and all other beats remain outlined. In 4/4 the four fixed positions therefore read as Tick–Tack–Tack–Tack, with the filled marker moving through the bar and returning to the first position on the next bar. When the count reaches zero the popover disappears and normal gate generation starts from the shared phase-zero boundary. With a locked external clock, Pre-Count remains edge-owned and converts accepted SYNC pulses into the configured **master-meter beat unit** using PPQN. A quarter note is therefore one count beat in `/4`, two beats in `/8`, four beats in `/16`, and half a beat in `/2`; the scheduler does not double-advance the count between accepted external edges.

<h6 align="center">From Munich with &#9829;</h6>
