/**
 * @file output_mode_resolver.h
 * @brief Resolves top-level operating modes into physical per-output configurations.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>

#include "domain/clock_types.h"

namespace clockfw::engine {

/**
 * @brief Resolves one physical output configuration from the selected operating mode.
 * @param state Complete persistent application state.
 * @param channelIndex Zero-based physical output index.
 * @return Effective channel configuration consumed by the real-time scheduler.
 */
ChannelConfig resolvePhysicalOutputConfiguration(
    const ClockState& state,
    std::size_t channelIndex);

}  // namespace clockfw::engine
