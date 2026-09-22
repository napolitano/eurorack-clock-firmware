/**
 * @file menu_model_groove.cpp
 * @brief Formatting helper for Stage-1 Groove settings.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/menu_model_groove.h"
#include <cstdio>
#include "domain/groove_catalog.h"
#include "ui_text.h"
namespace clockfw::ui {
MenuRow buildGrooveMenuRow(
    const std::uint8_t rowIndex,
    const std::uint8_t selectedChannel,
    const ClockState& state) {
    MenuRow row{};
    const GrooveSettings& settings = state.operatingMode == OperatingMode::UnifiedClock
        ? state.unifiedClock.groove
        : state.channels[selectedChannel].common.groove;
    if (rowIndex == 0U) {
        std::snprintf(row.label, sizeof(row.label), "%s", text::get(text::TextId::GroovePreset));
        std::snprintf(row.value, sizeof(row.value), "%s", groove::presetLabel(settings.preset));
    } else if (rowIndex == 1U) {
        std::snprintf(row.label, sizeof(row.label), "%s", text::get(text::TextId::Amount));
        std::snprintf(row.value, sizeof(row.value), "%u%%", settings.amountPercent);
    } else if (rowIndex == 2U) {
        std::snprintf(row.label, sizeof(row.label), "%s", text::get(text::TextId::Rotate));
        if (settings.preset == GroovePreset::Custom) {
            std::snprintf(row.value, sizeof(row.value), "%u", settings.rotation);
        } else {
            const std::uint8_t length = groove::patternLength(settings.preset);
            std::snprintf(
                row.value,
                sizeof(row.value),
                "%u/%u",
                settings.rotation,
                length > 0U ? length - 1U : 0U);
        }
    } else if (rowIndex == 3U) {
        std::snprintf(row.label, sizeof(row.label), "%s", text::get(text::TextId::Editor));
        std::snprintf(row.value, sizeof(row.value), "%s", text::get(text::TextId::Arrow));
    }
    return row;
}
}  // namespace clockfw::ui
