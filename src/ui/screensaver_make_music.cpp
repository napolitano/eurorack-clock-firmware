/**
 * @file screensaver_make_music.cpp
 * @brief Progressive MAKE MUSIC NOT WAR text-stream screensaver.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/screensaver_renderer.h"

#include <array>
#include <cstddef>
#include <cstdint>

namespace clockfw::ui {
namespace {

constexpr std::int16_t kCellWidth = 6;
constexpr std::int16_t kCellHeight = 8;
constexpr std::int16_t kColumns = 19;
constexpr std::int16_t kRows = 8;
constexpr std::int16_t kStartX = 7;
constexpr std::uint32_t kCellCount = static_cast<std::uint32_t>(kColumns * kRows);
constexpr std::uint32_t kHoldFrames = 12U;
constexpr std::array<char, 19U> kMessage{{
    'M','A','K','E','\0',
    'M','U','S','I','C','\0',
    'N','O','T','\0',
    'W','A','R','\0'}};

void drawMidDot(hal::OledDisplay& display, const std::int16_t x, const std::int16_t y) {
    display.fillRectangle(
        static_cast<std::int16_t>(x + 2),
        static_cast<std::int16_t>(y + 3),
        2,
        2);
}

}  // namespace

void ScreensaverRenderer::renderMakeMusic(const std::uint32_t frameIndex) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    const std::uint32_t cycleLength = kCellCount + kHoldFrames;
    const std::uint32_t cycleFrame = frameIndex % cycleLength;
    const std::uint32_t visibleCells = cycleFrame < kCellCount
        ? cycleFrame + 1U
        : kCellCount;

    for (std::uint32_t index = 0U; index < visibleCells; ++index) {
        const std::int16_t column = static_cast<std::int16_t>(index % static_cast<std::uint32_t>(kColumns));
        const std::int16_t row = static_cast<std::int16_t>(index / static_cast<std::uint32_t>(kColumns));
        const std::int16_t x = static_cast<std::int16_t>(kStartX + column * kCellWidth);
        const std::int16_t y = static_cast<std::int16_t>(row * kCellHeight);
        const char token = kMessage[index % kMessage.size()];
        if (token == '\0') {
            drawMidDot(display_, x, y);
        } else {
            display_.drawCharacter(x, y, token);
        }
    }

    display_.present();
}

}  // namespace clockfw::ui
