<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK VCV Rack Trial-First Port

Status: **EXPERIMENTAL / post-1.1.0**.

The VCV Rack track exists so prospective builders can try CLOCK before committing to the hardware build. Its product requirement is therefore unusually strict: it should behave like CLOCK, not merely reproduce a similar feature list.

The initial implementation lives in [`../vcv/`](../vcv/). It builds the production CLOCK sources and uses the same host runtime boundary already exercised by the native simulator. The Rack-specific code is responsible for Rack sample-time adaptation, Rack ports/controls, framebuffer presentation and Rack patch serialization. Musical timing, Groove/Euclid/Sequencer behavior, transport, settings and rendering stay in the production code. Front-panel geometry is not Rack-specific: `sim/panel_layout.ini` is the shared millimetre source for the simulator, manual illustration and generated Rack panel.

## Functional panel contract

The Trial-First module must teach the hardware layout rather than merely expose the same feature list. `scripts/generate_vcv_panel.py` therefore derives the Rack panel and C++ widget coordinates directly from `sim/panel_layout.ini`. The current geometry is the same 10 HP / 3U arrangement used by the simulator: OLED at the upper left, push encoder at the upper right, the three transport buttons across one row, the two inputs below, and eight output jacks in a 4x2 matrix with 3 mm activity LEDs immediately above their matching outputs.

The VCV functional plate is black with white labelling and identifies the module as **South Signal Lab Clock**. A larger **CLOCK** title is centred at the top and **SOUTH SIGNAL LAB** is centred at the bottom. The user-facing labels are **PLAY / PAUSE**, **TAP / SHIFT**, **STOP / BACK**, **IN 1 / SYNC**, **IN 2 / RST**, and outputs **1–8**; every button/jack label sits below its hardware and the encoder is intentionally unlabelled. Geometry changes belong in the simulator INI and are then regenerated into Rack; hand-editing an independent VCV placement is a regression. This remains functional VCV artwork, not a manufacturing drawing for the later physical panel.

The encoder uses a Rack-specific interaction overlay without changing physical geometry: its outer area remains the rotary control, while the transparent centre is the momentary push surface. The visible knob keeps the simulator-defined diameter, while only the Rack hit viewport is enlarged and the centre-push target narrowed to make rotation reliable. Dragging is forced to Rack's linear relative mode and wheel events map directly to encoder detents. Holding the centre passes a continuous button level into the production control path, which is required for firmware long-press timing.

Rack's NanoSVG loader does not render SVG `<text>` nodes. The generated `CLOCK.svg` therefore remains useful as a standalone preview, but all actual in-Rack panel lettering is redrawn by a NanoVG overlay using Rack's own UI font. This avoids bundling an additional font while keeping the black/white functional panel legible.

Output LEDs are presentation only: a short gate refreshes a 75 ms activity hold so a 10 ms gate cannot disappear between GUI frames. OUT 1–8 themselves remain tied to the instantaneous production gate state and are not pulse-stretched.

## Current verified boundary

The Rack-independent adapter is built and exercised by normal CMake/CTest. It has been verified locally against the production source for:

- normal CLOCK boot path;
- PLAY-driven engine start;
- eight coherent 0/+5-V logical gate outputs;
- factory One Clock edge periodicity at 44.1, 48 and 96 kHz host sample rates;
- the real CLOCK OLED framebuffer;
- held encoder push through the real long-press path;
- raw CLOCK persistence-image export/import;
- Rack-sample input transition capture before the 20 kHz firmware boundary, including one-sample SYNC/RST pulses.

This does **not** yet constitute a real Rack SDK build or in-Rack runtime test. The repository CI workflow `.github/workflows/vcv.yml` performs the first real SDK build using the pinned Rack SDK.

## Deliberate first-alpha limitation

The current host HAL and application callback thunks are process-global. The first VCV alpha therefore supports exactly one active CLOCK instance per Rack process. Additional instances are disabled rather than allowed to corrupt each other's timer/GPIO/input state.

The correct multi-instance solution is an instance-context host HAL shared by `sim/` and `vcv/`, followed by removal of the static single-active-application assumptions. That refactor is a separate architecture step.

## Deployment path

Development path:

```text
production CLOCK code
        -> Rack-independent VCV adapter tests
        -> Rack SDK plugin build
        -> .vcvplugin CI artifact
        -> manual Rack smoke test
        -> Windows/macOS/Linux package matrix
        -> tagged CLOCK release artifact under dist/vcv/
```

See [`VCV_DEVELOPMENT.md`](VCV_DEVELOPMENT.md) for the complete local SDK/build/package/install/smoke-test workflow and [`../vcv/README.md`](../vcv/README.md) for the adapter overview.

<h6 align="center">From Munich with &#9829;</h6>
