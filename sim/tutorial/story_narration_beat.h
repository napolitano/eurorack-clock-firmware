/**
 * @file story_narration_beat.h
 * @brief Tracks one narration-first tutorial beat across multiple real Storybook actions.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <optional>
#include <string>

#include "tutorial/story_model.h"

namespace clockfw::sim::tutorial {

struct StoryNarrationBeatFinish final {
    std::string id;
    std::uint64_t remainingUs = 0ULL;
    std::optional<FocusTarget> focusToClear;
};

class StoryNarrationBeat final {
public:
    bool begin(const StoryAction& action, std::uint64_t presentationUs, std::string& error);
    bool finish(
        const StoryAction& action,
        std::uint64_t presentationUs,
        StoryNarrationBeatFinish& result,
        std::string& error);
    bool active() const;
    void retainFocus(FocusTarget target);

private:
    std::optional<std::string> id_;
    std::optional<FocusTarget> focus_;
    std::uint64_t startUs_ = 0ULL;
    std::uint64_t budgetUs_ = 0ULL;
};

}  // namespace clockfw::sim::tutorial
