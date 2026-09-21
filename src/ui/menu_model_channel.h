/**
 * @file menu_model_channel.h
 * @brief Compact mode-aware channel settings hierarchy.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

#include "domain/clock_types.h"

namespace clockfw::ui {

struct MenuRow;

/** @brief One selectable entry on the Channel root page. */
enum class ChannelMenuAction : std::uint8_t {
    Mode,
    Timing,
    Clock,
    Euclid,
    Sequencer,
    Output
};

/** @brief Returns the selectable row count for one Independent channel root. */
std::uint8_t channelMenuItemCount(ChannelMode mode);

/** @brief Maps one Channel root row to its semantic destination. */
ChannelMenuAction channelMenuAction(ChannelMode mode, std::uint8_t rowIndex);

/** @brief Finds the row carrying one root action, or zero when unavailable. */
std::uint8_t channelMenuIndexForAction(ChannelMode mode, ChannelMenuAction action);

/** @brief Formats one row of the compact mode-aware Channel root page. */
MenuRow buildChannelMenuRow(std::uint8_t rowIndex, const ChannelConfig& channel);

/** @brief Formats one row of the selected channel Timing group. */
MenuRow buildChannelTimingMenuRow(std::uint8_t rowIndex, const ChannelConfig& channel);

/** @brief Formats one row of the selected channel Output group. */
MenuRow buildChannelOutputMenuRow(std::uint8_t rowIndex, const ChannelConfig& channel);

/** @brief Formats one row of the One Clock root group page. */
MenuRow buildUnifiedClockMenuRow(std::uint8_t rowIndex);

/** @brief Formats one row of the One Clock Timing group. */
MenuRow buildUnifiedTimingMenuRow(std::uint8_t rowIndex, const UnifiedClockSettings& settings);

/** @brief Formats one row of the One Clock Output group. */
MenuRow buildUnifiedOutputMenuRow(std::uint8_t rowIndex, const UnifiedClockSettings& settings);

}  // namespace clockfw::ui
