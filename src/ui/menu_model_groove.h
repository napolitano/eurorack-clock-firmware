/**
 * @file menu_model_groove.h
 * @brief Formatting helper for Stage-1 Groove settings.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once
#include <cstdint>
#include "domain/clock_types.h"
#include "ui/menu_model.h"
namespace clockfw::ui {
/** @brief Formats one row of the active global/per-channel Groove assignment. */
MenuRow buildGrooveMenuRow(std::uint8_t rowIndex, std::uint8_t selectedChannel, const ClockState& state);
}  // namespace clockfw::ui
