/**
 * @file system_clock.cpp
 * @brief Thin HAL wrapper around framework-independent monotonic time and delay primitives.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/system_clock.h"

#include "hal/platform_io.h"

namespace clockfw::hal {

std::uint32_t SystemClock::milliseconds() {
    return platform::milliseconds();
}

std::uint32_t SystemClock::microseconds() {
    return platform::microseconds();
}

void SystemClock::delayMilliseconds(const std::uint32_t durationMs) {
    platform::delayMilliseconds(durationMs);
}

}  // namespace clockfw::hal
