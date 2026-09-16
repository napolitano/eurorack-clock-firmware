<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK 0.19.0-beta.13

CLOCK is an eight-output Eurorack master clock, divider, Euclidean rhythm source, and gate sequencer for the STM32F401 BlackPill. This beta focuses on making releases easier to install and easier to understand, while tightening a few user-facing details around the display and the hidden arcade modes.

## Highlights

- **Safer firmware downloads:** releases now provide persistence-safe DfuSe firmware images instead of a flat binary that could overwrite saved settings and presets.
- **Five Easter-egg builds:** the normal firmware ships with **BEATKNECHT** as the default Easter egg, with separate Pixel Raid, Formula 1, Breakout, and Egg Journey images for users who prefer one of the arcade variants.
- **High-score reset:** ranked arcade builds expose a guarded **HI-SCORES / CLEAR** entry in Settings after that Easter egg has been launched at least once.
- **Tap feedback placement:** the Tap Tempo animation is now anchored to the right edge of the performance display while the BPM value remains centered.
- **Improved manual browsing:** Screensavers and Easter eggs now have compact two-across screenshot galleries with short captions.
- **Release documentation:** every release now carries the versioned ODT/PDF manual, full changelog, and this short user-oriented summary alongside the firmware images.

## Compatibility

The musical clock engine, persistence layout, panel wiring, and preset format are unchanged. Firmware updates continue to preserve the dedicated A/B persistence sectors when the provided DfuSe image is used.
