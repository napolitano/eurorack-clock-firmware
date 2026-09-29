/**
 * @file tutorial_scene_renderer.cpp
 * @brief Implements professional full-frame Storybook presentation scenes and subtitle strip.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/tutorial_scene_renderer.h"

#include <algorithm>
#include <stdexcept>

#include "tutorial/story_text_renderer.h"

namespace clockfw::sim::tutorial {
namespace {

constexpr int kFrameMargin = 64;

}  // namespace

void renderStorySubtitle(TutorialSurface& frame, const std::string& subtitle, const StoryTheme& theme) {
    const int width = static_cast<int>(frame.width());
    const int height = static_cast<int>(frame.height());
    const int stripHeight = static_cast<int>(theme.subtitleHeightPx);
    const int safeBottom = static_cast<int>(theme.subtitleSafeBottomPx);
    if (stripHeight <= 0 || safeBottom < 0 || stripHeight + safeBottom >= height) {
        throw std::runtime_error("subtitle safe-area configuration does not fit output frame");
    }
    const TutorialRect strip{0, height - safeBottom - stripHeight, width, stripHeight};
    frame.fillRect(strip, theme.subtitleBackground);
    frame.fillRect({0, strip.y, width, 3}, theme.accent);
    if (subtitle.empty()) return;
    const StoryTextLayout layout = layoutStoryText(
        subtitle, theme.subtitle, width - 2 * kFrameMargin, stripHeight - 28, theme.lineSpacingPx);
    const int x = (width - layout.widthPx) / 2;
    const int y = strip.y + (strip.height - layout.heightPx) / 2;
    drawStoryText(frame, layout, theme.subtitle, x, y, theme.subtitleForeground);
}

void renderStoryChapter(TutorialSurface& frame, const StoryScene& scene, const StoryTheme& theme) {
    frame.clear(theme.chapterBackground);
    const int width = static_cast<int>(frame.width());
    const int contentBottom = static_cast<int>(frame.height());
    int dividerY = 260;
    int titleY = 315;
    if (scene.number != 0U) {
        StoryFontStyle numberFont = theme.monospace;
        numberFont.sizePx = numberFont.backend == StoryFontBackend::Builtin ? 42U : 44U;
        numberFont.weight = 700U;
        const std::string number = std::to_string(scene.number);
        const int numberWidth = measureStoryTextWidth(number, numberFont);
        const TutorialColor numberColor{255U, 255U, 255U, 190U};
        (void)drawStoryTextLine(frame, number, numberFont, (width - numberWidth) / 2, 220, numberColor);
        dividerY = 310;
        titleY = 365;
    }
    frame.fillRect({width / 2 - 42, dividerY, 84, 4}, {255U, 255U, 255U, 210U});
    const StoryTextLayout title = layoutStoryText(scene.title, theme.chapter, width - 360, 190, theme.lineSpacingPx);
    drawStoryText(frame, title, theme.chapter, (width - title.widthPx) / 2, titleY, theme.chapterForeground);
    if (!scene.subtitle.empty()) {
        const StoryTextLayout subtitle = layoutStoryText(scene.subtitle, theme.heading, width - 440, 190, theme.lineSpacingPx);
        drawStoryText(frame, subtitle, theme.heading, (width - subtitle.widthPx) / 2,
                      std::min(contentBottom - subtitle.heightPx - 180, titleY + 195), theme.chapterMuted);
    }
}

void renderStoryTextScene(TutorialSurface& frame, const StoryScene& scene, const StoryTheme& theme) {
    frame.clear(theme.background);
    const int width = static_cast<int>(frame.width());
    const int contentBottom = static_cast<int>(frame.height());
    const int x = 200;
    const int maxWidth = width - 400;
    frame.fillRect({x, 132, 92, 5}, theme.accent);
    const StoryTextLayout title = layoutStoryText(scene.title, theme.heading, maxWidth, 160, theme.lineSpacingPx);
    drawStoryText(frame, title, theme.heading, x, 176, theme.foreground);
    const int bodyY = 176 + title.heightPx + 78;
    const StoryTextLayout body = layoutStoryText(scene.body, theme.body, maxWidth,
                                                 contentBottom - bodyY - 100, theme.lineSpacingPx);
    drawStoryText(frame, body, theme.body, x, bodyY, theme.foreground);
}

void renderStoryCallout(TutorialSurface& frame, const StoryScene& scene, const StoryTheme& theme) {
    if (!scene.calloutKind) throw std::runtime_error("callout scene has no type");
    frame.clear(theme.background);
    TutorialColor background{};
    TutorialColor foreground{};
    std::string label;
    switch (*scene.calloutKind) {
        case CalloutKind::Tip:
            background = theme.tipBackground; foreground = theme.tipForeground; label = "TIP"; break;
        case CalloutKind::Warning:
            background = theme.warningBackground; foreground = theme.warningForeground; label = "WARNING"; break;
        case CalloutKind::Recipe:
            background = theme.recipeBackground; foreground = theme.recipeForeground; label = "RECIPE"; break;
    }
    const int width = static_cast<int>(frame.width());
    const int contentBottom = static_cast<int>(frame.height());
    const TutorialRect box{180, 140, width - 360, contentBottom - 260};
    frame.fillRect(box, background);
    frame.strokeRect(box, theme.tutorialSurfaceBorder, 2);
    frame.fillRect({box.x, box.y, 8, box.height}, theme.accent);
    StoryFontStyle labelFont = theme.monospace;
    labelFont.weight = 700U;
    (void)drawStoryTextLine(frame, label, labelFont, box.x + 56, box.y + 48, theme.accent);
    const StoryTextLayout title = layoutStoryText(scene.title, theme.heading, box.width - 112, 120, theme.lineSpacingPx);
    drawStoryText(frame, title, theme.heading, box.x + 56, box.y + 110, foreground);
    std::string content = scene.body;
    if (*scene.calloutKind == CalloutKind::Recipe) {
        content.clear();
        for (std::size_t index = 0U; index < scene.recipeSteps.size(); ++index) {
            if (!content.empty()) content += '\n';
            content += std::to_string(index + 1U) + ". " + scene.recipeSteps[index];
        }
    }
    const int bodyY = box.y + 110 + title.heightPx + 58;
    const StoryTextLayout body = layoutStoryText(content, theme.body, box.width - 112,
                                                 box.y + box.height - bodyY - 48, theme.lineSpacingPx);
    drawStoryText(frame, body, theme.body, box.x + 56, bodyY, foreground);
}

}  // namespace clockfw::sim::tutorial
