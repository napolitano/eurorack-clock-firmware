/**
 * @file screensaver_fireworks.cpp
 * @brief Pixel-rocket and compact burst screensaver.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/screensaver_renderer.h"

#include <array>
#include <cstdint>

namespace clockfw::ui {
namespace {

struct Direction final {
    std::int8_t x;
    std::int8_t y;
};

constexpr std::array<std::int16_t, 10U> kLaunchPositions{{12, 28, 44, 61, 78, 96, 113, 52, 87, 20}};
constexpr std::array<Direction, 16U> kDirections{{
    {0,-16},{6,-15},{11,-11},{15,-6},{16,0},{15,6},{11,11},{6,15},
    {0,16},{-6,15},{-11,11},{-15,6},{-16,0},{-15,-6},{-11,-11},{-6,-15}}};
constexpr std::uint32_t kCycleFrames = 28U;
constexpr std::uint32_t kLaunchFrames = 10U;
constexpr std::uint32_t kBurstFrames = 13U;

std::uint32_t fireworkHash(std::uint32_t value) {
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    value *= 0x846CA68BU;
    return value ^ (value >> 16U);
}

}  // namespace

void ScreensaverRenderer::renderFireworks(const std::uint32_t frameIndex) {
    display_.clear();
    const std::uint32_t cycle = frameIndex / kCycleFrames;
    const std::uint32_t cycleFrame = frameIndex % kCycleFrames;
    const std::uint32_t seed = fireworkHash(0x46495245U + cycle * 0x9E3779B9U);
    const std::int16_t launchX = kLaunchPositions[cycle % kLaunchPositions.size()];
    const std::int16_t targetY = static_cast<std::int16_t>(8U + ((seed >> 8U) % 12U));
    const std::int16_t drift = static_cast<std::int16_t>(static_cast<std::int32_t>(seed % 11U) - 5);
    const std::int16_t burstX = static_cast<std::int16_t>(launchX + drift);

    if (cycleFrame < kLaunchFrames) {
        const std::int32_t travel = static_cast<std::int32_t>(hal::OledDisplay::kHeight - 1 - targetY);
        const std::int16_t y = static_cast<std::int16_t>(
            hal::OledDisplay::kHeight - 1 -
            (travel * static_cast<std::int32_t>(cycleFrame + 1U)) / static_cast<std::int32_t>(kLaunchFrames));
        const std::int16_t x = static_cast<std::int16_t>(
            launchX + (static_cast<std::int32_t>(drift) * static_cast<std::int32_t>(cycleFrame + 1U)) /
                static_cast<std::int32_t>(kLaunchFrames));
        display_.setPixel(x, y);
    } else if (cycleFrame < kLaunchFrames + kBurstFrames) {
        const std::uint32_t age = cycleFrame - kLaunchFrames;
        if (age == 0U) display_.fillRectangle(burstX - 1, targetY - 1, 3, 3);
        const std::int32_t radius = static_cast<std::int32_t>(age + 1U);
        const std::int32_t gravity = static_cast<std::int32_t>((age * age) / 10U);
        const std::size_t rotation = static_cast<std::size_t>(seed % kDirections.size());
        for (std::size_t particle = 0U; particle < kDirections.size(); ++particle) {
            const std::uint32_t particleSeed = fireworkHash(seed + static_cast<std::uint32_t>(particle) * 31U);
            if (age > 8U && ((particleSeed >> (age & 7U)) & 1U) == 0U) continue;
            const Direction direction = kDirections[(particle + rotation) % kDirections.size()];
            const std::int16_t x = static_cast<std::int16_t>(
                burstX + (static_cast<std::int32_t>(direction.x) * radius) / 16);
            const std::int16_t y = static_cast<std::int16_t>(
                targetY + (static_cast<std::int32_t>(direction.y) * radius) / 16 + gravity);
            display_.setPixel(x, y);
        }
    }

    display_.present();
}

}  // namespace clockfw::ui
