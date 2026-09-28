/**
 * @file tutorial_focus_renderer.cpp
 * @brief Implements deterministic rings, arrows and explanatory focus overlays for tutorials.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/tutorial_focus_renderer.h"

#include <algorithm>
#include <cmath>

#include "tutorial/story_text_renderer.h"

namespace clockfw::sim::tutorial {
namespace {

constexpr double kPi = 3.14159265358979323846;
constexpr int kDetailCropWidth = 220;
constexpr int kDetailCropHeight = 190;

int rounded(const float value) { return static_cast<int>(std::lround(value)); }

TutorialRect panelBounds(const layout::PanelLayout& layout) {
    return {rounded(layout.panel.x), rounded(layout.panel.y), rounded(layout.panel.width), rounded(layout.panel.height)};
}

TutorialRect clampAround(const TutorialRect bounds, const int centerX, const int centerY, const int width, const int height) {
    const int useWidth = std::min(width, bounds.width);
    const int useHeight = std::min(height, bounds.height);
    const int maxX = bounds.x + bounds.width - useWidth;
    const int maxY = bounds.y + bounds.height - useHeight;
    return {
        std::clamp(centerX - useWidth / 2, bounds.x, maxX),
        std::clamp(centerY - useHeight / 2, bounds.y, maxY),
        useWidth, useHeight};
}

void drawArrowHead(TutorialSurface& frame, const int tipX, const int tipY, const int fromX, const int fromY,
                   const TutorialColor color) {
    const double angle = std::atan2(static_cast<double>(tipY - fromY), static_cast<double>(tipX - fromX));
    constexpr double wing = 0.62;
    constexpr double length = 22.0;
    const int ax = tipX - static_cast<int>(std::lround(std::cos(angle - wing) * length));
    const int ay = tipY - static_cast<int>(std::lround(std::sin(angle - wing) * length));
    const int bx = tipX - static_cast<int>(std::lround(std::cos(angle + wing) * length));
    const int by = tipY - static_cast<int>(std::lround(std::sin(angle + wing) * length));
    frame.drawLine(tipX, tipY, ax, ay, color, 5);
    frame.drawLine(tipX, tipY, bx, by, color, 5);
}

}  // namespace

ResolvedTutorialFocus resolveTutorialFocus(const PhysicalPresentationState& state, const StoryTheme& theme) {
    ResolvedTutorialFocus focus{};
    if (state.explicitFocus != FocusTarget::None) {
        focus.target = state.explicitFocus;
        focus.oledX = state.focusX;
        focus.oledY = state.focusY;
        focus.oledWidth = state.focusWidth;
        focus.oledHeight = state.focusHeight;
        focus.label = state.focusLabel;
        focus.explicitFocus = true;
        return focus;
    }
    const std::uint64_t lingerUs = static_cast<std::uint64_t>(theme.focusLingerMs) * 1000ULL;
    if (state.automaticFocus != FocusTarget::None && state.automaticFocusAgeUs <= lingerUs) {
        focus.target = state.automaticFocus;
    }
    return focus;
}

std::optional<layout::Point> panelFocusPoint(const layout::PanelLayout& layout, const FocusTarget target) {
    switch (target) {
        case FocusTarget::Encoder: return layout.encoderCenter;
        case FocusTarget::Play: return layout.playButton.center;
        case FocusTarget::Tap: return layout.tapButton.center;
        case FocusTarget::StopBack: return layout.stopButton.center;
        case FocusTarget::Sync: return layout.syncInputCenter;
        case FocusTarget::Reset: return layout.resetInputCenter;
        case FocusTarget::None:
        case FocusTarget::OledRegion: return std::nullopt;
    }
    return std::nullopt;
}

TutorialRect panelFocusSourceRect(const layout::PanelLayout& layout, const FocusTarget target) {
    const TutorialRect bounds = panelBounds(layout);
    const auto point = panelFocusPoint(layout, target);
    if (!point) return bounds;
    return clampAround(bounds, rounded(point->x), rounded(point->y), kDetailCropWidth, kDetailCropHeight);
}

std::optional<TutorialRect> panelFocusDestinationRect(
    const layout::PanelLayout& layout, const FocusTarget target,
    const TutorialRect source, const TutorialRect destination) {
    const auto point = panelFocusPoint(layout, target);
    if (!point || source.width <= 0 || source.height <= 0) return std::nullopt;
    const int x = destination.x + static_cast<int>(std::lround(
        (static_cast<double>(point->x) - source.x) * destination.width / source.width));
    const int y = destination.y + static_cast<int>(std::lround(
        (static_cast<double>(point->y) - source.y) * destination.height / source.height));
    const int radius = std::max(28, std::min(destination.width, destination.height) / 9);
    return TutorialRect{x - radius, y - radius, radius * 2, radius * 2};
}

std::string focusTargetLabel(const FocusTarget target) {
    switch (target) {
        case FocusTarget::Encoder: return "ENCODER";
        case FocusTarget::Play: return "PLAY";
        case FocusTarget::Tap: return "TAP";
        case FocusTarget::StopBack: return "STOP / BACK";
        case FocusTarget::Sync: return "SYNC IN";
        case FocusTarget::Reset: return "RST IN";
        case FocusTarget::OledRegion: return "DISPLAY";
        case FocusTarget::None: return {};
    }
    return {};
}

void drawTutorialFocusRing(
    TutorialSurface& frame, const int centerX, const int centerY, const int radius, const StoryTheme& theme) {
    const int outer = std::max(12, radius);
    const int inner = std::max(1, outer - 7);
    const int outerSquared = outer * outer;
    const int innerSquared = inner * inner;
    for (int y = -outer; y <= outer; ++y) {
        for (int x = -outer; x <= outer; ++x) {
            const int distance = x * x + y * y;
            if (distance <= outerSquared && distance >= innerSquared) {
                frame.blendPixel(centerX + x, centerY + y, theme.focusForeground);
            }
        }
    }
    const TutorialColor halo{theme.accent.red, theme.accent.green, theme.accent.blue, 150U};
    for (int offset = 8; offset <= 12; offset += 2) {
        const int ring = outer + offset;
        for (int step = 0; step < 180; ++step) {
            const double angle = (2.0 * kPi * static_cast<double>(step)) / 180.0;
            frame.blendPixel(centerX + static_cast<int>(std::lround(std::cos(angle) * ring)),
                             centerY + static_cast<int>(std::lround(std::sin(angle) * ring)), halo);
        }
    }
}

void drawTutorialFocusArrow(
    TutorialSurface& frame, const TutorialRect target, const std::string& label,
    const StoryTheme& theme, const bool preferLeftLabel) {
    const int targetX = target.x + target.width / 2;
    const int targetY = target.y + target.height / 2;
    const int lineStartX = preferLeftLabel ? std::max(70, target.x - 170) : std::min(static_cast<int>(frame.width()) - 70, target.x + target.width + 170);
    const int lineStartY = std::max(70, target.y - 80);
    frame.drawLine(lineStartX, lineStartY, targetX, targetY, theme.focusForeground, 5);
    drawArrowHead(frame, targetX, targetY, lineStartX, lineStartY, theme.focusForeground);
    if (label.empty()) return;
    StoryFontStyle font = theme.body;
    font.sizePx = std::max<std::uint32_t>(24U, std::min<std::uint32_t>(font.sizePx, 32U));
    const int labelWidth = measureStoryTextWidth(label, font);
    const int x = preferLeftLabel ? std::max(28, lineStartX - labelWidth) : std::min(lineStartX, static_cast<int>(frame.width()) - labelWidth - 28);
    const int y = std::max(24, lineStartY - static_cast<int>(font.sizePx) - 18);
    frame.fillRect({x - 16, y - 10, labelWidth + 32, static_cast<int>(font.sizePx) + 24}, {4U, 6U, 10U, 220U});
    frame.strokeRect({x - 16, y - 10, labelWidth + 32, static_cast<int>(font.sizePx) + 24}, theme.accent, 2);
    (void)drawStoryTextLine(frame, label, font, x, y, theme.focusForeground);
}

void drawOledRegionFocus(
    TutorialSurface& frame, const TutorialRect oledRaster,
    const ResolvedTutorialFocus& focus, const StoryTheme& theme) {
    if (focus.target != FocusTarget::OledRegion || focus.oledWidth <= 0 || focus.oledHeight <= 0) return;
    const TutorialRect region{
        oledRaster.x + (focus.oledX * oledRaster.width) / 128,
        oledRaster.y + (focus.oledY * oledRaster.height) / 64,
        std::max(2, (focus.oledWidth * oledRaster.width) / 128),
        std::max(2, (focus.oledHeight * oledRaster.height) / 64)};
    frame.strokeRect({region.x - 8, region.y - 8, region.width + 16, region.height + 16}, theme.accent, 8);
    frame.strokeRect({region.x - 3, region.y - 3, region.width + 6, region.height + 6}, theme.focusForeground, 3);
    drawTutorialFocusArrow(frame, region, focus.label, theme, false);
}

}  // namespace clockfw::sim::tutorial
