<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK VCV Rack Developer Guide

Status: **EXPERIMENTAL / post-1.1.0**.

This guide describes how to build, install, package, inspect and smoke-test the CLOCK Trial-First plugin locally without vendoring the VCV Rack SDK into this repository.

**Windows is the primary local development path.** CLOCK development remains a normal Windows/VSCodium/PlatformIO/CMake workflow. Only the final Rack plugin binary uses VCV's required **MSYS2 MinGW 64-bit** toolchain. Your existing repository stays on the normal Windows drive and is visible from MSYS2 as `/c/...`; no second clone is required.

The guiding rule is simple: **Rack is the host container; CLOCK remains the product code.** The VCV shell may depend on the Rack API, but `src/`, `lib/clock_core/`, `sim/` and the embedded build must not acquire Rack dependencies.

## 1. Windows x64 — recommended first setup

VCV's official build instructions require **MSYS2 MinGW 64-bit**, not the plain MSYS shell. Do not use Visual Studio/MSVC for the Rack SDK Makefile build.

### 1.1 Install MSYS2 build tools

Install MSYS2, then launch **MSYS2 MinGW 64-bit** from the Start menu.

Update the package database:

```bash
pacman -Syu
```

Close and reopen the **MinGW 64-bit** shell when MSYS2 asks you to do so. Install the prerequisites in a separate command so an unavailable optional package cannot abort the database upgrade itself:

```bash
pacman -S --needed \
  git wget make tar unzip zip \
  mingw-w64-x86_64-gcc mingw-w64-x86_64-gdb \
  mingw-w64-x86_64-cmake autoconf automake libtool \
  jq python zstd mingw-w64-x86_64-pkgconf
```

Verify the two essentials before continuing:

```bash
command -v make
command -v g++
make --version
g++ --version
```

If `make` is missing, install it directly with `pacman -S --needed make`.

Verify that you are in the correct shell:

```bash
echo "$MSYSTEM"
```

Expected:

```text
MINGW64
```

If it prints `MSYS`, close that shell and open **MSYS2 MinGW 64-bit** instead.

### 1.2 Download and extract the Rack SDK

Download the pinned SDK:

```text
https://vcvrack.com/downloads/Rack-SDK-2.6.6-win-x64.zip
```

Extract it **outside the CLOCK repository**, for example below:

```text
C:\SDK\
```

The only important rule is that `RACK_DIR` must point to the directory that directly contains `plugin.mk`. Do not assume the extracted directory name.

From PowerShell, locate it with:

```powershell
Get-ChildItem C:\SDK -Filter plugin.mk -Recurse | Select-Object -ExpandProperty FullName
```

Example result:

```text
C:\SDK\Rack-SDK-2.6.6\plugin.mk
```

or, depending on the archive layout:

```text
C:\SDK\Rack-SDK\plugin.mk
```

### 1.3 PowerShell verification — do not use `export` here

If you are currently in PowerShell, use PowerShell syntax:

```powershell
$env:RACK_DIR = "C:\SDK\Rack-SDK"  # example; use the directory that actually contains plugin.mk

if (Test-Path "$env:RACK_DIR\plugin.mk") {
    Write-Host "Rack SDK OK: $env:RACK_DIR"
} else {
    Write-Error "plugin.mk not found below RACK_DIR"
}
```

This verifies the Windows path only. The actual plugin build should still be performed in **MSYS2 MinGW 64-bit**.

`export RACK_DIR=...` and `test -f ...` are **not PowerShell commands**.

### 1.4 Set `RACK_DIR` in MSYS2 MinGW64

Windows drives are mounted by MSYS2 below `/c`, `/d`, and so on. Therefore:

```text
C:\SDK\Rack-SDK
```

becomes:

```text
/c/SDK/Rack-SDK
```

Do not guess the extracted directory name. Find `plugin.mk` first:

```bash
find /c/SDK -maxdepth 3 -name plugin.mk -print
```

Then set and visibly verify the parent directory reported by that command, for example:

```bash
export RACK_DIR="/c/SDK/Rack-SDK"

if [ -f "$RACK_DIR/plugin.mk" ]; then
  echo "Rack SDK OK: $RACK_DIR"
else
  echo "ERROR: $RACK_DIR/plugin.mk not found"
  exit 1
fi
```

A bare command such as:

```bash
test -f "$RACK_DIR/plugin.mk"
```

is silent when it succeeds, which can look as if nothing happened. The explicit `if` block above is preferred for setup diagnostics.

If the check fails, locate the real SDK root from MSYS2:

```bash
find /c/SDK -maxdepth 3 -name plugin.mk -print
```

