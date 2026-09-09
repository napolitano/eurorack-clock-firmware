/**
 * @file performance_renderer.h
 * @brief Rendering of the normal performance screen and compact playback status line.
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

/** @brief Renders the calm, tempo-focused performance view used during normal operation. */
class PerformanceRenderer final {
public:
    /**
     * @brief Constructs the renderer around the shared OLED abstraction.
     * @param display Display HAL receiving all drawing commands.
     */
    explicit PerformanceRenderer(hal::OledDisplay& display);

    /**
     * @brief Draws the complete performance screen and presents the framebuffer.
     * @param state Current user configuration and transport state.
     * @param navigation UI state identifying the currently focused channel.
     * @param engineSnapshot Read-only real-time timing information.
     */
    void render(
        const ClockState& state,
        const NavigationState& navigation,
        const engine::EngineSnapshot& engineSnapshot);

private:
    /** @brief Draws PLAY, PAUSE, or STOP right-aligned in the status line. */
    void drawTransport(TransportState transport);

    /** @brief Draws centered tempo/status text using the requested native-resolution font. */
    void drawTempoText(const char* text, hal::DisplayFont font, std::int16_t topY);

    /** @brief Draws channel/global operating context after the role badge. */
    void drawChannelStatus(
        std::uint8_t channelIndex,
        OperatingMode operatingMode,
        ChannelMode mode,
        std::int16_t leftEdgeX);

    /** @brief Draws the master/slave badge at the far left and optional external-lock icon. */
    void drawClockRoleStatus(ClockSource source, bool externalLocked);

    /** @brief Draws the eight effective divider-bank rates in a compact 2x4 slot grid. */
    void drawDividerBankSlots(const ClockState& state);

    /** @brief Draws a graphical multiply/divide symbol followed by the integer factor. */
    void drawDividerRateSlot(std::int16_t x, std::int16_t y, const RateSettings& rate);

    /** @brief Draws a compact 5x7 padlock icon. */
    void drawLockIcon(std::int16_t x, std::int16_t y);

    /** @brief Draws the ONE CLOCK humanize status icon. */
    void drawHumanizeIcon(std::int16_t x, std::int16_t y);

    /** @brief Returns the one-character O/C/E/S status abbreviation for one channel mode. */
    static char statusModeCharacter(ChannelMode mode);

    hal::OledDisplay& display_;
    PatternStripRenderer patternStripRenderer_;
};

}  // namespace clockfw::ui
