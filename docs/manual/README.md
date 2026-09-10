<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Manual Workspace

This directory contains the maintained, publication-layout CLOCK end-user manual together with its reusable diagrams and deterministic UI screenshots. The repository User Guide remains the canonical GitHub-readable operating reference while the project is in alpha; the ODT is the editable source for the typeset release manual.


## Maintained user manual source

The editable end-user manual is maintained as [`clock-user-manual.odt`](clock-user-manual.odt). Its manual-specific license is documented in [`LICENSE.md`](LICENSE.md). The ODT currently represents the maintained working source; release preparation freezes a version-matched copy under `releases/<firmware-version>/`.

The release artifact naming contract follows the Quantizer/Drift convention:

```text
clock-user-manual.<version>.odt
clock-user-manual.<version>.pdf
```

Before creating a release tag, run:

```bash
python scripts/prepare_release_manual.py
```

This stamps the current `CLOCK_FIRMWARE_VERSION` into the ODT body, metadata, front cover, and back cover, then writes the frozen source to `docs/manual/releases/<version>/clock-user-manual.<version>.odt`. The release workflow refuses to publish when that exact frozen source is missing or version-mismatched.

The release workflow converts the frozen ODT with LibreOffice and publishes **both ODT and PDF** as GitHub Release assets. The release PDF is checked for a non-zero page count, the expected firmware version, and Ubuntu-family font embedding. A separate `Manual publication smoke test` workflow exercises the same publication path for documentation changes.

Local helpers:

```bash
python scripts/prepare_release_manual.py
python scripts/build_user_manual.py
```

`prepare_release_manual.py` requires the Ubuntu font family for release-quality cover stamping. `--allow-font-substitution` exists only for local non-release previews.

## Asset policy

[`assets/`](assets/) contains both explanatory SVG diagrams and deterministic simulator screenshots.

The SVG files use explicit `viewBox` dimensions, no embedded fonts, no external links, and no manufacturing tolerances unless a drawing explicitly says otherwise. OLED screenshots are generated from the production firmware renderer and real 128×64 framebuffer, then enlarged with integer-only nearest-neighbour scaling. They are not hand-drawn UI mockups.

The boot logo is no longer reconstructed from Bézier/line primitives at runtime. `clock-logo.svg` is the canonical artwork; it is rasterized once to the checked-in 1-bit `clock-logo-1bit.png`, and the firmware uses the corresponding packed bitmap data in `src/ui/clock_logo_bitmap.h`. This guarantees that the OLED logo is the exact approved raster rather than a second interpretation of the vector geometry.

| Asset | Intended manual use |
| --- | --- |
| [`front-panel-anatomy.svg`](assets/front-panel-anatomy.svg) | identify controls, display, input, outputs, and front-panel groups |
| [`operating-modes.svg`](assets/operating-modes.svg) | explain Independent, One Clock, and Divider Bank |
| [`timing-swing-phase.svg`](assets/timing-swing-phase.svg) | explain ideal grid, Swing, phase, and One Clock Humanize |
| [`external-sync-flow.svg`](assets/external-sync-flow.svg) | separate implemented sync model from pending physical capture |
| [`persistence-ab.svg`](assets/persistence-ab.svg) | explain the two-slot internal-Flash commit strategy |
| [`simulator-scope.svg`](assets/simulator-scope.svg) | explain the developer oscilloscope and musical ruler |
| [`clock-logo.svg`](assets/clock-logo.svg) | canonical CLOCK vector wordmark used as the logo source |
| [`clock-logo-1bit.png`](assets/clock-logo-1bit.png) | exact 128×41 monochrome raster used to derive the firmware boot-logo bitmap |
| [`clock-logo.png`](assets/clock-logo.png) | transparent raster export of the CLOCK logo for documents that need PNG assets |
| [`manual-screenshots.tsv`](assets/manual-screenshots.tsv) | generated screenshot catalog and human-readable state descriptions |
| [`boot-1000-1x.png`](assets/boot-1000-1x.png) | exact 1:1 pixel export of the fully rendered OLED boot screen |

> [!NOTE]
> These are explanatory illustrations. `front-panel-anatomy.svg` is **not** a drill template, PCB drawing, or dimensional manufacturing source. Final mechanical drawings belong with the hardware CAD/KiCad deliverables.

## Regenerating all OLED screenshots

Run the VS Code task **`CLOCK: Generate manual screenshots`**, or execute:

```bash
python scripts/generate_manual_screenshots.py
```

The task configures the SDL-free simulator preset, builds `clock-manual-screenshot-generator`, renders the complete curated firmware-state catalog, converts every native 128×64 framebuffer to a 4× grayscale PNG with nearest-neighbour scaling, removes only stale files recorded by the previous generated manifest, and writes the result directly beside the SVG assets in `docs/manual/assets/`.

The catalog currently covers:

- boot at 0/25/50/75/100 percent and power-off;
- PLAY, PAUSE, STOP, Off, Clock, Euclid, Sequencer, One Clock, and Divider Bank;
- Internal, External and Auto source states including locked/unlocked external timing;
- channel/global overviews, all six mode-palette selections, mode confirmation, and Sequencer editor;
- every Settings page plus an active edit state;
- templates, preset load/save/name/overwrite flows;
- Clock, Plug, Heartbeat, Acid, Spectrum, Field, Blox, Matrix, Cube Cover, Fractal, and Orbit screensavers;
- individual retro intros for Pixel Raid, Formula 1, Breakout, Egg Journey, and BEATKNECHT; shared initials-entry/Top-100 arcade screens; Pixel Raid, Formula 1 (including curve/crash states), Breakout (including modifier state), Egg Journey (action, broken-shell, and flattened-egg states), and BEATKNECHT gameplay.

The catalog is intentionally finite and representative. It covers every distinct user-visible operating/UI state rather than attempting the meaningless Cartesian product of every parameter value.

## Manual structure

The maintained manual follows this user-oriented publication structure:

1. Safety and installation
2. Front-panel tour
3. Transport and master clock
4. Independent channels
5. One Clock and Humanize
6. Divider Bank
7. Euclidean patterns
8. Gate Sequencer
9. Swing, phase, probability, and reset semantics
10. Internal / External / Auto sync
11. Presets and persistence
12. Display and screensavers
13. Configuration reference
14. Troubleshooting
15. Firmware update / recovery
16. Technical appendix

The editable manual follows development continuously. A release-frozen manual must not be treated as final hardware documentation until External Sync comparator/Input-Capture HIL, SPI/I2C timing HIL, and the final scheduler implementation are validated.



## Release-version contract

The editable `clock-user-manual.odt` may follow ongoing development without a frozen copy for every intermediate source revision. A frozen manual is required only when preparing/publishing a release. The release workflow resolves the firmware version, requires `releases/<version>/clock-user-manual.<version>.odt`, validates that ODT, converts it to PDF, validates the PDF, and publishes both artifacts.

<h6 align="center">From Munich with &#9829;</h6>
