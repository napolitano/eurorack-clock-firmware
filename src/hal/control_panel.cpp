/**
 * @file control_panel.cpp
 * @brief HAL driver for the rotary encoder and three front-panel buttons.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/control_panel.h"

#include "hal/platform_io.h"

#include "pin_map.h"

namespace clockfw::hal {
namespace {

/** Debounce interval applied to active-low button inputs. */
constexpr std::uint32_t kDebounceMs = 25UL;

}  // namespace

ControlPanel::DebouncedButton::DebouncedButton(const std::uint32_t pin) : pin_(pin) {}

void ControlPanel::DebouncedButton::begin(const std::uint32_t nowMs) {
    platform::configureInputPullup(pin_);
    rawPressed_ = !platform::read(pin_);
    stablePressed_ = rawPressed_;
    rawChangedAtMs_ = nowMs;
}

ButtonSample ControlPanel::DebouncedButton::sample(const std::uint32_t nowMs) {
    const bool currentRawPressed = !platform::read(pin_);
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
    // PA0/PA1 are handled by the dedicated quadrature backend. On STM32F401 the
    // production implementation is TIM2 encoder mode (x4), so the first physical
    // detent and every later direction change are counted independently of EXTI
    // latency or foreground/display work.
    (void)platform::beginQuadratureEncoder(
        pinmap::kEncoderPhaseAPin,
        pinmap::kEncoderPhaseBPin);
    encoderLastTransitionCount_ = platform::quadratureEncoderCount();
    encoderTransitionRemainder_ = 0;
    pendingEncoderDetents_ = 0;

    const std::uint32_t nowMs = platform::milliseconds();
    encoderButton_.begin(nowMs);
    transportButton_.begin(nowMs);
    tapButton_.begin(nowMs);
    resetButton_.begin(nowMs);
}

ControlSample ControlPanel::sample(const std::uint32_t nowMs) {
    std::int8_t encoderDelta = sampleEncoder();
    if (encoderDirectionReversed_) {
        encoderDelta = static_cast<std::int8_t>(-encoderDelta);
    }
    return {
        encoderDelta,
        encoderButton_.sample(nowMs),
        transportButton_.sample(nowMs),
        tapButton_.sample(nowMs),
        resetButton_.sample(nowMs)
    };
}

void ControlPanel::setEncoderDirectionReversed(const bool reversed) {
    encoderDirectionReversed_ = reversed;
}

std::int8_t ControlPanel::sampleEncoder() {
    constexpr std::int32_t kTransitionsPerDetent = 4;
    constexpr std::int16_t kMaximumPendingDetents = 32767;
    constexpr std::int16_t kMinimumPendingDetents = -32767;
    constexpr std::int16_t kMaximumReportedDelta = 127;
    constexpr std::int16_t kMinimumReportedDelta = -127;

    const std::uint32_t currentCount = platform::quadratureEncoderCount();
    const std::int32_t transitionDelta = static_cast<std::int32_t>(
        currentCount - encoderLastTransitionCount_);
    encoderLastTransitionCount_ = currentCount;

    if (transitionDelta != 0) {
        const std::int64_t accumulatedTransitions =
            static_cast<std::int64_t>(encoderTransitionRemainder_) + transitionDelta;
        const std::int32_t newDetents = static_cast<std::int32_t>(
            accumulatedTransitions / kTransitionsPerDetent);
        encoderTransitionRemainder_ = static_cast<std::int8_t>(
            accumulatedTransitions -
            static_cast<std::int64_t>(newDetents) * kTransitionsPerDetent);

        const std::int32_t pending =
            static_cast<std::int32_t>(pendingEncoderDetents_) + newDetents;
        pendingEncoderDetents_ = static_cast<std::int16_t>(
            pending > kMaximumPendingDetents
                ? kMaximumPendingDetents
                : (pending < kMinimumPendingDetents
                    ? kMinimumPendingDetents
                    : pending));
    }

    if (pendingEncoderDetents_ == 0) {
        return 0;
    }

    const std::int16_t bounded = pendingEncoderDetents_ > kMaximumReportedDelta
        ? kMaximumReportedDelta
        : (pendingEncoderDetents_ < kMinimumReportedDelta
            ? kMinimumReportedDelta
            : pendingEncoderDetents_);
    pendingEncoderDetents_ = static_cast<std::int16_t>(pendingEncoderDetents_ - bounded);
    return static_cast<std::int8_t>(bounded);
}

}  // namespace clockfw::hal
