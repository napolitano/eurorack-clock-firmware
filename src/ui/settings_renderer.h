/**
 * @file settings_renderer.h
 * @brief Rendering of scrollable settings and factory-template lists.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include "domain/clock_types.h"
#include "hal/oled_display.h"
#include "services/persistent_state_service.h"
#include "ui/ui_types.h"

namespace clockfw::ui {

/** @brief Renders compact text-based configuration screens using one consistent visual grammar. */
class SettingsRenderer final {
public:
    /**
     * @brief Constructs the renderer around the shared OLED abstraction.
     * @param display Display HAL receiving all drawing commands.
     */
    explicit SettingsRenderer(hal::OledDisplay& display);

    /**
     * @brief Draws the currently selected settings page.
     * @param state Complete application configuration used to build row values.
     * @param navigation Current settings page, cursor, scroll, and edit state.
     */
    void renderSettings(const ClockState& state, const NavigationState& navigation);

    /**
     * @brief Draws the factory-template selection list.
     * @param navigation Current template cursor and scroll state.
     */
    void renderTemplates(const NavigationState& navigation);

    /** @brief Draws the explicit user-preset slot list for load/save operations. */
    void renderPresetSlots(
        const NavigationState& navigation,
        const services::PersistentStateService& persistentState);

    /** @brief Draws the overwrite confirmation for an occupied user preset slot. */
    void renderOverwriteConfirm(
        const NavigationState& navigation,
        const services::PersistentStateService& persistentState);

    /** @brief Draws the 16-character encoder-driven preset-name editor and character band. */
    void renderNameEntry(const NavigationState& navigation);

private:
    hal::OledDisplay& display_;
};

}  // namespace clockfw::ui