Then set `RACK_DIR` to the parent directory containing that file.

### 1.5 Open the CLOCK repository in the same MSYS2 shell

A normal Windows checkout is directly accessible. For example, if Explorer/VSCodium uses:

```text
C:\Users\you\Downloads\free-modular-clock-stm32f401-ui
```

then MSYS2 uses:

```bash
cd /c/Users/you/Downloads/free-modular-clock-stm32f401-ui
pwd
```

No copy or second Git clone is needed. Avoid spaces in the repository path and SDK path because the Rack Makefile build system is sensitive to them.

### 1.6 Run the SDK-independent CLOCK/VCV tests first

```bash
cmake --preset simulator-headless
cmake --build --preset simulator-headless
ctest --preset simulator-headless --output-on-failure

python scripts/check_architecture.py
python scripts/check_documentation.py
python -m unittest discover -s scripts/tests -p 'test_*.py' -v
python scripts/check_vcv_runtime_coverage.py
```

The CTest suite includes `vcv_runtime_adapter_tests`. These tests exercise the real CLOCK boot path, scheduler, controls, gate outputs, persistence image and external-input bridge without requiring Rack itself. `check_vcv_runtime_coverage.py` rebuilds that adapter with GCC coverage and enforces 95% line, 95% function and 90% non-throw decision-branch coverage.

### 1.7 Verify generated panel assets

```bash
python scripts/generate_vcv_panel.py --check
```

If this fails after an intentional `sim/panel_layout.ini` change:

```bash
python scripts/generate_vcv_panel.py
git diff -- vcv/res vcv/generated_panel_layout.hpp
```

Do not manually move VCV controls independently from the simulator geometry.

### 1.8 Build the VCV plugin

From the CLOCK repository root in **MSYS2 MinGW64**:

```bash
make -C vcv clean RACK_DIR="$RACK_DIR"
make -C vcv -j4 RACK_DIR="$RACK_DIR"
```

A successful build produces the Windows plugin binary below `vcv/`.

### 1.9 Create the distributable package

```bash
make -C vcv dist RACK_DIR="$RACK_DIR"
```

The resulting package is written below:

```text
vcv/dist/
```

and follows Rack's naming form:

```text
<slug>-<version>-win-x64.vcvplugin
```

List it with:

```bash
ls -lh vcv/dist/*.vcvplugin
```

### 1.10 Install into Rack

The normal local installation path is:

```bash
make -C vcv install RACK_DIR="$RACK_DIR"
```

Rack's default Windows user folder is:

```text
C:\Users\<username>\AppData\Local\Rack2\
```

The plugin package is installed below the matching `plugins-win-x64` directory.

For an isolated developer profile:

```bash
rm -rf build/rack-user
mkdir -p build/rack-user

make -C vcv install \
  RACK_DIR="$RACK_DIR" \
  RACK_USER_DIR="$(pwd)/build/rack-user"
```

This avoids changing your normal Rack profile.

### 1.11 Windows smoke test

Start VCV Rack 2 and locate **South Signal Lab Clock** in the Module Browser.

Minimum Trial-First smoke test:

1. Add one CLOCK module to an empty patch and observe the normal CLOCK boot sequence.
2. Press **PLAY / PAUSE**. Patch output **1** to a scope or gate-visible destination and verify 0/+5-V behavior; in factory One Clock mode, outputs 1–8 must fire coherently.
3. Drag/scroll the encoder to turn it. Use a stationary short click for encoder push, then hold it for more than 650 ms and verify that the production long-press action occurs.
4. Hover CLOCK and verify the keyboard bridge: `Shift` holds TAP/SHIFT, `Enter/E` holds encoder push, `Up/Down` turns the encoder, `P` drives PLAY/PAUSE, `T` drives TAP, and `S/Backspace` drives STOP/BACK. Verify `Shift+Enter` and `Shift+Up/Down` as the two-control chords.
5. Right-click CLOCK, choose **General Settings...**, and verify that the OLED opens the production `GENERAL SETTINGS` page rather than a separate Rack settings panel.
6. Exercise **PLAY / PAUSE**, **TAP / SHIFT** and **STOP / BACK** and compare OLED transitions with the simulator.
7. Patch a clock into **IN 1 / SYNC** and verify acquisition, lock, source behavior and loss handling. Also try a one-sample/very-short Rack trigger source; accepted edges must not depend sporadically on alignment with the 50 us CLOCK scheduler tick.
8. Patch a trigger/gate into **IN 2 / RST** and verify the configured RESET role/behavior.
9. Exercise One Clock, Independent, Divider Bank, Clock, Euclid, Sequencer, Swing, Groove, Humanize and Pre-Count as applicable.
10. Save the Rack patch, close/reopen Rack, reload it and verify that CLOCK settings/presets/Custom Grooves restore while transport still boots in STOP.
11. Add a second CLOCK instance. Until the host-HAL refactor lands, it must show `ONE INSTANCE` and keep all outputs LOW.
12. Remove and reinsert SYNC/RST cables while HIGH and verify that no stale asserted input survives cable removal.

