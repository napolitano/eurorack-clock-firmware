<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK 0.19.0-beta.14

CLOCK is an eight-output Eurorack master clock, divider, Euclidean rhythm source, and gate sequencer for the STM32F401 BlackPill. This beta focuses on making the default BEATKNECHT rhythm Easter egg behave like a real performance tool instead of a one-shot demo.

## Highlights

- **Proper BEATKNECHT transport:** PLAY starts the rhythm and toggles PLAY/PAUSE; STOP/BACK stops immediately and resets the pattern to step 1.
- **Musically useful pause:** PAUSE forces every gate LOW but preserves the remaining timing to the next step, so PLAY resumes the pattern without an artificial retrigger.
- **Safer stop behavior:** STOP disables the external gate-output stage until PLAY is pressed again.
- **Cleaner controls:** TAP is now only the rhythm-style selector and the encoder remains the tempo control; the intro is started with the dedicated PLAY button.
- **Clear state feedback:** the BEATKNECHT display now shows a compact PLAY, PAUSE, or STOP symbol.
- **Guarded BEATKNECHT exit:** a long encoder push now opens a NO/YES confirmation. The prompt silences active gates; NO restores the previous transport state and YES exits safely with the output stage disabled.

## Compatibility

The clock engine, persistence schema, preset format, panel wiring, and release-image format are unchanged. This beta only changes the BEATKNECHT boot Easter egg and its documentation/tests.
