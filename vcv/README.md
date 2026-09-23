<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK for VCV Rack — experimental

This directory contains the first **Trial First** VCV Rack port of CLOCK. The goal is not a separate software interpretation of the module. The Rack shell compiles the real CLOCK application, engine, services, UI renderer and `clock_core`, then replaces only the physical STM32/front-panel boundary.

## Current status

`EXPERIMENTAL` — post-1.1.0 development.

Implemented in this first slice:

- `vcv/` is a peer platform directory beside `sim/`;
- the VCV bridge advances the real CLOCK 20 kHz / 50 us scheduler rather than implementing a Rack-specific clock engine;
- the real 128x64 CLOCK OLED framebuffer is drawn in Rack;
- PLAY/PAUSE, TAP/SHIFT, STOP/BACK, encoder turn and encoder push feed the same debounced control path as the hardware/simulator;
- the encoder centre is a real momentary push surface, so holding it reaches the production 650 ms long-press path instead of emitting a synthetic fixed-duration click;
- IN 1 / SYNC and IN 2 / RST use Rack-voltage hysteresis plus an allocation-free transition queue before the 20 kHz firmware boundary, so one-sample Rack triggers are not lost between 50 us scheduler ticks;
- OUT 1..8 expose the actual logical gate states as **0/+5 V**, matching the hardware contract;
- Rack patch state embeds CLOCK's complete logical persistence image instead of inventing a second preset/schema format;
- the functional 10 HP Rack panel is generated from `../sim/panel_layout.ini`, so OLED, encoder, transport buttons, SYNC/RST, eight 3 mm activity LEDs and the 4x2 output matrix use the same millimetre geometry as the native simulator; no independent Rack layout is maintained.

Current limitation: the existing host HAL and `ClockApplication` callback boundary own process-global state. The experimental Rack shell therefore permits **one CLOCK instance per Rack process** and visibly disables additional instances rather than allowing silent cross-instance corruption. Multi-instance support requires a later host-HAL instance-context refactor; it is not papered over in this initial port.

## Front-panel geometry

`sim/panel_layout.ini` is the single geometry source for the native simulator, the manual front-panel illustration and the VCV Rack shell. Run:

```bash
python scripts/generate_vcv_panel.py
```

after changing panel geometry. This regenerates the Rack panel SVG, the C++ millimetre coordinate header, the encoder artwork and the three transport-button frame pairs. CI runs the same command in `--check` mode and rejects stale Rack panel assets.

The Rack module uses a black functional front plate with white labelling while retaining the simulator-defined geometry. The Rack-visible identity is **South Signal Lab Clock**. Controls are labelled **PLAY / PAUSE**, **TAP / SHIFT** and **STOP / BACK**; the two inputs are **IN 1 / SYNC** and **IN 2 / RST**; outputs are numbered **1–8** in the simulator-defined 4x2 matrix with one 3 mm red activity LED above each jack. Rack's NanoSVG loader does not render SVG `<text>` nodes, so the actual module draws this lettering as a NanoVG overlay with Rack's bundled UI font; the generated SVG keeps matching text for standalone previews.

The push encoder deliberately has two Rack interaction zones: drag/scroll the outer knob area to turn it, and click/hold the transparent centre area to press it. The visible knob remains at the simulator-defined physical diameter, but Rack receives a larger transparent rotary hit area and a smaller centre-push target so rotation is not confined to a narrow annulus. Dragging uses forced linear relative movement; mouse-wheel events map directly to detents instead of depending on Rack's global knob-scroll sensitivity. A centre hold is passed through continuously, so short press, long press and TAP+encoder-push timing are handled by the real CLOCK debounce/gesture code.

The gate voltage itself is never visually stretched. Because the factory 10 ms gate can be shorter than a GUI frame, the Rack activity LED is held visibly on for 75 ms after gate activity, matching the native simulator's perceptual LED behavior while OUT 1–8 continue to expose the real instantaneous 0/+5 V state.

## Source sharing

The Rack build compiles project sources directly from:

```text
../src/
../lib/clock_core/src/
../sim/   (host runtime/HAL boundary only)
```

There is no copied ClockEngine, Groove Engine, settings model or OLED renderer under `vcv/`.