If the plugin does not appear or fails to load, inspect:

```text
C:\Users\<username>\AppData\Local\Rack2\log.txt
```

Rack also exposes the user directory through:

```text
Help -> Open user folder
```

## 2. Repository and SDK boundary

The repository contains only CLOCK-owned source plus the thin Rack adapter under `vcv/`:

```text
src/                    production CLOCK application/engine/UI/services
lib/clock_core/         deterministic musical core
sim/                    native host platform and simulator
vcv/                    Rack-only adapter, widget and package manifest
```

The Rack SDK is **not** a submodule, vendored directory or release asset. Keep it outside the repository.

Do not place a `Rack-SDK` directory in the repository tree. Local SDK directories are ignored by `.gitignore`, and CI verifies that Rack API use remains confined to `vcv/`.

CLOCK currently pins VCV development to **Rack SDK 2.6.6**. The matching official SDK archives are:

```text
https://vcvrack.com/downloads/Rack-SDK-2.6.6-win-x64.zip
https://vcvrack.com/downloads/Rack-SDK-2.6.6-mac-x64.zip
https://vcvrack.com/downloads/Rack-SDK-2.6.6-mac-arm64.zip
https://vcvrack.com/downloads/Rack-SDK-2.6.6-lin-x64.zip
```

The CI workflow downloads the Linux archive directly from `vcvrack.com` into the ephemeral GitHub runner and deletes it automatically when the job ends.

## 3. What is compiled into the plugin

The plugin build compiles CLOCK production sources directly from `src/` and `lib/clock_core/`, plus the project-owned host boundary reused from `sim/`. Rack-specific source lives only in `vcv/`.

The dependency direction is one-way:

```text
CLOCK production code  <-  vcv/ adapter  ->  Rack API
```

The following are forbidden outside `vcv/`:

```text
#include <rack.hpp>
#include "rack.hpp"
rack::
```

`scripts/check_architecture.py` enforces that boundary in CI.

The Rack SDK contributes headers and build/link metadata. It is not copied into the `.vcvplugin` package. On Linux the resulting plugin dynamically depends on Rack's `libRack.so`; Rack supplies that library at runtime.

## 4. macOS setup

With Homebrew:

```bash
brew install git wget cmake autoconf automake libtool jq python zstd pkg-config
```

Use the SDK matching the CPU architecture of the Rack build you want to load:

```text
Rack-SDK-2.6.6-mac-x64.zip
Rack-SDK-2.6.6-mac-arm64.zip
```

Set `RACK_DIR` to the directory containing `plugin.mk`:

```bash
export RACK_DIR="$HOME/SDK/Rack-SDK-2.6.6"

if [ -f "$RACK_DIR/plugin.mk" ]; then
  echo "Rack SDK OK: $RACK_DIR"
else
  echo "ERROR: plugin.mk not found"
  exit 1
fi
```

Then use the common build/test/package commands in section 6.

## 5. Linux x64 setup

For Ubuntu/Debian, VCV currently documents:

```bash
sudo apt install unzip git gdb curl cmake \
  libx11-dev libglu1-mesa-dev libxrandr-dev libxinerama-dev \
  libxcursor-dev libxi-dev zlib1g-dev libasound2-dev \
  libgtk2.0-dev libgtk-3-dev libjack-jackd2-dev jq zstd \
  libpulse-dev pkg-config autoconf libtool
```

Extract `Rack-SDK-2.6.6-lin-x64.zip` outside the CLOCK repository and set `RACK_DIR` to the directory containing `plugin.mk`:

```bash
export RACK_DIR="$HOME/SDK/Rack-SDK-2.6.6"

if [ -f "$RACK_DIR/plugin.mk" ]; then
  echo "Rack SDK OK: $RACK_DIR"
else
  echo "ERROR: plugin.mk not found"
  exit 1
fi
```

Then use the common build/test/package commands in section 6.

## 6. Common macOS/Linux build and test flow

Run the Rack-independent tests first:

```bash
cmake --preset simulator-headless
cmake --build --preset simulator-headless
ctest --preset simulator-headless --output-on-failure

python scripts/check_architecture.py
python scripts/check_documentation.py
python -m unittest discover -s scripts/tests -p 'test_*.py' -v
python scripts/generate_vcv_panel.py --check
```

