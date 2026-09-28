/**
 * @file panel_led_visual.cpp
 * @brief Implements shared simulator/tutorial output-LED visual persistence.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "panel_led_visual.h"

#include <algorithm>

namespace clockfw::sim {

bool panelLedVisuallyLit(
    const ChannelTelemetry& telemetry,
    const std::uint64_t nowUs,
    const double speedMultiplier) {
    if (telemetry.logicHigh) {
        return true;
    }
    if (telemetry.lastRisingUs == 0ULL || nowUs < telemetry.lastRisingUs) {
        return false;
    }
    const double safeSpeed = std::max(speedMultiplier, 1.0);
    const auto persistenceUs = static_cast<std::uint64_t>(
        static_cast<double>(kPanelLedVisualPersistenceUs) * safeSpeed);
    return nowUs - telemetry.lastRisingUs <= persistenceUs;
}

}  // namespace clockfw::sim
