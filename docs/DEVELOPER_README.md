<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Developer Workstation Guide

This guide describes a reproducible development setup for the **CLOCK** firmware using **VSCodium**. Windows is the primary workstation platform; macOS and Linux are fully supported for editing and PlatformIO builds. The repository's full host coverage pipeline is GNU GCC/gcov based, so Linux or WSL2 is the simplest environment for running the exact same coverage and sanitizer checks as CI.

The firmware targets the **STM32F401CCU6 Black Pill** and uses PlatformIO with the STM32duino Arduino framework. No third-party PlatformIO libraries are required.

## 1. What the toolchain contains

There are three distinct layers in the development toolchain:

1. **VSCodium** — source editor, integrated terminal, task launcher, search, Git UI.
2. **PlatformIO Core** — resolves the STM32 platform/toolchain, builds firmware, runs the native core tests, and uploads through DFU.
3. **Host GCC toolchain** — builds the complete firmware against deterministic fake hardware for coverage, ASan, and UBSan.

PlatformIO downloads the ARM compiler, STM32duino framework, and target tools into its own package directory. Do **not** install a separate ARM GCC toolchain unless a specific debugging workflow requires it.

## 2. Repository layout developers should know first

```text
.
├── platformio.ini               PlatformIO environments and target policy
├── src/
│   ├── config.h                 Global compile-time behavior
│   ├── defaults.h               Factory defaults
│   ├── pin_map.h                Human-readable hardware wiring
│   ├── ui_text.h/.cpp           Localized static UI text
│   ├── version.h                Firmware version source of truth
│   ├── app/                     Composition root / lifecycle
│   ├── domain/                  State and domain model
│   ├── engine/                  Real-time scheduler
│   ├── hal/                     Hardware and framework boundary
│   ├── services/                Persistence, tap tempo, templates
│   └── ui/                      Navigation, editing, rendering
├── lib/clock_core/              Hardware-independent timing/pattern core
├── test/                        Native and whole-firmware host tests
├── sim/                         Native host simulator and hardware shim
├── CMakeLists.txt               Native simulator build
├── CMakePresets.json            Portable simulator/headless presets
├── scripts/                     CI, coverage, release, memory-budget tools
├── docs/                        Architecture, user, HIL and developer docs
└── .vscode/tasks.json           Ready-to-run VSCodium tasks
```

A useful rule of thumb is: **hardware details belong in `src/hal/` or `src/pin_map.h`; musical behavior does not.**

## 3. Common prerequisites

Install these before opening the project:

- Git
- VSCodium
- Python 3.9 or newer; Python 3.11/3.12 is a conservative local choice
- PlatformIO Core
- USB access to the STM32 Black Pill for DFU uploads

For full host coverage/sanitizer testing, also install:

- GNU `g++`
- GNU `gcov` from the same GCC release

Verify the basic command-line tools:

```text
git --version
python --version
pio --version
```

On systems where Python is exposed as `python3`, use `python3` consistently in the commands below.

---

# Windows — primary development environment

## 4. Install VSCodium

The simplest Windows installation uses WinGet:

```powershell
winget install vscodium
```

A normal installer from the VSCodium project is equally suitable.

After installation, verify that `codium` is available from a new PowerShell window if command-line launching is desired:

```powershell
codium --version
```

If the GUI works but `codium` is not on PATH, this is not a firmware-build blocker; VSCodium can still open the repository normally.

## 5. Install Python on Windows

PlatformIO requires Python 3.9+.

When using the standard Python installer, enable **Add Python to PATH**. Then open a new PowerShell window and check:

```powershell
python --version
```

If several Python installations are present, verify which one is active:

```powershell
where.exe python
py -0p
```

Avoid mixing unrelated system-wide PlatformIO installations. One PlatformIO Core per user is much easier to diagnose.

## 6. Install PlatformIO Core on Windows

PlatformIO's recommended installer creates an isolated Python environment under the user profile. Run the official `get-platformio.py` installer as a normal user, not from an elevated Administrator shell.

After installation, add this directory to the **user** `Path` environment variable:

```text
%USERPROFILE%\.platformio\penv\Scripts\
```

Close and reopen PowerShell and VSCodium, then verify:

```powershell
pio --version
where.exe pio
```

