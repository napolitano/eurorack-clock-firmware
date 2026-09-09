/**
 * @file text_formatter.h
 * @brief Compact string formatting helpers for the 128x64 OLED UI.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>

#include "domain/clock_types.h"

namespace clockfw::ui {

/** @brief Formats a channel rate using compact xN, /N, and optional N:D notation. */
void formatRate(const CommonChannelSettings& settings, char* output, std::size_t outputSize);

/** @brief Formats the primary performance-screen value for one channel. */
void formatChannelDetail(const ChannelConfig& channel, char* output, std::size_t outputSize);

/** @brief Formats the compact second-line summary shown in a channel overview tile. */
void formatChannelSummary(const ChannelConfig& channel, char* output, std::size_t outputSize);

}  // namespace clockfw::ui
