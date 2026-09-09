/**
 * @file screensaver_matrix.cpp
 * @brief Original monochrome procedural digital-rain screensaver.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/screensaver_renderer.h"

#include <array>
#include <cstdint>

namespace clockfw::ui {
namespace {
constexpr std::array<char, 16U> kGlyphs{{'0','1','2','3','4','5','6','7','8','9','+','-','/','*','<','>'}};
std::uint32_t mix(std::uint32_t v) { v ^= v >> 16U; v *= 0x7FEB352DU; v ^= v >> 15U; v *= 0x846CA68BU; return v ^ (v >> 16U); }
}  // namespace

void ScreensaverRenderer::renderMatrix(const std::uint32_t frameIndex) {
    display_.clear(); display_.setFont(hal::DisplayFont::Small);
    constexpr std::int16_t kColumnWidth = 8;
    constexpr std::int16_t kRowHeight = 8;
    for (std::int16_t column = 0; column < 16; ++column) {
        const std::uint32_t seed = mix(static_cast<std::uint32_t>(column) * 0x91E10DA5U + 0x4D415452U);
        const std::uint32_t speed = 2U + seed % 4U;
        const std::int16_t tail = static_cast<std::int16_t>(3U + (seed >> 8U) % 5U);
        const std::int16_t cycle = static_cast<std::int16_t>(8 + tail + 3);
        const std::int16_t head = static_cast<std::int16_t>((frameIndex / speed + (seed >> 16U) % static_cast<std::uint32_t>(cycle)) % static_cast<std::uint32_t>(cycle));
        for (std::int16_t t = 0; t < tail; ++t) {
            const std::int16_t row = static_cast<std::int16_t>(head - t);
            if (row < 0 || row >= 8) continue;
            const std::uint32_t glyphSeed = mix(seed + frameIndex / 4U + static_cast<std::uint32_t>(row * 17 + t));
            const char glyph = kGlyphs[glyphSeed % kGlyphs.size()];
            const std::int16_t x = static_cast<std::int16_t>(column * kColumnWidth + 1);
            const std::int16_t y = static_cast<std::int16_t>(row * kRowHeight);
            if (t == 0) {
                display_.fillRectangle(x, y, 6, 7);
                display_.setTextColor(hal::PixelColor::Black);
                display_.drawCharacter(x, y, glyph);
                display_.setTextColor(hal::PixelColor::White);
            } else if (t < 3 || ((glyphSeed >> 9U) & 1U) != 0U) {
                display_.drawCharacter(x, y, glyph);
            }
        }
    }
    display_.present();
}

}  // namespace clockfw::ui
