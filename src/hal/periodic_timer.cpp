/**
 * @file periodic_timer.cpp
 * @brief HAL wrapper for the STM32 hardware timer used by the scheduler.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/periodic_timer.h"

#include "config.h"

namespace clockfw::hal {

PeriodicTimer::PeriodicTimer() : timer_(TIM3) {}

void PeriodicTimer::start(const std::uint32_t frequencyHz, const Callback callback) {
    timer_.pause();
    timer_.setOverflow(frequencyHz, HERTZ_FORMAT);
    timer_.setInterruptPriority(
        config::kSchedulerInterruptPreemptPriority,
        config::kSchedulerInterruptSubPriority);
    timer_.attachInterrupt(callback);
    timer_.resume();
}

void PeriodicTimer::stop() {
    timer_.pause();
}

}  // namespace clockfw::hal
