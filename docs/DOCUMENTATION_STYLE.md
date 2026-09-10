<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Documentation Style

This file defines the repository documentation conventions. It exists to keep the User Guide, developer documentation, maintained typeset manual, code comments, and GitHub landing page consistent as the project grows.

## Language and tone

- Canonical repository prose is **US English**.
- Prefer direct technical language over marketing language.
- Explain the behavior a user or developer can rely on; separate implemented behavior from planned work.
- Introduce acronyms at first use unless they are universally obvious in context (`BPM`, `GPIO`, `ISR`).
- Use `One Clock`, `Divider Bank`, `Euclid`, and `Sequencer` as user-facing names; internal C++ identifiers may differ.

## GitHub Markdown

Use GitHub-native features when they improve comprehension:

- tables for reference data;
- fenced code blocks with language tags;
- `[!NOTE]`, `[!TIP]`, `[!IMPORTANT]`, `[!WARNING]`, and `[!CAUTION]` admonitions;
- Mermaid for software architecture, sequence, state, and dependency diagrams;
- `<details>` only for genuinely optional long material;
- relative repository links so documentation works on branches and forks.

Do not hide required safety or qualification information inside collapsible blocks.

## Images

- Production OLED screenshots must come from the real framebuffer/renderer.
- Pixel UI screenshots are enlarged with nearest-neighbor scaling.
- Reusable explanatory diagrams are plain SVG under `docs/manual/assets/`.
- SVGs must not embed fonts, scripts, external resources, or raster data unless there is a documented reason.
- Conceptual front-panel diagrams must not be presented as manufacturing drawings.
- Every Markdown image needs meaningful alt text.

## Numbers and units

- Use a space between value and SI unit in prose: `50 µs`, `16 KiB`, `20 kHz`.
- Keep firmware/UI literals exact when quoting the display: `MIN BPM`, `ONE CLOCK`, `FRACTAL`.
- Use decimal commas only in translated/localized prose; canonical US-English repository text uses decimal points.
- State whether a limit is a factory default, user boundary, technical boundary, or current implementation limit.

## Code documentation

Each project-owned C/C++ file starts with Doxygen metadata:

```cpp
/**
 * @file example.h
 * @brief One-sentence purpose of this translation unit or interface.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
```

Public API declarations belong in headers and use concise Doxygen comments:

```cpp
/**
 * @brief Rebuilds one channel on the global musical reference grid.
 * @param channelIndex Zero-based physical output index.
 * @param includeCurrentBoundary Whether an event exactly at the current position is eligible.
 */
void scheduleChannel(std::size_t channelIndex, bool includeCurrentBoundary);
```

Document parameters, return semantics, units, ownership/lifetime, thread/ISR restrictions, and non-obvious side effects where they matter. Do not duplicate header API prose above the implementation definition; implementation comments should explain algorithms and invariants instead.

## Version-sensitive statements

The active alpha version may be stated in landing pages, test baselines, and release instructions. Historical changelog sections remain immutable except for factual corrections.

When a feature is not implemented, say so explicitly. Do not describe the intended External Sync capture path as shipping hardware until comparator/input-capture HIL tests exist.

## README footer

README-style documents end with the same small centered footer:

```html
<h6 align="center">From Munich with &#9829;</h6>
```

The HTML entity renders a text heart rather than a colored emoji glyph, keeping the heart the same monochrome color as the surrounding text.
