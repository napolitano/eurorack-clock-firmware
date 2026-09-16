/**
 * @file periodic_timer.h
 * @brief HAL wrapper for the STM32 TIM3 scheduler interrupt.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once
#include <cstdint>
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
#include <HardwareTimer.h>
#endif
namespace clockfw::hal {
class PeriodicTimer final {
public:
    using Callback = void (*)();
    /** @brief Constructs the TIM3 scheduler wrapper. */
    PeriodicTimer();
    /** @brief Starts periodic TIM3 interrupts at the requested frequency. */
    void start(std::uint32_t frequencyHz, Callback callback);
    /** @brief Stops TIM3 scheduler interrupts. */
    void stop();
private:
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
    HardwareTimer timer_;
#endif
};
} // namespace clockfw::hal
