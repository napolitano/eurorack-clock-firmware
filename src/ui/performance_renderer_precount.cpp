/**
 * @file performance_renderer_precount.cpp
 * @brief Centered animated Pre-Count overlay for the Performance screen.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/performance_renderer.h"

#include <cstdio>

namespace clockfw::ui {

void PerformanceRenderer::drawPreCountOverlay(
    const engine::EngineSnapshot& engineSnapshot) {
    if (!engineSnapshot.preCountActive || engineSnapshot.preCountRemaining == 0U) {
        return;
    }

    constexpr std::int16_t kCenterX = hal::OledDisplay::kWidth / 2;
    constexpr std::int16_t kCenterY = hal::OledDisplay::kHeight / 2;
    constexpr std::int16_t kMaximumRadius = 29;
    constexpr std::int16_t kMinimumRadius = 17;
    constexpr std::int16_t kRadiusSpan = kMaximumRadius - kMinimumRadius;

    const std::uint32_t phase255 = static_cast<std::uint32_t>(
        (engineSnapshot.preCountPhaseQ32 >> 24U) & 0xFFU);
    const std::int16_t radius = static_cast<std::int16_t>(
        kMaximumRadius - static_cast<std::int16_t>((phase255 * kRadiusSpan) / 255U));

    // Erase only the circular overlay area so the normal Performance view remains
    // visible around the count-in. Integer scanlines avoid floating point and keep
    // rendering deterministic on both STM32 and simulator builds.
    const std::int32_t radiusSquared = static_cast<std::int32_t>(radius) * radius;
    for (std::int16_t y = static_cast<std::int16_t>(-radius); y <= radius; ++y) {
        std::int16_t x = radius;
        const std::int32_t ySquared = static_cast<std::int32_t>(y) * y;
        while (x > 0 && static_cast<std::int32_t>(x) * x + ySquared > radiusSquared) {
            --x;
        }
        display_.drawHorizontalLine(
            static_cast<std::int16_t>(kCenterX - x),
            static_cast<std::int16_t>(kCenterY + y),
            static_cast<std::int16_t>(x * 2 + 1),
            hal::PixelColor::Black);
    }

    // Midpoint circle outline.
    std::int16_t x = radius;
    std::int16_t y = 0;
    std::int16_t error = 1 - radius;
    while (x >= y) {
        display_.setPixel(kCenterX + x, kCenterY + y);
        display_.setPixel(kCenterX + y, kCenterY + x);
        display_.setPixel(kCenterX - y, kCenterY + x);
        display_.setPixel(kCenterX - x, kCenterY + y);
        display_.setPixel(kCenterX - x, kCenterY - y);
        display_.setPixel(kCenterX - y, kCenterY - x);
        display_.setPixel(kCenterX + y, kCenterY - x);
        display_.setPixel(kCenterX + x, kCenterY - y);
        ++y;
        if (error < 0) {
            error = static_cast<std::int16_t>(error + 2 * y + 1);
        } else {
            --x;
            error = static_cast<std::int16_t>(error + 2 * (y - x) + 1);
        }
    }

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
    const std::int16_t textY = static_cast<std::int16_t>(
        (static_cast<int>(hal::OledDisplay::kHeight) - static_cast<int>(bounds.height)) / 2);
    display_.setTextColor(hal::PixelColor::White);
    display_.drawText(textX, textY, countText);
    display_.setFont(hal::DisplayFont::Small);
}

void PerformanceRenderer::presentPerformanceFrame(
    const engine::EngineSnapshot& engineSnapshot) {
    drawPreCountOverlay(engineSnapshot);
    display_.present();
}


}  // namespace clockfw::ui