For CI parity, this repository currently tests with PlatformIO Core 6.1.19. A local newer stable Core may work, but when diagnosing a difference from CI, reproduce with the CI-pinned version first.

## 7. Recommended Windows repository location

A normal local path is sufficient, for example:

```text
C:\dev\clock
```

Avoid deeply nested paths when possible. If an antivirus, cloud-sync client, or indexing service causes `.pio` file-locking problems, move the working copy to a plain local development directory before investigating the firmware itself.

## 8. Open the project in VSCodium

Open the directory that contains `platformio.ini`, not `src/` by itself.

From PowerShell:

```powershell
cd C:\dev\clock
codium .
```

Or use **File → Open Folder**.

The command line remains authoritative: no editor extension is required to build or test the firmware. The repository nevertheless ships a curated `.vscode/extensions.json` recommendation set for VSCodium/Open VSX because it improves day-to-day navigation without making the build depend on editor state.

## 9. Recommended VSCodium extensions

VSCodium reads the repository's `.vscode/extensions.json` and can offer these Open VSX extensions:

| Extension ID | Purpose |
| --- | --- |
| `LordImmaculate.platformio-ide` | PlatformIO integration/tasks/status for VSCodium |
| `llvm-vs-code-extensions.vscode-clangd` | C/C++ navigation, diagnostics and symbol indexing |
| `ms-vscode.cmake-tools` | Native simulator CMake configure/build/test integration |
| `EditorConfig.EditorConfig` | Applies repository `.editorconfig` rules |
| `streetsidesoftware.code-spell-checker` | Catches documentation/comment spelling errors |
| `PKief.material-icon-theme` | Clear source/test/docs folder and file icons |
| `johnpapa.vscode-peacock` | Gives this workspace a distinct title/activity-bar accent |

These are conveniences only. `pio`, the Python scripts and GitHub Actions remain the source of truth. Avoid installing multiple competing C/C++ language servers at once; when `clangd` is active, disable overlapping IntelliSense diagnostics from other extensions if they produce duplicate errors.

The committed `.vscode/settings.json` contains only portable editor preferences. It selects Material Icon Theme, assigns project folder icons, and gives the workspace a Peacock accent color. It contains **no user names, absolute paths, compiler locations or machine-specific settings**.

### Compile database privacy

`compile_commands.json` is intentionally ignored by Git. PlatformIO/clang tooling can generate it locally, but its entries normally contain absolute workstation paths such as `C:\Users\...` or `/Users/...`. Treat it as disposable local metadata. The same rule applies to clangd caches, browse databases and VSCodium-generated local launch/index files covered by `.gitignore`.

## 10. Windows: build the I2C firmware

In the VSCodium terminal:

```powershell
pio run -e blackpill_f401cc
```

The first build takes longer because PlatformIO downloads the pinned STM32 platform, STM32duino framework, ARM toolchain, and upload utilities.

The build must also pass the post-link memory gate:

- firmware-owned Flash: 224 KiB (sector 0 plus sectors 3..5)
- maximum allowed Flash use: 90% of that total
- STM32F401CC SRAM: 64 KiB
- maximum allowed static RAM use: 90%

A memory-budget violation fails the build rather than printing a warning.

## 11. Windows: build the current SPI OLED firmware

```powershell
pio run -e blackpill_f401cc_spi_ssd1315
```

SPI remains the reference transport because it minimizes display service time. The SSD1315 profile defaults to PA5=SCK, PA7=MOSI, PA4=CS, PB9=D/C and PB15=active-low OLED RESET; `blackpill_f401cc_spi_ssd1306` selects SSD1306. I2C is also a supported hardware target for procurement flexibility: `blackpill_f401cc_i2c_ssd1306` and `blackpill_f401cc_i2c_ssd1315` use deferred 24-byte-chunk refresh so display traffic cannot become the musical scheduler.

## 12. Windows: upload through DFU

The project uses a persistence-preserving custom DFU upload command. The ELF is split into the sector-0 boot/vector image and the sectors-3..5 application image; Flash sectors 1 and 2 are never included in either transfer. Do not replace this with PlatformIO's default contiguous `firmware.bin` upload, because that raw image would span and overwrite the persistent A/B sectors.

