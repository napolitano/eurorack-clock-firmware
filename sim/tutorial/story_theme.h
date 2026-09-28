/**
 * @file story_theme.h
 * @brief Deterministic CLOCK Storybook theme model and loader.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "tutorial/tutorial_surface.h"

namespace clockfw::sim::tutorial {

/** @brief One explicitly requested deterministic Storybook font face and size. */
struct StoryFontStyle {
    std::string family;
    std::uint32_t sizePx = 28U;
};

/** @brief Complete Phase-1 data-driven visual theme. */
struct StoryTheme {
    std::string id;
    StoryFontStyle body;
    StoryFontStyle heading;
    StoryFontStyle chapter;
    StoryFontStyle subtitle;
    StoryFontStyle monospace;
    std::uint32_t lineSpacingPx = 12U;
    TutorialColor background{};
    TutorialColor foreground{};
    TutorialColor muted{};
    TutorialColor accent{};
    TutorialColor tipBackground{};
    TutorialColor tipForeground{};
    TutorialColor warningBackground{};
    TutorialColor warningForeground{};
    TutorialColor recipeBackground{};
    TutorialColor recipeForeground{};
};

/** @brief Returns true only for deterministic font faces compiled into Storybook. */
bool storybookFontAvailable(const std::string& family);

/** @brief Loads and strictly validates one theme by id from docs/tutorials/themes. */
StoryTheme loadStoryTheme(const std::filesystem::path& tutorialRoot, const std::string& themeId);

}  // namespace clockfw::sim::tutorial
