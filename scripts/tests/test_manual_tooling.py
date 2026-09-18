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
REFRESH = load("refresh_manual_screenshots", ROOT / "scripts" / "refresh_manual_screenshots.py")


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

    def test_firmware_stamping_never_rewrites_license_versions(self) -> None:
        import zipfile

        with tempfile.TemporaryDirectory() as directory:
            directory_path = Path(directory)
            stable = directory_path / "clock-user-manual.1.0.0.odt"
            next_release = directory_path / "clock-user-manual.1.1.0.odt"

            PREPARE.stamp_odt(
                self.working,
                stable,
                "1.0.0",
                allow_font_substitution=True,
            )
            PREPARE.stamp_odt(
                stable,
                next_release,
                "1.1.0",
                allow_font_substitution=True,
            )

            with zipfile.ZipFile(next_release, "r") as archive:
                content = archive.read("content.xml").decode("utf-8")
                metadata = archive.read("meta.xml").decode("utf-8")

            self.assertIn("CLOCK 1.1.0", content)
            self.assertIn("firmware 1.1.0", content.lower())
            self.assertIn("PolyForm Noncommercial License 1.0.0", content)
            self.assertNotIn("PolyForm Noncommercial License 1.1.0", content)
            self.assertIn("PolyForm Noncommercial License 1.0.0", metadata)
            self.assertNotIn("PolyForm Noncommercial License 1.1.0", metadata)
            CHECK.validate_odt(next_release, "1.1.0")

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
        self.assertIn("scripts/generate_manual_screenshots.py", workflow)
        self.assertIn("scripts/prepare_release_manual.py", workflow)
        self.assertIn("scripts/refresh_manual_screenshots.py", workflow)
        self.assertIn("scripts/build_user_manual.py", workflow)
        self.assertIn("scripts/check_user_manual.py", workflow)
        self.assertIn('clock-user-manual.${VERSION}.odt', workflow)
        self.assertIn('clock-user-manual.${VERSION}.pdf', workflow)
        self.assertIn("--require-ubuntu-fonts", workflow)
        self.assertLess(
            workflow.index("scripts/generate_manual_screenshots.py"),
            workflow.index("scripts/prepare_release_manual.py"),
        )
        self.assertLess(
            workflow.index("scripts/prepare_release_manual.py"),
            workflow.index("scripts/refresh_manual_screenshots.py"),
        )
        self.assertLess(
            workflow.index("scripts/refresh_manual_screenshots.py"),
            workflow.index("scripts/build_user_manual.py"),
        )

    def test_ci_and_release_install_manual_tooling_dependencies_before_tests(self) -> None:
        for workflow_name in ("ci.yml", "release.yml"):
            workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text(encoding="utf-8")
            install = 'python -m pip install "platformio==6.1.19" pillow lxml'
            self.assertIn(install, workflow, workflow_name)
            self.assertIn("Run release-tooling tests", workflow, workflow_name)
            self.assertLess(
                workflow.index(install),
                workflow.index("Run release-tooling tests"),
                workflow_name,
            )

    def test_direct_manual_screenshot_contract_is_complete(self) -> None:
        targets = REFRESH.direct_frame_targets(
            self.working, ROOT / "docs" / "manual" / "assets"
        )
        self.assertEqual(len(targets), len(REFRESH.DIRECT_SCREENSHOTS))
        self.assertIn("ManualImage15", targets)
        self.assertEqual(REFRESH.DIRECT_SCREENSHOTS["ManualImage15"], "settings-sync.png")

    def test_refresh_replaces_direct_frames_and_galleries(self) -> None:
        import zipfile
        from lxml import etree

        with tempfile.TemporaryDirectory() as directory:
            output = Path(directory) / "manual.odt"
            direct_count, gallery_count = REFRESH.refresh(
                self.working, output, ROOT / "docs" / "manual" / "assets"
            )
            self.assertEqual(direct_count, len(REFRESH.DIRECT_SCREENSHOTS))
            self.assertEqual(gallery_count, 22)
            with zipfile.ZipFile(output, "r") as archive:
                root = etree.fromstring(archive.read("content.xml"))
                ns = REFRESH.NS
                frame = root.xpath(
                    '//draw:frame[@draw:name="ManualImage15"]/draw:image', namespaces=ns
                )[0]
                href = frame.get(f"{{{ns['xlink']}}}href")
                self.assertEqual(
                    archive.read(href),
                    (ROOT / "docs" / "manual" / "assets" / "settings-sync.png").read_bytes(),
                )
                self.assertEqual(
                    archive.read("Pictures/gallery_manualgalleryscreensavers_01_01.png"),
                    (ROOT / "docs" / "manual" / "assets" / "screensaver-clock.png").read_bytes(),
                )

    def test_manual_screenshot_generator_recovers_from_generator_mismatch(self) -> None:
        generator = load(
            "generate_manual_screenshots_test",
            ROOT / "scripts" / "generate_manual_screenshots.py",
        )
        with tempfile.TemporaryDirectory() as directory:
            build_dir = Path(directory) / "simulator-headless"
            build_dir.mkdir(parents=True)
            (build_dir / "CMakeCache.txt").write_text(
                "CMAKE_GENERATOR:INTERNAL=Unix Makefiles\n",
                encoding="utf-8",
            )
            (build_dir / "stale-artifact").write_text("stale", encoding="utf-8")
            generator.ensure_compatible_build_tree(build_dir)
            self.assertFalse(build_dir.exists())

    def test_ci_and_release_use_headless_cmake_preset_consistently(self) -> None:
        for workflow_name in ("ci.yml", "release.yml"):
            workflow = (ROOT / ".github" / "workflows" / workflow_name).read_text(encoding="utf-8")
            self.assertIn("cmake --preset simulator-headless", workflow, workflow_name)
            self.assertIn("cmake --build --preset simulator-headless", workflow, workflow_name)
            self.assertIn("ctest --preset simulator-headless", workflow, workflow_name)
            self.assertNotIn("cmake -S . -B build/simulator-headless", workflow, workflow_name)

    def test_manual_publication_smoke_workflow_exists(self) -> None:
        workflow = ROOT / ".github" / "workflows" / "manual-publication.yml"
        self.assertTrue(workflow.is_file())
        text = workflow.read_text(encoding="utf-8")
        self.assertIn("fonts-ubuntu", text)
        self.assertIn("libreoffice-writer", text)
        self.assertIn("clock-user-manual", text)
        self.assertIn("scripts/generate_manual_screenshots.py", text)
        self.assertIn("scripts/refresh_manual_screenshots.py", text)


if __name__ == "__main__":
    unittest.main()