When upgrading from the pre-A/B layout, the uploader first checks sectors 1/2. If neither contains a valid A/B generation, it reads the former 4 KiB STM32duino EEPROM image from sector 5 and, when it recognizes schema-v3/v4 CURRENT or preset records, wraps that image in the new slot format and commits it to sector 1 **before** sector 5 is reused by the application. Subsequent A/B-era uploads leave both persistence sectors untouched.

Put the Black Pill into DFU mode using its BOOT0/NRST procedure, then run:

```powershell
pio run -e blackpill_f401cc -t upload
```

For the SPI build:

```powershell
pio run -e blackpill_f401cc_spi_ssd1315 -t upload
```

If PlatformIO cannot see the device, first verify that Windows sees the board in Device Manager while it is in DFU mode. Do not change firmware pins or upload settings to compensate for a host USB/driver problem.

## 13. Windows: run the fast native core tests

```powershell
pio test -e native
```

These tests exercise the production `clock_core` implementation and are useful after every timing, ratio, Euclid, sequencer, probability, phase, swing, or reset change.

## 14. Windows: run repository policy/tooling tests

```powershell
python scripts\check_architecture.py
python -m unittest discover -s scripts\tests -p "test_*.py" -v
```

These checks validate architecture boundaries, version/release tooling, packaging, and the STM32 memory-budget parser.

## 15. Windows: full host coverage and sanitizers

The complete host runner is deliberately GNU GCC/gcov based. The most reproducible Windows method is **WSL2 with Ubuntu**, because that matches GitHub Actions closely.

Inside WSL2:

```bash
sudo apt update
sudo apt install -y build-essential python3 python3-venv git
```

Then from the repository directory:

```bash
python3 scripts/run_host_tests.py --skip-sanitizers
python3 scripts/run_host_tests.py --sanitizers-only
```

The first command produces repository-wide coverage. The second runs ASan/UBSan independently.

The source tree can be accessed through `/mnt/c/...`, although a clone inside the WSL filesystem is faster for repeated full coverage builds. The authoritative result is always GitHub Actions, so local WSL coverage is optional rather than a prerequisite for ordinary firmware editing.


## 15A. Windows: build and run the native simulator

The simulator uses **CMake + SDL3**, independently of PlatformIO. Install CMake, Ninja, Git, and a native C++ compiler. Visual Studio 2022 Build Tools with the Desktop C++ workload is the simplest Windows choice; MSYS2/UCRT64 is also valid. Keep `cmake`, `ninja`, and `git` on the user/system PATH. When using MSVC with the committed Ninja preset, launch the terminal/VSCodium from **Developer PowerShell for VS 2022** (or initialize the matching Visual Studio developer environment first) so `cl.exe`, the Windows SDK, and linker are visible. With MSYS2, run from the UCRT64 environment instead. Do not commit compiler paths or Visual Studio installation paths.

The portable preset fetches the pinned SDL3 source on first configure:

```powershell
cmake --preset simulator
cmake --build --preset simulator
python scripts\run_simulator.py
```

If network access is unavailable, install SDL3 separately and configure with `CLOCK_SIMULATOR_FETCH_SDL=OFF`; see `docs/SIMULATOR.md`.

For the SDL-free firmware/simulator integration tests:

```powershell
python scripts\run_simulator_tests.py
```

VSCodium users can run the equivalent committed tasks: **Clock: Configure native simulator**, **Clock: Build native simulator**, **Clock: Run native simulator**, and **Clock: Headless simulator tests**. CMake Tools is available through Open VSX and understands the committed presets; its use remains optional because the command line is authoritative.

The simulator's local persistence file and `CMakeUserPresets.json` are intentionally ignored by Git. Machine-specific kits, compiler paths, and local SDL locations belong in `CMakeUserPresets.json` or the shell environment, never in committed `.vscode` files.

Panel geometry is separately configurable in `sim/panel_layout.ini`. Coordinates and control dimensions are physical millimetres from the 10 HP / 3U panel origin, resolved through one uniform `pixels_per_mm` scale. D6R visible-actuator/behind-panel-body diameters, per-button colors, TS/TRS Thonkiconn nut/bushing/opening diameters, knob diameter, and 3 mm LED diameter are all data rather than renderer constants. `python scripts/run_simulator.py` passes this file automatically. Use `--layout path/to/layout.ini` for another geometry or `--panel-image path/to/panel.png` to override only the front-panel background. SDL3 3.4+ loads BMP, PNG, and JPEG directly, so no SDL_image package is required. Local experimental layouts may be stored as `sim/panel_layout.local.ini`, which is ignored by Git. See `docs/SIMULATOR.md` for the complete schema.

