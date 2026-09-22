/**
 * @file mode_functions.h
 * @brief Mapping between graphical mode-selector actions and persistent clock state.
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

/** Stable left-to-right order used by the horizontal mode carousel.
 *
 * The array intentionally uses class template argument deduction so adding a
 * future mode does not require a second, easy-to-forget item-count update.
 */
inline constexpr std::array kModeFunctions{
    ModeFunction::UnifiedClock,
    ModeFunction::DividerBank,
    ModeFunction::Clock,
    ModeFunction::Euclid,
    ModeFunction::Sequencer,
    ModeFunction::Off};

/** @brief Returns the currently active carousel entry for one selected channel/context. */
std::uint8_t modeFunctionIndexForState(const ClockState& state, std::uint8_t selectedChannel);

/** @brief Applies one mode-selector action to the persistent state model. */
void applyModeFunction(
    ClockState& state,
    std::uint8_t selectedChannel,
    ModeFunction modeFunction);

}  // namespace clockfw::ui
