"""Tests for release/version/package tooling.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import hashlib
import json
import importlib.util
import re
import runpy
import subprocess
import sys
import tempfile
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
PYTHON = sys.executable
VERSION_RE = re.compile(r'^#define\s+CLOCK_FIRMWARE_VERSION\s+"([^"]+)"\s*$', re.MULTILINE)


def current_version() -> str:
    """Return the firmware version declared by the repository under test."""
    text = (ROOT / "src/version.h").read_text(encoding="utf-8")
    match = VERSION_RE.search(text)
    if match is None:
        raise AssertionError("CLOCK_FIRMWARE_VERSION is missing from src/version.h")
    return match.group(1)


def load_memory_module():
    """Load the standalone memory-budget helper without requiring package installation."""
    path = ROOT / "scripts/check_memory_budget.py"
    spec = importlib.util.spec_from_file_location("check_memory_budget", path)
    if spec is None or spec.loader is None:
        raise AssertionError("Unable to load check_memory_budget.py")
    module = importlib.util.module_from_spec(spec)
    sys.modules[spec.name] = module
    spec.loader.exec_module(module)
    return module


class VersionToolTests(unittest.TestCase):
    def run_version(self, *args: str) -> subprocess.CompletedProcess[str]:
        return subprocess.run(
            [PYTHON, str(ROOT / "scripts/version.py"), *args],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )

    def test_prints_current_version(self) -> None:
        result = self.run_version()
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stdout.strip(), current_version())

    def test_accepts_exact_release_tag(self) -> None:
        result = self.run_version("--check-tag", f"v{current_version()}")
        self.assertEqual(result.returncode, 0)

    def test_rejects_mismatched_release_tag(self) -> None:
        result = self.run_version("--check-tag", "v0.18.0")
        self.assertEqual(result.returncode, 2)
        self.assertIn("Version mismatch", result.stderr)

    def test_missing_version_define_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            header = Path(tmp) / "version.h"
            header.write_text("#pragma once\n", encoding="utf-8")
            result = self.run_version("--file", str(header))
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Required version define not found", result.stderr + result.stdout)


class ProjectMetadataTests(unittest.TestCase):
    def test_citation_metadata_matches_release_identity(self) -> None:
        citation = (ROOT / "CITATION.cff").read_text(encoding="utf-8")
        self.assertIn('cff-version: 1.2.0', citation)
        self.assertIn('title: "South Signal Lab CLOCK"', citation)
        self.assertIn(f'version: {current_version()}', citation)
        self.assertIn('license: PolyForm-Noncommercial-1.0.0', citation)
        self.assertIn('given-names: Axel', citation)
        self.assertIn('family-names: Napolitano', citation)

    def test_codemeta_matches_release_identity(self) -> None:
        metadata = json.loads((ROOT / "codemeta.json").read_text(encoding="utf-8"))
        self.assertEqual(metadata["@context"], "https://w3id.org/codemeta/3.0")
        self.assertEqual(metadata["@type"], "SoftwareSourceCode")
        self.assertEqual(metadata["name"], "South Signal Lab CLOCK")
        self.assertEqual(metadata["softwareVersion"], current_version())
        self.assertEqual(metadata["author"]["name"], "Axel Napolitano")
        self.assertEqual(metadata["publisher"]["name"], "South Signal Lab")
        self.assertEqual(
            metadata["license"],
            "https://spdx.org/licenses/PolyForm-Noncommercial-1.0.0.html",
        )

    def test_no_unminted_doi_placeholder_is_committed(self) -> None:
        citation = (ROOT / "CITATION.cff").read_text(encoding="utf-8")
        self.assertNotRegex(citation, r"(?m)^doi\s*:")
        metadata = json.loads((ROOT / "codemeta.json").read_text(encoding="utf-8"))
        self.assertNotIn("identifier", metadata)

    def test_repository_funding_exposes_github_and_patreon(self) -> None:
        funding = (ROOT / ".github" / "FUNDING.yml").read_text(encoding="utf-8")
        self.assertRegex(funding, r"(?m)^github:\s+napolitano\s*$")
        self.assertRegex(funding, r"(?m)^patreon:\s+southsignallab\s*$")

    def test_doxygen_output_is_clean_checkout_safe(self) -> None:
        doxyfile = (ROOT / "Doxyfile").read_text(encoding="utf-8")
        self.assertRegex(doxyfile, r"(?m)^OUTPUT_DIRECTORY\s*=\s*build\s*$")
        self.assertRegex(doxyfile, r"(?m)^HTML_OUTPUT\s*=\s*doxygen\s*$")

    def test_architecture_allows_framework_fakes_in_any_test_suite(self) -> None:
        module = runpy.run_path(str(ROOT / "scripts" / "check_architecture.py"))
        allows = module["is_framework_include_allowed_path"]
        self.assertTrue(allows(ROOT / "test/test_host_firmware/test_main.cpp"))
        self.assertTrue(allows(ROOT / "test/host_firmware/test_main.cpp"))
        self.assertFalse(allows(ROOT / "src/services/external_sync_controller.cpp"))


    def test_embedded_platformio_forces_gcc7_cpp17_mode(self) -> None:
        platformio = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        self.assertIn("platformio/toolchain-gccarmnoneeabi@1.70201.0", platformio)
        self.assertIn("-std=gnu++1z", platformio)
        base = re.search(r"\[env:blackpill_f401cc\](.*?)(?=\n\[|\Z)", platformio, re.S).group(1)
        build_flags = re.search(r"(?ms)^build_flags\s*=\s*\n(.*?)(?=^\S|\Z)", base).group(1)
        self.assertNotIn("-std=gnu++17", build_flags)

    def test_stm32_encoder_uses_tim4_hardware_quadrature(self) -> None:
        platform = (ROOT / "src/hal/platform_io.cpp").read_text(encoding="utf-8")
        controls = (ROOT / "src/hal/control_panel.cpp").read_text(encoding="utf-8")
        self.assertIn("TIM4", platform)
        self.assertIn("GPIO_AF2_TIM4", platform)
        self.assertIn("TIM_ENCODERMODE_TI12", platform)
        self.assertIn("HAL_TIM_Encoder_Start", platform)
        self.assertIn("phaseA != mcu::PB6 || phaseB != mcu::PB7", platform)
        self.assertIn("beginQuadratureEncoder", controls)
        self.assertIn("quadratureEncoderCount", controls)
        self.assertIn("quadratureEncoderState", controls)
        self.assertIn("encoderDetentPhase_", controls)
        self.assertIn("atDetentPhase", controls)
        self.assertNotIn("encoderInterruptThunk", controls)

    def test_oled_180_degree_mode_rotates_transfer_bytes_not_scan_commands(self) -> None:
        display = (ROOT / "src/hal/oled_display.cpp").read_text(encoding="utf-8")
        self.assertIn("rotation180_ = rotated", display)
        self.assertIn("sendCommand(0xA1U)", display)
        self.assertIn("sendCommand(0xC8U)", display)
        self.assertNotIn("rotated ? 0xA0U", display)
        self.assertNotIn("rotated ? 0xC0U", display)
        self.assertIn("destinationPage = kPageCount - 1U - page", display)
        self.assertIn("kPageWidth - 1U - x", display)
        self.assertIn("reverseBits(source[sourceIndex])", display)

    def test_native_test_inventory_gate_passes(self) -> None:
        result = subprocess.run(
            [PYTHON, str(ROOT / "scripts/check_test_inventory.py")],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("test_clock_core=44", result.stdout)
        self.assertIn("test_realtime=24", result.stdout)
        self.assertIn("test_sync_behavior=85", result.stdout)
        self.assertIn("test_swing=22", result.stdout)
        self.assertIn("test_humanize=12", result.stdout)
        self.assertIn("test_tap_tempo=24", result.stdout)
        self.assertIn("test_controls=51", result.stdout)
        self.assertIn("test_settings=56", result.stdout)
        self.assertIn("test_screensavers=19", result.stdout)
        self.assertIn("test_easter_eggs=36", result.stdout)
        self.assertIn("test_host_firmware=28", result.stdout)
        self.assertIn("total=401", result.stdout)



class ReleaseNotesTests(unittest.TestCase):
    def test_combines_user_summary_with_requested_changelog_section(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "notes.md"
            result = subprocess.run(
                [
                    PYTHON,
                    str(ROOT / "scripts/release_notes.py"),
                    current_version(),
                    "--output",
                    str(output),
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            text = output.read_text(encoding="utf-8")
            self.assertIn(f"# CLOCK {current_version()}", text)
            self.assertIn("## Highlights", text)
            self.assertIn("## Detailed changelog", text)
            self.assertIn(f"## [{current_version()}]", text)
            self.assertNotIn("## [0.19.0-alpha.3]", text)

    def test_missing_changelog_version_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            summary = Path(tmp) / "summary.md"
            summary.write_text("# Missing version test\n", encoding="utf-8")
            result = subprocess.run(
                [
                    PYTHON,
                    str(ROOT / "scripts/release_notes.py"),
                    "99.99.99",
                    "--summary",
                    str(summary),
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("No CHANGELOG section", result.stderr + result.stdout)


class PackageReleaseTests(unittest.TestCase):
    @staticmethod
    def release_module() -> dict[str, object]:
        scripts = str(ROOT / "scripts")
        sys.path.insert(0, scripts)
        try:
            return runpy.run_path(str(ROOT / "scripts/package_release.py"))
        finally:
            sys.path.remove(scripts)

    def test_binary_packaging_is_enabled_after_stm32cube_migration(self) -> None:
        script = (ROOT / "scripts/package_release.py").read_text(encoding="utf-8")
        self.assertNotIn("acknowledge-lgpl-static-link", script)
        self.assertNotIn("Binary packaging is intentionally disabled", script)

    def test_public_firmware_filename_contract(self) -> None:
        module = self.release_module()
        version = current_version()
        self.assertEqual(
            module["firmware_filename"](version, "default"),
            f"eurorack-clock-firmware-default-{version}.dfu",
        )
        self.assertEqual(
            module["firmware_filename"](version, "formula-1"),
            f"eurorack-clock-firmware-formula-1-{version}.dfu",
        )

    def test_linker_map_parser_reports_runtime_archives(self) -> None:
        module = self.release_module()
        text = (
            "/toolchain/lib/libc_nano.a(lib_a-memcpy.o) symbol\n"
            "/toolchain/lib/gcc/arm-none-eabi/7.2.1/libgcc.a(_udivsi3.o) symbol\n"
            "/toolchain/lib/libc_nano.a(lib_a-memset.o) symbol\n"
        )
        archives = module["linked_archives"](text)
        self.assertEqual(
            archives,
            [
                "/toolchain/lib/gcc/arm-none-eabi/7.2.1/libgcc.a",
                "/toolchain/lib/libc_nano.a",
            ],
        )

    def test_release_process_documents_complete_legal_payload(self) -> None:
        text = (ROOT / "docs" / "RELEASE_PROCESS.md").read_text(encoding="utf-8")
        self.assertIn("eurorack-clock-firmware-default-<version>.dfu", text)
        self.assertIn("GNU-ARM-EMBEDDED-7.2.1-LICENSES.zip", text)
        self.assertIn("BUILD-INFO.txt", text)
        self.assertIn("SHA256SUMS.txt", text)
        self.assertIn("MD5SUMS.txt", text)

    def test_dfuse_round_trip_preserves_sparse_flash_elements(self) -> None:
        module = self.release_module()
        upload = runpy.run_path(str(ROOT / "scripts/upload_preserving_persistence.py"))
        boot_address = upload["BOOT_ADDRESS"]
        app_address = upload["APP_ADDRESS"]
        boot = b"BOOT" + bytes(range(32))
        app = b"APP" + bytes(range(64))
        image = module["build_dfuse"]([(boot_address, boot), (app_address, app)])
        self.assertEqual(module["parse_dfuse"](image), [(boot_address, boot), (app_address, app)])
        # The persistence sectors are represented by no image element at all.
        addresses = [address for address, _ in module["parse_dfuse"](image)]
        self.assertNotIn(upload["PERSIST_A_ADDRESS"], addresses)
        self.assertNotIn(upload["PERSIST_B_ADDRESS"], addresses)

    def test_firmware_cli_writes_default_and_variant_names(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            dist = tmp_path / "dist"
            boot = tmp_path / "boot.bin"
            app = tmp_path / "app.bin"
            boot.write_bytes(b"B" * 128)
            app.write_bytes(b"A" * 256)
            for variant in ("default", "egg-journey"):
                result = subprocess.run(
                    [
                        PYTHON,
                        str(ROOT / "scripts/package_release.py"),
                        "firmware",
                        "--out-dir",
                        str(dist),
                        "--boot-bin",
                        str(boot),
                        "--app-bin",
                        str(app),
                        "--variant",
                        variant,
                    ],
                    cwd=ROOT,
                    text=True,
                    capture_output=True,
                    check=False,
                )
                self.assertEqual(result.returncode, 0, result.stderr)
                expected = dist / f"eurorack-clock-firmware-{variant}-{current_version()}.dfu"
                self.assertTrue(expected.is_file())
                elements = self.release_module()["parse_dfuse"](expected.read_bytes())
                self.assertEqual(elements[0][1], boot.read_bytes())
                self.assertEqual(elements[1][1], app.read_bytes())

    def test_finalize_packages_manual_licenses_provenance_and_checksums(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            dist = tmp_path / "dist"
            dist.mkdir()
            for flavor in ("default", "pixel-raid", "formula-1", "breakout", "egg-journey"):
                (dist / f"eurorack-clock-firmware-{flavor}-{current_version()}.dfu").write_bytes(flavor.encode())
            (dist / "GNU-ARM-EMBEDDED-7.2.1-LICENSES.zip").write_bytes(b"licenses")
            (dist / "BUILD-INFO.txt").write_text("build info\n", encoding="utf-8")
            odt = tmp_path / f"clock-user-manual.{current_version()}.odt"
            pdf = tmp_path / f"clock-user-manual.{current_version()}.pdf"
            changelog = tmp_path / "CHANGELOG.md"
            summary = tmp_path / "RELEASE_SUMMARY.md"
            notes = tmp_path / "RELEASE_NOTES.md"
            odt.write_bytes(b"odt-test")
            pdf.write_bytes(b"pdf-test")
            changelog.write_text("# Changelog\n", encoding="utf-8")
            summary.write_text("# Summary\n", encoding="utf-8")
            notes.write_text("# Notes\n", encoding="utf-8")
            result = subprocess.run(
                [
                    PYTHON, str(ROOT / "scripts/package_release.py"), "finalize",
                    "--out-dir", str(dist),
                    "--manual-odt", str(odt),
                    "--manual-pdf", str(pdf),
                    "--changelog", str(changelog),
                    "--summary", str(summary),
                    "--release-notes", str(notes),
                ],
                cwd=ROOT, text=True, capture_output=True, check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            for name in (
                "LICENSE.md", "NOTICE.txt", "THIRD_PARTY_NOTICES.md", "MANUAL-LICENSE.md",
                "LICENSE-Apache-2.0.txt", "LICENSE-BSD-3-Clause.txt", "LICENSE-SDL-zlib.txt",
                "SHA256SUMS.txt", "MD5SUMS.txt",
            ):
                self.assertTrue((dist / name).is_file(), name)
            sha_names = {line.split("  ", 1)[1] for line in (dist / "SHA256SUMS.txt").read_text().splitlines()}
            md5_names = {line.split("  ", 1)[1] for line in (dist / "MD5SUMS.txt").read_text().splitlines()}
            self.assertEqual(sha_names, md5_names)
            self.assertIn(f"eurorack-clock-firmware-default-{current_version()}.dfu", sha_names)
            self.assertIn(odt.name, sha_names)
            self.assertIn(pdf.name, sha_names)
            self.assertIn("GNU-ARM-EMBEDDED-7.2.1-LICENSES.zip", sha_names)
            self.assertIn("BUILD-INFO.txt", sha_names)

    def test_finalize_rejects_release_without_complete_firmware_matrix(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            for name in ("manual.odt", "manual.pdf", "CHANGELOG.md", "RELEASE_SUMMARY.md", "RELEASE_NOTES.md"):
                (tmp_path / name).write_bytes(b"x")
            result = subprocess.run(
                [
                    PYTHON, str(ROOT / "scripts/package_release.py"), "finalize",
                    "--out-dir", str(tmp_path / "dist"),
                    "--manual-odt", str(tmp_path / "manual.odt"),
                    "--manual-pdf", str(tmp_path / "manual.pdf"),
                    "--changelog", str(tmp_path / "CHANGELOG.md"),
                    "--summary", str(tmp_path / "RELEASE_SUMMARY.md"),
                    "--release-notes", str(tmp_path / "RELEASE_NOTES.md"),
                ],
                cwd=ROOT, text=True, capture_output=True, check=False,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Required firmware images missing", result.stderr + result.stdout)

    def test_release_workflow_declares_exact_firmware_matrix_and_excludes_simulator(self) -> None:
        workflow = (ROOT / ".github/workflows/release.yml").read_text(encoding="utf-8")
        for environment in (
            "release_default|default",
            "release_pixel_raid|pixel-raid",
            "release_formula_1|formula-1",
            "release_breakout|breakout",
            "release_egg_journey|egg-journey",
        ):
            self.assertIn(environment, workflow)
        self.assertIn("Simulator binaries are deliberately not release assets", workflow)
        self.assertIn("dist/vcv/*", workflow)
        self.assertNotRegex(workflow, r'ASSETS=\([^)]*simulator')
        self.assertIn("GNU-ARM-EMBEDDED-7.2.1-LICENSES.zip", workflow)
        self.assertIn("BUILD-INFO.txt", workflow)
        self.assertIn("MD5SUMS.txt", workflow)
        self.assertIn("eurorack-clock-firmware-default-${VERSION}.dfu", workflow)

    def test_release_summary_is_complete_and_scaffold_requires_editing(self) -> None:
        summary = ROOT / "docs" / "releases" / current_version() / "RELEASE_SUMMARY.md"
        result = subprocess.run(
            [PYTHON, str(ROOT / "scripts/check_release_summary.py"), str(summary), "--version", current_version()],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        with tempfile.TemporaryDirectory() as tmp:
            output = Path(tmp) / "scaffold.md"
            result = subprocess.run(
                [
                    PYTHON,
                    str(ROOT / "scripts/prepare_release_summary.py"),
                    "--version",
                    current_version(),
                    "--output",
                    str(output),
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)
            check = subprocess.run(
                [PYTHON, str(ROOT / "scripts/check_release_summary.py"), str(output), "--version", current_version()],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(check.returncode, 0)
            self.assertIn("unfinished placeholder", check.stderr + check.stdout)


class PlatformioMemoryGateTests(unittest.TestCase):
    def test_post_action_accepts_scons_keyword_arguments(self) -> None:
        class FakeBoard:
            @staticmethod
            def get(key: str) -> int:
                return {
                    "upload.maximum_size": 229376,
                    "upload.maximum_ram_size": 65536,
                }[key]

        class FakeEnv:
            def __init__(self) -> None:
                self.callback = None

            @staticmethod
            def IsIntegrationDump() -> bool:
                return False

            @staticmethod
            def subst(value: str) -> str:
                if value == "$PROJECT_DIR":
                    return str(ROOT)
                if value == "$SIZETOOL":
                    return "arm-none-eabi-size"
                return value

            @staticmethod
            def BoardConfig() -> FakeBoard:
                return FakeBoard()

            @staticmethod
            def GetProjectOption(name: str, default: str) -> str:
                self = name  # keep signature close to SCons without using state
                del self
                return default

            def AddPostAction(self, target: str, callback) -> None:
                self.callback = callback

        fake_env = FakeEnv()
        namespace = runpy.run_path(
            str(ROOT / "scripts/platformio_memory_gate.py"),
            init_globals={"Import": lambda _: None, "env": fake_env},
        )
        self.assertIsNotNone(fake_env.callback)
        captured: dict[str, object] = {}

        def fake_inspect(size_tool, elf_path, flash_capacity, ram_capacity, maximum_percent):
            captured.update(
                size_tool=size_tool,
                elf_path=elf_path,
                flash_capacity=flash_capacity,
                ram_capacity=ram_capacity,
                maximum_percent=maximum_percent,
            )

        callback = fake_env.callback
        callback.__globals__["inspect_elf"] = fake_inspect
        callback(target=[ROOT / "dummy.elf"], source=[], env=fake_env)
        self.assertEqual(captured["flash_capacity"], 229376)
        self.assertEqual(captured["ram_capacity"], 65536)
        self.assertEqual(captured["maximum_percent"], 90.0)


class MemoryBudgetTests(unittest.TestCase):
    def setUp(self) -> None:
        self.memory = load_memory_module()

    def test_parses_gnu_size_and_calculates_flash_ram(self) -> None:
        usage = self.memory.parse_size_output(
            "   text    data     bss     dec     hex filename\n"
            "  90000    1000    8000   99000   182b8 firmware.elf\n"
        )
        self.assertEqual(usage.flash_bytes, 91000)
        self.assertEqual(usage.ram_bytes, 9000)
        self.assertEqual(self.memory.budget_bytes(229376, 90.0), 206438)
        self.assertEqual(self.memory.budget_bytes(65536, 90.0), 58982)

    def test_rejects_invalid_size_and_budget_inputs(self) -> None:
        with self.assertRaises(ValueError):
            self.memory.parse_size_output("not size output")
        with self.assertRaises(ValueError):
            self.memory.budget_bytes(0, 90.0)
        with self.assertRaises(ValueError):
            self.memory.budget_bytes(1024, 0.0)
        with self.assertRaises(ValueError):
            self.memory.budget_bytes(1024, 101.0)

    def test_enforces_both_flash_and_ram_limits(self) -> None:
        safe = self.memory.MemoryUsage(180000, 1000, 8000)
        self.memory.enforce_memory_budget(safe, 229376, 65536, 90.0)
        report = self.memory.format_report(safe, 229376, 65536, 90.0)
        self.assertIn("Flash", report)
        self.assertIn("RAM", report)

        flash_over = self.memory.MemoryUsage(206500, 1000, 0)
        with self.assertRaisesRegex(RuntimeError, "Flash"):
            self.memory.enforce_memory_budget(flash_over, 229376, 65536, 90.0)

        ram_over = self.memory.MemoryUsage(1000, 1000, 58000)
        with self.assertRaisesRegex(RuntimeError, "RAM"):
            self.memory.enforce_memory_budget(ram_over, 229376, 65536, 90.0)

        both_over = self.memory.MemoryUsage(207000, 1000, 59000)
        with self.assertRaisesRegex(RuntimeError, "Flash.*RAM"):
            self.memory.enforce_memory_budget(both_over, 229376, 65536, 90.0)


class RepositoryPolicyTests(unittest.TestCase):
    def test_architecture_policy_passes_for_repository(self) -> None:
        result = subprocess.run(
            [PYTHON, str(ROOT / "scripts/check_architecture.py")],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)

    def test_v1_scope_and_forward_compatibility_contracts_are_published(self) -> None:
        readme = (ROOT / "README.md").read_text(encoding="utf-8")
        roadmap = (ROOT / "docs" / "ROADMAP.md").read_text(encoding="utf-8")
        audit = (ROOT / "docs" / "V1_FORWARD_COMPATIBILITY.md").read_text(encoding="utf-8")
        self.assertIn("Why no general CV modulation?", readme)
        self.assertIn("digital timing, gate, and trigger", roadmap)
        self.assertIn("0.19.0-beta.1", roadmap)
        self.assertIn("2,522 bytes", audit)
        self.assertIn("61 bytes", audit)
        self.assertIn("schema v8", audit)

    def test_hil_qualification_contract_is_staged_from_1_5(self) -> None:
        ci = (ROOT / ".github" / "workflows" / "ci.yml").read_text(encoding="utf-8")
        release = (ROOT / ".github" / "workflows" / "release.yml").read_text(encoding="utf-8")
        acceptance = (ROOT / "docs" / "qualification" / "V1_ACCEPTANCE.md").read_text(encoding="utf-8")
        bench = (ROOT / "docs" / "qualification" / "V1_BENCH_MATRIX.md").read_text(encoding="utf-8")
        ledger = (ROOT / "docs" / "qualification" / "v1_qualification.json").read_text(encoding="utf-8")
        gate = "python scripts/check_hil_qualification.py --require-pass-from 1.5.0"
        self.assertIn(gate, ci)
        self.assertIn(gate, release)
        self.assertIn("50 µs", acceptance)
        self.assertIn("advisory through 1.4.x", acceptance)
        self.assertIn("1.5.0", acceptance)
        self.assertIn("1,000 events", acceptance)
        self.assertIn("QP-07", bench)
        self.assertIn('"overall_status": "IN_PROGRESS"', ledger)

    def test_polyform_license_is_present_and_identified(self) -> None:
        license_text = (ROOT / "LICENSE.md").read_text(encoding="utf-8")
        readme = (ROOT / "README.md").read_text(encoding="utf-8")
        notice = (ROOT / "NOTICE.txt").read_text(encoding="utf-8")
        required_notice = "Required Notice: Copyright © 2026 Axel Napolitano."
        self.assertIn("PolyForm Noncommercial License 1.0.0", license_text)
        self.assertIn("https://polyformproject.org/licenses/noncommercial/1.0.0", license_text)
        self.assertTrue(license_text.startswith(required_notice))
        self.assertTrue(notice.startswith(required_notice))
        self.assertIn("PolyForm-Noncommercial-1.0.0", readme)
        third_party = (ROOT / "THIRD_PARTY_NOTICES.md").read_text(encoding="utf-8")
        self.assertNotIn("STM32duino / Arduino Core", third_party)
        self.assertNotIn("## STM32duino / Arduino Core", third_party)
        self.assertIn("framework = stm32cube", third_party)
        self.assertIn("STM32CubeF4", third_party)
        self.assertIn("BSD-3-Clause", third_party)
        self.assertIn("CMSIS", third_party)
        self.assertIn("Apache", third_party)
        self.assertIn("Roboto Condensed Bold", third_party)
        for filename in (
            "LICENSE-Apache-2.0.txt",
            "LICENSE-BSD-3-Clause.txt",
        ):
            self.assertTrue((ROOT / "third_party" / filename).is_file())


if __name__ == "__main__":
    unittest.main()
