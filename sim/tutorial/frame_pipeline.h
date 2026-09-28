/**
 * @file frame_pipeline.h
 * @brief Deterministic Storybook frame scheduling, PNG output, subtitle sidecars, and generation manifest.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "panel_layout.h"
#include "tutorial/story_model.h"
#include "tutorial/story_runner.h"
#include "tutorial/story_simulator_port.h"
#include "tutorial/subtitle_writer.h"

namespace clockfw::sim::tutorial {

/** @brief Controls whether every logical frame is materialized or repeated frames share one PNG. */
enum class FrameStorageMode : std::uint8_t { Materialized, SparseForPublication };

/** @brief One lossless generated frame and its deterministic logical identity. */
struct StoryFrameRecord {
    std::size_t index = 0U;
    std::uint64_t presentationUs = 0ULL;
    std::filesystem::path relativePath;
    std::string rgbaFnv1a64;
};

/** @brief Complete output/result of one atomic Storybook frame-generation run. */
struct StoryFramePipelineResult {
    StoryRunResult run;
    std::vector<StoryFrameRecord> frames;
    std::vector<SubtitleCue> subtitles;
    std::vector<StoryIssue> issues;
    std::filesystem::path outputDirectory;
    std::filesystem::path srtPath;
    std::filesystem::path vttPath;
    std::filesystem::path manifestPath;
    std::filesystem::path narrationPath;

    /** @brief True only when the story and every generated artifact completed successfully. */
    explicit operator bool() const;
};

/**
 * @brief Executes one story once and samples the live deterministic runtime into lossless tutorial artifacts.
 */
class StoryFramePipeline final {
public:
    StoryFramePipeline(
        std::filesystem::path tutorialRoot,
        layout::PanelLayout panelLayout,
        std::string simulatorSourceRevision);

    /** @brief Generates frames/SRT/VTT/manifest atomically into outputDirectory. */
    StoryFramePipelineResult generate(
        const Story& story,
        StorySimulatorPort& port,
        const std::filesystem::path& outputDirectory,
        FrameStorageMode storageMode = FrameStorageMode::Materialized);

private:
    std::filesystem::path tutorialRoot_;
    layout::PanelLayout panelLayout_;
    std::string simulatorSourceRevision_;
};

}  // namespace clockfw::sim::tutorial
