/**
 * @file system_clock.h
 * @brief Thin HAL wrapper around Arduino monotonic time and delay primitives.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

namespace clockfw::hal {

/** @brief Provides monotonic timing services without exposing Arduino calls to higher layers. */
class SystemClock final {
public:
    /** @brief Returns milliseconds elapsed since MCU startup. */
    static std::uint32_t milliseconds();

    /** @brief Returns the wrapping 32-bit microsecond timestamp used for edge capture. */
    static std::uint32_t microseconds();

    /**
     * @brief Blocks execution for the requested number of milliseconds.
     * @param durationMs Delay duration in milliseconds.
     */
    static void delayMilliseconds(std::uint32_t durationMs);
};

}  // namespace clockfw::hal
