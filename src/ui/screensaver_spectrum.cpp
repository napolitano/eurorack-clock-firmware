/**
 * @file screensaver_spectrum.cpp
 * @brief Synthetic spectrum-analyser screensaver implementation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/screensaver_renderer.h"

#include <algorithm>
#include <array>
#include <cstdint>

namespace clockfw::ui {
namespace {
constexpr std::array<std::int8_t, 32U> kSpectrumSine{{
    0, 25, 49, 71, 90, 106, 117, 125,
    127, 125, 117, 106, 90, 71, 49, 25,
    0, -25, -49, -71, -90, -106, -117, -125,
    -127, -125, -117, -106, -90, -71, -49, -25}};

std::int8_t spectrumSine(const std::uint32_t phase) {
    return kSpectrumSine[phase % kSpectrumSine.size()];
}
}  // namespace

void ScreensaverRenderer::renderSpectrum(const std::uint32_t frameIndex) {
    constexpr std::uint8_t kBandCount = 16U;
    constexpr std::uint8_t kMaximumLevel = 54U;
    constexpr std::uint8_t kPeakHoldFrames = 3U;
    constexpr std::int16_t kBarWidth = 5;
    constexpr std::int16_t kBarPitch = 8;
    constexpr std::int16_t kBottomY = 62;

    if (lastSpectrumFrameIndex_ == kInvalidFrameIndex || frameIndex <= lastSpectrumFrameIndex_) {
        spectrumLevels_.fill(0U);
        spectrumPeaks_.fill(0U);
        spectrumPeakHold_.fill(0U);
        spectrumRandomState_ = 0x51EC7A2DU;
        lastSpectrumFrameIndex_ = 0U;
    }

    const std::uint32_t steps = std::min<std::uint32_t>(32U, frameIndex - lastSpectrumFrameIndex_);
    for (std::uint32_t step = 0U; step < steps; ++step) {
        const std::uint32_t absoluteFrame = lastSpectrumFrameIndex_ + step + 1U;
        for (std::uint8_t band = 0U; band < kBandCount; ++band) {
            spectrumRandomState_ = spectrumRandomState_ * 1'664'525U + 1'013'904'223U;
            const std::uint8_t noise = static_cast<std::uint8_t>((spectrumRandomState_ >> 27U) & 0x1FU);
            const std::int16_t lowWave = static_cast<std::int16_t>(spectrumSine(absoluteFrame * 2U + band * 2U));
            const std::int16_t highWave = static_cast<std::int16_t>(spectrumSine(absoluteFrame * 5U + band * 5U));
            const std::uint8_t spectralTilt = static_cast<std::uint8_t>(18U - band / 2U);
            const std::int16_t targetRaw = static_cast<std::int16_t>(
                12 + spectralTilt + noise / 2 + (lowWave + 127) / 13 + (highWave + 127) / 24);
            const std::uint8_t target = static_cast<std::uint8_t>(
                std::clamp<std::int16_t>(targetRaw, 2, kMaximumLevel));

            std::uint8_t& level = spectrumLevels_[band];
            if (target > level) {
                const std::uint8_t attack = static_cast<std::uint8_t>(std::min<int>(9, target - level));
                level = static_cast<std::uint8_t>(level + attack);
            } else {
                const std::uint8_t decay = static_cast<std::uint8_t>(std::min<int>(3, level - target));
                level = static_cast<std::uint8_t>(level - decay);
            }

            if (level >= spectrumPeaks_[band]) {
                spectrumPeaks_[band] = level;
                spectrumPeakHold_[band] = kPeakHoldFrames;
            } else if (spectrumPeakHold_[band] > 0U) {
                --spectrumPeakHold_[band];
            } else if (spectrumPeaks_[band] > 0U) {
                --spectrumPeaks_[band];
            }
        }
    }
    lastSpectrumFrameIndex_ = frameIndex;

    display_.clear();
    for (std::uint8_t band = 0U; band < kBandCount; ++band) {
        const std::int16_t x = static_cast<std::int16_t>(1 + static_cast<std::int16_t>(band) * kBarPitch);
        const std::int16_t height = static_cast<std::int16_t>(spectrumLevels_[band]);
        const std::int16_t top = static_cast<std::int16_t>(kBottomY - height + 1);

        for (std::int16_t y = kBottomY; y >= top; y = static_cast<std::int16_t>(y - 4)) {
            const std::int16_t segmentHeight = static_cast<std::int16_t>(
                std::min<std::int16_t>(3, static_cast<std::int16_t>(y - top + 1)));
            display_.fillRectangle(
                x,
                static_cast<std::int16_t>(y - segmentHeight + 1),
                kBarWidth,
                segmentHeight);
        }

        const std::int16_t peakY = static_cast<std::int16_t>(
            kBottomY - static_cast<std::int16_t>(spectrumPeaks_[band]));
        display_.drawHorizontalLine(x, peakY, kBarWidth);
    }
    display_.present();
}

}  // namespace clockfw::ui
