/**
 * @file default_configuration.h
 * @brief Factory-default state construction for the clock firmware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include "domain/clock_types.h"

namespace clockfw {

/**
 * @brief Restores the complete application state to deterministic factory defaults.
 *
 * All eight channels start in CLOCK mode at the master rate with 100% probability,
 * zero swing, and GLOBAL reset behavior.
 *
 * @param state State object to initialize.
 */
void initializeFactoryDefaults(ClockState& state);

}  // namespace clockfw
