/**
 * @file story_narration_beat.cpp
 * @brief Implements narration-first beat lifetime and resolved-audio budget accounting.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_narration_beat.h"

namespace clockfw::sim::tutorial {
namespace {
constexpr std::uint64_t kUsPerMs = 1000ULL;
}

bool StoryNarrationBeat::begin(
    const StoryAction& action,
    const std::uint64_t presentationUs,
    std::string& error) {
    if (!action.narrationId) {
        error = "beat requires narration id";
        return false;
    }
    if (id_) {
        error = "nested narration beats are not supported";
        return false;
    }
    id_ = *action.narrationId;
    startUs_ = presentationUs;
    budgetUs_ = static_cast<std::uint64_t>(action.durationMs) * kUsPerMs;
    focus_.reset();
    return true;
}

bool StoryNarrationBeat::finish(
    const StoryAction& action,
    const std::uint64_t presentationUs,
    StoryNarrationBeatFinish& result,
    std::string& error) {
    if (!action.narrationId || !id_ || *action.narrationId != *id_) {
        error = "beat_end must match the currently open narration beat";
        return false;
    }
    const std::uint64_t elapsedUs = presentationUs - startUs_;
    if (elapsedUs > budgetUs_) {
        error = "narration beat actions exceed resolved audio budget for " + *id_;
        return false;
    }
    result.id = *id_;
    result.remainingUs = budgetUs_ - elapsedUs;
    result.focusToClear = focus_;
    id_.reset();
    focus_.reset();
    startUs_ = 0ULL;
    budgetUs_ = 0ULL;
    return true;
}

bool StoryNarrationBeat::active() const {
    return id_.has_value();
}

void StoryNarrationBeat::retainFocus(const FocusTarget target) {
    focus_ = target;
}

}  // namespace clockfw::sim::tutorial
