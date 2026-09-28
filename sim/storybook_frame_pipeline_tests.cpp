/**
 * @file storybook_frame_pipeline_tests.cpp
 * @brief Regression tests for deterministic Storybook frame scheduling, PNG output, sidecars and atomic failure.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "panel_layout.h"
#include "simulator_runtime.h"
#include "tutorial/frame_pipeline.h"
#include "tutorial/story_parser.h"
#include "tutorial/story_simulator_port.h"

namespace {

using clockfw::sim::SimulatorRuntime;
using clockfw::sim::layout::loadPanelLayout;
using clockfw::sim::tutorial::Story;
using clockfw::sim::tutorial::StoryFramePipeline;
using clockfw::sim::tutorial::StoryFramePipelineResult;
using clockfw::sim::tutorial::FrameStorageMode;
using clockfw::sim::tutorial::StoryParseResult;
using clockfw::sim::tutorial::StorySimulatorPort;
using clockfw::sim::tutorial::parseStoryText;

bool require(const bool condition, const char* const message) {
    if (!condition) {
        std::cerr << "Storybook frame-pipeline failure: " << message << '\n';
        return false;
    }
    return true;
}

std::string readText(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

std::vector<std::uint8_t> readBytes(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::vector<std::uint8_t>(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

bool validPngHeader(const std::filesystem::path& path, const std::uint32_t width, const std::uint32_t height) {
    const std::vector<std::uint8_t> bytes = readBytes(path);
    const std::array<std::uint8_t, 8U> signature{{137U, 80U, 78U, 71U, 13U, 10U, 26U, 10U}};
    if (bytes.size() < 24U) return false;
    for (std::size_t index = 0U; index < signature.size(); ++index) {
        if (bytes[index] != signature[index]) return false;
    }
    const auto be32 = [&](const std::size_t offset) {
        return (static_cast<std::uint32_t>(bytes[offset]) << 24U) |
               (static_cast<std::uint32_t>(bytes[offset + 1U]) << 16U) |
               (static_cast<std::uint32_t>(bytes[offset + 2U]) << 8U) |
               static_cast<std::uint32_t>(bytes[offset + 3U]);
    };
    return be32(16U) == width && be32(20U) == height;
}

Story parseOrExit(const std::string& yaml) {
    StoryParseResult parsed = parseStoryText(yaml);
    if (!parsed) {
        for (const auto& issue : parsed.issues) std::cerr << issue.reason << '\n';
        std::exit(2);
    }
    return *parsed.story;
}

StoryFramePipelineResult generate(
    const Story& story,
    const std::filesystem::path& tutorialRoot,
    const std::filesystem::path& output,
    const std::filesystem::path& statePath,
    const FrameStorageMode storageMode = FrameStorageMode::Materialized) {
    std::filesystem::remove(statePath);
    SimulatorRuntime runtime(statePath);
    runtime.begin();
    StorySimulatorPort port(runtime);
    StoryFramePipeline pipeline(
        tutorialRoot,
        loadPanelLayout(std::filesystem::path(CLOCK_SOURCE_ROOT) / "sim" / "panel_layout.ini"),
        "r48f-test");
    StoryFramePipelineResult result = pipeline.generate(story, port, output, storageMode);
    runtime.flushPersistence();
    std::filesystem::remove(statePath);
    return result;
}

}  // namespace

int main() {
    const std::filesystem::path root = CLOCK_SOURCE_ROOT;
    const std::filesystem::path tutorialRoot = root / "docs" / "tutorials";
    const std::filesystem::path temp = std::filesystem::temp_directory_path() / "clock-storybook-frame-pipeline-tests";
    std::filesystem::remove_all(temp);
    std::filesystem::create_directories(temp);

    const Story story = parseOrExit(R"YAML(
schema: 1
id: frame-pipeline-smoke
title: "Frame Pipeline Smoke"
language: en
theme: south-signal-lab-ci
interaction_profile: HUMAN_NORMAL
output:
  width: 1280
  height: 900
  fps: 10
setup:
  factory_reset: true
  power: on
scenes:
  - chapter:
      number: 1
      title: "Frame Pipeline"
      subtitle: "One deterministic execution"
      narration: "s01"
      duration_ms: 100
  - tutorial:
      subtitle: "Press PLAY."
      actions:
        - narration: "s02"
          wait_ms: 200
)YAML");

    const std::filesystem::path outA = temp / "run-a";
    const std::filesystem::path outB = temp / "run-b";
    const StoryFramePipelineResult a = generate(story, tutorialRoot, outA, temp / "state-a.bin");
    const StoryFramePipelineResult b = generate(story, tutorialRoot, outB, temp / "state-b.bin");

    bool ok = true;
    ok &= require(static_cast<bool>(a) && static_cast<bool>(b), "both deterministic generation runs must succeed");
    ok &= require(a.run.presentationDurationUs == 300000ULL, "story duration must be exactly 300 ms");
    ok &= require(a.frames.size() == 3U && b.frames.size() == 3U, "10-fps 300-ms story must produce exactly three frames");
    if (a.frames.size() == 3U && b.frames.size() == 3U) {
        ok &= require(a.frames[0].presentationUs == 0ULL && a.frames[1].presentationUs == 100000ULL &&
                      a.frames[2].presentationUs == 200000ULL,
                      "frame scheduler timestamps must use exact rational presentation times");
        for (std::size_t index = 0U; index < a.frames.size(); ++index) {
            ok &= require(a.frames[index].rgbaFnv1a64 == b.frames[index].rgbaFnv1a64,
                          "independent runs must produce identical raw-RGBA frame hashes");
            const auto bytesA = readBytes(outA / a.frames[index].relativePath);
            const auto bytesB = readBytes(outB / b.frames[index].relativePath);
            ok &= require(bytesA == bytesB, "independent runs must produce byte-identical deterministic PNG frames");
            ok &= require(validPngHeader(outA / a.frames[index].relativePath, 1280U, 900U),
                          "generated frame must be a correctly dimensioned PNG");
        }
    }

    ok &= require(a.subtitles.size() == 1U, "tutorial subtitle must generate exactly one sidecar cue");
    if (a.subtitles.size() == 1U) {
        ok &= require(a.subtitles[0].startUs == 100000ULL && a.subtitles[0].endUs == 300000ULL,
                      "subtitle cue must match the same tutorial scene timeline used by burned-in rendering");
        ok &= require(a.subtitles[0].text == "Press PLAY.", "subtitle cue text must come from Story content");
    }
    const std::string srtA = readText(a.srtPath);
    const std::string vttA = readText(a.vttPath);
    ok &= require(srtA.find("00:00:00,100 --> 00:00:00,300") != std::string::npos,
                  "SRT timing must match Story presentation time");
    ok &= require(vttA.find("00:00:00.100 --> 00:00:00.300") != std::string::npos,
                  "WebVTT timing must match Story presentation time");
    ok &= require(readText(a.srtPath) == readText(b.srtPath) && readText(a.vttPath) == readText(b.vttPath),
                  "subtitle sidecars must be byte-identical across deterministic runs");
    const std::string manifestA = readText(a.manifestPath);
    const std::string manifestB = readText(b.manifestPath);
    ok &= require(manifestA == manifestB, "generation manifests must be byte-identical across deterministic runs");
    ok &= require(manifestA.find("\"frame_count\": 3") != std::string::npos &&
                  manifestA.find("\"narration_cue_count\": 2") != std::string::npos &&
                  manifestA.find("\"simulator_source_revision\": \"r48f-test\"") != std::string::npos &&
                  manifestA.find("\"frame_hash_algorithm\": \"fnv1a64-rgba8\"") != std::string::npos,
                  "manifest must record deterministic output identity, narration and frame-hash contract");
    const std::string narrationA = readText(a.narrationPath);
    ok &= require(narrationA.find("\"id\": \"s01\"") != std::string::npos &&
                  narrationA.find("\"id\": \"s02\"") != std::string::npos,
                  "narration sidecar must preserve both scene and action timing anchors");

    Story staticStory = story;
    staticStory.id = "frame-pipeline-static-scenes";
    staticStory.scenes.resize(1U);
    staticStory.scenes[0].durationMs = 200U;
    clockfw::sim::tutorial::StoryScene nextScene;
    nextScene.kind = clockfw::sim::tutorial::SceneKind::Text;
    nextScene.title = "Next scene";
    nextScene.body = "A distinct image after the chapter.";
    nextScene.durationMs = 100U;
    staticStory.scenes.push_back(nextScene);
    const std::filesystem::path staticOutput = temp / "static-scenes";
    const StoryFramePipelineResult staticResult = generate(
        staticStory, tutorialRoot, staticOutput, temp / "state-static.bin");
    ok &= require(static_cast<bool>(staticResult) && staticResult.frames.size() == 3U,
                  "static scenes must generate every scheduled frame");
    if (staticResult.frames.size() == 3U) {
        const auto first = staticOutput / staticResult.frames[0].relativePath;
        const auto repeated = staticOutput / staticResult.frames[1].relativePath;
        const auto changed = staticOutput / staticResult.frames[2].relativePath;
        ok &= require(first != repeated && std::filesystem::exists(first) &&
                      std::filesystem::exists(repeated) && std::filesystem::exists(changed),
                      "reused frames must retain distinct numbered PNG paths");
        ok &= require(staticResult.frames[0].rgbaFnv1a64 == staticResult.frames[1].rgbaFnv1a64 &&
                      readBytes(first) == readBytes(repeated),
                      "frames within one static scene must have identical image bytes and hashes");
        const auto probe = staticOutput / "hard-link-probe.png";
        std::error_code linkError;
        std::filesystem::create_hard_link(first, probe, linkError);
        if (!linkError) {
            ok &= require(std::filesystem::equivalent(first, repeated),
                          "static frames must share the encoded PNG when hard links are supported");
            std::filesystem::remove(probe);
        }
        ok &= require(staticResult.frames[1].rgbaFnv1a64 != staticResult.frames[2].rgbaFnv1a64 &&
                      readBytes(repeated) != readBytes(changed),
                      "a new static scene must render a new image");
    }

    const std::filesystem::path sparseOutput = temp / "static-scenes-sparse";
    const StoryFramePipelineResult sparseResult = generate(
        staticStory, tutorialRoot, sparseOutput, temp / "state-sparse.bin", FrameStorageMode::SparseForPublication);
    ok &= require(static_cast<bool>(sparseResult) && sparseResult.frames.size() == 3U,
                  "sparse publication mode must preserve the logical frame timeline");
    if (sparseResult.frames.size() == 3U) {
        ok &= require(sparseResult.frames[0].relativePath == sparseResult.frames[1].relativePath &&
                      sparseResult.frames[1].relativePath != sparseResult.frames[2].relativePath,
                      "sparse publication mode must share one PNG across an unchanged frame run");
    }

    Story overflow = story;
    overflow.id = "frame-pipeline-overflow";
    overflow.scenes.clear();
    clockfw::sim::tutorial::StoryScene textScene;
    textScene.kind = clockfw::sim::tutorial::SceneKind::Text;
    textScene.title = "Overflow";
    textScene.body = std::string(600U, 'X');
    textScene.durationMs = 100U;
    overflow.scenes.push_back(textScene);
    const std::filesystem::path protectedOutput = temp / "protected-output";
    std::filesystem::create_directories(protectedOutput);
    std::ofstream(protectedOutput / "sentinel.txt") << "keep";
    const StoryFramePipelineResult failed = generate(overflow, tutorialRoot, protectedOutput, temp / "state-fail.bin");
    ok &= require(!static_cast<bool>(failed), "render overflow must fail the frame pipeline");
    ok &= require(std::filesystem::exists(protectedOutput / "sentinel.txt"),
                  "failed generation must not replace a previously successful output directory");
    ok &= require(!std::filesystem::exists(temp / "protected-output.staging"),
                  "failed generation must remove partial staging artifacts");

    std::filesystem::remove_all(temp);
    if (!ok) return 1;
    std::cout << "Storybook frame pipeline tests passed\n";
    return 0;
}
