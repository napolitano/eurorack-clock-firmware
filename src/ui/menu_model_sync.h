/**
 * @file menu_model_sync.h
 * @brief External-sync settings row formatting split from the main menu model.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "domain/clock_types.h"
#include "ui/menu_model.h"

namespace clockfw::ui {

/** @brief Formats one external synchronization settings row. */
MenuRow buildSyncMenuRow(std::uint8_t rowIndex, const ClockState& state);

}  // namespace clockfw::ui
