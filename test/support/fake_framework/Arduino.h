/**
 * @file Arduino.h
 * @brief Host-test fake for the small Arduino API surface used by the firmware HAL.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <map>
#include <stdexcept>
#include <vector>
#include <functional>

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

namespace fakefw {
struct PinWrite { std::uint32_t pin; std::uint8_t value; };
inline std::map<std::uint32_t,std::uint8_t> pinModes{};
inline std::map<std::uint32_t,std::uint8_t> pinValues{};
inline std::vector<PinWrite> writes{};
inline std::uint32_t nowMs = 0U;
inline std::uint32_t nowUs = 0U;
struct InterruptBinding { voidFuncPtr callback = nullptr; int mode = CHANGE; };
inline std::map<std::uint32_t, InterruptBinding> interruptBindings{};
inline unsigned noInterruptCalls = 0U;
inline unsigned interruptCalls = 0U;
inline bool throwOnDelay = false;
inline std::uint32_t delayAdvanceOverrideMs = 0U;

inline void resetArduino() {
    pinModes.clear(); pinValues.clear(); writes.clear(); interruptBindings.clear(); nowMs=0U; nowUs=0U;
    noInterruptCalls=0U; interruptCalls=0U; throwOnDelay=false; delayAdvanceOverrideMs=0U;
}
inline void setPin(std::uint32_t pin, std::uint8_t value) {
    const auto existing = pinValues.find(pin);
    const std::uint8_t previous = existing == pinValues.end() ? HIGH : existing->second;
    pinValues[pin]=value;
    if (previous == value) return;
    const auto binding = interruptBindings.find(pin);
    if (binding == interruptBindings.end() || binding->second.callback == nullptr) return;
    const bool rising = previous == LOW && value == HIGH;
    const bool falling = previous == HIGH && value == LOW;
    if (binding->second.mode == CHANGE || (binding->second.mode == RISING && rising) ||
        (binding->second.mode == FALLING && falling)) {
        binding->second.callback();
    }
}
}

inline void pinMode(std::uint32_t pin, std::uint8_t mode) { fakefw::pinModes[pin]=mode; }
inline int digitalRead(std::uint32_t pin) {
    const auto it=fakefw::pinValues.find(pin);
    return it==fakefw::pinValues.end()?HIGH:it->second;
}
inline void digitalWrite(std::uint32_t pin, std::uint8_t value) {
    fakefw::pinValues[pin]=value; fakefw::writes.push_back({pin,value});
}
inline std::uint32_t millis() { return fakefw::nowMs; }
inline std::uint32_t micros() { return fakefw::nowUs != 0U ? fakefw::nowUs : fakefw::nowMs * 1000U; }
inline std::uint32_t digitalPinToInterrupt(std::uint32_t pin) { return pin; }
inline void attachInterrupt(std::uint32_t pin, voidFuncPtr callback, int mode) { fakefw::interruptBindings[pin] = {callback, mode}; }
inline void detachInterrupt(std::uint32_t pin) { fakefw::interruptBindings.erase(pin); }
inline void delay(std::uint32_t durationMs) {
    fakefw::nowMs += fakefw::delayAdvanceOverrideMs != 0U ? fakefw::delayAdvanceOverrideMs : durationMs;
    if (fakefw::throwOnDelay) throw std::runtime_error("fake delay abort");
}
inline void noInterrupts() { ++fakefw::noInterruptCalls; }
inline void interrupts() { ++fakefw::interruptCalls; }
extern "C" void setup();
extern "C" void loop();
