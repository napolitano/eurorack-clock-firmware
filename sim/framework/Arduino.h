/**
 * @file Arduino.h
 * @brief Native-simulator shim for the small Arduino API surface used by the firmware HAL.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <map>
#include <vector>

using PinName = std::uint32_t;

inline constexpr std::uint8_t LOW = 0U;
inline constexpr std::uint8_t HIGH = 1U;
inline constexpr std::uint8_t INPUT_PULLUP = 2U;
inline constexpr std::uint8_t OUTPUT = 3U;
inline constexpr int CHANGE = 4;
inline constexpr int RISING = 5;
inline constexpr int FALLING = 6;
using voidFuncPtr = void (*)();

inline constexpr std::uint32_t PA0=0xA00, PA1=0xA01, PA2=0xA02, PA3=0xA03, PA4=0xA04, PA5=0xA05, PA6=0xA06, PA7=0xA07, PA8=0xA08, PA9=0xA09, PA10=0xA0A;
inline constexpr std::uint32_t PB0=0xB00, PB1=0xB01, PB3=0xB03, PB4=0xB04, PB5=0xB05, PB6=0xB06, PB7=0xB07, PB8=0xB08, PB9=0xB09, PB10=0xB0A, PB12=0xB0C, PB13=0xB0D, PB14=0xB0E, PB15=0xB0F;
inline constexpr PinName PA_5=PA5, PA_6=PA6, PA_7=PA7, PB_6=0xB06, PB_7=0xB07;
inline constexpr std::uint32_t TIM3 = 3U;

namespace clockfw::simfw {

/** @brief One GPIO transition captured by the simulator for timing visualization. */
struct PinWrite {
    std::uint32_t pin = 0U;
    std::uint8_t value = LOW;
    std::uint64_t timestampUs = 0ULL;
};

/** Mutable host-side state backing the Arduino compatibility shim. */
inline std::map<std::uint32_t, std::uint8_t> pinModes{};
inline std::map<std::uint32_t, std::uint8_t> pinValues{};
inline std::vector<PinWrite> writes{};
inline std::uint64_t nowUs = 0ULL;
inline unsigned interruptDisableDepth = 0U;
struct InterruptBinding { voidFuncPtr callback = nullptr; int mode = CHANGE; };
inline std::map<std::uint32_t, InterruptBinding> interruptBindings{};

/** @brief Resets all simulated GPIO and time state to power-on defaults. */
inline void resetArduino() {
    pinModes.clear();
    pinValues.clear();
    writes.clear();
    nowUs = 0ULL;
    interruptDisableDepth = 0U;
    interruptBindings.clear();
}

/** @brief Sets one input pin without recording an output transition. */
inline void setPin(const std::uint32_t pin, const std::uint8_t value) {
    const auto existing = pinValues.find(pin);
    const std::uint8_t previous = existing == pinValues.end() ? HIGH : existing->second;
    pinValues[pin] = value;
    if (previous == value || interruptDisableDepth != 0U) return;
    const auto binding = interruptBindings.find(pin);
    if (binding == interruptBindings.end() || binding->second.callback == nullptr) return;
    const bool rising = previous == LOW && value == HIGH;
    const bool falling = previous == HIGH && value == LOW;
    if (binding->second.mode == CHANGE || (binding->second.mode == RISING && rising) ||
        (binding->second.mode == FALLING && falling)) binding->second.callback();
}

/** @brief Returns the current simulated logic level, defaulting to pulled-up HIGH. */
inline std::uint8_t pinValue(const std::uint32_t pin) {
    const auto iterator = pinValues.find(pin);
    return iterator == pinValues.end() ? HIGH : iterator->second;
}

/** @brief Advances the simulator's monotonic virtual clock. */
inline void advanceMicroseconds(const std::uint64_t durationUs) {
    nowUs += durationUs;
}

}  // namespace clockfw::simfw

inline void pinMode(const std::uint32_t pin, const std::uint8_t mode) {
    clockfw::simfw::pinModes[pin] = mode;
}

inline int digitalRead(const std::uint32_t pin) {
    return clockfw::simfw::pinValue(pin);
}

inline void digitalWrite(const std::uint32_t pin, const std::uint8_t value) {
    const std::uint8_t previous = clockfw::simfw::pinValue(pin);
    clockfw::simfw::pinValues[pin] = value;
    if (previous != value) {
        clockfw::simfw::writes.push_back({pin, value, clockfw::simfw::nowUs});
    }
}

inline std::uint32_t millis() {
    return static_cast<std::uint32_t>(clockfw::simfw::nowUs / 1000ULL);
}

inline std::uint32_t micros() {
    return static_cast<std::uint32_t>(clockfw::simfw::nowUs);
}

inline std::uint32_t digitalPinToInterrupt(const std::uint32_t pin) { return pin; }
inline void attachInterrupt(const std::uint32_t pin, voidFuncPtr callback, const int mode) {
    clockfw::simfw::interruptBindings[pin] = {callback, mode};
}
inline void detachInterrupt(const std::uint32_t pin) { clockfw::simfw::interruptBindings.erase(pin); }

inline void delay(const std::uint32_t durationMs) {
    clockfw::simfw::advanceMicroseconds(static_cast<std::uint64_t>(durationMs) * 1000ULL);
}

inline void noInterrupts() {
    ++clockfw::simfw::interruptDisableDepth;
}

inline void interrupts() {
    if (clockfw::simfw::interruptDisableDepth > 0U) {
        --clockfw::simfw::interruptDisableDepth;
    }
}
