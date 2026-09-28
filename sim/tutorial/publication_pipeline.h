/**
 * @file publication_pipeline.h
 * @brief FFmpeg-backed host-only CLOCK Storybook MP4/WebM publication and intro/outro composition.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <vector>

#include "tutorial/frame_pipeline.h"
#include "tutorial/story_contract.h"
#include "tutorial/story_model.h"

namespace clockfw::sim::tutorial {

/** @brief Media facts obtained from ffprobe before/after normalization. */
struct StoryMediaProbe {
    std::uint64_t durationUs = 0ULL;
    std::uint32_t width = 0U;
    std::uint32_t height = 0U;
    bool hasAudio = false;
};

/** @brief Successful or failed publication result for one generated Storybook tutorial. */
struct StoryPublicationResult {
    std::vector<StoryIssue> issues;
    std::filesystem::path outputDirectory;
    std::optional<std::filesystem::path> mp4Path;
    std::optional<std::filesystem::path> webmPath;
    std::filesystem::path srtPath;
    std::filesystem::path vttPath;
    std::filesystem::path manifestPath;
    std::filesystem::path narrationPath;
    std::uint64_t introDurationUs = 0ULL;
    std::uint64_t outroDurationUs = 0ULL;

    explicit operator bool() const { return issues.empty(); }
};

/**
 * @brief Publication-only compositor. It never executes or mutates CLOCK and depends only on completed SB-6 output.
 */
class StoryPublicationPipeline final {
public:
    StoryPublicationPipeline(std::filesystem::path ffmpeg, std::filesystem::path ffprobe);

    /** @brief Creates requested final media formats atomically from one successful frame-pipeline result. */
    StoryPublicationResult publish(
        const Story& story,
        const std::filesystem::path& storySourcePath,
        const StoryFramePipelineResult& generated,
        const std::filesystem::path& outputDirectory,
        const std::vector<PublicationFormat>& formats = {PublicationFormat::Mp4H264}) const;

private:
    std::filesystem::path ffmpeg_;
    std::filesystem::path ffprobe_;
};

}  // namespace clockfw::sim::tutorial
