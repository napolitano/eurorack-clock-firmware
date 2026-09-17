/**
 * @file screensaver_starfield.cpp
 * @brief Three-depth parallax starfield screensaver.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/screensaver_renderer.h"

#include <cstdint>

namespace clockfw::ui {
namespace {

std::uint32_t starHash(std::uint32_t value) {
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    value *= 0x846CA68BU;
    return value ^ (value >> 16U);
}

void drawLayer(
    hal::OledDisplay& display,
    const std::uint32_t frameIndex,
    const std::uint32_t layerSeed,
    const std::uint8_t starCount,
    const std::uint8_t speed,
    const std::int16_t length) {
    for (std::uint8_t star = 0U; star < starCount; ++star) {
        const std::uint32_t seed = starHash(layerSeed + static_cast<std::uint32_t>(star) * 0x9E3779B9U);
        const std::uint32_t baseX = seed & 0x7FU;
        const std::int16_t y = static_cast<std::int16_t>((seed >> 8U) & 0x3FU);
        const std::uint32_t movement = frameIndex * static_cast<std::uint32_t>(speed);
        const std::int16_t x = static_cast<std::int16_t>((baseX + 128U - (movement & 0x7FU)) & 0x7FU);
        if (length <= 1) {
            display.setPixel(x, y);
        } else {
            display.drawHorizontalLine(x, y, length);
            if (x + length > hal::OledDisplay::kWidth) {
                display.drawHorizontalLine(0, y, static_cast<std::int16_t>(x + length - hal::OledDisplay::kWidth));
            }
        }
    }
}

}  // namespace

void ScreensaverRenderer::renderStarfield(const std::uint32_t frameIndex) {
    display_.clear();
    drawLayer(display_, frameIndex / 2U, 0x53544131U, 28U, 1U, 1);
    drawLayer(display_, frameIndex, 0x53544132U, 16U, 1U, 2);
    drawLayer(display_, frameIndex, 0x53544133U, 8U, 3U, 3);
    display_.present();
}

}  // namespace clockfw::ui
