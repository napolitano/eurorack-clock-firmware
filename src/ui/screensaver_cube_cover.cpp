/**
 * @file screensaver_cube_cover.cpp
 * @brief 8x8 cube-tile cover screensaver with changing traversal patterns.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/screensaver_renderer.h"

#include <algorithm>
#include <cstdint>

namespace clockfw::ui {
namespace {
constexpr std::int16_t kColumns = 16;
constexpr std::int16_t kRows = 8;
constexpr std::uint32_t kFillFrames = 128U;
constexpr std::uint32_t kHoldFrames = 14U;
constexpr std::uint32_t kCycleFrames = kFillFrames + kHoldFrames;

std::uint16_t rankForward(const std::int16_t x, const std::int16_t y) { return static_cast<std::uint16_t>(y * kColumns + x); }
std::uint16_t rankSnake(const std::int16_t x, const std::int16_t y) { return static_cast<std::uint16_t>(y * kColumns + ((y & 1) != 0 ? kColumns - 1 - x : x)); }
std::uint16_t rankCenter(const std::int16_t x, const std::int16_t y) {
    const std::int16_t dx2 = static_cast<std::int16_t>(std::abs(x * 2 - 15));
    const std::int16_t dy2 = static_cast<std::int16_t>(std::abs(y * 2 - 7));
    return static_cast<std::uint16_t>(std::max(dx2 * 4, dy2 * 8) + rankForward(x,y) / 32);
}
std::uint16_t rankRing(const std::int16_t x, const std::int16_t y) {
    const std::int16_t ring = std::min({x, static_cast<std::int16_t>(15-x), y, static_cast<std::int16_t>(7-y)});
    return static_cast<std::uint16_t>(ring * 40 + rankSnake(x,y) % 40);
}
std::uint16_t rankFor(const std::uint8_t pattern, const std::int16_t x, const std::int16_t y) {
    switch (pattern) {
        case 0U: return rankForward(x,y);
        case 1U: return static_cast<std::uint16_t>(127U - rankForward(x,y));
        case 2U: return rankCenter(x,y);
        case 3U: return static_cast<std::uint16_t>(127U - std::min<std::uint16_t>(127U, rankCenter(x,y) * 2U));
        case 4U: return rankSnake(x,y);
        default: return rankRing(x,y);
    }
}
void drawCube(hal::OledDisplay& display, const std::int16_t cellX, const std::int16_t cellY, const bool alternate) {
    const std::int16_t x = static_cast<std::int16_t>(cellX * 8);
    const std::int16_t y = static_cast<std::int16_t>(cellY * 8);
    display.drawRectangle(x, y, 8, 8);
    if (!alternate) {
        display.drawLine(x, y, static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 3));
        display.drawLine(static_cast<std::int16_t>(x + 7), y, static_cast<std::int16_t>(x + 4), static_cast<std::int16_t>(y + 3));
        display.drawLine(static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 3), static_cast<std::int16_t>(x + 4), static_cast<std::int16_t>(y + 3));
        display.drawVerticalLine(static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 3), 5);
    } else {
        display.drawLine(x, static_cast<std::int16_t>(y + 7), static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 4));
        display.drawLine(static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 7), static_cast<std::int16_t>(x + 4), static_cast<std::int16_t>(y + 4));
        display.drawHorizontalLine(static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 4), 2);
        display.drawVerticalLine(static_cast<std::int16_t>(x + 4), y, 5);
    }
}
}  // namespace

void ScreensaverRenderer::renderCubeCover(const std::uint32_t frameIndex) {
    display_.clear();
    const std::uint32_t cycle = frameIndex / kCycleFrames;
    const std::uint32_t phase = frameIndex % kCycleFrames;
    const std::uint8_t pattern = static_cast<std::uint8_t>((cycle * 5U + 1U) % 6U);
    const std::uint16_t threshold = phase >= kFillFrames ? 127U : static_cast<std::uint16_t>(phase);
    for (std::int16_t y = 0; y < kRows; ++y) {
        for (std::int16_t x = 0; x < kColumns; ++x) {
            if (rankFor(pattern, x, y) <= threshold) drawCube(display_, x, y, ((static_cast<unsigned>(x + y) + static_cast<unsigned>(pattern)) & 1U) != 0U);
        }
    }
    display_.present();
}

}  // namespace clockfw::ui