## Host-side adapter test

The Rack-independent bridge is part of the normal CMake/CTest graph:

```bash
cmake --preset simulator-headless
cmake --build --preset simulator-headless
ctest --preset simulator-headless --output-on-failure
```

`vcv_runtime_adapter_tests` currently verifies real boot, held encoder long-press, physical PLAY, coherent 0/+5-V gate output across all eight channels, factory One Clock rising-edge periodicity at 44.1/48/96 kHz, persistence-image round trip, and one-Rack-sample SYNC/RST pulse capture at 48 kHz.

## Build against the real Rack SDK

The complete local workstation procedure now lives in [`../docs/VCV_DEVELOPMENT.md`](../docs/VCV_DEVELOPMENT.md). It covers Windows/MSYS2, macOS and Linux prerequisites; external SDK placement; `RACK_DIR`; adapter tests; panel regeneration; `make`, `make dist` and `make install`; isolated Rack user directories; package inspection; logs; manual smoke testing; CI parity; and the licensing boundary.

On Windows, use **MSYS2 MinGW 64-bit** for the Rack build. PowerShell uses different environment-variable syntax and is not the shell for the Rack Makefile workflow. The Windows short path in MSYS2 is:

```bash
export RACK_DIR="/c/SDK/Rack-SDK"
[ -f "$RACK_DIR/plugin.mk" ] && echo "Rack SDK OK: $RACK_DIR"
python scripts/generate_vcv_panel.py --check
cmake --preset simulator-headless
cmake --build --preset simulator-headless
ctest --preset simulator-headless --output-on-failure
make -C vcv clean RACK_DIR="$RACK_DIR"
make -C vcv -j4 RACK_DIR="$RACK_DIR"
make -C vcv dist RACK_DIR="$RACK_DIR"
make -C vcv install RACK_DIR="$RACK_DIR"
```

`RACK_DIR` must be the directory that directly contains `plugin.mk`; see the full developer guide for PowerShell path verification and SDK discovery.

The Rack SDK is deliberately kept outside the repository and is never shipped inside the CLOCK `.vcvplugin`.

## Manual smoke test in Rack

For the first functional validation:

1. Add **South Signal Lab Clock** to an empty patch and wait for the normal CLOCK boot sequence.
2. Press **PLAY / PAUSE** and patch output **1** to a scope or gate-visible destination. The output must switch only between 0 V and +5 V; outputs 1–8 must remain phase-coherent in factory One Clock mode.
3. Drag/scroll the encoder outer ring to turn it. Click the centre for a short push and hold the centre for more than 650 ms to verify the production long-press path. Exercise **TAP / SHIFT** and **STOP / BACK** as well.
4. Patch a Rack clock or short trigger into **IN 1 / SYNC** and verify acquisition/lock and loss behavior. Include a one-sample/very-short trigger source if available; edges must not be accepted only sporadically because of Rack/scheduler phase.
5. Patch a gate/trigger into **IN 2 / RST** and verify the configured RESET role/behavior through the real CLOCK settings.
6. Save the Rack patch, remove/reload it, and verify that CLOCK settings, presets and custom-groove persistence restore while transport still boots in STOP.
7. Attempt to add a second CLOCK. The experimental single-instance guard must show `ONE INSTANCE` and keep its outputs LOW.

For plugin-load failures or crashes, inspect Rack's `log.txt` in the Rack user folder.

## Packaging direction

The first CI job builds a Linux x64 `.vcvplugin` artifact independently of firmware releases. CI also opens the package, verifies the manifest/binary/panel payload and exercises Rack SDK's own `make install` target into an isolated Rack user directory.

For an Actions artifact, extract the workflow artifact first; the contained `.vcvplugin` is the actual Rack package. On Linux x64 it can be copied to `~/.local/share/Rack2/plugins-lin-x64/` and Rack restarted. `make -C vcv install` performs the same placement when building locally.

Once the real Rack SDK build and Rack smoke tests are stable on all target platforms, the normal CLOCK release pipeline can promote VCV packages into `dist/vcv/` and attach them to tagged releases.

<h6 align="center">From Munich with &#9829;</h6>
