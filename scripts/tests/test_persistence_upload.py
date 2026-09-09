"""Tests for the persistence-preserving STM32 DFU upload helper.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import importlib.util
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
MODULE_PATH = ROOT / "scripts" / "upload_preserving_persistence.py"
SPEC = importlib.util.spec_from_file_location("upload_preserving_persistence", MODULE_PATH)
if SPEC is None or SPEC.loader is None:
    raise RuntimeError("Unable to load persistence upload helper")
UPLOAD = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(UPLOAD)


class PersistenceUploadTests(unittest.TestCase):
    def test_flash_regions_leave_exact_two_sector_gap(self) -> None:
        self.assertEqual(UPLOAD.BOOT_ADDRESS, 0x08000000)
        self.assertEqual(UPLOAD.BOOT_CAPACITY, 16 * 1024)
        self.assertEqual(UPLOAD.APP_ADDRESS, 0x0800C000)
        self.assertEqual(UPLOAD.APP_CAPACITY, 208 * 1024)
        self.assertEqual(
            UPLOAD.APP_ADDRESS - (UPLOAD.BOOT_ADDRESS + UPLOAD.BOOT_CAPACITY),
            32 * 1024,
        )
        self.assertEqual(UPLOAD.BOOT_CAPACITY + UPLOAD.APP_CAPACITY, 224 * 1024)

    def test_objcopy_selects_only_owned_flash_sections(self) -> None:
        command = UPLOAD.objcopy_command(
            "arm-none-eabi-objcopy",
            Path("firmware.elf"),
            Path("boot.bin"),
            UPLOAD.BOOT_SECTIONS,
        )
        self.assertIn(".isr_vector", command)
        self.assertIn(".flash_boot", command)
        self.assertNotIn(".text", command)

        app = UPLOAD.objcopy_command(
            "arm-none-eabi-objcopy",
            Path("firmware.elf"),
            Path("app.bin"),
            UPLOAD.APP_SECTIONS,
        )
        self.assertIn(".text", app)
        self.assertIn(".data", app)
        self.assertNotIn(".isr_vector", app)

    def test_dfu_commands_use_explicit_nonpersistent_addresses(self) -> None:
        tool = Path("dfu-util")
        boot = UPLOAD.dfu_command(tool, Path("boot.bin"), UPLOAD.BOOT_ADDRESS, False, "0483:df11")
        app = UPLOAD.dfu_command(tool, Path("app.bin"), UPLOAD.APP_ADDRESS, True, "0483:df11")
        self.assertIn("0x08000000", boot)
        self.assertIn("0x0800C000:leave", app)

    def test_region_size_guard_rejects_empty_and_oversized_images(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            path = Path(directory) / "image.bin"
            path.write_bytes(b"")
            with self.assertRaises(RuntimeError):
                UPLOAD.ensure_region_size(path, 16, "test")
            path.write_bytes(b"x" * 17)
            with self.assertRaises(RuntimeError):
                UPLOAD.ensure_region_size(path, 16, "test")
            path.write_bytes(b"x" * 16)
            UPLOAD.ensure_region_size(path, 16, "test")

    def test_read_command_uses_bounded_dfuse_range(self) -> None:
        command = UPLOAD.dfu_read_command(
            Path("dfu-util"), Path("readback.bin"), 0x08004000, 4128, "0483:df11")
        self.assertIn("0x08004000:4128", command)
        self.assertIn("-U", command)

    def test_slot_wrapper_matches_firmware_validation_contract(self) -> None:
        payload = b"CUR4" + (b"\xFF" * (UPLOAD.PERSIST_PAYLOAD_BYTES - 4))
        image = UPLOAD.build_slot_image(payload, generation=7)
        self.assertEqual(len(image), UPLOAD.PERSIST_HEADER_BYTES + UPLOAD.PERSIST_PAYLOAD_BYTES)
        self.assertTrue(UPLOAD.slot_image_is_valid(image))
        corrupted = bytearray(image)
        corrupted[-1] ^= 1
        self.assertFalse(UPLOAD.slot_image_is_valid(bytes(corrupted)))
        self.assertFalse(UPLOAD.slot_image_is_valid(image[:20]))
        with self.assertRaises(ValueError):
            UPLOAD.build_slot_image(b"short")

    def test_legacy_detection_rejects_blank_or_application_bytes(self) -> None:
        blank = b"\xFF" * UPLOAD.PERSIST_PAYLOAD_BYTES
        app = b"APP!" + (b"\x00" * (UPLOAD.PERSIST_PAYLOAD_BYTES - 4))
        current = b"CUR4" + (b"\xFF" * (UPLOAD.PERSIST_PAYLOAD_BYTES - 4))
        preset = bytearray(blank)
        preset[128:132] = b"PRE3"
        self.assertFalse(UPLOAD.legacy_payload_is_plausible(blank))
        self.assertFalse(UPLOAD.legacy_payload_is_plausible(app))
        self.assertTrue(UPLOAD.legacy_payload_is_plausible(current))
        self.assertTrue(UPLOAD.legacy_payload_is_plausible(bytes(preset)))
        self.assertFalse(UPLOAD.legacy_payload_is_plausible(b"short"))

    def test_finds_posix_and_windows_dfu_tools(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            package = Path(directory) / "tool-dfuutil" / "bin"
            package.mkdir(parents=True)
            posix = package / "dfu-util"
            posix.write_text("", encoding="utf-8")
            self.assertEqual(UPLOAD.find_dfu_tool(Path(directory), "dfu-util"), posix)
            posix.unlink()
            windows = package / "dfu-util.exe"
            windows.write_text("", encoding="utf-8")
            self.assertEqual(UPLOAD.find_dfu_tool(Path(directory), "dfu-util"), windows)
            windows.unlink()
            with self.assertRaises(FileNotFoundError):
                UPLOAD.find_dfu_tool(Path(directory), "dfu-util")

    def test_platformio_uses_custom_persistence_preserving_uploader(self) -> None:
        config = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        self.assertIn("upload_protocol = custom", config)
        self.assertIn("upload_preserving_persistence.py", config)
        self.assertIn("board_upload.maximum_size = 229376", config)
        self.assertIn("board_build.ldscript = ld/stm32f401cc_clock.ld", config)


if __name__ == "__main__":
    unittest.main()
