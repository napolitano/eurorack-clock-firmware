/**
 * @file story_text_renderer.h
 * @brief Measured deterministic bitmap text layout for CLOCK Storybook presentation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <string>
#include <vector>

#include "tutorial/story_theme.h"
#include "tutorial/tutorial_surface.h"

namespace clockfw::sim::tutorial {

/** @brief One measured wrapped line with exact pixel dimensions. */
struct StoryTextLine {
    std::string text;
    int widthPx = 0;
};

/** @brief Complete measured text block; rendering is rejected if it cannot fit the requested box. */
struct StoryTextLayout {
    std::vector<StoryTextLine> lines;
    int widthPx = 0;
    int heightPx = 0;
    int lineHeightPx = 0;
};

/** @brief Measures one line without wrapping. */
int measureStoryTextWidth(const std::string& text, const StoryFontStyle& font);

/** @brief Wraps and measures text, throwing instead of silently clipping/overflowing. */
StoryTextLayout layoutStoryText(
    const std::string& text,
    const StoryFontStyle& font,
    int maxWidthPx,
    int maxHeightPx,
    std::uint32_t lineSpacingPx);

/** @brief Draws one previously measured text block at the given origin. */
void drawStoryText(
    TutorialSurface& surface,
    const StoryTextLayout& layout,
    const StoryFontStyle& font,
    int x,
    int y,
    TutorialColor color);

/** @brief Draws a single line and returns its measured width. */
int drawStoryTextLine(
    TutorialSurface& surface,
    const std::string& text,
    const StoryFontStyle& font,
    int x,
    int y,
    TutorialColor color);

}  // namespace clockfw::sim::tutorial