For panel controls, virtual time, persistent-state files, macOS/Linux setup, and current limitations, see [`SIMULATOR.md`](SIMULATOR.md).

---

# macOS

## 16. Install VSCodium on macOS

With Homebrew:

```bash
brew install --cask vscodium
```

Install Git/Python if required by the workstation:

```bash
brew install git python
```

Verify:

```bash
git --version
python3 --version
```

## 17. Install PlatformIO Core on macOS

PlatformIO's isolated installer is the preferred approach:

```bash
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```

To make `pio` available in ordinary shells, make sure `$HOME/.local/bin` is on PATH and create the PlatformIO symlinks documented by PlatformIO:

```bash
mkdir -p "$HOME/.local/bin"
ln -sf "$HOME/.platformio/penv/bin/platformio" "$HOME/.local/bin/platformio"
ln -sf "$HOME/.platformio/penv/bin/pio" "$HOME/.local/bin/pio"
ln -sf "$HOME/.platformio/penv/bin/piodebuggdb" "$HOME/.local/bin/piodebuggdb"
```

For Zsh, a typical `~/.zprofile` PATH entry is:

```bash
export PATH="$PATH:$HOME/.local/bin"
```

Start a new terminal and verify:

```bash
pio --version
```

## 18. macOS build and upload

```bash
pio run -e blackpill_f401cc
pio run -e blackpill_f401cc_spi_ssd1315
pio test -e native
```

Upload in DFU mode with:

```bash
pio run -e blackpill_f401cc -t upload
```

## 19. macOS full coverage

Apple's `/usr/bin/g++` is normally Clang, while this repository's coverage aggregator consumes GNU gcov JSON. Install GNU GCC if local full coverage is required:

```bash
brew install gcc
```

Homebrew installs version-suffixed binaries. Find the installed compiler and matching gcov:

```bash
ls "$(brew --prefix)/bin/g++-"*
ls "$(brew --prefix)/bin/gcov-"*
```

Then export matching tools, for example with the actual suffix installed on that machine:

```bash
export CXX="$(brew --prefix)/bin/g++-<major>"
export GCOV="$(brew --prefix)/bin/gcov-<major>"
python3 scripts/run_host_tests.py --skip-sanitizers
python3 scripts/run_host_tests.py --sanitizers-only
```

Do not mix a GCC coverage build with a different `gcov` major release.

---

# Linux

## 20. Linux prerequisites

For Debian/Ubuntu-family systems:

```bash
sudo apt update
sudo apt install -y git curl build-essential python3 python3-venv
```

Install VSCodium using the VSCodium repository/package appropriate for the distribution, or a supported package format such as Snap/Flatpak.

PlatformIO also recommends installing its `99-platformio-udev.rules` on Linux so non-root users can access supported USB/serial devices. Follow PlatformIO's current udev-rule instructions for the distribution.

Never run ordinary PlatformIO builds with `sudo`.

## 21. Install PlatformIO Core on Linux

```bash
curl -fsSL -o get-platformio.py https://raw.githubusercontent.com/platformio/platformio-core-installer/master/get-platformio.py
python3 get-platformio.py
```

Add `$HOME/.local/bin` to PATH and create the same symlinks described in the macOS section:

```bash
mkdir -p "$HOME/.local/bin"
ln -sf "$HOME/.platformio/penv/bin/platformio" "$HOME/.local/bin/platformio"
ln -sf "$HOME/.platformio/penv/bin/pio" "$HOME/.local/bin/pio"
ln -sf "$HOME/.platformio/penv/bin/piodebuggdb" "$HOME/.local/bin/piodebuggdb"
```

For Bash, add to `~/.profile`:

```bash
export PATH="$PATH:$HOME/.local/bin"
```

Restart the login session or source the profile, then verify:

```bash
pio --version
g++ --version
gcov --version
```

## 22. Linux build, tests, coverage, and upload

