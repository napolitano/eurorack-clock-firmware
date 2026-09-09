/**
 * @file HardwareTimer.h
 * @brief Native-simulator replacement for STM32duino HardwareTimer.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "Arduino.h"

inline constexpr std::uint8_t HERTZ_FORMAT = 1U;

namespace clockfw::simfw {
inline void (*timerCallback)() = nullptr;
inline std::uint32_t timerFrequencyHz = 0U;
inline bool timerRunning = false;

/** @brief Resets the simulated timer peripheral to its MCU power-on state. */
inline void resetTimer() {
    timerCallback = nullptr;
    timerFrequencyHz = 0U;
    timerRunning = false;
}

/** @brief Fires the registered scheduler callback once when the virtual timer is running. */
inline void fireTimer() {
    if (timerRunning && timerCallback != nullptr && interruptDisableDepth == 0U) {
        timerCallback();
    }
}
}  // namespace clockfw::simfw

class HardwareTimer {
public:
    using Callback = void (*)();

    explicit HardwareTimer(std::uint32_t instance) : instance_(instance) {}

    void pause() { clockfw::simfw::timerRunning = false; }
    void setOverflow(std::uint32_t value, std::uint8_t) { clockfw::simfw::timerFrequencyHz = value; }
    void setInterruptPriority(std::uint32_t, std::uint32_t) {}
    void attachInterrupt(Callback callback) { clockfw::simfw::timerCallback = callback; }
    void resume() { clockfw::simfw::timerRunning = true; }

private:
    std::uint32_t instance_ = 0U;
};
