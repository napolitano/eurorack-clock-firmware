/**
 * @file story_focus_regions.cpp
 * @brief Canonical focus regions derived from the production CLOCK UI renderers.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_focus_regions.h"

#include <array>
#include <utility>

namespace clockfw::sim::tutorial {
namespace {
using Entry = std::pair<std::string_view, StoryOledFocusRegion>;

// These rectangles use the same canonical 128x64 coordinate system as the
// production OLED framebuffer. They intentionally follow renderer geometry,
// not tutorial-composition pixels, so tutorial overlays remain stable when the
// 1080p composition changes.
constexpr std::array<Entry, 23U> kRegions{{
    {"oled_full", {0, 0, 128, 64}},
    {"oled_top_bar", {0, 0, 128, 10}},
    {"oled_tempo", {16, 13, 96, 31}},
    {"oled_mode_carousel", {0, 10, 128, 46}},
    {"oled_channel_grid", {2, 14, 124, 46}},
    {"oled_settings_rows", {0, 10, 126, 51}},
    {"oled_settings_top3", {0, 10, 126, 30}},
    {"oled_settings_top4", {0, 10, 126, 40}},
    {"oled_settings_bottom2", {0, 40, 126, 20}},
    {"oled_settings_row_1", {0, 10, 124, 10}},
    {"oled_settings_row_2", {0, 20, 124, 10}},
    {"oled_settings_row_3", {0, 30, 124, 10}},
    {"oled_settings_row_4", {0, 40, 124, 10}},
    {"oled_settings_row_5", {0, 50, 124, 10}},
    {"oled_divider_grid", {0, 38, 128, 26}},
    {"oled_pattern_strip", {0, 51, 128, 13}},
    {"oled_groove_grid", {0, 10, 128, 41}},
    {"oled_groove_status", {0, 54, 101, 10}},
    {"oled_groove_zoom", {101, 54, 27, 10}},
    {"oled_groove_slots", {0, 10, 126, 52}},
    {"oled_groove_name", {0, 10, 128, 44}},
    {"oled_groove_confirm", {0, 14, 128, 40}},
    {"oled_sequencer_editor", {0, 10, 128, 44}},
}};
}  // namespace

std::optional<StoryOledFocusRegion> storyOledFocusRegion(const std::string_view name) {
    for (const auto& [entryName, region] : kRegions) {
        if (entryName == name) return region;
    }
    return std::nullopt;
}

}  // namespace clockfw::sim::tutorial
