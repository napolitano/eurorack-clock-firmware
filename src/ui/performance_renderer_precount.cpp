/**
 * @file performance_renderer_precount.cpp
 * @brief Static Pre-Count popover for the Performance screen.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/performance_renderer.h"

#include <algorithm>
#include <cstdio>

namespace clockfw::ui {
namespace {

constexpr std::int16_t kPopoverSize = 54;
constexpr std::int16_t kPopoverX =
    (hal::OledDisplay::kWidth - kPopoverSize) / 2;
constexpr std::int16_t kPopoverY =
    (hal::OledDisplay::kHeight - kPopoverSize) / 2;
constexpr std::int16_t kIndicatorInsetX = 3;
constexpr std::int16_t kIndicatorY = kPopoverY + kPopoverSize - 7;
constexpr std::int16_t kIndicatorHeight = 4;
constexpr std::int16_t kMaximumIndicatorWidth = 6;
constexpr std::int16_t kMinimumIndicatorWidth = 2;

}  // namespace

void PerformanceRenderer::drawPreCountOverlay(
    const ClockState& state,
    const engine::EngineSnapshot& engineSnapshot) {
    if (!engineSnapshot.preCountActive || engineSnapshot.preCountRemaining == 0U) {
        return;
    }

    // Keep the count-in deliberately calm: cover the underlying Performance area
    // with one fixed black square and a one-pixel white border. Beat changes update
    // the number and meter row, but there is no phase-driven size animation.
    display_.fillRectangle(
        kPopoverX,
        kPopoverY,
        kPopoverSize,
        kPopoverSize,
        hal::PixelColor::Black);
    display_.drawRectangle(
        kPopoverX,
        kPopoverY,
        kPopoverSize,
        kPopoverSize,
        hal::PixelColor::White);

    char countText[4]{};
    std::snprintf(
        countText,
        sizeof(countText),
        "%u",
        static_cast<unsigned>(engineSnapshot.preCountRemaining));
    display_.setFont(hal::DisplayFont::TempoLarge);
    const hal::TextBounds bounds = display_.measureText(countText, 0, 0);
    const std::int16_t textX = static_cast<std::int16_t>(
        (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(bounds.width)) / 2);
    constexpr std::int16_t kNumberTop = kPopoverY + 5;
    constexpr std::int16_t kNumberBottom = kIndicatorY - 2;
    const std::int16_t numberAreaHeight = kNumberBottom - kNumberTop;
    const std::int16_t textY = static_cast<std::int16_t>(
        kNumberTop + std::max<int>(0, (numberAreaHeight - static_cast<int>(bounds.height)) / 2));
    display_.setTextColor(hal::PixelColor::White);
    display_.drawText(textX, textY, countText);
    display_.setFont(hal::DisplayFont::Small);

    // The bottom row visualizes the current step inside the configured master meter.
    // For 4/4 the four fixed positions read as Tick-Tack-Tack-Tack: exactly the
    // active beat is filled, while the other three positions remain outlined.
    const std::uint8_t beatCount = std::clamp<std::uint8_t>(state.masterMeter.beats, 1U, 16U);
    const std::uint8_t configuredCount = state.preCountSteps;
    const std::uint8_t elapsed = configuredCount >= engineSnapshot.preCountRemaining
        ? static_cast<std::uint8_t>(configuredCount - engineSnapshot.preCountRemaining)
        : 0U;
    const std::uint8_t currentBeat = static_cast<std::uint8_t>(elapsed % beatCount);

    const std::int16_t gap = beatCount <= 8U ? 2 : 1;
    constexpr std::int16_t kAvailableWidth = kPopoverSize - (kIndicatorInsetX * 2);
    const std::int16_t availableForCells = static_cast<std::int16_t>(
        kAvailableWidth - gap * static_cast<std::int16_t>(beatCount - 1U));
    const std::int16_t cellWidth = std::clamp<std::int16_t>(
        static_cast<std::int16_t>(availableForCells / beatCount),
        kMinimumIndicatorWidth,
        kMaximumIndicatorWidth);
    const std::int16_t rowWidth = static_cast<std::int16_t>(
        cellWidth * beatCount + gap * static_cast<std::int16_t>(beatCount - 1U));
    const std::int16_t rowX = static_cast<std::int16_t>(
        (static_cast<int>(hal::OledDisplay::kWidth) - rowWidth) / 2);

    for (std::uint8_t beat = 0U; beat < beatCount; ++beat) {
        const std::int16_t x = static_cast<std::int16_t>(
            rowX + static_cast<std::int16_t>(beat) * (cellWidth + gap));
        if (beat == currentBeat) {
            display_.fillRectangle(x, kIndicatorY, cellWidth, kIndicatorHeight);
        } else {
            display_.drawRectangle(x, kIndicatorY, cellWidth, kIndicatorHeight);
        }
    }
}

void PerformanceRenderer::presentPerformanceFrame(
    const ClockState& state,
    const engine::EngineSnapshot& engineSnapshot) {
    drawPreCountOverlay(state, engineSnapshot);
    display_.present();
}

}  // namespace clockfw::ui
