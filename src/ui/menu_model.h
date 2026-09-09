/**
 * @file menu_model.h
 * @brief Static settings-page metadata and row formatting.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/clock_types.h"
#include "ui/ui_types.h"

namespace clockfw::ui {

/** @brief One rendered settings row with compact fixed-size label/value buffers. */
struct MenuRow {
    char label[18]{};
    char value[16]{};
};

/** @brief Returns the compact title for a settings page. */
const char* settingsPageTitle(SettingsPage page);

/** @brief Returns the number of selectable rows on a settings page. */
std::uint8_t settingsPageItemCount(SettingsPage page, ChannelMode mode = ChannelMode::Clock);

/** @brief Returns true when the page is scoped to the currently selected channel. */
bool isChannelSettingsPage(SettingsPage page);

/**
 * @brief Formats one settings row from the current application state.
 * @param page Settings page being rendered.
 * @param rowIndex Zero-based row index.
 * @param selectedChannel Zero-based selected channel.
 * @param state Complete clock state.
 * @return Formatted row; fields are empty when the row is invalid.
 */
MenuRow buildMenuRow(
    SettingsPage page,
    std::uint8_t rowIndex,
    std::uint8_t selectedChannel,
    const ClockState& state);

/** @brief Returns the index of the current integer rate option. */
std::size_t findRateOptionIndex(const CommonChannelSettings& settings);

/** @brief Returns the index of the current gate-length option. */
std::size_t findGateLengthOptionIndex(std::uint16_t gateLengthMs);

/** @brief Returns the index of the current meter-unit option. */
std::size_t findBeatUnitOptionIndex(std::uint8_t beatUnit);

}  // namespace clockfw::ui
