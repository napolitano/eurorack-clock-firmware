/**
 * @file clock_engine_sync.cpp
 * @brief External synchronization and phase-alignment logic for ClockEngine.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "engine/clock_engine.h"

#include <cstdint>

#include "clock_core.h"
#include "hal/interrupt_lock.h"

namespace clockfw::engine {
namespace {

void shiftPosition(
    volatile std::uint64_t& position,
    const std::uint64_t expected,
    const std::uint64_t observed) {
    if (observed >= expected) {
        position += observed - expected;
    } else {
        const std::uint64_t correction = expected - observed;
        position = position > correction ? position - correction : 0U;
    }
}

void shiftPosition(
    std::uint64_t& position,
    const std::uint64_t expected,
    const std::uint64_t observed) {
    if (observed >= expected) {
        position += observed - expected;
    } else {
        const std::uint64_t correction = expected - observed;
        position = position > correction ? position - correction : 0U;
    }
}

}  // namespace

void ClockEngine::setExternalLock(const bool locked, const std::uint32_t bpmMilli) {
    hal::InterruptLock interruptLock;
    setExternalLockFromIsr(locked, bpmMilli);
}

void ClockEngine::setExternalLockFromIsr(const bool locked, const std::uint32_t bpmMilli) {
    externalLocked_ = locked;
    externalBpmMilli_ = bpmMilli;
    if (!locked) {
        externalPhaseInitialized_ = false;
        externalPulseRemainder_ = 0U;
        externalMusicalPositionQ32_ = 0U;
    }
}

void ClockEngine::setExternalResetGateFromIsr(const bool high) {
    if (externalResetGate_ == high) {
        return;
    }
    externalResetGate_ = high;
    resetRuntime();
}

void ClockEngine::acceptExternalPulse(
    const std::uint32_t bpmMilli,
    const std::uint8_t pulsesPerQuarterNote) {
    hal::InterruptLock interruptLock;
    acceptExternalPulseUnsafe(bpmMilli, pulsesPerQuarterNote);
}

void ClockEngine::acceptExternalPulseFromIsr(
    const std::uint32_t bpmMilli,
    const std::uint8_t pulsesPerQuarterNote) {
    acceptExternalPulseUnsafe(bpmMilli, pulsesPerQuarterNote);
}

void ClockEngine::acceptExternalPulseUnsafe(
    const std::uint32_t bpmMilli,
    std::uint8_t pulsesPerQuarterNote) {
    externalLocked_ = true;
    externalBpmMilli_ = bpmMilli;
    if (configuration_.source == ClockSource::Internal) {
        return;
    }
    if (pulsesPerQuarterNote == 0U) {
        pulsesPerQuarterNote = 1U;
    }

    if (!externalPhaseInitialized_) {
        externalReferenceQ32_ = masterPositionQ32_;
        externalMusicalPositionQ32_ = 0U;
        externalPulseRemainder_ = 0U;
        resetRuntime();
        externalPhaseInitialized_ = true;
        return;
    }

    const std::uint32_t pulseNumerator =
        static_cast<std::uint32_t>(4U) * pulsesPerQuarterNote;
    const std::uint32_t pulseDenominator =
        configuration_.beatUnit != 0U ? configuration_.beatUnit : 4U;
    const std::uint64_t pulseIntervalQ32 = core::calculateNextIntervalQ32(
        pulseNumerator,
        pulseDenominator,
        externalPulseRemainder_);
    const std::uint64_t expectedPulsePositionQ32 = externalReferenceQ32_ + pulseIntervalQ32;
    externalMusicalPositionQ32_ += pulseIntervalQ32;

    alignGlobalSchedulesToExternalPulse(expectedPulsePositionQ32);
    externalReferenceQ32_ = masterPositionQ32_;
    masterBeatPhaseQ32_ = externalMusicalPositionQ32_ % core::kQ32One;

    const std::uint64_t completedBeats = externalMusicalPositionQ32_ / core::kQ32One;
    masterBeatSerial_ = static_cast<std::uint32_t>(completedBeats & 0xFFFFFFFFULL);
    const std::uint8_t beatsPerBar =
        configuration_.beatsPerBar != 0U ? configuration_.beatsPerBar : 1U;
    masterBeat_ = static_cast<std::uint8_t>(completedBeats % beatsPerBar) + 1U;
    masterBar_ = static_cast<std::uint16_t>((completedBeats / beatsPerBar) % 999ULL) + 1U;
}

void ClockEngine::alignGlobalSchedulesToExternalPulse(
    const std::uint64_t expectedPulsePositionQ32) {
    const std::uint64_t observedPulsePositionQ32 = masterPositionQ32_;
    if (expectedPulsePositionQ32 == observedPulsePositionQ32) {
        return;
    }

    shiftPosition(globalScheduleEpochQ32_, expectedPulsePositionQ32, observedPulsePositionQ32);
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        if (configuration_.channels[channelIndex].common.resetMode != ResetMode::Global) {
            continue;
        }
        ChannelRuntime& runtime = channelRuntime_[channelIndex];
        shiftPosition(runtime.scheduleEpochQ32, expectedPulsePositionQ32, observedPulsePositionQ32);
        shiftPosition(runtime.nextEventQ32, expectedPulsePositionQ32, observedPulsePositionQ32);
    }
}

}  // namespace clockfw::engine
