"""Repository contract tests for the host-only CLOCK Storybook foundation.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]


class StorybookToolingTests(unittest.TestCase):
    """Keep Storybook dependencies, generated output and semantic adapters in their intended layer."""

    def test_storybook_is_host_only(self) -> None:
        """Storybook targets belong to CMake and never to the embedded PlatformIO graph."""
        cmake = (ROOT / "CMakeLists.txt").read_text(encoding="utf-8")
        platformio = (ROOT / "platformio.ini").read_text(encoding="utf-8")
        self.assertIn("clock-storybook-contract", cmake)
        self.assertIn("clock-storybook-simulator-port", cmake)
        self.assertIn("clock-storybook-model", cmake)
        self.assertIn("clock-storybook-parser-tests", cmake)
        self.assertIn("clock-storybook-runner", cmake)
        self.assertIn("clock-storybook-runner-tests", cmake)
        self.assertIn("clock-storybook-reference-stories-tests", cmake)
        self.assertIn("storybook_reference_stories_tests", cmake)
        self.assertIn("clock-storybook-frame-pipeline", cmake)
        self.assertIn("clock-storybook-frame-pipeline-tests", cmake)
        self.assertIn("clock-storybook-publication", cmake)
        self.assertIn("clock-storybook-publication-pipeline-tests", cmake)
        self.assertNotIn("storybook", platformio.lower())
        self.assertNotIn("yaml-cpp", platformio.lower())
        self.assertTrue((ROOT / "docs/tutorials/stories/README.md").is_file())
        self.assertTrue((ROOT / "docs/tutorials/interaction_profiles.yaml").is_file())
        self.assertTrue((ROOT / "docs/tutorials/themes/south-signal-lab-default.yaml").is_file())
        self.assertTrue((ROOT / "docs/tutorials/assets/video/README.md").is_file())

    def test_architecture_guard_forbids_embedded_storybook_media_dependencies(self) -> None:
        """The architecture gate must reject reverse dependencies into embedded code."""
        checker = (ROOT / "scripts/check_architecture.py").read_text(encoding="utf-8")
        self.assertIn("check_storybook_dependency_boundary", checker)
        self.assertIn("yaml-cpp", checker)
        self.assertIn("ffmpeg", checker)
        self.assertIn("libavcodec", checker)

    def test_generated_storybook_media_is_ignored(self) -> None:
        """Generated tutorial/publication artifacts must never be source-bundle inputs."""
        gitignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
        self.assertIn("docs/tutorials/generated/", gitignore)
        self.assertIn("docs/tutorials/.cache/", gitignore)
        self.assertIn("*.storybook-manifest.json", gitignore)
        self.assertIn("*.storybook-probe.json", gitignore)

    def test_publication_assets_and_scope_are_schema_contracts(self) -> None:
        """Schema 1 must retain optional media assets and visible-channel scope semantics."""
        schema = (ROOT / "docs/tutorials/STORY_SCHEMA_1.md").read_text(encoding="utf-8")
        self.assertIn('intro_video:', schema)
        self.assertIn('outro_video:', schema)
        self.assertIn('channel: visible', schema)
        self.assertIn('state: off', schema)
        self.assertIn('state: on', schema)

    def test_story_port_never_sets_clock_source_directly(self) -> None:
        """The semantic adapter may observe SOURCE but must not assign it."""
        source = (ROOT / "sim/tutorial/story_simulator_port.cpp").read_text(encoding="utf-8")
        self.assertNotIn("state().source =", source)
        self.assertNotIn("setClockSource", source)
        self.assertNotIn("ClockSource::Auto;", source)

    def test_story_sources_live_under_docs_not_simulator_code(self) -> None:
        """Authored stories/themes belong under docs/tutorials; sim/tutorial is implementation only."""
        expected = {
            "getting-started.yaml", "play-stop.yaml", "changing-tempo.yaml", "tap-tempo.yaml",
            "selecting-operating-topology.yaml", "selecting-independent-channel.yaml", "clock-mode.yaml",
            "euclidean-mode.yaml", "sequencer-mode.yaml", "divider-bank.yaml",
            "saving-loading-preset.yaml", "external-sync.yaml", "external-rst.yaml",
        }
        actual = {path.name for path in (ROOT / "docs/tutorials/stories").glob("*.yaml")}
        self.assertEqual(expected, actual)
        self.assertFalse(any((ROOT / "sim/tutorial").rglob("*.yaml")))

    def test_phase1_acceptance_matrix_has_exactly_40_evidence_rows(self) -> None:
        """SB-8 closes the frozen 40-point Phase-1 implementation gate with repository evidence."""
        acceptance = (ROOT / "docs/tutorials/PHASE1_ACCEPTANCE.md").read_text(encoding="utf-8")
        rows = [line for line in acceptance.splitlines() if line.startswith("| ") and len(line) > 3 and line[2].isdigit()]
        self.assertEqual(40, len(rows))
        self.assertIn("storybook_reference_stories_tests", acceptance)
        self.assertIn("13 reference stories", acceptance)

    def test_reference_authoring_supports_real_modifier_and_long_push_gestures(self) -> None:
        """Reference stories must express existing physical chords without direct state mutation."""
        schema = (ROOT / "docs/tutorials/STORY_SCHEMA_1.md").read_text(encoding="utf-8")
        self.assertIn("button.state", schema)
        self.assertIn("encoder_push.hold_ms", schema)
        preset = (ROOT / "docs/tutorials/stories/saving-loading-preset.yaml").read_text(encoding="utf-8")
        self.assertIn("hold_ms: 800", preset)
        topology = (ROOT / "docs/tutorials/stories/selecting-operating-topology.yaml").read_text(encoding="utf-8")
        self.assertIn("state: down", topology)
        self.assertIn("state: up", topology)

    def test_sb3_runner_keeps_firmware_behaviour_behind_story_port(self) -> None:
        """The runner must orchestrate the strict port rather than mutate production state directly."""
        runner = (ROOT / "sim/tutorial/story_runner.cpp").read_text(encoding="utf-8")
        self.assertIn("StorySimulatorPort", runner)
        self.assertIn("wait_until timeout", runner)
        self.assertNotIn("state().source =", runner)
        self.assertNotIn("setClockSource", runner)
        self.assertNotIn("setTransport", runner)

    def test_sb6_output_pipeline_is_single_run_and_host_only(self) -> None:
        """Frame/subtitle output must observe one runner execution and stay outside embedded code."""
        pipeline = (ROOT / "sim/tutorial/frame_pipeline.cpp").read_text(encoding="utf-8")
        runner = (ROOT / "sim/tutorial/story_runner.cpp").read_text(encoding="utf-8")
        architecture = (ROOT / "docs/tutorials/STORYBOOK_ARCHITECTURE.md").read_text(encoding="utf-8")
        self.assertIn("StoryExecutionObserver", runner)
        self.assertIn("StoryRunner runner", pipeline)
        self.assertIn("writeTutorialPng", pipeline)
        self.assertIn("writeSubtitleSidecars", pipeline)
        self.assertIn("No second Story replay", architecture)
        self.assertFalse(any((ROOT / "src").rglob("*storybook*")))

    def test_sb7_publication_is_external_media_only(self) -> None:
        """Final video composition may invoke ffmpeg/ffprobe but must not feed media state back into CLOCK."""
        publication = (ROOT / "sim/tutorial/publication_pipeline.cpp").read_text(encoding="utf-8")
        header = (ROOT / "sim/tutorial/publication_pipeline.h").read_text(encoding="utf-8")
        architecture = (ROOT / "docs/tutorials/STORYBOOK_ARCHITECTURE.md").read_text(encoding="utf-8")
        self.assertIn("ffprobe", publication)
        self.assertIn("libx264", publication)
        self.assertIn("libvpx-vp9", publication)
        self.assertIn("libopus", publication)
        self.assertIn("StoryFramePipelineResult", header)
        self.assertNotIn("SimulatorRuntime", publication)
        self.assertNotIn("StoryRunner", publication)
        self.assertIn("normalized intro duration", architecture)
        self.assertFalse(any((ROOT / "src").rglob("*publication_pipeline*")))


if __name__ == "__main__":
    unittest.main()