```bash
# Current I2C prototype
pio run -e blackpill_f401cc

# Planned SPI display transport
pio run -e blackpill_f401cc_spi_ssd1315

# Production core through PlatformIO Unity
pio test -e native

# Architecture and release tooling
python3 scripts/check_architecture.py
python3 -m unittest discover -s scripts/tests -p 'test_*.py' -v

# Complete host firmware coverage
python3 scripts/run_host_tests.py --skip-sanitizers

# ASan + UBSan matrix
python3 scripts/run_host_tests.py --sanitizers-only

# DFU upload
pio run -e blackpill_f401cc -t upload
```

---

# VSCodium workflow

## 23. Repository tasks

The repository provides `.vscode/tasks.json`, which VSCodium understands directly.

Open the Command Palette and select:

```text
Tasks: Run Task
```

Available tasks include:

- `Clock: Build I2C firmware`
- `Clock: Build SPI firmware`
- `Clock: Upload I2C firmware (DFU)`
- `Clock: Native core tests`
- `Clock: Architecture policy`
- `Clock: Python tooling tests`
- `Clock: Full host coverage`
- `Clock: Host sanitizers`

The task definitions call the same commands documented here. They are conveniences, not a parallel build system.

## 24. Editing configuration safely

While the hardware is still prerelease, most board adaptation should require only these top-level files:

### `src/pin_map.h`

Change this when physical wiring changes. Names describe connected functions rather than arbitrary MCU pins:

```cpp
kEncoderPhaseAPin
kEncoderPhaseBPin
kPlayPauseButtonPin
kChannel1GateLedPin
kGateBufferOutputEnablePin
kDisplaySpiDataCommandPin
```

Do not scatter pin literals through HAL or application code.

### `src/config.h`

Use this for compile-time firmware policy:

- display transport/bus speed
- scheduler frequency
- UI timing
- BPM limits
- persistence timing
- screensaver frame cadence and dim contrast

### `src/defaults.h`

Use this for editable factory user settings:

- BPM and meter
- channel defaults
- Euclid defaults
- sequencer defaults
- sync defaults
- screensaver mode/start/dim/off defaults

### `src/ui_text.h` / `src/ui_text.cpp`

All static user-visible strings belong here. UI renderers refer to text IDs rather than embedding English strings, keeping the code ready for complete language catalogs.

### `src/version.h`

This is the only firmware-version source of truth used by code and release tooling.

## 25. Before committing a firmware change

For a small UI or service change, the minimum local sequence is:

```bash
python scripts/check_architecture.py
pio test -e native
pio run -e blackpill_f401cc
```

For a timing, HAL, persistence, or release-related change, also run the complete host suite on a GNU/Linux environment:

```bash
python scripts/run_host_tests.py --skip-sanitizers
python scripts/run_host_tests.py --sanitizers-only
```

And compile the alternate display transport:

```bash
pio run -e blackpill_f401cc_spi_ssd1315
```

## 26. Coverage policy

The CI gates are intentionally strict without making normal maintenance brittle:

```text
Executable lines:        >= 95%
Production functions:    >= 95%
Decision branches:       >= 90%
ASan:                    clean
UBSan:                   clean
Host compiler warnings:  zero (-Werror)
```

The gates are deliberately high but no longer absolute. The goal is meaningful behavioral coverage rather than artificial tests written solely to reach a percentage. Uncovered production functions are still reported explicitly by the coverage script and should normally be justified or tested before merge.

Coverage reports are written to:

```text
coverage/full_coverage.txt
coverage/full_coverage.json
```

## 27. Memory policy

The STM32F401CC provides 256 KiB physical Flash and 64 KiB SRAM. Flash sectors 1 and 2 (16 KiB each) are reserved as independent power-loss-safe persistence slots. Firmware owns sector 0 and sectors 3..5, for 224 KiB total firmware Flash. Under the normal 90% build policy the application remains below the 208 KiB contiguous high region, so sector 0 normally contains only the vector table and reserved boot headroom.

The post-link memory gate requires at least 10% headroom inside the firmware-owned Flash and SRAM:

```text
Firmware Flash capacity:     229376 bytes
Flash build limit (90%):     206438 bytes
Static RAM capacity:          65536 bytes
RAM build limit (90%):        58982 bytes
```

The gate uses the final ELF and GNU `size`; it is not an estimate based on source files.

## 28. Boot/output safety contract

The firmware deliberately separates **stored transport metadata** from **power-up execution state**.

A previous PLAY value may be present in persistent storage, but boot always performs this sequence:

