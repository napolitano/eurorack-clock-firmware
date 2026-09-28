/**
 * @file publication_narration.h
 * @brief Publication-timeline narration cue extraction and JSON serialization for CLOCK Storybook.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <string>
#include <vector>

#include "tutorial/story_model.h"
#include "tutorial/story_runner.h"

namespace clockfw::sim::tutorial {

/** @brief One narration cue positioned on the final publication timeline. */
struct PublicationNarrationCue final {
    std::string id;
    std::uint64_t startUs = 0ULL;
    std::uint64_t endUs = 0ULL;
};

/** @brief Converts Story Runner narration trace events to publication-relative cues. */
std::vector<PublicationNarrationCue> publicationNarrationCues(
    const StoryRunResult& run,
    std::uint64_t introOffsetUs);

/** @brief Serializes publication narration cues as the local audio-mux sidecar. */
std::string publicationNarrationManifest(
    const Story& story,
    const std::vector<PublicationNarrationCue>& cues);

}  // namespace clockfw::sim::tutorial
