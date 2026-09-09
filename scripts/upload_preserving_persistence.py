#!/usr/bin/env python3
"""Upload the split STM32F401 image without touching persistent Flash sectors.

The clock linker layout intentionally leaves sectors 1 and 2 out of the ELF:

    0x08000000..0x08003FFF  sector 0   boot/vector image
    0x08004000..0x08007FFF  sector 1   persistence A
    0x08008000..0x0800BFFF  sector 2   persistence B
    0x0800C000..0x0803FFFF  sectors 3-5 application image

A normal raw firmware.bin would fill the address gap and DFU would overwrite the
persistent sectors. This helper extracts and uploads the two load regions
separately so firmware updates preserve settings and presets.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import argparse
import struct
import subprocess
import sys
import zlib
from pathlib import Path
from typing import Sequence

BOOT_ADDRESS = 0x08000000
BOOT_CAPACITY = 16 * 1024
APP_ADDRESS = 0x0800C000
APP_CAPACITY = 208 * 1024
DEFAULT_USB_ID = "0483:df11"
PERSIST_A_ADDRESS = 0x08004000
PERSIST_B_ADDRESS = 0x08008000
LEGACY_EEPROM_ADDRESS = 0x08020000
PERSIST_PAYLOAD_BYTES = 4096
PERSIST_HEADER_BYTES = 32
PERSIST_SLOT_MAGIC = 0x434C4B50
PERSIST_FORMAT_VERSION = 1
PERSIST_COMMIT_MARKER = 0xC10C17ED

BOOT_SECTIONS = (".isr_vector", ".flash_boot")
APP_SECTIONS = (
    ".text",
    ".rodata",
    ".ARM.extab",
    ".ARM",
    ".preinit_array",
    ".init_array",
    ".fini_array",
    ".data",
)


def _section_args(sections: Sequence[str]) -> list[str]:
    args: list[str] = []
    for section in sections:
        args.extend(("--only-section", section))
    return args


def objcopy_command(objcopy: str, elf: Path, output: Path, sections: Sequence[str]) -> list[str]:
    """Build one deterministic objcopy command for a selected load region."""
    return [objcopy, "-O", "binary", *_section_args(sections), str(elf), str(output)]


def dfu_command(dfu_util: Path, image: Path, address: int, leave: bool, usb_id: str) -> list[str]:
    """Build one dfu-util download command for an explicit Flash address."""
    address_arg = f"0x{address:08X}" + (":leave" if leave else "")
    return [
        str(dfu_util),
        "-d", usb_id,
        "-a", "0",
        "-s", address_arg,
        "-D", str(image),
    ]


def dfu_read_command(
    dfu_util: Path,
    output: Path,
    address: int,
    size: int,
    usb_id: str,
) -> list[str]:
    """Build one DfuSe upload command that reads a bounded Flash range."""
    return [
        str(dfu_util),
        "-d", usb_id,
        "-a", "0",
        "-s", f"0x{address:08X}:{size}",
        "-U", str(output),
    ]


def build_slot_image(payload: bytes, generation: int = 1) -> bytes:
    """Wrap one legacy logical image in the firmware's committed A/B slot format."""
    if len(payload) != PERSIST_PAYLOAD_BYTES:
        raise ValueError(f"Expected {PERSIST_PAYLOAD_BYTES} persistence bytes")
    payload_crc = zlib.crc32(payload) & 0xFFFFFFFF
    first_five_words = struct.pack(
        "<IIIII",
        PERSIST_SLOT_MAGIC,
        PERSIST_FORMAT_VERSION,
        generation & 0xFFFFFFFF,
        PERSIST_PAYLOAD_BYTES,
        payload_crc,
    )
    header_crc = zlib.crc32(first_five_words) & 0xFFFFFFFF
    header = first_five_words + struct.pack(
        "<III",
        header_crc,
        0xFFFFFFFF,
        PERSIST_COMMIT_MARKER,
    )
    if len(header) != PERSIST_HEADER_BYTES:
        raise AssertionError("Unexpected persistence header size")
    return header + payload


def slot_image_is_valid(slot: bytes) -> bool:
    """Validate one header+payload readback using the firmware's exact slot contract."""
    if len(slot) < PERSIST_HEADER_BYTES + PERSIST_PAYLOAD_BYTES:
        return False
    words = struct.unpack("<IIIIIIII", slot[:PERSIST_HEADER_BYTES])
    magic, version, _generation, payload_size, payload_crc, header_crc, _reserved, commit = words
    if (
        magic != PERSIST_SLOT_MAGIC
        or version != PERSIST_FORMAT_VERSION
        or payload_size != PERSIST_PAYLOAD_BYTES
        or commit != PERSIST_COMMIT_MARKER
    ):
        return False
    if (zlib.crc32(slot[:20]) & 0xFFFFFFFF) != header_crc:
        return False
    payload = slot[PERSIST_HEADER_BYTES:PERSIST_HEADER_BYTES + PERSIST_PAYLOAD_BYTES]
    return (zlib.crc32(payload) & 0xFFFFFFFF) == payload_crc


def legacy_payload_is_plausible(payload: bytes) -> bool:
    """Recognize the pre-A/B logical image without pretending arbitrary Flash is state."""
    if len(payload) != PERSIST_PAYLOAD_BYTES or all(byte == 0xFF for byte in payload):
        return False
    record_magics = (b"CUR3", b"CUR4", b"PRE3", b"PRE4")
    return payload[:4] in record_magics or any(magic in payload for magic in record_magics[2:])


