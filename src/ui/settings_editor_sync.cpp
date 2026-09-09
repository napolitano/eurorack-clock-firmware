/**
 * @file settings_editor_sync.cpp
 * @brief External-sync settings mutation split from the main settings editor.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/settings_editor.h"

#include <cstddef>

#include "domain/clock_options.h"

namespace clockfw::ui {

void SettingsEditor::adjustSync(const std::uint8_t rowIndex, const std::int8_t delta) {
    if (rowIndex == 0U) {
        state_.source = static_cast<ClockSource>(clampInt(
            static_cast<int>(state_.source) + delta, 0, 2));
    } else if (rowIndex == 1U) {
        std::size_t currentIndex = 0U;
        for (std::size_t index = 0U; index < kExternalPpqnOptions.size(); ++index) {
            if (kExternalPpqnOptions[index] == state_.externalSync.pulsesPerQuarterNote) {
                currentIndex = index;
                break;
            }
        }
        const std::size_t newIndex = static_cast<std::size_t>(clampInt(
            static_cast<int>(currentIndex) + delta,
            0,
            static_cast<int>(kExternalPpqnOptions.size() - 1U)));
        state_.externalSync.pulsesPerQuarterNote = kExternalPpqnOptions[newIndex];
    } else if (rowIndex == 2U) {
        state_.externalSync.edge = delta > 0 ? SyncEdge::Falling : SyncEdge::Rising;
    } else if (rowIndex == 3U) {
        state_.externalSync.lossMode = static_cast<SyncLossMode>(clampInt(
            static_cast<int>(state_.externalSync.lossMode) + delta, 0, 2));
    } else if (rowIndex == 4U) {
        state_.externalSync.resetMode = delta > 0
            ? ExternalResetMode::Gate
            : ExternalResetMode::Trigger;
    } else if (rowIndex == 5U) {
        state_.externalSync.glitchFilterUs = static_cast<std::uint16_t>(clampInt(
            static_cast<int>(state_.externalSync.glitchFilterUs) + delta * 250,
            0,
            5000));
    } else if (rowIndex == 6U) {
        state_.externalSync.timeoutMs = static_cast<std::uint16_t>(clampInt(
            static_cast<int>(state_.externalSync.timeoutMs) + delta * 100,
            200,
            5000));
    }

    engine_.updateConfiguration(state_, false);
}

}  // namespace clockfw::ui
