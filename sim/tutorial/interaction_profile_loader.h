/**
 * @file interaction_profile_loader.h
 * @brief Loads deterministic CLOCK Storybook human-interaction timing profiles from docs resources.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <filesystem>
#include <optional>
#include <vector>

#include "tutorial/story_model.h"

namespace clockfw::sim::tutorial {

/** @brief Result of resolving one named human-interaction timing profile. */
struct InteractionTimingResult {
    std::optional<InteractionTiming> timing;
    std::vector<StoryIssue> issues;

    /** @brief Returns true only when a complete profile was loaded without issues. */
    explicit operator bool() const;
};

/** @brief Loads one schema-1 timing profile from docs/tutorials/interaction_profiles.yaml. */
InteractionTimingResult loadInteractionTiming(
    const std::filesystem::path& tutorialRoot,
    InteractionProfile profile);

}  // namespace clockfw::sim::tutorial
