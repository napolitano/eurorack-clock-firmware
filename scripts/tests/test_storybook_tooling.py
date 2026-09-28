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
        self.assertNotIn("storybook", platformio.lower())
        self.assertNotIn("yaml-cpp", platformio.lower())
        self.assertTrue((ROOT / "docs/tutorials/stories/README.md").is_file())
        self.assertTrue((ROOT / "docs/tutorials/interaction_profiles.yaml").is_file())
        self.assertTrue((ROOT / "docs/tutorials/themes/south-signal-lab-default.yaml").is_file())

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
        self.assertTrue((ROOT / "docs/tutorials/stories/external-sync.yaml").is_file())
        self.assertFalse(any((ROOT / "sim/tutorial").rglob("*.yaml")))

    def test_sb3_runner_keeps_firmware_behaviour_behind_story_port(self) -> None:
        """The runner must orchestrate the strict port rather than mutate production state directly."""
        runner = (ROOT / "sim/tutorial/story_runner.cpp").read_text(encoding="utf-8")
        self.assertIn("StorySimulatorPort", runner)
        self.assertIn("wait_until timeout", runner)
        self.assertNotIn("state().source =", runner)
        self.assertNotIn("setClockSource", runner)
        self.assertNotIn("setTransport", runner)


if __name__ == "__main__":
    unittest.main()
