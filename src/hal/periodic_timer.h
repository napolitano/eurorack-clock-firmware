/**
 * @file periodic_timer.h
 * @brief HAL wrapper for the STM32 hardware timer used by the scheduler.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <HardwareTimer.h>

#include <cstdint>

namespace clockfw::hal {

/** @brief Owns the STM32 TIM3 instance and exposes only periodic interrupt behavior. */
class PeriodicTimer final {
public:
    /** Callback signature accepted by the STM32duino HardwareTimer API. */
    using Callback = void (*)();

    /** @brief Constructs a timer wrapper for TIM3. */
    PeriodicTimer();

    /**
     * @brief Starts the timer at a fixed interrupt frequency.
     * @param frequencyHz Interrupt rate in hertz.
     * @param callback ISR-compatible callback invoked on every timer overflow.
     */
    void start(std::uint32_t frequencyHz, Callback callback);

    /** @brief Stops the timer and prevents further periodic callbacks. */
    void stop();

private:
    HardwareTimer timer_;
};

}  // namespace clockfw::hal
