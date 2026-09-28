"""Repository contract tests for the host-only CLOCK Storybook foundation.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""

from __future__ import annotations

import json
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
        self.assertIn("add_executable(clock-storybook", cmake)
        self.assertIn("clock-storybook-example-stories-tests", cmake)
        self.assertNotIn("storybook", platformio.lower())
        self.assertNotIn("yaml-cpp", platformio.lower())
        self.assertTrue((ROOT / "docs/tutorials/stories/README.md").is_file())
        self.assertTrue((ROOT / "docs/tutorials/interaction_profiles.yaml").is_file())
        self.assertTrue((ROOT / "docs/tutorials/themes/south-signal-lab-default.yaml").is_file())
        self.assertTrue((ROOT / "docs/tutorials/assets/video/README.md").is_file())
        self.assertTrue((ROOT / "docs/tutorials/VIDEO_GENERATION.md").is_file())
        self.assertTrue((ROOT / "docs/tutorials/examples/README.md").is_file())
        self.assertTrue((ROOT / "docs/tutorials/voiceover/README.md").is_file())
        self.assertTrue((ROOT / "docs/tutorials/voiceover/segments.json").is_file())
        self.assertTrue((ROOT / "scripts/render_tutorials.py").is_file())
        self.assertTrue((ROOT / ".env.example").is_file())

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
        for token in (
            "/tutorial-output/", "docs/tutorials/generated/", "docs/tutorials/.cache/",
            "*.frames/", "*.staging/", "**/frames/frame-*.png", "**/frames/frame-*.rgba",
            "*.mp4", "*.webm", "*.srt", "*.vtt",
            "*.wav", "*.mp3", "*.m4a", "*.storybook-manifest.json", "*.storybook-probe.json",
        ):
            self.assertIn(token, gitignore)
        self.assertIn("!docs/tutorials/assets/video/south-signal-lab-intro.mp4", gitignore)
        self.assertIn("!docs/tutorials/assets/video/south-signal-lab-outro.mp4", gitignore)

    def test_local_narration_pipeline_keeps_secrets_and_generated_media_out_of_source(self) -> None:
        """Local ElevenLabs publication must be reproducible without committing secrets or rendered media."""
        gitignore = (ROOT / ".gitignore").read_text(encoding="utf-8")
        env_example = (ROOT / ".env.example").read_text(encoding="utf-8")
        runner = (ROOT / "scripts/render_tutorials.py").read_text(encoding="utf-8")
        guide = (ROOT / "docs/tutorials/VIDEO_GENERATION.md").read_text(encoding="utf-8")
        segments = json.loads((ROOT / "docs/tutorials/voiceover/segments.json").read_text(encoding="utf-8"))
        self.assertIn(".env", gitignore)
        self.assertIn(".env.*", gitignore)
        self.assertIn("!.env.example", gitignore)
        self.assertFalse((ROOT / ".env").exists(), "source tree must not ship a real .env")
        self.assertIn("ELEVENLABS_API_KEY=", env_example)
        self.assertIn("ELEVENLABS_VOICE_ID=", env_example)
        self.assertIn("ELEVENLABS_MODEL_ID=auto-v4", env_example)
        self.assertNotRegex(env_example, r"ELEVENLABS_API_KEY=\S+")
        self.assertIn("/v1/models", runner)
        self.assertIn("/with-timestamps", runner)
        self.assertIn("cache_key", runner)
        self.assertIn("SparseForPublication", (ROOT / "sim/tutorial/frame_pipeline.h").read_text(encoding="utf-8"))
        self.assertIn("tutorial-frames.ffconcat", (ROOT / "sim/tutorial/publication_pipeline.cpp").read_text(encoding="utf-8"))
        self.assertIn("python scripts/render_tutorials.py --all", guide)
        self.assertEqual(1, segments.get("schema"))
        self.assertEqual(11, len(segments.get("stories", {})))
        for stem, entry in segments["stories"].items():
            story = (ROOT / "docs/tutorials/examples" / f"{stem}.yaml").read_text(encoding="utf-8")
            ids = [str(item["id"]) for item in entry["segments"]]
            self.assertTrue(ids)
            self.assertEqual(len(ids), len(set(ids)))
            for narration_id in ids:
                self.assertRegex(story, rf'narration:\s+["\']?{narration_id}["\']?')
        self.assertIn("beat_end:", (ROOT / "docs/tutorials/examples/11-eight-independent-clocks-walkthrough.yaml").read_text(encoding="utf-8"))

    def test_publication_assets_and_scope_are_schema_contracts(self) -> None:
        """Schema 1 must retain optional media assets and visible-channel scope semantics."""
        schema = (ROOT / "docs/tutorials/STORY_SCHEMA_1.md").read_text(encoding="utf-8")
        self.assertIn('intro_video:', schema)
        self.assertIn('outro_video:', schema)
        self.assertIn('channel: visible', schema)
        self.assertIn('state: off', schema)
        self.assertIn('state: on', schema)
        self.assertIn('beat:', schema)
        self.assertIn('beat_end:', schema)
        self.assertIn('<story-id>.narration.json', schema)

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

    def test_teaching_examples_are_separate_and_complete(self) -> None:
        """Eleven editable examples must exist outside the canonical reference-story directory."""
        examples = ROOT / "docs/tutorials/examples"
        expected = {
            "01-power-and-first-clock.yaml", "02-transport-basics.yaml", "03-encoder-tempo.yaml",
            "04-tap-tempo.yaml", "05-topology-and-channel.yaml", "06-clock-mode.yaml",
            "07-euclidean-rhythm.yaml", "08-sequencer-basics.yaml", "09-divider-bank.yaml",
            "10-external-sync.yaml", "11-eight-independent-clocks-walkthrough.yaml",
        }
        self.assertEqual(expected, {path.name for path in examples.glob("*.yaml")})
        guide = (ROOT / "docs/tutorials/VIDEO_GENERATION.md").read_text(encoding="utf-8")
        self.assertIn("clock-storybook validate", guide)
        self.assertIn("clock-storybook video", guide)
        self.assertIn("--format both", guide)
        self.assertIn("intro_video:", guide)

    def test_examples_use_tracked_bumpers_and_voiceover_scripts(self) -> None:
        """Editable examples share the approved bumpers and narration contract."""
        examples = ROOT / "docs/tutorials/examples"
        voiceover = ROOT / "docs/tutorials/voiceover"
        intro = ROOT / "docs/tutorials/assets/video/south-signal-lab-intro.mp4"
        outro = ROOT / "docs/tutorials/assets/video/south-signal-lab-outro.mp4"
        self.assertTrue(intro.is_file() and intro.stat().st_size > 0)
        self.assertTrue(outro.is_file() and outro.stat().st_size > 0)
        stories = sorted(examples.glob("*.yaml"))
        scripts = sorted(voiceover.glob("*.txt"))
        self.assertEqual(11, len(stories))
        self.assertEqual(11, len(scripts))
        for story in stories:
            text = story.read_text(encoding="utf-8")
            self.assertIn('intro_video: ../assets/video/south-signal-lab-intro.mp4', text)
            self.assertIn('outro_video: ../assets/video/south-signal-lab-outro.mp4', text)
            self.assertIn('title: Thanks for watching', text)
            self.assertIn('duration_ms: 18000', text)
            self.assertIn('duration_ms: 20000', text)
        for script in scripts:
            text = script.read_text(encoding="utf-8")
            self.assertIn("Hi, this is South Signal Lab. My name is Axel, and today", text)
            self.assertIn("Ko-fi", text)
        detailed = (examples / "11-eight-independent-clocks-walkthrough.yaml").read_text(encoding="utf-8")
        self.assertIn("hold_ms: 800", detailed)
        self.assertIn('label: MODE · TIMING · CLOCK · OUTPUT', detailed)
        self.assertIn('label: Probability · Gate · Phase · Reset · Mute', detailed)
        self.assertIn('text: Change channel 4 from ×1 to ×2.', detailed)

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


    def test_publication_theme_tracks_manual_visual_contract(self) -> None:
        """Publication Storybook defaults must follow the CLOCK manual identity and slower tutorial pacing."""
        theme = (ROOT / "docs/tutorials/themes/south-signal-lab-default.yaml").read_text(encoding="utf-8")
        ci_theme = (ROOT / "docs/tutorials/themes/south-signal-lab-ci.yaml").read_text(encoding="utf-8")
        profiles = (ROOT / "docs/tutorials/interaction_profiles.yaml").read_text(encoding="utf-8")
        self.assertIn('family: "Ubuntu"', theme)
        self.assertIn('family: "Ubuntu Mono"', theme)
        self.assertIn('chapter_background: "#0B4FC0"', theme)
        self.assertIn('tutorial_background: "#000000"', theme)
        self.assertIn('background_image_mode: cover', theme)
        self.assertIn('family: "CLOCK UI"', ci_theme)
        self.assertIn('HUMAN_NORMAL:', profiles)
        self.assertIn('encoder_detent_ms: 320', profiles)
        self.assertIn('after_major_screen_change_ms: 2000', profiles)
        self.assertIn('HUMAN_FAST:', profiles)
        self.assertIn('patch_action_ms: 300', profiles)


    def test_focus_arrows_are_collision_aware_and_high_contrast(self) -> None:
        """Tutorial callouts must not depend on a white line drawn through the highlighted target."""
        source = (ROOT / "sim/tutorial/tutorial_focus_renderer.cpp").read_text(encoding="utf-8")
        schema = (ROOT / "docs/tutorials/STORY_SCHEMA_1.md").read_text(encoding="utf-8")
        self.assertIn("drawContrastLeader", source)
        self.assertIn("theme.accent, 4", source)
        self.assertIn("kArrowTargetGap", source)
        self.assertIn("chooseArrowSide", source)
        self.assertIn("placement: right", schema)
        self.assertIn("auto` (default), `left`, `right`, `above`, or `below", schema)

    def test_video_render_uses_run_scoped_frame_workspace(self) -> None:
        """Video rerenders must not replace a fixed frame tree that Windows may still have locked."""
        source = (ROOT / "sim/storybook_cli.cpp").read_text(encoding="utf-8")
        guide = (ROOT / "docs/tutorials/VIDEO_GENERATION.md").read_text(encoding="utf-8")
        self.assertIn("runScopedFramesDirectory", source)
        self.assertIn(".frames.run-", source)
        self.assertIn('options.command == "frames" ? output : runScopedFramesDirectory(output)', source)
        self.assertIn("<output>.frames.run-<token>", guide)

    def test_windows_host_process_macros_are_idempotent(self) -> None:
        """Windows SDK compatibility macros must not trigger MinGW/MSVC redefinition warnings."""
        source = (ROOT / "sim/tutorial/external_process.cpp").read_text(encoding="utf-8")
        self.assertIn("#ifndef WIN32_LEAN_AND_MEAN\n#define WIN32_LEAN_AND_MEAN\n#endif", source)
        self.assertIn("#ifndef NOMINMAX\n#define NOMINMAX\n#endif", source)

    def test_windows_system_font_widths_use_checked_win32_conversion(self) -> None:
        """GDI LONG dimensions must not leak into int renderer arithmetic under MinGW -Werror."""
        source = (ROOT / "sim/tutorial/story_text_renderer.cpp").read_text(encoding="utf-8")
        self.assertIn("int winLongToInt(const LONG value", source)
        self.assertIn("paddedPositiveDimension(extent.cx, 4", source)
        self.assertNotIn("std::max(1, extent.cx + 4)", source)
        self.assertNotIn("return size.cx;", source)
        self.assertNotIn("return extent.cx;", source)

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
        self.assertIn("publicationNarrationCues", publication)
        self.assertIn("writeSparseFrameConcat", publication)
        self.assertNotIn("SimulatorRuntime", publication)
        self.assertNotIn("StoryRunner", publication)
        self.assertIn("normalized intro duration", architecture)
        self.assertFalse(any((ROOT / "src").rglob("*publication_pipeline*")))


if __name__ == "__main__":
    unittest.main()
