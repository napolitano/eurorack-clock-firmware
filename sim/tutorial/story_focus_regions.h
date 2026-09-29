/**
 * @file story_focus_regions.h
 * @brief Canonical 128x64 OLED regions used by Storybook focus overlays.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <optional>
#include <string_view>

namespace clockfw::sim::tutorial {

struct StoryOledFocusRegion final {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

/** @brief Resolves a stable Storybook OLED focus alias into canonical 128x64 coordinates. */
std::optional<StoryOledFocusRegion> storyOledFocusRegion(std::string_view name);

}  // namespace clockfw::sim::tutorial
