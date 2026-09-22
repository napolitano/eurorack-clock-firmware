/**
 * @file performance_renderer_groove.cpp
 * @brief Compact active-groove status rendering for the Performance screen.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/performance_renderer.h"

#include <cstdio>

#include "domain/groove_catalog.h"
#include "ui_text.h"

namespace clockfw::ui {

void PerformanceRenderer::drawGrooveStatus(
    const GrooveSettings& settings,
    const std::int16_t y) {
    if (settings.preset == GroovePreset::Off || settings.amountPercent == 0U) {
        return;
    }

    const char* grooveLabel = groove::presetLabel(settings.preset);
    char customName[services::CustomGrooveStore::kNameLength + 1U]{};
    if (settings.preset == GroovePreset::Custom && customGrooveStore_ != nullptr) {
        customGrooveStore_->name(settings.customSlot, customName, sizeof(customName));
        if (customName[0] != '\0') {
            grooveLabel = customName;
        }
    }

    char label[21]{};
    std::snprintf(
        label,
        sizeof(label),
        "%s%s",
        text::get(text::TextId::GroovePrefix),
        grooveLabel);
    display_.setFont(hal::DisplayFont::Small);
    display_.drawText(1, y, label);
}

}  // namespace clockfw::ui
