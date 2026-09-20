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

    char label[21]{};
    std::snprintf(
        label,
        sizeof(label),
        "%s%s",
        text::get(text::TextId::GroovePrefix),
        groove::presetLabel(settings.preset));
    display_.setFont(hal::DisplayFont::Small);
    display_.drawText(1, y, label);
}

}  // namespace clockfw::ui
