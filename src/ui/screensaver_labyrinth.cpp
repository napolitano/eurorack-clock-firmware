/**
 * @file screensaver_labyrinth.cpp
 * @brief Random perfect-maze screensaver for the 128x64 OLED.
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

constexpr std::uint8_t kNorth = 0x01U;
constexpr std::uint8_t kEast = 0x02U;
constexpr std::uint8_t kSouth = 0x04U;
constexpr std::uint8_t kWest = 0x08U;
constexpr std::uint8_t kAllWalls = kNorth | kEast | kSouth | kWest;
constexpr std::size_t kColumns = 16U;
constexpr std::size_t kRows = 8U;
constexpr std::size_t kCells = kColumns * kRows;
constexpr std::int16_t kCellSize = 8;
constexpr std::uint32_t kCycleFrames = 60U;
constexpr std::uint32_t kRevealFrames = 32U;

std::uint32_t mazeRandom(std::uint32_t& state) {
    state ^= state << 13U;
    state ^= state >> 17U;
    state ^= state << 5U;
    return state;
}

struct Neighbor final {
    std::uint8_t cell;
    std::uint8_t wallFromCurrent;
    std::uint8_t wallFromNeighbor;
};

void generateMaze(
    const std::uint32_t seed,
    std::array<std::uint8_t, kCells>& walls) {
    walls.fill(kAllWalls);
    std::array<bool, kCells> visited{};
    std::array<std::uint8_t, kCells> stack{};
    std::size_t stackSize = 0U;
    std::uint32_t randomState = seed == 0U ? 0x4D415A45U : seed;
    std::uint8_t current = static_cast<std::uint8_t>(mazeRandom(randomState) % kCells);
    visited[current] = true;
    std::size_t visitedCount = 1U;

    while (visitedCount < kCells) {
        const std::size_t column = static_cast<std::size_t>(current) % kColumns;
        const std::size_t row = static_cast<std::size_t>(current) / kColumns;
        std::array<Neighbor, 4U> candidates{};
        std::size_t candidateCount = 0U;

        if (row > 0U) {
            const auto neighbor = static_cast<std::uint8_t>(current - static_cast<std::uint8_t>(kColumns));
            if (!visited[neighbor]) candidates[candidateCount++] = {neighbor, kNorth, kSouth};
        }
        if (column + 1U < kColumns) {
            const auto neighbor = static_cast<std::uint8_t>(current + 1U);
            if (!visited[neighbor]) candidates[candidateCount++] = {neighbor, kEast, kWest};
        }
        if (row + 1U < kRows) {
            const auto neighbor = static_cast<std::uint8_t>(current + static_cast<std::uint8_t>(kColumns));
            if (!visited[neighbor]) candidates[candidateCount++] = {neighbor, kSouth, kNorth};
        }
        if (column > 0U) {
            const auto neighbor = static_cast<std::uint8_t>(current - 1U);
            if (!visited[neighbor]) candidates[candidateCount++] = {neighbor, kWest, kEast};
        }

        if (candidateCount == 0U) {
            if (stackSize == 0U) break;
            current = stack[--stackSize];
            continue;
        }

        const Neighbor selected = candidates[mazeRandom(randomState) % candidateCount];
        walls[current] = static_cast<std::uint8_t>(walls[current] & static_cast<std::uint8_t>(~selected.wallFromCurrent));
        walls[selected.cell] = static_cast<std::uint8_t>(walls[selected.cell] & static_cast<std::uint8_t>(~selected.wallFromNeighbor));
        stack[stackSize++] = current;
        current = selected.cell;
        visited[current] = true;
        ++visitedCount;
    }
}

void drawCellWalls(
    hal::OledDisplay& display,
    const std::array<std::uint8_t, kCells>& walls,
    const std::size_t cell) {
    const std::size_t column = cell % kColumns;
    const std::size_t row = cell / kColumns;
    const std::int16_t x = static_cast<std::int16_t>(column * static_cast<std::size_t>(kCellSize));
    const std::int16_t y = static_cast<std::int16_t>(row * static_cast<std::size_t>(kCellSize));
    const std::uint8_t wall = walls[cell];

    if ((wall & kNorth) != 0U) display.drawHorizontalLine(x, y, kCellSize);
    if ((wall & kWest) != 0U) display.drawVerticalLine(x, y, kCellSize);
    if (column + 1U == kColumns && (wall & kEast) != 0U) {
        display.drawVerticalLine(hal::OledDisplay::kWidth - 1, y, kCellSize);
    }
    if (row + 1U == kRows && (wall & kSouth) != 0U) {
        display.drawHorizontalLine(x, hal::OledDisplay::kHeight - 1, kCellSize);
    }
}

}  // namespace

void ScreensaverRenderer::renderLabyrinth(const std::uint32_t frameIndex) {
    display_.clear();
    const std::uint32_t cycle = frameIndex / kCycleFrames;
    const std::uint32_t cycleFrame = frameIndex % kCycleFrames;
    std::array<std::uint8_t, kCells> walls{};
    generateMaze(0x6C616279U ^ (cycle * 0x9E3779B9U), walls);

    const std::size_t visibleCells = cycleFrame < kRevealFrames
        ? static_cast<std::size_t>(((cycleFrame + 1U) * static_cast<std::uint32_t>(kCells) + kRevealFrames - 1U) / kRevealFrames)
        : kCells;
    const std::size_t offset = static_cast<std::size_t>((cycle * 37U) % static_cast<std::uint32_t>(kCells));
    const std::size_t step = static_cast<std::size_t>(((cycle * 10U) + 29U) | 1U);

    for (std::size_t order = 0U; order < visibleCells; ++order) {
        const std::size_t cell = (offset + order * step) % kCells;
        drawCellWalls(display_, walls, cell);
    }
    display_.present();
}

}  // namespace clockfw::ui
