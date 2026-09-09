/**
 * @file system_clock.cpp
 * @brief Thin HAL wrapper around Arduino monotonic time and delay primitives.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/system_clock.h"

#include <Arduino.h>

namespace clockfw::hal {

std::uint32_t SystemClock::milliseconds() {
    return millis();
}

std::uint32_t SystemClock::microseconds() {
    return micros();
}

void SystemClock::delayMilliseconds(const std::uint32_t durationMs) {
    delay(durationMs);
}

}  // namespace clockfw::hal
