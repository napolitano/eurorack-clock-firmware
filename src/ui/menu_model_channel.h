/**
 * @file menu_model_channel.h
 * @brief Flat priority-ordered channel settings model and visual section metadata.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

#include "domain/clock_types.h"
#include "ui/ui_types.h"

namespace clockfw::ui {

struct MenuRow;

/** @brief Semantic action represented by one selectable row in the flat channel menu. */
enum class ChannelMenuAction : std::uint8_t {
    Mode,
    Rate,
    PolyNumerator,
    PolyDenominator,
    Swing,
    Groove,
    MeterBeats,
    MeterUnit,
    EuclidSteps,
    EuclidHits,
    EuclidRotate,
    SequencerLength,
    SequencerRotate,
    SequencerPattern,
    Probability,
    Gate,
    Phase,
    Reset,
    Mute
};

/** @brief Non-selectable visual section heading used by flat performance-oriented menus. */
enum class SettingsSection : std::uint8_t {
    None,
    Timing,
    Clock,
    Euclid,
    Sequencer,
    Output
};

/** @brief Returns the selectable row count for one Independent channel mode. */
std::uint8_t channelMenuItemCount(ChannelMode mode);

/** @brief Maps one flat Channel row to its semantic action. */
ChannelMenuAction channelMenuAction(ChannelMode mode, std::uint8_t rowIndex);

/** @brief Finds the row carrying one action, or zero when the action is unavailable. */
std::uint8_t channelMenuIndexForAction(ChannelMode mode, ChannelMenuAction action);

/** @brief Formats one row of the flat priority-ordered Channel menu. */
MenuRow buildFlatChannelMenuRow(std::uint8_t rowIndex, const ChannelConfig& channel);

/** @brief Returns the visual group for one selectable settings row. */
SettingsSection settingsRowSection(SettingsPage page, ChannelMode mode, std::uint8_t rowIndex);

/** @brief Returns the localized non-selectable heading for one visual settings group. */
const char* settingsSectionLabel(SettingsSection section);

/** @brief Returns how many selectable rows fit in the six-line grouped settings viewport. */
std::uint8_t groupedSettingsVisibleItemCount(
    SettingsPage page,
    ChannelMode mode,
    std::uint8_t startRow,
    std::uint8_t itemCount);

}  // namespace clockfw::ui
