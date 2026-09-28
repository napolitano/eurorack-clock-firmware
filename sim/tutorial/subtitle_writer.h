/**
 * @file subtitle_writer.h
 * @brief Subtitle timeline extraction plus deterministic SRT/WebVTT output for CLOCK Storybook.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "tutorial/story_runner.h"

namespace clockfw::sim::tutorial {

/** @brief One timed subtitle interval derived from Story Runner subtitle/scene events. */
struct SubtitleCue {
    std::uint64_t startUs = 0ULL;
    std::uint64_t endUs = 0ULL;
    std::string text;
};

/** @brief Extracts subtitle intervals from the same Story Runner trace that drove burned-in text. */
std::vector<SubtitleCue> subtitleCuesFromTrace(const StoryRunResult& run);

/** @brief Serializes one cue list as UTF-8 SRT. */
std::string renderSrt(const std::vector<SubtitleCue>& cues);

/** @brief Serializes one cue list as UTF-8 WebVTT. */
std::string renderWebVtt(const std::vector<SubtitleCue>& cues);

/** @brief Writes SRT and WebVTT sidecars next to generated frames. */
void writeSubtitleSidecars(
    const std::filesystem::path& srtPath,
    const std::filesystem::path& vttPath,
    const std::vector<SubtitleCue>& cues);

}  // namespace clockfw::sim::tutorial
