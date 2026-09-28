/**
 * @file panel_led_visual.h
 * @brief Shared perceptual activity-LED presentation derived only from real simulator telemetry.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "simulator_runtime.h"

namespace clockfw::sim {

/** @brief Minimum perceived LED-on time at normal simulator speed. */
constexpr std::uint64_t kPanelLedVisualPersistenceUs = 75000ULL;

/**
 * @brief Returns whether one output LED should be visibly lit from actual gate telemetry.
 *
 * The short perceptual hold mirrors the interactive simulator display only; it never creates or
 * changes a gate. A currently high gate always lights the LED.
 */
bool panelLedVisuallyLit(
    const ChannelTelemetry& telemetry,
    std::uint64_t nowUs,
    double speedMultiplier);

}  // namespace clockfw::sim
