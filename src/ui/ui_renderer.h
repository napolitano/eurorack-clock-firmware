/**
 * @file ui_renderer.h
 * @brief Top-level UI rendering dispatcher and boot-screen renderer.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

#include "domain/clock_types.h"
#include "engine/clock_engine.h"
#include "hal/oled_display.h"
#include "services/persistent_state_service.h"
#include "ui/channel_navigation_renderer.h"
#include "ui/performance_renderer.h"
#include "ui/settings_renderer.h"
#include "ui/screensaver_renderer.h"
#include "ui/ui_types.h"

namespace clockfw::ui {

/**
 * @brief Coordinates screen-specific renderers without owning navigation or application logic.
 *
 * Keeping this class as a small dispatcher prevents screen drawing, input handling, and
 * musical state mutation from accumulating in one monolithic UI object.
 */
class UiRenderer final {
public:
    /**
     * @brief Constructs all screen renderers around one OLED HAL instance.
     * @param display Display abstraction receiving the actual drawing commands.
     */
    UiRenderer(hal::OledDisplay& display, const services::PersistentStateService& persistentState);

    /**
     * @brief Renders the screen selected by the current navigation state.
     * @param state Complete musical/application configuration.
     * @param navigation Current UI navigation state.
     * @param engineSnapshot Read-only real-time timing snapshot.
     */
    void render(
        const ClockState& state,
        const NavigationState& navigation,
        const engine::EngineSnapshot& engineSnapshot);

    /**
     * @brief Renders the one-second boot screen using the generated 1-bit CLOCK logo bitmap and progress bar.
     * @param elapsedMs Milliseconds elapsed in the boot-screen interval.
     */
    void renderBootScreen(std::uint32_t elapsedMs);

    /** @brief Renders one STOP-mode screensaver frame. */
    void renderScreensaver(ScreensaverMode mode, std::uint32_t frameIndex);

    /** @brief Applies normal or dimmed OLED contrast. */
    void setDisplayDimmed(bool dimmed);

    /** @brief Switches the OLED panel on or off without clearing display RAM. */
    void setDisplayPower(bool enabled);

private:
    hal::OledDisplay& display_;
    const services::PersistentStateService& persistentState_;
    PerformanceRenderer performanceRenderer_;
    ChannelNavigationRenderer channelNavigationRenderer_;
    SettingsRenderer settingsRenderer_;
    ScreensaverRenderer screensaverRenderer_;
};

}  // namespace clockfw::ui