def find_dfu_tool(packages_dir: Path, executable: str) -> Path:
    """Resolve PlatformIO's bundled dfu-util/dfu-suffix on Windows or POSIX."""
    tool_dir = packages_dir / "tool-dfuutil" / "bin"
    candidates = (tool_dir / executable, tool_dir / f"{executable}.exe")
    for candidate in candidates:
        if candidate.is_file():
            return candidate
    raise FileNotFoundError(f"Unable to find {executable} below {tool_dir}")


def ensure_region_size(path: Path, capacity: int, label: str) -> None:
    """Reject a split image that would cross its physical Flash region."""
    size = path.stat().st_size
    if size == 0:
        raise RuntimeError(f"{label} image is empty: {path}")
    if size > capacity:
        raise RuntimeError(
            f"{label} image is {size} bytes but its region is only {capacity} bytes")


def run_checked(command: Sequence[str]) -> None:
    """Run one external tool and propagate a non-zero result."""
    subprocess.run(list(command), check=True)


def main() -> int:
    parser = argparse.ArgumentParser()
    parser.add_argument("--elf", required=True)
    parser.add_argument("--objcopy", required=True)
    parser.add_argument("--packages-dir", required=True)
    parser.add_argument("--usb-id", default=DEFAULT_USB_ID)
    args = parser.parse_args()

    elf = Path(args.elf).resolve()
    if not elf.is_file():
        raise SystemExit(f"ELF not found: {elf}")

    build_dir = elf.parent
    boot_bin = build_dir / "firmware-boot.bin"
    app_bin = build_dir / "firmware-app.bin"
    slot_a_readback = build_dir / ".persist-a-readback.bin"
    slot_b_readback = build_dir / ".persist-b-readback.bin"
    legacy_readback = build_dir / ".legacy-eeprom-readback.bin"
    migrated_slot = build_dir / ".persist-migration-slot.bin"

    try:
        dfu_util = find_dfu_tool(Path(args.packages_dir), "dfu-util")
        dfu_suffix = find_dfu_tool(Path(args.packages_dir), "dfu-suffix")

        run_checked(objcopy_command(args.objcopy, elf, boot_bin, BOOT_SECTIONS))
        run_checked(objcopy_command(args.objcopy, elf, app_bin, APP_SECTIONS))
        ensure_region_size(boot_bin, BOOT_CAPACITY, "boot")
        ensure_region_size(app_bin, APP_CAPACITY, "application")

        # Preserve upgrades from the old sector-5 STM32duino EEPROM layout.
        # Existing A/B state wins. Only when both new slots are invalid do we
        # read the old 4-KiB logical image and seed slot A before sector 5 is
        # reused by the new application image.
        slot_read_size = PERSIST_HEADER_BYTES + PERSIST_PAYLOAD_BYTES
        run_checked(dfu_read_command(
            dfu_util, slot_a_readback, PERSIST_A_ADDRESS, slot_read_size, args.usb_id))
        run_checked(dfu_read_command(
            dfu_util, slot_b_readback, PERSIST_B_ADDRESS, slot_read_size, args.usb_id))
        have_new_persistence = (
            slot_image_is_valid(slot_a_readback.read_bytes())
            or slot_image_is_valid(slot_b_readback.read_bytes())
        )
        if not have_new_persistence:
            run_checked(dfu_read_command(
                dfu_util,
                legacy_readback,
                LEGACY_EEPROM_ADDRESS,
                PERSIST_PAYLOAD_BYTES,
                args.usb_id,
            ))
            legacy_payload = legacy_readback.read_bytes()
            if legacy_payload_is_plausible(legacy_payload):
                migrated_slot.write_bytes(build_slot_image(legacy_payload))
                vid, pid = args.usb_id.split(":", maxsplit=1)
                run_checked([
                    str(dfu_suffix),
                    "-v", f"0x{vid}",
                    "-p", f"0x{pid}",
                    "-d", "0xffff",
                    "-a", str(migrated_slot),
                ])
                run_checked(dfu_command(
                    dfu_util,
                    migrated_slot,
                    PERSIST_A_ADDRESS,
                    False,
                    args.usb_id,
                ))

        # Match PlatformIO's STM32 DFU behavior: attach the ST VID/PID suffix to
        # each raw image before handing it to dfu-util.
        vid, pid = args.usb_id.split(":", maxsplit=1)
        for image in (boot_bin, app_bin):
            run_checked([
                str(dfu_suffix),
                "-v", f"0x{vid}",
                "-p", f"0x{pid}",
                "-d", "0xffff",
                "-a", str(image),
            ])

        run_checked(dfu_command(dfu_util, boot_bin, BOOT_ADDRESS, False, args.usb_id))
        run_checked(dfu_command(dfu_util, app_bin, APP_ADDRESS, True, args.usb_id))
    except (FileNotFoundError, RuntimeError, subprocess.CalledProcessError, ValueError) as exc:
        print(f"Persistence-preserving DFU upload failed: {exc}", file=sys.stderr)
        return 1
    finally:
        # Split images are upload intermediates. The ELF remains the canonical
        # sparse build artifact; do not leave confusing raw binaries around.
        for image in (
            boot_bin,
            app_bin,
            slot_a_readback,
            slot_b_readback,
            legacy_readback,
            migrated_slot,
        ):
            try:
                image.unlink()
            except FileNotFoundError:
                pass

    return 0


if __name__ == "__main__":
    raise SystemExit(main())
