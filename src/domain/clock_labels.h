/**
 * @file clock_labels.h
 * @brief Human-readable labels for domain enum values.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include "domain/clock_types.h"

namespace clockfw {

/**
 * @brief Returns the compact three-character label for a channel mode.
 * @param mode Channel mode to format.
 * @return Static null-terminated label suitable for the 128x64 OLED.
 */
const char* channelModeShortLabel(ChannelMode mode);

/**
 * @brief Returns the full human-readable label for a channel mode.
 * @param mode Channel mode to format.
 * @return Static null-terminated label.
 */
const char* channelModeLongLabel(ChannelMode mode);

/**
 * @brief Returns the compact display label for a clock source.
 * @param source Clock source to format.
 * @return Static null-terminated label.
 */
const char* clockSourceLabel(ClockSource source);

}  // namespace clockfw
