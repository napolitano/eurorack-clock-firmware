/**
 * @file control_panel.cpp
 * @brief HAL driver for the rotary encoder and three front-panel buttons.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/control_panel.h"

#include <Arduino.h>

#include "pin_map.h"
#include "hal/interrupt_lock.h"

namespace clockfw::hal {
namespace {

/** Debounce interval applied to active-low button inputs. */
constexpr std::uint32_t kDebounceMs = 25UL;

/** Gray-code transition lookup used to reject invalid encoder transitions. */
constexpr std::int8_t kEncoderTransitions[16] = {
    0, -1, 1, 0,
    1, 0, 0, -1,
    -1, 0, 0, 1,
    0, 1, -1, 0
};

}  // namespace

ControlPanel* ControlPanel::activeInstance_ = nullptr;

ControlPanel::DebouncedButton::DebouncedButton(const std::uint32_t pin) : pin_(pin) {}

void ControlPanel::DebouncedButton::begin(const std::uint32_t nowMs) {
    pinMode(pin_, INPUT_PULLUP);
    rawPressed_ = digitalRead(pin_) == LOW;
    stablePressed_ = rawPressed_;
    rawChangedAtMs_ = nowMs;
}

ButtonSample ControlPanel::DebouncedButton::sample(const std::uint32_t nowMs) {
    const bool currentRawPressed = digitalRead(pin_) == LOW;
    if (currentRawPressed != rawPressed_) {
        rawPressed_ = currentRawPressed;
        rawChangedAtMs_ = nowMs;
    }

    ButtonEdge edge = ButtonEdge::None;
    if (rawPressed_ != stablePressed_ && nowMs - rawChangedAtMs_ >= kDebounceMs) {
        stablePressed_ = rawPressed_;
        edge = stablePressed_ ? ButtonEdge::Pressed : ButtonEdge::Released;
    }

    return {edge, stablePressed_};
}

ControlPanel::ControlPanel()
    : encoderButton_(pinmap::kEncoderPushButtonPin),
      transportButton_(pinmap::kPlayPauseButtonPin),
      tapButton_(pinmap::kTapTempoButtonPin),
      resetButton_(pinmap::kResetBackButtonPin) {}

void ControlPanel::begin() {
    pinMode(pinmap::kEncoderPhaseAPin, INPUT_PULLUP);
    pinMode(pinmap::kEncoderPhaseBPin, INPUT_PULLUP);

    previousEncoderState_ =
        (digitalRead(pinmap::kEncoderPhaseAPin) == HIGH ? 2U : 0U) |
        (digitalRead(pinmap::kEncoderPhaseBPin) == HIGH ? 1U : 0U);
    encoderAccumulator_ = 0;
    pendingEncoderDetents_ = 0;
    activeInstance_ = this;
    attachInterrupt(digitalPinToInterrupt(pinmap::kEncoderPhaseAPin), encoderInterruptThunk, CHANGE);
    attachInterrupt(digitalPinToInterrupt(pinmap::kEncoderPhaseBPin), encoderInterruptThunk, CHANGE);

    const std::uint32_t nowMs = millis();
    encoderButton_.begin(nowMs);
    transportButton_.begin(nowMs);
    tapButton_.begin(nowMs);
    resetButton_.begin(nowMs);
}

ControlSample ControlPanel::sample(const std::uint32_t nowMs) {
    return {
        sampleEncoder(),
        encoderButton_.sample(nowMs),
        transportButton_.sample(nowMs),
        tapButton_.sample(nowMs),
        resetButton_.sample(nowMs)
    };
}

std::int8_t ControlPanel::sampleEncoder() {
    // A 16-bit aligned load is atomic on Cortex-M4. Avoid globally masking IRQs
    // on the overwhelmingly common idle path; if an ISR adds a detent just after
    // this zero check it remains pending for the next foreground iteration.
    if (pendingEncoderDetents_ == 0) {
        return 0;
    }

    InterruptLock interruptLock;
    constexpr std::int16_t kMaximumReportedDelta = 127;
    constexpr std::int16_t kMinimumReportedDelta = -127;
    const std::int16_t bounded = pendingEncoderDetents_ > kMaximumReportedDelta
        ? kMaximumReportedDelta
        : (pendingEncoderDetents_ < kMinimumReportedDelta
            ? kMinimumReportedDelta
            : pendingEncoderDetents_);
    pendingEncoderDetents_ -= bounded;
    return static_cast<std::int8_t>(bounded);
}

void ControlPanel::encoderInterruptThunk() {
    if (activeInstance_ != nullptr) {
        activeInstance_->handleEncoderEdgeFromIsr();
    }
}

void ControlPanel::handleEncoderEdgeFromIsr() {
    const std::uint8_t currentState =
        (digitalRead(pinmap::kEncoderPhaseAPin) == HIGH ? 2U : 0U) |
        (digitalRead(pinmap::kEncoderPhaseBPin) == HIGH ? 1U : 0U);
    const std::uint8_t transitionIndex = static_cast<std::uint8_t>(
        (static_cast<std::uint16_t>(previousEncoderState_) << 2U) | currentState);
    previousEncoderState_ = currentState;
    encoderAccumulator_ = static_cast<std::int8_t>(
        encoderAccumulator_ + kEncoderTransitions[transitionIndex]);

    if (encoderAccumulator_ >= 4) {
        encoderAccumulator_ = static_cast<std::int8_t>(encoderAccumulator_ - 4);
        if (pendingEncoderDetents_ < 32767) {
            ++pendingEncoderDetents_;
        }
    } else if (encoderAccumulator_ <= -4) {
        encoderAccumulator_ = static_cast<std::int8_t>(encoderAccumulator_ + 4);
        if (pendingEncoderDetents_ > -32767) {
            --pendingEncoderDetents_;
        }
    }
}

}  // namespace clockfw::hal
