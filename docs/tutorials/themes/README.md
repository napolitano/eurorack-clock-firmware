<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Storybook Themes

The schema/resource layer provides `south-signal-lab-default.yaml`. SB-5 now resolves the requested Phase-1 font registry and rejects unavailable faces rather than substituting system fonts.


## Phase-1 font registry

Storybook rendering is deterministic and does not depend on system font substitution. Phase 1 exposes two project-local faces backed by the CLOCK-owned 5×7 glyph set:

- `CLOCK UI` — proportional advance for body/headings/subtitles;
- `CLOCK Mono` — fixed advance for counters, scope labels and technical text.

Each theme font entry is a mapping with `family` and `size_px`. `size_px` must be a positive multiple of 7 so glyph scaling remains integer and reproducible. A requested family that is not in the registry is a hard validation/rendering error.

<h6 align="center">From Munich with &#9829;</h6>
