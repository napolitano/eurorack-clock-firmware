"""Tests for CLOCK user-manual versioning and release-publication contracts.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import importlib.util
import tempfile
import unittest
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
SCRIPTS = ROOT / "scripts"
if str(SCRIPTS) not in sys.path:
    sys.path.insert(0, str(SCRIPTS))


def load(name: str, path: Path):
    spec = importlib.util.spec_from_file_location(name, path)
    if spec is None or spec.loader is None:
        raise AssertionError(f"Unable to load {path}")
    module = importlib.util.module_from_spec(spec)
    spec.loader.exec_module(module)
    return module


PREPARE = load("prepare_release_manual", ROOT / "scripts" / "prepare_release_manual.py")
CHECK = load("check_user_manual", ROOT / "scripts" / "check_user_manual.py")
BUILD = load("build_user_manual", ROOT / "scripts" / "build_user_manual.py")


class ManualToolingTests(unittest.TestCase):
    def setUp(self) -> None:
        self.version = PREPARE.current_version()
        self.working = ROOT / "docs" / "manual" / "clock-user-manual.odt"
        self.frozen = (
            ROOT / "docs" / "manual" / "releases" / self.version /
            f"clock-user-manual.{self.version}.odt"
        )

    def test_working_manual_exists(self) -> None:
        self.assertTrue(self.working.is_file())

    def test_existing_frozen_manuals_match_their_version_scope(self) -> None:
        releases = ROOT / "docs" / "manual" / "releases"
        frozen_manuals = sorted(releases.glob("*/clock-user-manual.*.odt"))
        self.assertTrue(frozen_manuals)
        for manual in frozen_manuals:
            version = manual.parent.name
            self.assertEqual(manual.name, f"clock-user-manual.{version}.odt")
            CHECK.validate_odt(manual, version)

    def test_prepare_output_path_is_version_scoped(self) -> None:
        expected = (
            ROOT / "docs" / "manual" / "releases" / self.version /
            f"clock-user-manual.{self.version}.odt"
        )
        self.assertEqual(PREPARE.default_output(self.version), expected)
        self.assertEqual(BUILD.default_source(self.version), expected)

    def test_release_stage_labels_follow_semver_prerelease_phase(self) -> None:
        self.assertEqual(PREPARE.release_stage_label("0.19.0-alpha.63"), "Prototype")
        self.assertEqual(PREPARE.release_stage_label("0.19.0-beta.1"), "Beta")
        self.assertEqual(PREPARE.release_stage_label("1.0.0-rc.2"), "Release candidate")
        self.assertEqual(PREPARE.release_stage_label("1.0.0"), "Release")

    def test_stamping_rewrites_manual_version_contract(self) -> None:
        with tempfile.TemporaryDirectory() as directory:
            target_version = "0.19.0-alpha.99"
            output = Path(directory) / f"clock-user-manual.{target_version}.odt"
            PREPARE.stamp_odt(
                self.working,
                output,
                target_version,
                allow_font_substitution=True,
            )
            CHECK.validate_odt(output, target_version)

    def test_pdf_font_check_accepts_libreoffice_subset_without_light_suffix(self) -> None:
        fonts = (
            "name                                 type              encoding         emb sub uni object ID\n"
            "------------------------------------ ----------------- ---------------- --- --- --- ---------\n"
            "ABCDEE+Ubuntu                        TrueType          WinAnsi          yes yes yes     12  0\n"
        )
        CHECK.validate_ubuntu_pdf_fonts(fonts)

    def test_pdf_font_check_rejects_missing_ubuntu_family(self) -> None:
        fonts = (
            "name                                 type              encoding         emb sub uni object ID\n"
            "ABCDEE+NotoSans                      TrueType          WinAnsi          yes yes yes     12  0\n"
        )
        with self.assertRaisesRegex(RuntimeError, "Ubuntu-family"):
            CHECK.validate_ubuntu_pdf_fonts(fonts)

    def test_pdf_font_check_rejects_unembedded_ubuntu(self) -> None:
        fonts = (
            "name                                 type              encoding         emb sub uni object ID\n"
            "Ubuntu                               TrueType          WinAnsi           no  no yes     12  0\n"
        )
        with self.assertRaisesRegex(RuntimeError, "embedded/subset"):
            CHECK.validate_ubuntu_pdf_fonts(fonts)

    def test_release_workflow_publishes_odt_and_pdf(self) -> None:
        workflow = (ROOT / ".github" / "workflows" / "release.yml").read_text(encoding="utf-8")
        self.assertIn("scripts/build_user_manual.py", workflow)
        self.assertIn("scripts/check_user_manual.py", workflow)
        self.assertIn('clock-user-manual.${VERSION}.odt', workflow)
        self.assertIn('clock-user-manual.${VERSION}.pdf', workflow)
        self.assertIn("--require-ubuntu-fonts", workflow)

    def test_manual_publication_smoke_workflow_exists(self) -> None:
        workflow = ROOT / ".github" / "workflows" / "manual-publication.yml"
        self.assertTrue(workflow.is_file())
        text = workflow.read_text(encoding="utf-8")
        self.assertIn("fonts-ubuntu", text)
        self.assertIn("libreoffice-writer", text)
        self.assertIn("clock-user-manual", text)


if __name__ == "__main__":
    unittest.main()