Build and package:

```bash
make -C vcv clean RACK_DIR="$RACK_DIR"
make -C vcv -j4 RACK_DIR="$RACK_DIR"
make -C vcv dist RACK_DIR="$RACK_DIR"
make -C vcv install RACK_DIR="$RACK_DIR"
```

The package is written below `vcv/dist/`.

For an isolated Rack profile:

```bash
rm -rf build/rack-user
mkdir -p build/rack-user
make -C vcv install \
  RACK_DIR="$RACK_DIR" \
  RACK_USER_DIR="$(pwd)/build/rack-user"
```

## 7. Inspect the distributable `.vcvplugin`

On systems with GNU tar and zstd support:

```bash
package="$(find vcv/dist -maxdepth 1 -name '*.vcvplugin' -print -quit)"
tar --zstd -tf "$package"
```

The CLOCK package must contain its manifest, plugin binary and `res/` assets. It must **not** contain a Rack SDK directory, Rack headers, `libRack`, or copied Rack/Core resources.

The GitHub Actions build enforces the same packaging rule.

## 8. Logs and load failures

If the plugin does not appear or crashes while loading, inspect Rack's `log.txt` in the Rack user folder.

Current default user folders are:

```text
Windows  C:\Users\<username>\AppData\Local\Rack2\
macOS    ~/Library/Application Support/Rack2/
Linux    ~/.local/share/Rack2/
```

Rack can also be launched with an explicit user directory using `-u`, and it supports `RACK_USER_DIR` for development isolation.

## 9. CI parity

`.github/workflows/vcv.yml` deliberately does **not** commit or cache the SDK in the repository. The current dedicated Rack job is **Linux x64** and:

1. checks out CLOCK with the current GitHub-maintained checkout action and read-only repository permissions;
2. installs the explicit host build/package tools used by the job;
3. runs the VCV tooling contract tests and the focused `clock_vcv_runtime.cpp` coverage gate;
4. downloads the pinned official Rack SDK archive to `$RUNNER_TEMP`;
5. validates generated panel assets;
6. builds the plugin with `RACK_DIR=$RUNNER_TEMP/Rack-SDK`;
7. runs `make dist`;
8. inspects the `.vcvplugin` payload and Linux dynamic dependency metadata;
9. rejects bundled Rack SDK/Rack runtime content;
10. runs `make install` against an isolated Rack user directory;
11. uploads only the resulting CLOCK Linux `.vcvplugin` as the workflow artifact.

The runner and downloaded SDK disappear after the job. This workflow proves the Rack API shell against the pinned Linux SDK; it does **not** currently build a Windows `.vcvplugin`. Windows remains the primary local development path and is verified with the official MinGW64/Rack-SDK procedure above. A Windows Rack CI job is therefore still an explicit portability gap, not something inferred from the Linux build.

## 10. Licensing boundary

CLOCK project-owned source, including `vcv/`, remains under **PolyForm Noncommercial License 1.0.0**.

VCV Rack itself is GPL-3.0-or-later with the **VCV Rack Non-Commercial Plugin License Exception**. The upstream exception permits a plugin distributed free of charge to use the Rack API in source and binary form and link to Rack regardless of the plugin's own license terms. It does not authorize copying significant non-API Rack source into CLOCK.

CLOCK therefore follows these hard rules:

- Rack SDK: external build dependency, never vendored;
- Rack API use: confined to `vcv/`;
- `src/`, `lib/clock_core/` and `sim/`: no `rack.hpp`, no `rack::`;
- `libRack`: runtime host dependency, never bundled in `.vcvplugin`;
- Rack/Core source: not copied;
- VCV Component Library/Core panel graphics: not copied;
- CLOCK panel/widget artwork: project-generated from `sim/panel_layout.ini`;
- plugin distribution: free of charge while relying on the non-commercial plugin exception.

If the VCV plugin is ever sold rather than distributed free of charge, this licensing assumption must be reviewed before distribution.

## 11. Useful official references

- Rack build environment and SDK workflow: <https://vcvrack.com/manual/Building>
- Plugin development/install workflow: <https://vcvrack.com/manual/PluginDevelopmentTutorial>
- Rack user folder and command-line options: <https://vcvrack.com/manual/Installing>
- Rack licensing and plugin exception: <https://github.com/VCVRack/Rack/blob/v2/LICENSE.md>
- Rack SDK archive: <https://vcvrack.com/downloads/>

<h6 align="center">From Munich with &#9829;</h6>
