/**
 * @file persistent_state_validation.h
 * @brief Semantic validation boundary for persisted ClockState payloads.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include "domain/clock_types.h"

namespace clockfw::services {

/** @brief Returns true when every persisted field is inside the supported semantic range. */
bool isPersistentStateValid(const ClockState& state);

}  // namespace clockfw::services
