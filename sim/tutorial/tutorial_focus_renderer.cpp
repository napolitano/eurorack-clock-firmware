/**
 * @file tutorial_focus_renderer.cpp
 * @brief Implements deterministic rings, arrows and explanatory focus overlays for tutorials.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/tutorial_focus_renderer.h"

#include <algorithm>
#include <array>
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

enum class ArrowSide : std::uint8_t { Left, Right, Above, Below };

struct ArrowGeometry {
    ArrowSide side = ArrowSide::Right;
    int tipX = 0;
    int tipY = 0;
    int lineStartX = 0;
    int lineStartY = 0;
    TutorialRect labelBox{};
    int labelX = 0;
    int labelY = 0;
};

constexpr int kArrowTargetGap = 16;
constexpr int kArrowLeaderLength = 54;
constexpr int kArrowMargin = 26;

void drawArrowHeadLayer(
    TutorialSurface& frame, const int tipX, const int tipY, const int fromX, const int fromY,
    const TutorialColor color, const int thickness) {
    const double angle = std::atan2(static_cast<double>(tipY - fromY), static_cast<double>(tipX - fromX));
    constexpr double wing = 0.62;
    constexpr double length = 23.0;
    const int ax = tipX - static_cast<int>(std::lround(std::cos(angle - wing) * length));
    const int ay = tipY - static_cast<int>(std::lround(std::sin(angle - wing) * length));
    const int bx = tipX - static_cast<int>(std::lround(std::cos(angle + wing) * length));
    const int by = tipY - static_cast<int>(std::lround(std::sin(angle + wing) * length));
    frame.drawLine(tipX, tipY, ax, ay, color, thickness);
    frame.drawLine(tipX, tipY, bx, by, color, thickness);
}

void drawContrastLeader(
    TutorialSurface& frame, const int fromX, const int fromY, const int tipX, const int tipY,
    const StoryTheme& theme) {
    const TutorialColor shadow{0U, 0U, 0U, 230U};
    frame.drawLine(fromX, fromY, tipX, tipY, shadow, 11);
    drawArrowHeadLayer(frame, tipX, tipY, fromX, fromY, shadow, 11);
    frame.drawLine(fromX, fromY, tipX, tipY, theme.focusForeground, 7);
    drawArrowHeadLayer(frame, tipX, tipY, fromX, fromY, theme.focusForeground, 7);
    frame.drawLine(fromX, fromY, tipX, tipY, theme.accent, 4);
    drawArrowHeadLayer(frame, tipX, tipY, fromX, fromY, theme.accent, 4);
}

ArrowSide requestedSide(const FocusPlacement placement) {
    switch (placement) {
        case FocusPlacement::Left: return ArrowSide::Left;
        case FocusPlacement::Right: return ArrowSide::Right;
        case FocusPlacement::Above: return ArrowSide::Above;
        case FocusPlacement::Below: return ArrowSide::Below;
        case FocusPlacement::Auto: return ArrowSide::Right;
    }
    return ArrowSide::Right;
}

int usableBottom(const TutorialSurface& frame, const StoryTheme& theme) {
    const int height = static_cast<int>(frame.height());
    const std::uint64_t requested = static_cast<std::uint64_t>(theme.subtitleHeightPx) + theme.subtitleSafeBottomPx;
    const int reserved = static_cast<int>(std::min<std::uint64_t>(requested, static_cast<std::uint64_t>(height / 2)));
    return std::max(kArrowMargin + 1, height - reserved - kArrowMargin);
}

ArrowSide chooseArrowSide(
    const TutorialSurface& frame, const TutorialRect target, const int labelWidth, const int labelHeight,
    const StoryTheme& theme, const FocusPlacement placement) {
    if (placement != FocusPlacement::Auto) return requestedSide(placement);
    const int right = static_cast<int>(frame.width()) - kArrowMargin - (target.x + target.width);
    const int left = target.x - kArrowMargin;
    const int above = target.y - kArrowMargin;
    const int below = usableBottom(frame, theme) - (target.y + target.height);
    const int needHorizontal = kArrowTargetGap + kArrowLeaderLength + labelWidth;
    const int needVertical = kArrowTargetGap + kArrowLeaderLength + labelHeight;
    const std::array<std::pair<ArrowSide, int>, 4> scores{{
        {ArrowSide::Right, right - needHorizontal},
        {ArrowSide::Left, left - needHorizontal},
        {ArrowSide::Below, below - needVertical},
        {ArrowSide::Above, above - needVertical},
    }};
    return std::max_element(scores.begin(), scores.end(), [](const auto& a, const auto& b) {
        return a.second < b.second;
    })->first;
}

ArrowGeometry makeArrowGeometry(
    TutorialSurface& frame, const TutorialRect target, const StoryFontStyle& font, const std::string& label,
    const StoryTheme& theme, const FocusPlacement placement) {
    const int labelTextWidth = label.empty() ? 0 : measureStoryTextWidth(label, font);
    const int boxWidth = label.empty() ? 0 : labelTextWidth + 34;
    const int boxHeight = label.empty() ? 0 : static_cast<int>(font.sizePx) + 26;
    const ArrowSide side = chooseArrowSide(frame, target, boxWidth, boxHeight, theme, placement);
    const int centerX = target.x + target.width / 2;
    const int centerY = target.y + target.height / 2;
    const int frameWidth = static_cast<int>(frame.width());
    const int bottom = usableBottom(frame, theme);
    ArrowGeometry out{};
    out.side = side;
    switch (side) {
        case ArrowSide::Right:
            out.tipX = target.x + target.width + kArrowTargetGap;
            out.tipY = centerY;
            out.labelBox = {out.tipX + kArrowLeaderLength, centerY - boxHeight / 2, boxWidth, boxHeight};
            out.labelBox.x = std::min(out.labelBox.x, frameWidth - kArrowMargin - boxWidth);
            out.labelBox.y = std::clamp(out.labelBox.y, kArrowMargin, std::max(kArrowMargin, bottom - boxHeight));
            out.lineStartX = label.empty() ? out.tipX + kArrowLeaderLength : out.labelBox.x - 10;
            out.lineStartY = centerY;
            break;
        case ArrowSide::Left:
            out.tipX = target.x - kArrowTargetGap;
            out.tipY = centerY;
            out.labelBox = {out.tipX - kArrowLeaderLength - boxWidth, centerY - boxHeight / 2, boxWidth, boxHeight};
            out.labelBox.x = std::max(kArrowMargin, out.labelBox.x);
            out.labelBox.y = std::clamp(out.labelBox.y, kArrowMargin, std::max(kArrowMargin, bottom - boxHeight));
            out.lineStartX = label.empty() ? out.tipX - kArrowLeaderLength : out.labelBox.x + boxWidth + 10;
            out.lineStartY = centerY;
            break;
        case ArrowSide::Above:
            out.tipX = centerX;
            out.tipY = target.y - kArrowTargetGap;
            out.labelBox = {centerX - boxWidth / 2, out.tipY - kArrowLeaderLength - boxHeight, boxWidth, boxHeight};
            out.labelBox.x = std::clamp(out.labelBox.x, kArrowMargin, std::max(kArrowMargin, frameWidth - kArrowMargin - boxWidth));
            out.labelBox.y = std::max(kArrowMargin, out.labelBox.y);
            out.lineStartX = centerX;
            out.lineStartY = label.empty() ? out.tipY - kArrowLeaderLength : out.labelBox.y + boxHeight + 10;
            break;
        case ArrowSide::Below:
            out.tipX = centerX;
            out.tipY = target.y + target.height + kArrowTargetGap;
            out.labelBox = {centerX - boxWidth / 2, out.tipY + kArrowLeaderLength, boxWidth, boxHeight};
            out.labelBox.x = std::clamp(out.labelBox.x, kArrowMargin, std::max(kArrowMargin, frameWidth - kArrowMargin - boxWidth));
            out.labelBox.y = std::min(out.labelBox.y, bottom - boxHeight);
            out.lineStartX = centerX;
            out.lineStartY = label.empty() ? out.tipY + kArrowLeaderLength : out.labelBox.y - 10;
            break;
    }
    out.labelX = out.labelBox.x + 17;
    out.labelY = out.labelBox.y + 11;
    return out;
}

}  // namespace

ResolvedTutorialFocus resolveTutorialFocus(const PhysicalPresentationState& state, const StoryTheme& theme) {
    ResolvedTutorialFocus focus{};
    if (state.explicitFocus != FocusTarget::None) {
        focus.target = state.explicitFocus;
        focus.placement = state.focusPlacement;
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
    const StoryTheme& theme, const FocusPlacement placement) {
    StoryFontStyle font = theme.body;
    font.sizePx = std::max<std::uint32_t>(24U, std::min<std::uint32_t>(font.sizePx, 32U));
    const ArrowGeometry geometry = makeArrowGeometry(frame, target, font, label, theme, placement);
    drawContrastLeader(frame, geometry.lineStartX, geometry.lineStartY, geometry.tipX, geometry.tipY, theme);
    if (label.empty()) return;
    const TutorialColor shadow{0U, 0U, 0U, 235U};
    frame.fillRect({geometry.labelBox.x - 5, geometry.labelBox.y - 5,
                    geometry.labelBox.width + 10, geometry.labelBox.height + 10}, shadow);
    frame.fillRect(geometry.labelBox, {4U, 6U, 10U, 245U});
    frame.strokeRect(geometry.labelBox, theme.focusForeground, 4);
    frame.strokeRect({geometry.labelBox.x + 3, geometry.labelBox.y + 3,
                      std::max(1, geometry.labelBox.width - 6), std::max(1, geometry.labelBox.height - 6)},
                     theme.accent, 3);
    (void)drawStoryTextLine(frame, label, font, geometry.labelX, geometry.labelY, theme.focusForeground);
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
    const auto clampToOled = [&oledRaster](const TutorialRect rect) {
        const int left = std::max(oledRaster.x, rect.x);
        const int top = std::max(oledRaster.y, rect.y);
        const int right = std::min(oledRaster.x + oledRaster.width, rect.x + rect.width);
        const int bottom = std::min(oledRaster.y + oledRaster.height, rect.y + rect.height);
        return TutorialRect{left, top, std::max(1, right - left), std::max(1, bottom - top)};
    };
    const TutorialRect accentRect = clampToOled(
        {region.x - 8, region.y - 8, region.width + 16, region.height + 16});
    const TutorialRect contrastRect = clampToOled(
        {region.x - 3, region.y - 3, region.width + 6, region.height + 6});
    frame.strokeRect(accentRect, theme.accent, 8);
    frame.strokeRect(contrastRect, theme.focusForeground, 3);
    drawTutorialFocusArrow(frame, region, focus.label, theme, focus.placement);
}

}  // namespace clockfw::sim::tutorial
