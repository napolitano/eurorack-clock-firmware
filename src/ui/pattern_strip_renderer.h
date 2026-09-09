/**
 * @file pattern_strip_renderer.h
 * @brief Compact playback pattern visualizations for Euclid and sequencer modes.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

#include "domain/clock_types.h"
#include "hal/oled_display.h"

namespace clockfw::ui {

/** @brief Draws non-interactive rhythm strips shared by performance and editor screens. */
class PatternStripRenderer final {
public:
    /**
     * @brief Constructs the helper around the shared OLED framebuffer.
     * @param display Display HAL receiving all drawing primitives.
     */
    explicit PatternStripRenderer(hal::OledDisplay& display);

    /**
     * @brief Draws the active sixteen-step Euclidean playback block.
     * @param settings Euclidean Steps/Hits/Rotation configuration.
     * @param currentStep Zero-based currently sounding step.
     * @param topY Top pixel row of the pattern marks.
     */
    void drawEuclidPattern(
        const EuclidSettings& settings,
        std::uint8_t currentStep,
        std::int16_t topY);

    /**
     * @brief Draws the currently playing 16-step sequencer block.
     * @param settings Sequencer length, rotation, and binary pattern.
     * @param currentStep Zero-based currently sounding step.
     * @param topY Top pixel row of the 16 step cells.
     */
    void drawSequencerPlaybackBlock(
        const SequencerSettings& settings,
        std::uint8_t currentStep,
        std::int16_t topY);

    /**
     * @brief Draws 16-step block indicators at the two bottom OLED rows.
     * @param sequenceLength Active sequence length in steps.
     * @param activeStep Zero-based playback step used to select the active block.
     *
     * Inactive blocks use a one-pixel line on row 63. The active block is a
     * two-pixel-high line on rows 62-63, making playback position visible without
     * consuming normal content rows.
     */
    void drawSequencerBlockIndicator(
        std::uint8_t sequenceLength,
        std::uint8_t activeStep);

private:
    /** @brief Draws shared 16-step block progress for long Euclid/Sequencer patterns. */
    void drawPatternBlockIndicator(
        std::uint8_t sequenceLength,
        std::uint8_t activeStep);

    /** @brief Draws a two-pixel-high underline for one current pattern slot. */
    void drawCurrentStepUnderline(
        std::int16_t slotX,
        std::int16_t slotWidth);

    hal::OledDisplay& display_;
};

}  // namespace clockfw::ui
