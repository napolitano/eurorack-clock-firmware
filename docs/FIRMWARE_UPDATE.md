<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK Firmware Installation and Updates

CLOCK uses an STM32F401CCU6 Black Pill. There are two supported programming paths:

- **ST-LINK / SWD** - recommended for the first installation, recovery, and debugging;
- **USB DFU via PlatformIO** - recommended for routine firmware updates.

The project uses a split Flash layout so settings, presets, and arcade high scores survive normal updates. Use the procedures below rather than flashing an arbitrary flat binary.

> [!CAUTION]
> **USB and Eurorack power must not be connected at the same time.** Before connecting the Black Pill USB port, switch the Eurorack system off and preferably unplug CLOCK's Eurorack ribbon cable. Leave the ribbon cable disconnected until the USB cable has been removed.

## Which method should I use?

| Situation | Recommended method |
| --- | --- |
| Blank/new Black Pill | ST-LINK / SWD |
| Normal update on a working CLOCK | USB DFU via PlatformIO |
| Firmware does not boot | ST-LINK / SWD |
| USB DFU is not detected | ST-LINK / SWD after checking the USB cable/driver |
| Debugging a hardware fault | ST-LINK / SWD |

## Before you start

For either method you need the CLOCK source tree and PlatformIO. For most DIY builders, **PlatformIO IDE for Visual Studio Code** is the easiest installation; PlatformIO Core is included in the extension and does not need to be installed separately. If you prefer a standalone command line, install PlatformIO Core instead.

Official PlatformIO documentation:

- https://docs.platformio.org/en/stable/integration/ide/vscode.html
- https://docs.platformio.org/en/stable/core/installation/

The WeAct Black Pill F401CC is supported by PlatformIO as `blackpill_f401cc`, including both `stlink` and `dfu` upload protocols:

- https://docs.platformio.org/en/stable/boards/ststm32/blackpill_f401cc.html

## Firmware variants

The normal release build uses BEATKNECHT as the boot Easter egg:

```text
release_default       BEATKNECHT
release_pixel_raid    Pixel Raid
release_formula_1     Formula 1
release_breakout      Breakout
release_egg_journey   Egg Journey
```

Use the same environment name for both build and update commands.

## Method A - ST-LINK / SWD

ST-LINK talks directly to the STM32 debug interface. It does not depend on the firmware already installed, which makes it the safest first-install and recovery method.

### 1. Connect the probe

1. Switch the Eurorack system off before attaching wires.
2. Connect **GND**, **SWDIO**, and **SWCLK** between ST-LINK and the Black Pill's corresponding SWD pins.
3. Connect the probe's **VTref / VDD_TARGET** input to the Black Pill **3.3 V rail** so the probe can sense the target logic level. On an official ST-LINK this is a reference input, not a general-purpose power output.
4. **NRST** is optional for ordinary programming but recommended for recovery/debugging.
5. Power the target from **one source only**. For an installed module, the normal choice is the Eurorack supply. For a bench-only Black Pill, USB can power the board only when the Eurorack ribbon cable is disconnected.

### 2. Build the firmware

From the CLOCK source directory:

```bash
pio run -e release_default
```

For another Easter-egg variant, substitute the matching environment name from the table above.

The address-aware image is:

```text
.pio/build/release_default/firmware.elf
```

### 3. Program with STM32CubeProgrammer

1. Open STM32CubeProgrammer.
2. Select **ST-LINK** and **SWD**.
3. Connect to the target.
4. Open the generated `firmware.elf`.
5. Program/download it with verification enabled.
6. Reset or power-cycle CLOCK after programming.

For a normal update, **do not use Full chip erase**. A full erase also removes the Flash sectors that hold settings, presets, and high scores.

STM32CubeProgrammer information:

- https://www.st.com/en/development-tools/stm32cubeprog.html

## Method B - USB DFU with PlatformIO

