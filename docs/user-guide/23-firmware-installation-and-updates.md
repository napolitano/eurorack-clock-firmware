<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 23 Firmware installation and updates

CLOCK supports two firmware-programming paths: **ST-LINK / SWD** for first installation, recovery and debugging, and **USB DFU via PlatformIO** for routine updates.

> [!CAUTION]
> **Before connecting USB, switch the Eurorack system off and preferably unplug CLOCK's Eurorack ribbon cable. Do not power CLOCK from USB and the Eurorack bus at the same time.** After a USB update, the module can be booted and its display/controls tested directly from USB while the Eurorack cable remains disconnected.

For a routine update, enter the STM32 system-memory DFU bootloader with BOOT0/RESET and run the matching release environment, for example:

```bash
pio run -e release_default -t upload
```

The project upload helper writes the application and vector-table regions separately and preserves Flash sectors 1 and 2, which hold settings, presets and arcade high scores. Do not replace the supported update path with a flat contiguous `firmware.bin`.

The complete step-by-step procedure, including ST-LINK wiring, STM32CubeProgrammer, alternate Easter-egg variants, PlatformIO installation links, USB-only post-update testing and troubleshooting, is maintained in [`FIRMWARE_UPDATE.md`](../FIRMWARE_UPDATE.md).

<h6 align="center">From Munich with &#9829;</h6>
