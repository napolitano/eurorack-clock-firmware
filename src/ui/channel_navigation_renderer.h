/**
 * @file channel_navigation_renderer.h
 * @brief Graphical channel, mode, and sequencer-editor screen rendering.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

#include "domain/clock_types.h"
#include "engine/clock_engine.h"
#include "hal/oled_display.h"
#include "ui/pattern_strip_renderer.h"
#include "ui/ui_types.h"

namespace clockfw::ui {

/** @brief Renders graphical navigation and pattern-editing screens. */
class ChannelNavigationRenderer final {
public:
    /**
     * @brief Constructs the renderer around the shared OLED abstraction.
     * @param display Display HAL receiving all drawing commands.
     */
    explicit ChannelNavigationRenderer(hal::OledDisplay& display);

    /**
     * @brief Draws the 2x4 channel overview and selection grid.
     * @param state Complete channel configuration.
     * @param navigation Current cursor and selected-channel state.
     */

    /**
     * @brief Draws the compact eight-channel overview and selector.
     * @param state Complete channel configuration used for mode symbols.
     * @param navigation Current candidate channel.
     */
    void renderChannelQuickSelect(
        const ClockState& state,
        const NavigationState& navigation);

    /**
     * @brief Draws the six-function 2x3 graphical mode palette.
     * @param navigation Current selected channel and mode cursor.
     */
    void renderModeSelect(const NavigationState& navigation);

    /** @brief Draws the explicit YES/NO confirmation required before changing modes. */
    void renderModeChangeConfirm(const NavigationState& navigation);

    /**
     * @brief Draws one 16-step window of a channel's 64-step sequencer.
     * @param state Complete channel configuration.
     * @param navigation Current page, channel, and step cursor.
     * @param engineSnapshot Current playback step used by the bottom block indicator.
     */
    void renderSequencerEditor(
        const ClockState& state,
        const NavigationState& navigation,
        const engine::EngineSnapshot& engineSnapshot);

private:
    /** @brief Draws the simplified overview shown while a global operating mode is active. */
    void renderGlobalModeOverview(const ClockState& state);

    /** @brief Draws the disabled-channel pictogram used by OFF mode selection. */
    void drawOffModeIcon(std::int16_t x, std::int16_t y, hal::PixelColor color);

    /** @brief Draws the square-wave pictogram used by CLOCK mode selection. */
    void drawClockModeIcon(std::int16_t x, std::int16_t y, hal::PixelColor color);

    /** @brief Draws the radial hit pictogram used by EUCLID mode selection. */
    void drawEuclidModeIcon(std::int16_t centerX, std::int16_t centerY, hal::PixelColor color);

    /** @brief Draws the miniature gate pattern used by SEQ mode selection. */
    void drawSequencerModeIcon(std::int16_t x, std::int16_t y, hal::PixelColor color);

    /** @brief Draws the one-clock-to-eight-outputs fan-out pictogram. */
    void drawUnifiedModeIcon(std::int16_t x, std::int16_t y, hal::PixelColor color);

    /** @brief Draws the multi-rate divider-bank pictogram. */
    void drawDividerModeIcon(std::int16_t x, std::int16_t y, hal::PixelColor color);

    /** @brief Draws a compact mode pictogram beside one CHn label in the overview. */
    void drawOverviewModeIcon(
        const ClockState& state,
        std::uint8_t channelIndex,
        std::int16_t x,
        std::int16_t y,
        hal::PixelColor color);

    /** @brief Draws one large palette icon for the supplied mode function. */
    void drawModeFunctionIcon(
        ModeFunction modeFunction,
        std::int16_t x,
        std::int16_t y,
        hal::PixelColor color);

    hal::OledDisplay& display_;
    PatternStripRenderer patternStripRenderer_;
};

}  // namespace clockfw::ui
