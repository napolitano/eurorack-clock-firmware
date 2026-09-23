/**
 * @file groove_recorder.cpp
 * @brief Deterministic Custom Groove capture state machine implementation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "services/groove_recorder.h"

#include <algorithm>

#include "clock_core.h"

namespace clockfw::services {
void GrooveRecorder::reset(
    const GrooveRecordMode mode,
    const std::uint8_t countInBeats,
    const std::uint64_t capturedMask) {
    mode_ = mode;
    countInBeats_ = std::min<std::uint8_t>(countInBeats, 64U);
    countInRemaining_ = countInBeats_;
    state_ = GrooveRecordState::Ready;
    playheadStep_ = 0U;
    playheadPhase256_ = 0U;
    capturedMask_ = capturedMask;
    stepIntervalQ32_ = 0U;
    preCountEpochQ32_ = 0U;
    recordEpochQ32_ = 0U;
    recordStartTargetQ32_ = 0U;
}

void GrooveRecorder::setMode(const GrooveRecordMode mode) {
    mode_ = mode;
}

void GrooveRecorder::setCountInBeats(const std::uint8_t beats) {
    countInBeats_ = std::min<std::uint8_t>(beats, 64U);
    if (state_ == GrooveRecordState::Ready) {
        countInRemaining_ = countInBeats_;
    }
}

void GrooveRecorder::start(
    const std::uint64_t masterPositionQ32,
    const std::uint64_t stepIntervalQ32,
    const std::uint8_t length) {
    if (stepIntervalQ32 == 0U || length == 0U || length > kCustomGrooveMaximumSteps) {
        stop();
        return;
    }
    stepIntervalQ32_ = stepIntervalQ32;
    preCountEpochQ32_ = masterPositionQ32;
    playheadStep_ = 0U;
    playheadPhase256_ = 0U;
    countInRemaining_ = countInBeats_;

    const std::uint64_t countInQ32 = static_cast<std::uint64_t>(countInBeats_) * core::kQ32One;
    recordStartTargetQ32_ = masterPositionQ32 > UINT64_MAX - countInQ32
        ? UINT64_MAX
        : masterPositionQ32 + countInQ32;
    recordEpochQ32_ = recordStartTargetQ32_;
    state_ = countInBeats_ == 0U
        ? GrooveRecordState::Recording
        : GrooveRecordState::PreCount;
}

void GrooveRecorder::stop() {
    state_ = GrooveRecordState::Ready;
    playheadStep_ = 0U;
    playheadPhase256_ = 0U;
    countInRemaining_ = countInBeats_;
}

bool GrooveRecorder::service(
    const std::uint64_t masterPositionQ32,
    const std::uint8_t length) {
    const GrooveRecordState previousState = state_;
    const std::uint8_t previousPlayhead = playheadStep_;
    const std::uint8_t previousPhase = playheadPhase256_;
    const std::uint8_t previousCount = countInRemaining_;

    if (state_ == GrooveRecordState::PreCount) {
        if (masterPositionQ32 >= recordStartTargetQ32_) {
            state_ = GrooveRecordState::Recording;
            countInRemaining_ = 0U;
            playheadStep_ = 0U;
            playheadPhase256_ = 0U;
        } else {
            const std::uint64_t remainingQ32 = recordStartTargetQ32_ - masterPositionQ32;
            const std::uint64_t roundedBeats =
                (remainingQ32 + core::kQ32One - 1ULL) / core::kQ32One;
            countInRemaining_ = static_cast<std::uint8_t>(std::min<std::uint64_t>(roundedBeats, 64U));
            if (stepIntervalQ32_ != 0U && length > 0U && masterPositionQ32 >= preCountEpochQ32_) {
                const std::uint64_t elapsedQ32 = masterPositionQ32 - preCountEpochQ32_;
                const std::uint64_t serial = elapsedQ32 / stepIntervalQ32_;
                playheadStep_ = static_cast<std::uint8_t>(serial % length);
                const std::uint64_t remainderQ32 = elapsedQ32 % stepIntervalQ32_;
                playheadPhase256_ = static_cast<std::uint8_t>(
                    std::min<std::uint64_t>(255U, (remainderQ32 * 256ULL) / stepIntervalQ32_));
            }
        }
    }

    if (state_ == GrooveRecordState::Recording && stepIntervalQ32_ != 0U && length > 0U) {
        if (masterPositionQ32 < recordEpochQ32_) {
            playheadStep_ = 0U;
            playheadPhase256_ = 0U;
        } else {
            const std::uint64_t elapsedQ32 = masterPositionQ32 - recordEpochQ32_;
            const std::uint64_t serial = elapsedQ32 / stepIntervalQ32_;
            if (mode_ == GrooveRecordMode::OneShot && serial >= length) {
                stop();
            } else {
                playheadStep_ = static_cast<std::uint8_t>(serial % length);
                const std::uint64_t remainderQ32 = elapsedQ32 % stepIntervalQ32_;
                playheadPhase256_ = static_cast<std::uint8_t>(
                    std::min<std::uint64_t>(255U, (remainderQ32 * 256ULL) / stepIntervalQ32_));
            }
        }
    }

    return previousState != state_ || previousPlayhead != playheadStep_ ||
        previousPhase != playheadPhase256_ || previousCount != countInRemaining_;
}

bool GrooveRecorder::capture(
    const std::uint64_t tapPositionQ32,
    CustomGroovePattern& pattern) {
    if (state_ != GrooveRecordState::Recording || stepIntervalQ32_ == 0U ||
        !isCustomGroovePatternValid(pattern) || tapPositionQ32 < recordEpochQ32_) {
        return false;
    }

    const std::uint64_t relativeQ32 = tapPositionQ32 - recordEpochQ32_;
    const std::uint64_t nearestSerial =
        (relativeQ32 + stepIntervalQ32_ / 2ULL) / stepIntervalQ32_;
    if (mode_ == GrooveRecordMode::OneShot && nearestSerial >= pattern.length) {
        return false;
    }

    const std::uint8_t step = static_cast<std::uint8_t>(nearestSerial % pattern.length);
    const std::uint64_t serialOffsetQ32 = nearestSerial > UINT64_MAX / stepIntervalQ32_
        ? UINT64_MAX
        : nearestSerial * stepIntervalQ32_;
    const std::uint64_t nominalQ32 = serialOffsetQ32 > UINT64_MAX - recordEpochQ32_
        ? UINT64_MAX
        : recordEpochQ32_ + serialOffsetQ32;
    const bool late = tapPositionQ32 >= nominalQ32;
    const std::uint64_t magnitude = late
        ? tapPositionQ32 - nominalQ32
        : nominalQ32 - tapPositionQ32;
    const std::uint64_t scaled = (magnitude * 256ULL + stepIntervalQ32_ / 2ULL) / stepIntervalQ32_;
    const std::uint64_t boundedMagnitude = std::min<std::uint64_t>(
        scaled,
        static_cast<std::uint64_t>(kCustomGrooveMaximumOffset256));
    const std::int16_t signedOffset = late
        ? static_cast<std::int16_t>(boundedMagnitude)
        : -static_cast<std::int16_t>(boundedMagnitude);
    pattern.offsets256[step] = static_cast<std::int8_t>(signedOffset);
    capturedMask_ |= (1ULL << step);
    playheadStep_ = step;
    return true;
}

void GrooveRecorder::clearCaptured() {
    capturedMask_ = 0U;
}

GrooveRecorderView GrooveRecorder::view() const {
    return {
        mode_,
        state_,
        countInBeats_,
        countInRemaining_,
        playheadStep_,
        playheadPhase256_,
        capturedMask_};
}

}  // namespace clockfw::services
