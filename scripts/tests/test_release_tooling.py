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

    def test_native_test_inventory_gate_passes(self) -> None:
        result = subprocess.run(
            [PYTHON, str(ROOT / "scripts/check_test_inventory.py")],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertEqual(result.returncode, 0, result.stderr)
        self.assertIn("total=90", result.stdout)



class ReleaseNotesTests(unittest.TestCase):
    def test_extracts_only_requested_changelog_section(self) -> None:
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
            self.assertEqual(result.returncode, 0)
            text = output.read_text(encoding="utf-8")
            self.assertIn(f"## [{current_version()}]", text)
            self.assertNotIn("## [0.19.0-alpha.3]", text)

    def test_missing_changelog_version_fails(self) -> None:
        result = subprocess.run(
            [PYTHON, str(ROOT / "scripts/release_notes.py"), "99.99.99"],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("No CHANGELOG section", result.stderr + result.stdout)


class PackageReleaseTests(unittest.TestCase):
    def test_binary_packaging_requires_explicit_compliance_acknowledgement(self) -> None:
        result = subprocess.run(
            [PYTHON, str(ROOT / "scripts/package_release.py")],
            cwd=ROOT,
            text=True,
            capture_output=True,
            check=False,
        )
        self.assertNotEqual(result.returncode, 0)
        self.assertIn("Binary packaging is intentionally disabled", result.stderr + result.stdout)

    def test_packages_firmware_license_and_checksums(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            build = tmp_path / "build"
            dist = tmp_path / "dist"
            build.mkdir()
            firmware_bin = b"firmware-bin-test\x00\x01"
            firmware_elf = b"firmware-elf-test\x02\x03"
            (build / "firmware.bin").write_bytes(firmware_bin)
            (build / "firmware.elf").write_bytes(firmware_elf)

            result = subprocess.run(
                [
                    PYTHON,
                    str(ROOT / "scripts/package_release.py"),
                    "--acknowledge-lgpl-static-link",
                    "--build-dir",
                    str(build),
                    "--out-dir",
                    str(dist),
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertEqual(result.returncode, 0, result.stderr)

            stem = f"clock-v{current_version()}-stm32f401cc"
            bin_out = dist / f"{stem}.bin"
            elf_out = dist / f"{stem}.elf"
            license_out = dist / "LICENSE.md"
            notice_out = dist / "NOTICE.txt"
            third_party_notice_out = dist / "THIRD_PARTY_NOTICES.md"
            citation_out = dist / "CITATION.cff"
            codemeta_out = dist / "codemeta.json"
            third_party_dir_out = dist / "third_party"
            checksums = dist / "SHA256SUMS.txt"
            self.assertEqual(bin_out.read_bytes(), firmware_bin)
            self.assertEqual(elf_out.read_bytes(), firmware_elf)
            self.assertEqual(license_out.read_text(encoding="utf-8"), (ROOT / "LICENSE.md").read_text(encoding="utf-8"))
            self.assertEqual(notice_out.read_text(encoding="utf-8"), (ROOT / "NOTICE.txt").read_text(encoding="utf-8"))
            self.assertEqual(
                third_party_notice_out.read_text(encoding="utf-8"),
                (ROOT / "THIRD_PARTY_NOTICES.md").read_text(encoding="utf-8"),
            )
            self.assertEqual(citation_out.read_text(encoding="utf-8"), (ROOT / "CITATION.cff").read_text(encoding="utf-8"))
            self.assertEqual(codemeta_out.read_text(encoding="utf-8"), (ROOT / "codemeta.json").read_text(encoding="utf-8"))
            for source in sorted((ROOT / "third_party").iterdir()):
                if source.is_file():
                    self.assertEqual((third_party_dir_out / source.name).read_bytes(), source.read_bytes())

            lines = checksums.read_text(encoding="utf-8").splitlines()
            expected = {
                bin_out.name: hashlib.sha256(firmware_bin).hexdigest(),
                elf_out.name: hashlib.sha256(firmware_elf).hexdigest(),
                license_out.name: hashlib.sha256(license_out.read_bytes()).hexdigest(),
                notice_out.name: hashlib.sha256(notice_out.read_bytes()).hexdigest(),
                third_party_notice_out.name: hashlib.sha256(third_party_notice_out.read_bytes()).hexdigest(),
                citation_out.name: hashlib.sha256(citation_out.read_bytes()).hexdigest(),
                codemeta_out.name: hashlib.sha256(codemeta_out.read_bytes()).hexdigest(),
            }
            expected.update({
                f"third_party/{path.name}": hashlib.sha256(path.read_bytes()).hexdigest()
                for path in sorted(third_party_dir_out.iterdir()) if path.is_file()
            })
            parsed = {line.split("  ", 1)[1]: line.split("  ", 1)[0] for line in lines}
            self.assertEqual(parsed, expected)

    def test_missing_build_artifact_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            build = Path(tmp) / "build"
            dist = Path(tmp) / "dist"
            build.mkdir()
            (build / "firmware.bin").write_bytes(b"only bin")
            result = subprocess.run(
                [
                    PYTHON,
                    str(ROOT / "scripts/package_release.py"),
                    "--acknowledge-lgpl-static-link",
                    "--build-dir",
                    str(build),
                    "--out-dir",
                    str(dist),
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Build artifact missing", result.stderr + result.stdout)

    def test_missing_license_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            build = tmp_path / "build"
            dist = tmp_path / "dist"
            build.mkdir()
            (build / "firmware.bin").write_bytes(b"bin")
            (build / "firmware.elf").write_bytes(b"elf")
            result = subprocess.run(
                [
                    PYTHON,
                    str(ROOT / "scripts/package_release.py"),
                    "--acknowledge-lgpl-static-link",
                    "--build-dir",
                    str(build),
                    "--out-dir",
                    str(dist),
                    "--license-file",
                    str(tmp_path / "missing-license.md"),
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("License file missing", result.stderr + result.stdout)


    def test_missing_required_notice_fails(self) -> None:
        with tempfile.TemporaryDirectory() as tmp:
            tmp_path = Path(tmp)
            build = tmp_path / "build"
            dist = tmp_path / "dist"
            build.mkdir()
            (build / "firmware.bin").write_bytes(b"bin")
            (build / "firmware.elf").write_bytes(b"elf")
            result = subprocess.run(
                [
                    PYTHON,
                    str(ROOT / "scripts/package_release.py"),
                    "--acknowledge-lgpl-static-link",
                    "--build-dir",
                    str(build),
                    "--out-dir",
                    str(dist),
                    "--notice-file",
                    str(tmp_path / "missing-notice.txt"),
                ],
                cwd=ROOT,
                text=True,
                capture_output=True,
                check=False,
            )
            self.assertNotEqual(result.returncode, 0)
            self.assertIn("Required notice file missing", result.stderr + result.stdout)


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
        self.assertIn("2,504 bytes", audit)
        self.assertIn("63 bytes", audit)
        self.assertIn("schema v7", audit)

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
        self.assertIn("STM32duino", third_party)
        self.assertIn("LGPL", third_party)
        self.assertIn("STM32CubeF4", third_party)
        self.assertIn("BSD-3-Clause", third_party)
        self.assertIn("CMSIS", third_party)
        self.assertIn("Apache", third_party)
        self.assertIn("Roboto Condensed Bold", third_party)
        for filename in (
            "LICENSE-Apache-2.0.txt",
            "LICENSE-BSD-3-Clause.txt",
            "LICENSE-LGPL-2.1.txt",
        ):
            self.assertTrue((ROOT / "third_party" / filename).is_file())


if __name__ == "__main__":
    unittest.main()
