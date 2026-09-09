/**
 * @file mode_functions.h
 * @brief Mapping between the six graphical mode actions and persistent clock state.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstdint>

#include "domain/clock_types.h"
#include "ui/ui_types.h"

namespace clockfw::ui {

/** Stable display order used by the 2x3 mode palette. */
inline constexpr std::array<ModeFunction, 6U> kModeFunctions{{
    ModeFunction::UnifiedClock,
    ModeFunction::DividerBank,
    ModeFunction::Clock,
    ModeFunction::Euclid,
    ModeFunction::Sequencer,
    ModeFunction::Off}};

/** @brief Returns the currently active palette entry for one selected channel/context. */
std::uint8_t modeFunctionIndexForState(const ClockState& state, std::uint8_t selectedChannel);

/** @brief Applies one palette action to the persistent state model. */
void applyModeFunction(
    ClockState& state,
    std::uint8_t selectedChannel,
    ModeFunction modeFunction);

}  // namespace clockfw::ui
