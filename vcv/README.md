<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK for VCV Rack — experimental

This directory contains the first **Trial First** VCV Rack port of CLOCK. The goal is not a separate software interpretation of the module. The Rack shell compiles the real CLOCK application, engine, services, UI renderer and `clock_core`, then replaces only the physical STM32/front-panel boundary.

## Current status

`EXPERIMENTAL` — post-1.1.0 development.

Implemented in this first slice:

- `vcv/` is a peer platform directory beside `sim/`;
- the VCV bridge advances the real CLOCK 20 kHz / 50 us scheduler rather than implementing a Rack-specific clock engine;
- the real 128x64 CLOCK OLED framebuffer is drawn in Rack;
- PLAY, TAP, STOP/BACK, encoder turn and encoder push feed the same debounced control path as the hardware/simulator;
- SYNC and RST feed the real external-input and synchronization services after a small Rack-voltage hysteresis boundary;
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

The Rack module deliberately keeps the current functional appearance rather than inventing final front-panel artwork. The physical arrangement is not provisional: it follows the simulator specification — OLED upper left, push encoder upper right, PLAY/TAP/STOP in one row, SYNC/RST below them on the left, and OUT 1–8 as two rows of four with one 3 mm red activity LED above each jack. Clicking the encoder without dragging produces the encoder-push gesture; dragging or scrolling it turns the same encoder parameter.

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

`vcv_runtime_adapter_tests` currently verifies real boot, physical PLAY, 0/+5-V gate output, persistence-image round trip and direct external-input driving at a 48 kHz Rack sample rate.

## Build against the real Rack SDK

CLOCK currently pins VCV development validation to **Rack SDK 2.6.6**. Download the SDK for the development host from the official VCV downloads archive, extract it, and set `RACK_DIR`.

Linux/macOS example:

```bash
export RACK_DIR="$HOME/Rack-SDK"
make -C vcv clean
make -C vcv -j"$(getconf _NPROCESSORS_ONLN 2>/dev/null || sysctl -n hw.ncpu)" RACK_DIR="$RACK_DIR"
make -C vcv dist RACK_DIR="$RACK_DIR"
```

The distributable package is written below `vcv/dist/` as a `.vcvplugin` file. VCV's own plugin build system defines this package format.

To install directly into the local Rack user plugin directory:

```bash
make -C vcv install RACK_DIR="$RACK_DIR"
```

Restart Rack after installation.

## Manual smoke test in Rack

For the first functional validation:

1. Add **South Signal Lab CLOCK / CLOCK** to an empty patch and wait for the normal CLOCK boot sequence.
2. Press PLAY and patch OUT 1 to a scope or gate-visible destination. The output must switch only between 0 V and +5 V.
3. Exercise encoder turn/push and PLAY/TAP/STOP. The OLED must show the same menus and state transitions as the native simulator/hardware firmware.
4. Patch a Rack clock into SYNC, select the corresponding CLOCK external-source mode, and verify acquisition/lock and loss behavior from the normal CLOCK UI.
5. Patch a gate/trigger into RST and verify the configured RESET role/behavior through the real CLOCK settings.
6. Save the Rack patch, remove/reload it, and verify that CLOCK settings, presets and custom-groove persistence restore while transport still boots in STOP.
7. Attempt to add a second CLOCK. The experimental single-instance guard must show `ONE INSTANCE` and keep its outputs LOW.

For plugin-load failures or crashes, inspect Rack's `log.txt` in the Rack user folder.

## Packaging direction

The first CI job builds a Linux x64 `.vcvplugin` artifact independently of firmware releases. CI also opens the package, verifies the manifest/binary/panel payload and exercises Rack SDK's own `make install` target into an isolated Rack user directory.

For an Actions artifact, extract the workflow artifact first; the contained `.vcvplugin` is the actual Rack package. On Linux x64 it can be copied to `~/.local/share/Rack2/plugins-lin-x64/` and Rack restarted. `make -C vcv install` performs the same placement when building locally.

Once the real Rack SDK build and Rack smoke tests are stable on all target platforms, the normal CLOCK release pipeline can promote VCV packages into `dist/vcv/` and attach them to tagged releases.

<h6 align="center">From Munich with &#9829;</h6>
