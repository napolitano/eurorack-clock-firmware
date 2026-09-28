/**
 * @file story_theme.h
 * @brief CLOCK Storybook theme model and loader.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

#include "tutorial/tutorial_surface.h"

namespace clockfw::sim::tutorial {

enum class StoryFontBackend : std::uint8_t { Builtin, System };
enum class StoryBackgroundImageMode : std::uint8_t { Cover, Contain, Stretch };

/** @brief One explicitly requested Storybook font face and size. */
struct StoryFontStyle {
    std::string family;
    std::uint32_t sizePx = 28U;
    StoryFontBackend backend = StoryFontBackend::Builtin;
    std::uint16_t weight = 400U;
};

/** @brief Complete data-driven visual theme. */
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
    TutorialColor tutorialBackground{};
    TutorialColor tutorialSurface{};
    TutorialColor tutorialSurfaceBorder{};
    TutorialColor chapterBackground{};
    TutorialColor chapterForeground{};
    TutorialColor chapterMuted{};
    TutorialColor subtitleBackground{};
    TutorialColor subtitleForeground{};
    TutorialColor tipBackground{};
    TutorialColor tipForeground{};
    TutorialColor warningBackground{};
    TutorialColor warningForeground{};
    TutorialColor recipeBackground{};
    TutorialColor recipeForeground{};

    std::optional<std::filesystem::path> tutorialBackgroundImage;
    StoryBackgroundImageMode tutorialBackgroundImageMode = StoryBackgroundImageMode::Cover;
};

/** @brief Returns whether the named backend/family combination is structurally supported. */
bool storybookFontAvailable(const StoryFontStyle& font);

/** @brief Loads and strictly validates one theme by id from docs/tutorials/themes. */
StoryTheme loadStoryTheme(const std::filesystem::path& tutorialRoot, const std::string& themeId);

}  // namespace clockfw::sim::tutorial
