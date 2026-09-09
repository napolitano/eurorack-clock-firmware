/**
 * @file screensaver_renderer.h
 * @brief STOP-mode OLED idle-animation renderer.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstdint>
#include <limits>

#include "domain/clock_types.h"
#include "hal/oled_display.h"

namespace clockfw::ui {

/** @brief Renders low-frequency idle effects without participating in clock timing. */
class ScreensaverRenderer final {
public:
    /** @brief Constructs the renderer around the display HAL. */
    explicit ScreensaverRenderer(hal::OledDisplay& display);

    /**
     * @brief Renders one frame of the selected STOP-mode idle effect.
     * @param mode Configured screensaver mode.
     * @param frameIndex Monotonic animation frame number.
     */
    void render(ScreensaverMode mode, std::uint32_t frameIndex);

private:
    /** @brief Renders a progressively revealed fixed-point Barnsley-fern crop. */
    void renderFractal(std::uint32_t frameIndex);

    /** @brief Selects a different curated fern crop for the next growth cycle. */
    void selectNextFernViewport();

    /** @brief Renders a sparse, slowly moving orbital line animation. */
    void renderOrbit(std::uint32_t frameIndex);

    /** @brief Renders a damped plucked-string animation. */
    void renderPlug(std::uint32_t frameIndex);

    /** @brief Renders eight independently phased digital clock traces. */
    void renderClock(std::uint32_t frameIndex);

    /** @brief Renders a cyclic pulsating heart outline. */
    void renderHeartbeat(std::uint32_t frameIndex);

    /** @brief Renders a gravity-influenced bouncing and rotating acid smiley. */
    void renderAcid(std::uint32_t frameIndex);

    /** @brief Renders a synthetic WinAmp-style spectrum analyser with peak hold/decay. */
    void renderSpectrum(std::uint32_t frameIndex);

    /** @brief Renders animated metaball-like scalar-field isocontours. */
    void renderField(std::uint32_t frameIndex);

    /** @brief Renders falling triangle-built bodies that accumulate on a simple cell stack. */
    void renderBlox(std::uint32_t frameIndex);

    /** @brief Renders original monochrome digital-rain columns with procedural glyphs. */
    void renderMatrix(std::uint32_t frameIndex);

    /** @brief Covers the display with 8x8 cube tiles using changing traversal patterns. */
    void renderCubeCover(std::uint32_t frameIndex);

    static constexpr std::uint32_t kInvalidFrameIndex =
        std::numeric_limits<std::uint32_t>::max();

    hal::OledDisplay& display_;
    std::uint32_t fernViewportRandomState_ = 0xA341316CU;
    std::uint32_t lastFractalFrameIndex_ = kInvalidFrameIndex;
    std::uint8_t fernViewportIndex_ = 0U;
    std::uint32_t lastAcidFrameIndex_ = kInvalidFrameIndex;
    std::int32_t acidXQ8_ = 28 * 256;
    std::int32_t acidYQ8_ = 18 * 256;
    std::int32_t acidVelocityXQ8_ = 330;
    std::int32_t acidVelocityYQ8_ = -390;
    std::int32_t acidAngleQ8_ = 0;
    std::int32_t acidAngularVelocityQ8_ = 110;
    std::uint32_t lastSpectrumFrameIndex_ = kInvalidFrameIndex;
    std::uint32_t spectrumRandomState_ = 0x51EC7A2DU;
    std::array<std::uint8_t, 16U> spectrumLevels_{};
    std::array<std::uint8_t, 16U> spectrumPeaks_{};
    std::array<std::uint8_t, 16U> spectrumPeakHold_{};
    std::uint32_t lastBloxFrameIndex_ = kInvalidFrameIndex;
    std::uint32_t bloxRandomState_ = 0xB10C5EEDU;
    std::array<std::uint16_t, 8U> bloxRows_{};
    std::int8_t bloxShape_ = 0;
    std::int8_t bloxCellX_ = 6;
    std::int8_t bloxCellY_ = -2;
    std::uint8_t bloxFullHoldFrames_ = 0U;
    bool bloxActive_ = false;
};

}  // namespace clockfw::ui
