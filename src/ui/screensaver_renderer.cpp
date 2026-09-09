/**
 * @file screensaver_renderer.cpp
 * @brief STOP-mode OLED idle-animation rendering implementation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/screensaver_renderer.h"

#include <algorithm>
#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <utility>

namespace clockfw::ui {
namespace {

/** Q12 fixed-point unit used by the fractal renderer. */
constexpr std::int32_t kFixedOne = 4096;

/** Number of animation frames in one complete fern-growth cycle. */
constexpr std::uint32_t kFernCycleFrames = 48U;

/** Iterations hidden before plotting so the attractor has settled. */
constexpr std::uint32_t kFernBurnInIterations = 24U;

/** First visible frame already contains enough structure to be recognizable. */
constexpr std::uint32_t kFernInitialPoints = 96U;

/** Additional Barnsley iterations revealed on every 400-ms screensaver frame. */
constexpr std::uint32_t kFernPointsPerFrame = 28U;

/** One deliberately composed crop of the Barnsley attractor. */
struct FernViewport final {
    std::int32_t minX;
    std::int32_t maxX;
    std::int32_t minY;
    std::int32_t maxY;
};

/**
 * Curated views of the fern. Randomising arbitrary coordinates can yield empty
 * or visually weak OLED frames, so variety is selected only from crops that
 * retain a useful amount of recognisable leaflet structure.
 */
constexpr std::array<FernViewport, 6U> kFernViewports{{
    {-10'240, 11'469, 8'192, 41'370},   // broad crown
    {-7'782, 7'782, 21'299, 41'574},    // upper crown
    {-11'264, 2'867, 15'974, 38'912},   // left leaflets
    {-2'662, 11'674, 15'565, 38'912},   // right leaflets
    {-9'421, 9'421, 11'878, 35'226},    // middle crown
    {-10'650, 10'650, 4'096, 25'805},   // lower branching
}};

/** Compact signed sine table spanning one full turn in 32 samples. */
constexpr std::array<std::int8_t, 32U> kSineTable{{
    0, 25, 49, 71, 90, 106, 117, 125,
    127, 125, 117, 106, 90, 71, 49, 25,
    0, -25, -49, -71, -90, -106, -117, -125,
    -127, -125, -117, -106, -90, -71, -49, -25}};

/** Returns a signed table sample with wrap-around. */
std::int8_t sineSample(const std::uint32_t phase) {
    return kSineTable[phase % kSineTable.size()];
}

/** Deterministic small PRNG used only to select Barnsley affine transforms. */
std::uint32_t nextFernRandom(std::uint32_t& state) {
    state = state * 1'664'525U + 1'013'904'223U;
    return state;
}

/** Maps one Q12 fern point into the deliberately cropped OLED viewport.
 * Coordinates outside the crop deliberately map outside the framebuffer; the
 * OLED primitive clips them, keeping the crop path branch-free.
 */
void mapFernPoint(
    const std::int32_t x,
    const std::int32_t y,
    const FernViewport& viewport,
    std::int16_t& screenX,
    std::int16_t& screenY) {
    const std::int32_t width = viewport.maxX - viewport.minX;
    const std::int32_t height = viewport.maxY - viewport.minY;
    screenX = static_cast<std::int16_t>(
        (static_cast<std::int64_t>(x - viewport.minX) * (hal::OledDisplay::kWidth - 1)) / width);
    screenY = static_cast<std::int16_t>(
        (hal::OledDisplay::kHeight - 1) -
        (static_cast<std::int64_t>(y - viewport.minY) * (hal::OledDisplay::kHeight - 1)) / height);
}

}  // namespace

ScreensaverRenderer::ScreensaverRenderer(hal::OledDisplay& display) : display_(display) {}

