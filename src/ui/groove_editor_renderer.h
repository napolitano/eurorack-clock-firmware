/**
 * @file groove_editor_renderer.h
 * @brief Compact 128x64 Custom Groove editor and library rendering.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include "hal/oled_display.h"
#include "services/custom_groove_store.h"
#include "ui/ui_types.h"

namespace clockfw::ui {

/** @brief Renders the graphical Custom Groove editor on the production framebuffer. */
class GrooveEditorRenderer final {
public:
    /** @brief Constructs the renderer around the OLED HAL. */
    explicit GrooveEditorRenderer(hal::OledDisplay& display);

    /** @brief Renders the current draft with beat/step grid and signed microtiming markers. */
    void renderEditor(const NavigationState& navigation);

    /** @brief Renders the ten-slot fixed-record Custom Groove library. */
    void renderSlots(
        const NavigationState& navigation,
        const services::CustomGrooveStore& store);

    /** @brief Renders overwrite/discard confirmation screens. */
    void renderConfirm(const char* title, const NavigationState& navigation);

    /** @brief Renders the Custom Groove name editor. */
    void renderNameEntry(const NavigationState& navigation);

private:
    hal::OledDisplay& display_;
};

}  // namespace clockfw::ui