The STM32F401 contains an ST system-memory bootloader with USB DFU support. CLOCK uses that bootloader for routine updates and a project-owned PlatformIO upload helper that writes only the firmware-owned Flash regions.

### 1. Remove Eurorack power

1. Switch the Eurorack system off.
2. **Unplug CLOCK's Eurorack ribbon cable.**
3. Only then connect a **data-capable** USB cable to the Black Pill.

A charge-only cable will power the board but cannot perform a firmware update.

### 2. Enter STM32 DFU mode

With USB connected:

1. Hold **BOOT0**.
2. Press and release **NRST / RESET** while BOOT0 remains held.
3. Wait briefly, then release BOOT0.

The STM32F401xB/C system-memory bootloader supports USB DFU. On Windows the device normally appears as the STM32 bootloader/DFU device (`0483:DF11`). ST documents the F401 USB DFU bootloader in AN2606:

- https://www.st.com/resource/en/application_note/an2606-stm32-microcontroller-system-memory-boot-mode-stmicroelectronics.pdf

### 3. Upload from PlatformIO

From the CLOCK source directory:

```bash
pio run -e release_default -t upload
```

For another release variant, substitute the matching environment name.

CLOCK's custom upload helper deliberately does **not** flash one contiguous binary. It:

1. reads existing persistence when required for migration;
2. writes the application region beginning at `0x0800C000`;
3. writes sector 0/vector table at `0x08000000` last;
4. leaves Flash sectors 1 and 2 untouched;
5. asks the ROM bootloader to leave DFU mode from the vector-table address.

If CLOCK does not restart automatically after a successful upload, make sure BOOT0 is released and press RESET once.

### 4. Test directly from USB

With the Eurorack ribbon cable still disconnected, CLOCK can boot from the Black Pill's USB supply. This is useful for a first functional check after an update:

- OLED boot and orientation;
- encoder and push switch;
- PLAY, TAP, and STOP/BACK buttons;
- menus and settings;
- BEATKNECHT/arcade UI if required.

The Eurorack gate-output stage is not a substitute for a rack-level electrical test while the module is USB-only powered.

### 5. Return the module to the rack

1. Disconnect the USB cable.
2. Reconnect the Eurorack ribbon cable, verifying the red stripe at `-12 V`.
3. Only then power the Eurorack system again.

## Do not flash a flat `firmware.bin`

CLOCK reserves STM32F401 Flash sectors 1 and 2 for power-loss-tolerant A/B persistence:

```text
0x08000000  sector 0   firmware / vector table
0x08004000  sector 1   persistence A
0x08008000  sector 2   persistence B
0x0800C000  sectors 3-5 firmware application region
```

A conventional contiguous binary can span the reserved gap and overwrite saved state. Use one of the supported address-aware paths:

- the project PlatformIO USB upload command;
- the release DfuSe `.dfu` image with a DfuSe-aware tool;
- the generated `firmware.elf` through ST-LINK/SWD.

## Troubleshooting

### USB update is not detected

- Confirm that the USB cable supports data.
- Repeat the BOOT0/RESET sequence.
- Check that the host sees the STM32 DFU device before running the PlatformIO upload command.
- On Windows, installing the current STM32CubeProgrammer package also installs/supports the ST DFU tooling used for STM32 devices.
- On Linux, follow PlatformIO's current `99-platformio-udev.rules` guidance.

### ST-LINK cannot connect

- Check target power and common GND.
- Check VTref, SWDIO, and SWCLK.
- Connect NRST for recovery if the target behaves unexpectedly.
- Reduce SWD speed in STM32CubeProgrammer if wiring is long or temporary.

### Settings or presets disappeared

The normal CLOCK update paths are designed to preserve them. If they are gone, check whether a **Full chip erase** or a flat contiguous binary was used.

### Firmware still does not boot after an update

Use ST-LINK/SWD to program the address-aware ELF and debug the target. Do not repeatedly erase the whole device merely to retry an update; that destroys persistence without providing better diagnostic information.

<h6 align="center">From Munich with &#9829;</h6>
