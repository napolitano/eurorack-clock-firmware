/**
 * @file screensaver_blox.cpp
 * @brief Falling triangle-body stack screensaver.
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
constexpr std::int16_t kCell = 8;
constexpr std::int8_t kColumns = 16;
constexpr std::int8_t kRows = 8;

struct CellOffset final { std::int8_t x; std::int8_t y; };
struct Shape final { std::array<CellOffset, 4U> cells; std::uint8_t count; };
constexpr std::array<Shape, 6U> kShapes{{
    {{{{0,0},{1,0},{0,1},{1,1}}},4U},
    {{{{0,0},{0,1},{1,1},{2,1}}},4U},
    {{{{0,1},{1,1},{1,0},{2,0}}},4U},
    {{{{0,0},{1,0},{2,0},{1,1}}},4U},
    {{{{0,0},{0,1},{0,2},{1,2}}},4U},
    {{{{0,0},{1,0},{2,0},{3,0}}},4U}
}};

std::uint32_t nextRandom(std::uint32_t& state) {
    state ^= state << 13U; state ^= state >> 17U; state ^= state << 5U; return state;
}

bool occupied(const std::array<std::uint16_t, 8U>& rows, const std::int8_t x, const std::int8_t y) {
    if (x < 0 || x >= kColumns || y >= kRows) return true;
    if (y < 0) return false;
    return (rows[static_cast<std::size_t>(y)] & (static_cast<std::uint16_t>(1U) << static_cast<std::uint8_t>(x))) != 0U;
}

bool collides(const std::array<std::uint16_t, 8U>& rows, const Shape& shape, const std::int8_t baseX, const std::int8_t baseY) {
    for (std::uint8_t i = 0U; i < shape.count; ++i) {
        if (occupied(rows, static_cast<std::int8_t>(baseX + shape.cells[i].x), static_cast<std::int8_t>(baseY + shape.cells[i].y))) return true;
    }
    return false;
}

void drawTriangle(hal::OledDisplay& display, const std::int8_t cellX, const std::int8_t cellY, const bool flip) {
    if (cellY < 0 || cellY >= kRows || cellX < 0 || cellX >= kColumns) return;
    const std::int16_t x = static_cast<std::int16_t>(cellX * kCell);
    const std::int16_t y = static_cast<std::int16_t>(cellY * kCell);
    if (!flip) {
        display.drawLine(x, static_cast<std::int16_t>(y + 7), static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 7));
        display.drawLine(x, static_cast<std::int16_t>(y + 7), static_cast<std::int16_t>(x + 7), y);
        display.drawLine(static_cast<std::int16_t>(x + 7), y, static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 7));
    } else {
        display.drawLine(x, y, static_cast<std::int16_t>(x + 7), y);
        display.drawLine(x, y, static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 7));
        display.drawLine(static_cast<std::int16_t>(x + 7), y, static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 7));
    }
}
}  // namespace

void ScreensaverRenderer::renderBlox(const std::uint32_t frameIndex) {
    if (lastBloxFrameIndex_ == kInvalidFrameIndex || frameIndex < lastBloxFrameIndex_) {
        bloxRows_.fill(0U); bloxActive_ = false; bloxFullHoldFrames_ = 0U;
    }
    if (frameIndex != lastBloxFrameIndex_) {
        if (bloxFullHoldFrames_ > 0U) {
            --bloxFullHoldFrames_;
            if (bloxFullHoldFrames_ == 0U) { bloxRows_.fill(0U); bloxActive_ = false; }
        } else {
            if (!bloxActive_) {
                bloxShape_ = static_cast<std::int8_t>(nextRandom(bloxRandomState_) % kShapes.size());
                const Shape& shape = kShapes[static_cast<std::size_t>(bloxShape_)];
                std::int8_t maxX = 0;
                for (std::uint8_t i = 0U; i < shape.count; ++i) if (shape.cells[i].x > maxX) maxX = shape.cells[i].x;
                const std::uint8_t span = static_cast<std::uint8_t>(kColumns - maxX);
                bloxCellX_ = static_cast<std::int8_t>(nextRandom(bloxRandomState_) % span);
                bloxCellY_ = -3;
                bloxActive_ = true;
            }
            if (bloxActive_ && (frameIndex & 1U) == 0U) {
                const Shape& shape = kShapes[static_cast<std::size_t>(bloxShape_)];
                const std::int8_t nextY = static_cast<std::int8_t>(bloxCellY_ + 1);
                if (!collides(bloxRows_, shape, bloxCellX_, nextY)) {
                    bloxCellY_ = nextY;
                } else {
                    bool aboveTop = false;
                    for (std::uint8_t i = 0U; i < shape.count; ++i) {
                        const std::int8_t x = static_cast<std::int8_t>(bloxCellX_ + shape.cells[i].x);
                        const std::int8_t y = static_cast<std::int8_t>(bloxCellY_ + shape.cells[i].y);
                        if (y < 0) { aboveTop = true; continue; }
                        if (x >= 0 && x < kColumns && y < kRows) bloxRows_[static_cast<std::size_t>(y)] |= static_cast<std::uint16_t>(1U << static_cast<std::uint8_t>(x));
                    }
                    bloxActive_ = false;
                    if (aboveTop || bloxRows_[0] != 0U) bloxFullHoldFrames_ = 12U;
                }
            }
        }
        lastBloxFrameIndex_ = frameIndex;
    }

    display_.clear();
    for (std::int8_t y = 0; y < kRows; ++y) {
        for (std::int8_t x = 0; x < kColumns; ++x) {
            if ((bloxRows_[static_cast<std::size_t>(y)] & (static_cast<std::uint16_t>(1U) << static_cast<std::uint8_t>(x))) != 0U) {
                drawTriangle(display_, x, y, ((x + y) & 1) != 0);
            }
        }
    }
    if (bloxActive_) {
        const Shape& shape = kShapes[static_cast<std::size_t>(bloxShape_)];
        for (std::uint8_t i = 0U; i < shape.count; ++i) drawTriangle(display_, static_cast<std::int8_t>(bloxCellX_ + shape.cells[i].x), static_cast<std::int8_t>(bloxCellY_ + shape.cells[i].y), (i & 1U) != 0U);
    }
    display_.present();
}

}  // namespace clockfw::ui