void ScreensaverRenderer::render(
    const ScreensaverMode mode,
    const std::uint32_t frameIndex) {
    switch (mode) {
        case ScreensaverMode::Fractal:
            renderFractal(frameIndex);
            break;
        case ScreensaverMode::Orbit:
            renderOrbit(frameIndex);
            break;
        case ScreensaverMode::Plug:
            renderPlug(frameIndex);
            break;
        case ScreensaverMode::Clock:
            renderClock(frameIndex);
            break;
        case ScreensaverMode::Heartbeat:
            renderHeartbeat(frameIndex);
            break;
        case ScreensaverMode::Acid:
            renderAcid(frameIndex);
            break;
        case ScreensaverMode::Spectrum:
            renderSpectrum(frameIndex);
            break;
        case ScreensaverMode::Field:
            renderField(frameIndex);
            break;
        case ScreensaverMode::Blox:
            renderBlox(frameIndex);
            break;
        case ScreensaverMode::Matrix:
            renderMatrix(frameIndex);
            break;
        case ScreensaverMode::CubeCover:
            renderCubeCover(frameIndex);
            break;
        case ScreensaverMode::None:
        default:
            break;
    }
}

void ScreensaverRenderer::renderFractal(const std::uint32_t frameIndex) {
    display_.clear();

    // Keep the mathematically deterministic fern, but choose among curated
    // decorative crops at each growth-cycle boundary. Renderer state survives
    // STOP-mode wake/sleep cycles, so a later activation does not always start
    // with the same composition.
    const std::uint32_t cycleFrame = frameIndex % kFernCycleFrames;
    const bool firstFractalFrame = lastFractalFrameIndex_ == kInvalidFrameIndex;
    const bool restarted = !firstFractalFrame && frameIndex < lastFractalFrameIndex_;
    const bool newGrowthCycle =
        cycleFrame == 0U && frameIndex != lastFractalFrameIndex_;
    if (firstFractalFrame || restarted || newGrowthCycle) {
        selectNextFernViewport();
    }
    lastFractalFrameIndex_ = frameIndex;
    const FernViewport& viewport = kFernViewports[fernViewportIndex_];
    const std::uint32_t visiblePoints =
        kFernInitialPoints + cycleFrame * kFernPointsPerFrame;
    const std::uint32_t totalIterations = kFernBurnInIterations + visiblePoints;

    std::int32_t x = 0;
    std::int32_t y = 0;
    std::uint32_t randomState = 0x6D2B79F5U;

    for (std::uint32_t iteration = 0U; iteration < totalIterations; ++iteration) {
        const std::uint32_t selector = nextFernRandom(randomState) % 100U;
        const std::int32_t previousX = x;
        const std::int32_t previousY = y;

        if (selector < 1U) {
            // Stem: x' = 0, y' = 0.16y.
            x = 0;
            y = static_cast<std::int32_t>((655LL * previousY) / kFixedOne);
        } else if (selector < 86U) {
            // Dominant leaflet transform.
            x = static_cast<std::int32_t>(
                (3'482LL * previousX + 164LL * previousY) / kFixedOne);
            y = static_cast<std::int32_t>(
                (-164LL * previousX + 3'482LL * previousY) / kFixedOne) + 6'554;
        } else if (selector < 93U) {
            // Left leaflet transform.
            x = static_cast<std::int32_t>(
                (819LL * previousX - 1'065LL * previousY) / kFixedOne);
            y = static_cast<std::int32_t>(
                (942LL * previousX + 901LL * previousY) / kFixedOne) + 6'554;
        } else {
            // Right leaflet transform.
            x = static_cast<std::int32_t>(
                (-614LL * previousX + 1'147LL * previousY) / kFixedOne);
            y = static_cast<std::int32_t>(
                (1'065LL * previousX + 983LL * previousY) / kFixedOne) + 1'802;
        }

        if (iteration >= kFernBurnInIterations) {
            std::int16_t screenX = 0;
            std::int16_t screenY = 0;
            mapFernPoint(x, y, viewport, screenX, screenY);
            display_.setPixel(screenX, screenY);
        }
    }

    display_.present();
}

void ScreensaverRenderer::selectNextFernViewport() {
    const std::uint8_t previousIndex = fernViewportIndex_;
    fernViewportRandomState_ = nextFernRandom(fernViewportRandomState_);
    std::uint8_t candidate = static_cast<std::uint8_t>(
        fernViewportRandomState_ % static_cast<std::uint32_t>(kFernViewports.size()));
    if (candidate == previousIndex) {
        candidate = static_cast<std::uint8_t>(
            (static_cast<std::size_t>(candidate) + 1U) % kFernViewports.size());
    }
    fernViewportIndex_ = candidate;
}

void ScreensaverRenderer::renderOrbit(const std::uint32_t frameIndex) {
    display_.clear();

    constexpr std::int16_t centerX = hal::OledDisplay::kWidth / 2;
    constexpr std::int16_t centerY = hal::OledDisplay::kHeight / 2;

    std::int16_t previousX = centerX;
    std::int16_t previousY = centerY;
    for (std::uint8_t point = 0U; point < 5U; ++point) {
        const std::uint32_t phase = frameIndex + point * 6U;
        const std::int16_t x = static_cast<std::int16_t>(
            centerX + static_cast<std::int16_t>(sineSample(phase)) * (22 + point * 4) / 127);
        const std::int16_t y = static_cast<std::int16_t>(
            centerY + static_cast<std::int16_t>(sineSample(phase * 3U + 8U)) * (10 + point * 2) / 127);
        display_.drawLine(previousX, previousY, x, y);
        display_.fillRectangle(x - 1, y - 1, 3, 3);
        previousX = x;
        previousY = y;
    }

    display_.present();
}


void ScreensaverRenderer::renderPlug(const std::uint32_t frameIndex) {
    display_.clear();

    constexpr std::int16_t kLeft = 3;
    constexpr std::int16_t kRight = 124;
    constexpr std::int16_t kCenterY = 32;
    constexpr std::int16_t kPluckX = 43;
    constexpr std::uint32_t kCycleFrames = 64U;
    const std::uint32_t cycle = frameIndex % kCycleFrames;
    const std::int32_t decay = static_cast<std::int32_t>(64U - cycle);
    const std::int32_t temporal = static_cast<std::int32_t>(sineSample(frameIndex * 3U));

    std::int16_t previousX = kLeft;
    std::int16_t previousY = kCenterY;
    for (std::int16_t x = static_cast<std::int16_t>(kLeft + 2); x <= kRight; x = static_cast<std::int16_t>(x + 2)) {
        const std::int32_t spatialNumerator = x <= kPluckX
            ? static_cast<std::int32_t>(x - kLeft) * 127
            : static_cast<std::int32_t>(kRight - x) * 127;
        const std::int32_t spatialDenominator = x <= kPluckX
            ? static_cast<std::int32_t>(kPluckX - kLeft)
            : static_cast<std::int32_t>(kRight - kPluckX);
        const std::int32_t spatial = spatialDenominator > 0
            ? spatialNumerator / spatialDenominator
            : 0;
        const std::int32_t harmonic = static_cast<std::int32_t>(
            sineSample(static_cast<std::uint32_t>(x + static_cast<std::int16_t>(frameIndex * 2U))));
        const std::int32_t displacement =
            (spatial * decay * (temporal * 3 + harmonic)) / (127 * 64 * 20);
        const std::int16_t y = static_cast<std::int16_t>(kCenterY + displacement);
        display_.drawLine(previousX, previousY, x, y);
        previousX = x;
        previousY = y;
    }
    display_.fillRectangle(kLeft - 1, kCenterY - 4, 2, 9);
    display_.fillRectangle(kRight, kCenterY - 4, 2, 9);
    display_.present();
}

void ScreensaverRenderer::renderClock(const std::uint32_t frameIndex) {
    display_.clear();
    constexpr std::array<std::uint8_t, 8U> kPeriods{{8U, 12U, 16U, 20U, 24U, 28U, 36U, 44U}};
    constexpr std::array<std::uint8_t, 8U> kDuty{{4U, 3U, 8U, 5U, 12U, 7U, 9U, 15U}};
    constexpr std::array<std::uint8_t, 8U> kPhase{{0U, 3U, 7U, 2U, 11U, 5U, 17U, 23U}};

    for (std::uint8_t channel = 0U; channel < 8U; ++channel) {
        const std::int16_t baseY = static_cast<std::int16_t>(5 + static_cast<int>(channel) * 7);
        std::int16_t previousY = baseY;
        for (std::int16_t x = 0; x < static_cast<std::int16_t>(hal::OledDisplay::kWidth); ++x) {
            const std::uint32_t phase = frameIndex * 2U + static_cast<std::uint32_t>(x) + kPhase[channel];
            const bool high = (phase % kPeriods[channel]) < kDuty[channel];
            const std::int16_t y = static_cast<std::int16_t>(baseY - (high ? 4 : 0));
            if (x > 0) {
                if (y != previousY) {
                    display_.drawVerticalLine(x, std::min(y, previousY), 5);
                }
                display_.setPixel(x, y);
            }
            previousY = y;
        }
    }
    display_.present();
}

void ScreensaverRenderer::renderHeartbeat(const std::uint32_t frameIndex) {
    display_.clear();
    constexpr std::array<std::int16_t, 12U> kPulseScale{{16, 18, 23, 20, 17, 16, 16, 17, 20, 18, 16, 16}};
    const std::int16_t scale = kPulseScale[frameIndex % kPulseScale.size()];
    constexpr std::array<std::pair<std::int16_t, std::int16_t>, 12U> kHeart{{
        {-8, -3}, {-6, -7}, {-2, -8}, {0, -5}, {2, -8}, {6, -7},
        {8, -3}, {7, 1}, {4, 5}, {0, 9}, {-4, 5}, {-7, 1}}};
    constexpr std::int16_t centerX = 64;
    constexpr std::int16_t centerY = 31;

    auto scaledPoint = [scale](const std::pair<std::int16_t, std::int16_t>& point) {
        return std::pair<std::int16_t, std::int16_t>{
            static_cast<std::int16_t>(centerX + (static_cast<std::int32_t>(point.first) * scale) / 16),
            static_cast<std::int16_t>(centerY + (static_cast<std::int32_t>(point.second) * scale) / 16)};
    };

    for (std::size_t index = 0U; index < kHeart.size(); ++index) {
        const auto first = scaledPoint(kHeart[index]);
        const auto second = scaledPoint(kHeart[(index + 1U) % kHeart.size()]);
        display_.drawLine(first.first, first.second, second.first, second.second);
    }
    if ((frameIndex % 12U) == 2U || (frameIndex % 12U) == 8U) {
        display_.setPixel(centerX, centerY);
        display_.setPixel(centerX - 1, centerY);
        display_.setPixel(centerX + 1, centerY);
    }
    display_.present();
}

void ScreensaverRenderer::renderAcid(const std::uint32_t frameIndex) {
    constexpr std::int32_t kScale = 256;
    constexpr std::int16_t kRadius = 10;
    constexpr std::int32_t kMinX = (kRadius + 1) * kScale;
    constexpr std::int32_t kMaxX = (hal::OledDisplay::kWidth - kRadius - 2) * kScale;
    constexpr std::int32_t kMinY = (kRadius + 1) * kScale;
    constexpr std::int32_t kMaxY = (hal::OledDisplay::kHeight - kRadius - 2) * kScale;
    constexpr std::int32_t kGravityQ8 = 24;

    if (lastAcidFrameIndex_ == kInvalidFrameIndex || frameIndex <= lastAcidFrameIndex_) {
        acidXQ8_ = 28 * kScale;
        acidYQ8_ = 18 * kScale;
        acidVelocityXQ8_ = 330;
        acidVelocityYQ8_ = -390;
        acidAngleQ8_ = 0;
        acidAngularVelocityQ8_ = 110;
        lastAcidFrameIndex_ = 0U;
    }
    const std::uint32_t steps = std::min<std::uint32_t>(64U, frameIndex - lastAcidFrameIndex_);
    for (std::uint32_t step = 0U; step < steps; ++step) {
        acidVelocityYQ8_ += kGravityQ8;
        acidXQ8_ += acidVelocityXQ8_;
        acidYQ8_ += acidVelocityYQ8_;
        acidAngleQ8_ += acidAngularVelocityQ8_;
        acidAngularVelocityQ8_ = acidAngularVelocityQ8_ * 253 / 256;

        if (acidXQ8_ < kMinX) {
            acidXQ8_ = kMinX; acidVelocityXQ8_ = std::abs(acidVelocityXQ8_);
            acidAngularVelocityQ8_ = std::clamp<std::int32_t>(acidAngularVelocityQ8_ - acidVelocityYQ8_ / 5, -520, 520);
        } else if (acidXQ8_ > kMaxX) {
            acidXQ8_ = kMaxX; acidVelocityXQ8_ = -std::abs(acidVelocityXQ8_);
            acidAngularVelocityQ8_ = std::clamp<std::int32_t>(acidAngularVelocityQ8_ + acidVelocityYQ8_ / 5, -520, 520);
        }
        if (acidYQ8_ < kMinY) {
            acidYQ8_ = kMinY; acidVelocityYQ8_ = std::abs(acidVelocityYQ8_);
            acidAngularVelocityQ8_ = std::clamp<std::int32_t>(acidAngularVelocityQ8_ + acidVelocityXQ8_ / 5, -520, 520);
        } else if (acidYQ8_ > kMaxY) {
            acidYQ8_ = kMaxY; acidVelocityYQ8_ = -std::abs(acidVelocityYQ8_) * 235 / 256;
            if (std::abs(acidVelocityYQ8_) < 300) acidVelocityYQ8_ = -390;
            acidAngularVelocityQ8_ = std::clamp<std::int32_t>(acidAngularVelocityQ8_ - acidVelocityXQ8_ / 4, -520, 520);
        }
    }
    lastAcidFrameIndex_ = frameIndex;

    display_.clear();
    const std::int16_t cx = static_cast<std::int16_t>(acidXQ8_ / kScale);
    const std::int16_t cy = static_cast<std::int16_t>(acidYQ8_ / kScale);
    const std::int32_t angleRaw = (acidAngleQ8_ / 256) % 32;
    const std::uint32_t angle = static_cast<std::uint32_t>(angleRaw < 0 ? angleRaw + 32 : angleRaw);
    auto rotate = [angle](const std::int16_t x, const std::int16_t y) {
        const std::int32_t c = sineSample(angle + 8U);
        const std::int32_t s = sineSample(angle);
        return std::pair<std::int16_t, std::int16_t>{
            static_cast<std::int16_t>((static_cast<std::int32_t>(x) * c - static_cast<std::int32_t>(y) * s) / 127),
            static_cast<std::int16_t>((static_cast<std::int32_t>(x) * s + static_cast<std::int32_t>(y) * c) / 127)};
    };

    std::pair<std::int16_t, std::int16_t> previous{};
    for (std::uint32_t point = 0U; point <= 16U; ++point) {
        const std::uint32_t phase = (point * 32U / 16U) % 32U;
        const std::int16_t x = static_cast<std::int16_t>(static_cast<std::int32_t>(sineSample(phase + 8U)) * kRadius / 127);
        const std::int16_t y = static_cast<std::int16_t>(static_cast<std::int32_t>(sineSample(phase)) * kRadius / 127);
        const auto r = rotate(x, y);
        const std::pair<std::int16_t, std::int16_t> current{static_cast<std::int16_t>(cx + r.first), static_cast<std::int16_t>(cy + r.second)};
        if (point > 0U) display_.drawLine(previous.first, previous.second, current.first, current.second);
        previous = current;
    }
    for (const auto& eye : std::array<std::pair<std::int16_t, std::int16_t>, 2U>{{{-4, -3}, {4, -3}}}) {
        const auto r = rotate(eye.first, eye.second);
        display_.fillRectangle(static_cast<std::int16_t>(cx + r.first - 1), static_cast<std::int16_t>(cy + r.second - 1), 2, 2);
    }
    constexpr std::array<std::pair<std::int16_t, std::int16_t>, 5U> kSmile{{{-5, 2}, {-3, 5}, {0, 6}, {3, 5}, {5, 2}}};
    auto previousMouth = rotate(kSmile[0].first, kSmile[0].second);
    for (std::size_t index = 1U; index < kSmile.size(); ++index) {
        const auto currentMouth = rotate(kSmile[index].first, kSmile[index].second);
        display_.drawLine(
            static_cast<std::int16_t>(cx + previousMouth.first), static_cast<std::int16_t>(cy + previousMouth.second),
            static_cast<std::int16_t>(cx + currentMouth.first), static_cast<std::int16_t>(cy + currentMouth.second));
        previousMouth = currentMouth;
    }
    display_.present();
}

}  // namespace clockfw::ui