1. disable the external HCT244 output stage
2. configure all eight gate/LED source pins LOW
3. initialize controls/display
4. show the boot screen
5. force transport to STOP
6. initialize/start the scheduler from STOP
7. explicitly drive every channel LOW again
8. enable the external output stage

Host regression tests seed persistent storage with PLAY, boot the application, execute tens of thousands of scheduler ticks, and verify that no gate pin rises.

This safety behavior must not be weakened by a future "resume last state" feature.

## 29. Generated and disposable directories

These are build artifacts and may be deleted at any time:

```text
.pio/
build/
coverage/
dist/
```

A clean firmware rebuild is:

```bash
pio run -t clean
pio run -e blackpill_f401cc
```

If PlatformIO package metadata itself is corrupt, investigate the per-user `.platformio` directory separately; do not commit generated packages into the repository.

## 30. Common problems

### `pio` is not recognized

Check PATH first.

Windows:

```powershell
$env:Path -split ';' | Select-String platformio
where.exe pio
```

The usual PlatformIO installer location is:

```text
%USERPROFILE%\.platformio\penv\Scripts\
```

macOS/Linux:

```bash
which pio
echo "$PATH"
```

Ensure `$HOME/.local/bin` is in PATH and that the PlatformIO symlink exists.

### PlatformIO appears to use a different Core than expected

```bash
which pio
pio --version
```

On Windows also run:

```powershell
where.exe pio
```

Multiple installations are a common source of confusing package/version behavior.

### `undefined reference to setup` / `loop`

`src/main.cpp` must include `Arduino.h`, and global Arduino `setup()`/`loop()` must keep the framework's expected linkage. Do not move them into a namespace.

### Native coverage says `g++` or `gcov` is missing

The full coverage script is a GNU host workflow, separate from the ARM compiler PlatformIO downloads. Install native GCC/gcov or run the suite in WSL2/Linux/GitHub Actions.

### DFU upload cannot find the board

Confirm the board is actually in DFU mode and visible to the operating system. Firmware compilation success does not prove host USB permissions/driver state.

### A firmware change passes host coverage but misbehaves electrically

Host coverage is not a substitute for bench verification. Read `docs/TIMING.md` for the software timing contract and use `docs/HIL_TEST_PLAN.md` for gate timing, boot pulses, comparator thresholds, external sync, display-stress timing, and physical bus integrity.

## 31. GitHub Actions and releases

CI runs automatically on pushes and pull requests. It checks architecture, Python tooling, native tests, complete host coverage, sanitizers, the headless simulator on Windows/macOS/Linux, the SDL3 frontend build, the I2C firmware build, the SPI build, and memory budgets. Target BIN/ELF files are not uploaded while STM32duino remains statically linked; tagged releases are source-only.

Release tags must exactly match `src/version.h`:

```text
Firmware: 0.19.0-beta.3
Tag:      v0.19.0-beta.3
```

Prerelease tags containing `-` are published as GitHub prereleases.

`scripts/package_release.py` derives deterministic binary filenames from the version source of truth, but it is deliberately fail-closed while STM32duino remains statically linked. The explicit `--acknowledge-lgpl-static-link` flag is reserved for a separately compliance-reviewed binary distribution.

## 32. Recommended first-day validation

After setting up a new workstation, run these commands in order:

```bash
pio --version
python scripts/check_architecture.py
python -m unittest discover -s scripts/tests -p 'test_*.py' -v
pio test -e native
pio run -e blackpill_f401cc
pio run -e blackpill_f401cc_spi_ssd1315
```

On Linux/WSL with GNU GCC/gcov, continue with:

```bash
python scripts/run_host_tests.py --skip-sanitizers
python scripts/run_host_tests.py --sanitizers-only
```

If all commands pass, the workstation is functionally equivalent to the repository's CI development path.

## Reference sources

- VSCodium installation: https://vscodium.com/install
- PlatformIO Core installation: https://docs.platformio.org/en/stable/core/installation/
- PlatformIO system requirements: https://docs.platformio.org/en/stable/core/installation/requirements.html
- PlatformIO shell commands/PATH: https://docs.platformio.org/en/stable/core/installation/shell-commands.html
- PlatformIO environment variables: https://docs.platformio.org/en/stable/envvars.html

<h6 align="center">From Munich with &#9829;</h6>
