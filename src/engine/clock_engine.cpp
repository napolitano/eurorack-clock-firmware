/**
 * @file clock_engine.cpp
 * @brief Deterministic shared scheduler for all eight gate channels.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "engine/clock_engine.h"

#include "clock_core.h"
#include "config.h"
#include "hal/interrupt_lock.h"
#include "engine/output_mode_resolver.h"

namespace clockfw::engine {

ClockEngine::ClockEngine(hal::GateOutputDriver& gateOutputs)
    : gateOutputs_(gateOutputs) {}

void ClockEngine::begin(const ClockState& state) {
    hal::InterruptLock interruptLock;
    copyConfigurationUnsafe(state);

    globalScheduleEpochQ32_ = masterPositionQ32_;
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        // All channels start from one shared musical epoch. This is essential for
        // phase-coherent CLOCK/EUCLID/SEQ boundaries at equivalent rates.
        channelRuntime_[channelIndex].scheduleEpochQ32 = globalScheduleEpochQ32_;
        // Different deterministic seeds avoid identical probability streams on every output.
        channelRuntime_[channelIndex].randomState ^=
            0x9E3779B9UL * static_cast<std::uint32_t>(channelIndex + 1U);
        scheduleChannelFromCurrentPosition(channelIndex, true);
    }
    // Startup stays paused until the external output stage is safely enabled.
    playing_ = false;
    restartPending_ = true;
}

void ClockEngine::updateConfiguration(const ClockState& state, const bool rescheduleChannels) {
    hal::InterruptLock interruptLock;
    copyConfigurationUnsafe(state);
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        const ChannelConfig& channel = configuration_.channels[channelIndex];
        if (channel.common.mode == ChannelMode::Off || channel.common.muted) {
            setGateState(channelIndex, false);
        }
        if (rescheduleChannels) {
            scheduleChannelFromCurrentPosition(channelIndex);
        }
        synchronizeChannelStepPhase(channelIndex);
    }
}

void ClockEngine::updateMasterTempo(const std::uint16_t bpm) {
    if (configuration_.bpm == bpm) {
        return;
    }
    hal::InterruptLock interruptLock;
    configuration_.bpm = bpm;
}

void ClockEngine::updateChannel(
    const std::size_t channelIndex,
    const ChannelConfig& channelConfiguration,
    const bool rescheduleChannel) {
    if (channelIndex >= kChannelCount) {
        return;
    }

    hal::InterruptLock interruptLock;
    configuration_.channels[channelIndex] = channelConfiguration;
    if (channelConfiguration.common.mode == ChannelMode::Off ||
        channelConfiguration.common.muted) {
        setGateState(channelIndex, false);
    }
    if (rescheduleChannel) {
        scheduleChannelFromCurrentPosition(channelIndex);
    }
    synchronizeChannelStepPhase(channelIndex);
}

void ClockEngine::play() {
    hal::InterruptLock interruptLock;
    playing_ = true;
}

void ClockEngine::pause() {
    hal::InterruptLock interruptLock;
    playing_ = false;
    restartPending_ = false;
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        setGateState(channelIndex, false);
    }
}

void ClockEngine::stop() {
    hal::InterruptLock interruptLock;
    playing_ = false;
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        setGateState(channelIndex, false);
    }
    resetRuntime();
}

void ClockEngine::resetGlobalPhase() {
    hal::InterruptLock interruptLock;
    resetRuntime();
}

void ClockEngine::resetGlobalPhaseFromIsr() {
    resetRuntime();
}

void ClockEngine::processSchedulerTick() {
    ++schedulerTickCounter_;

    // Gate-off checks always run, even while transport is paused or stopped.
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        ChannelRuntime& runtime = channelRuntime_[channelIndex];
        if (runtime.gateHigh &&
            static_cast<std::int32_t>(schedulerTickCounter_ - runtime.gateOffTick) >= 0) {
            setGateState(channelIndex, false);
        }
    }

    if (externalResetGate_) {
        return;
    }
    if (!playing_) {
        return;
    }
    if (configuration_.source == ClockSource::External &&
        !externalLocked_ &&
        configuration_.syncLossMode == SyncLossMode::Stop) {
        return;
    }

    const std::uint64_t masterIncrementQ32 = core::calculateMasterIncrementMilliBpmQ32(
        effectiveBpmMilli(),
        configuration_.beatUnit,
        config::kSchedulerFrequencyHz,
        masterRemainder_);

    masterPositionQ32_ += masterIncrementQ32;
    masterBeatPhaseQ32_ += masterIncrementQ32;
    while (masterBeatPhaseQ32_ >= core::kQ32One) {
        masterBeatPhaseQ32_ -= core::kQ32One;
        ++masterBeatSerial_;
        ++masterBeat_;

        const std::uint8_t beatsPerBar =
            configuration_.beatsPerBar != 0U ? configuration_.beatsPerBar : 1U;
        if (masterBeat_ > beatsPerBar) {
            masterBeat_ = 1U;
            ++masterBar_;
            if (masterBar_ > 999U) {
                masterBar_ = 1U;
            }
        }
    }

    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        if (configuration_.channels[channelIndex].common.mode == ChannelMode::Off) {
            setGateState(channelIndex, false);
            continue;
        }
        std::uint8_t eventGuard = 0U;
        while (masterPositionQ32_ >= channelRuntime_[channelIndex].nextEventQ32 &&
               eventGuard++ < 8U) {
            fireChannelEvent(channelIndex);
            ChannelRuntime& runtime = channelRuntime_[channelIndex];
            ++runtime.nextEventSerial;
            runtime.nextEventQ32 = calculateEventPositionQ32(
                channelIndex,
                runtime.nextEventSerial);
        }
    }
    restartPending_ = false;
}

EngineSnapshot ClockEngine::snapshot() const {
    EngineSnapshot result{};
    hal::InterruptLock interruptLock;
    result.playing = playing_;
    result.masterPositionQ32 = masterPositionQ32_;
    result.masterBeatPhaseQ32 = masterBeatPhaseQ32_;
    result.masterBeatSerial = masterBeatSerial_;
    result.masterBeat = masterBeat_;
    result.masterBar = masterBar_;
    result.externalLocked = externalLocked_;
    result.externalResetHeld = externalResetGate_;
    result.externalBpmMilli = externalBpmMilli_;
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        result.channelStep[channelIndex] = channelRuntime_[channelIndex].displayedStep;
    }
    return result;
}

#ifdef CLOCK_HOST_TEST
std::uint32_t ClockEngine::resetRuntimeCountForTest() const {
    return resetRuntimeCountForTest_;
}
#endif

void ClockEngine::setGateState(const std::size_t channelIndex, const bool high) {
    ChannelRuntime& runtime = channelRuntime_[channelIndex];
    if (runtime.gateHigh == high) {
        return;
    }
    runtime.gateHigh = high;
    gateOutputs_.setChannelState(channelIndex, high);
}

void ClockEngine::fireChannelEvent(const std::size_t channelIndex) {
    const ChannelConfig& channel = configuration_.channels[channelIndex];
    ChannelRuntime& runtime = channelRuntime_[channelIndex];

    if (channel.common.mode == ChannelMode::Off) {
        setGateState(channelIndex, false);
        return;
    }

    runtime.displayedStep = runtime.step;
    bool hit = false;
    switch (channel.common.mode) {
        case ChannelMode::Clock:
            hit = true;
            break;
        case ChannelMode::Euclid:
            hit = core::isEuclideanHit(runtime.step, channel.euclid);
            break;
        case ChannelMode::Sequencer:
            hit = core::isSequencerHit(runtime.step, channel.sequencer);
            break;
        case ChannelMode::Off:
            return;
    }
    if (channel.common.mode == ChannelMode::Euclid) {
        runtime.step = core::advanceStep(runtime.step, channel.euclid.steps);
    } else if (channel.common.mode == ChannelMode::Sequencer) {
        runtime.step = core::advanceStep(runtime.step, channel.sequencer.length);
    } else {
        const std::uint8_t localCycleLength =
            channel.clock.meter.beats != 0U ? channel.clock.meter.beats : 1U;
        runtime.step = core::advanceStep(runtime.step, localCycleLength);
    }

    if (channel.common.muted) {
        hit = false;
    } else if (hit) {
        hit = core::passesProbability(channel.common.probabilityPercent, runtime.randomState);
    }

    if (!hit) {
        return;
    }

    setGateState(channelIndex, true);

    const std::uint64_t baseIntervalUs = calculateBaseIntervalUs(channelIndex);
    const std::uint8_t swingPercent = channel.common.swingPercent > 50U
        ? 50U
        : channel.common.swingPercent;
    const std::uint64_t shortestSwingIntervalUs =
        (baseIntervalUs * (100ULL - swingPercent)) / 100ULL;
    const std::uint64_t humanizeUs = effectiveHumanizeUs(channelIndex);
    const std::uint64_t humanizePairAllowanceUs = humanizeUs * 2ULL;
    const std::uint64_t shortestActualIntervalUs =
        shortestSwingIntervalUs > humanizePairAllowanceUs
            ? shortestSwingIntervalUs - humanizePairAllowanceUs
            : config::kSchedulerTickUs;

    const std::uint32_t gateTicks = core::calculateGatePulseTicks(
        channel.common.gateLengthMs,
        config::kSchedulerTickUs,
        shortestActualIntervalUs,
        0U);
    runtime.gateOffTick = schedulerTickCounter_ + gateTicks;
}

void ClockEngine::scheduleChannelFromCurrentPosition(
    const std::size_t channelIndex,
    const bool includeCurrentBoundary) {
    ChannelRuntime& runtime = channelRuntime_[channelIndex];
    const ChannelConfig& channel = configuration_.channels[channelIndex];
    if (channel.common.resetMode == ResetMode::Global) {
        runtime.scheduleEpochQ32 = globalScheduleEpochQ32_;
    }

    const bool mayUseCurrentBoundary = includeCurrentBoundary || restartPending_;
    const std::uint64_t firstEventQ32 = calculateEventPositionQ32(channelIndex, 0U);
    std::uint64_t eventSerial = 0U;

    if (masterPositionQ32_ > firstEventQ32 ||
        (masterPositionQ32_ == firstEventQ32 && !mayUseCurrentBoundary)) {
        const std::uint64_t elapsedQ32 = masterPositionQ32_ - firstEventQ32;
        const std::uint64_t baseIntervalQ32 = calculateNominalBaseIntervalQ32(channelIndex);
        eventSerial = baseIntervalQ32 != 0U ? elapsedQ32 / baseIntervalQ32 : 0U;

        // The estimate above is intentionally cheap. Rational grids use exact
        // cumulative offsets below, so correct both directions before selecting
        // the first event that is still eligible.
        while (eventSerial > 0U) {
            const std::uint64_t previousPositionQ32 = calculateEventPositionQ32(
                channelIndex,
                eventSerial - 1U);
            const bool previousEligible = mayUseCurrentBoundary
                ? previousPositionQ32 >= masterPositionQ32_
                : previousPositionQ32 > masterPositionQ32_;
            if (!previousEligible) {
                break;
            }
            --eventSerial;
        }

        for (;;) {
            const std::uint64_t eventPositionQ32 = calculateEventPositionQ32(
                channelIndex,
                eventSerial);
            const bool eligible = mayUseCurrentBoundary
                ? eventPositionQ32 >= masterPositionQ32_
                : eventPositionQ32 > masterPositionQ32_;
            if (eligible || eventSerial == UINT64_MAX) {
                break;
            }
            ++eventSerial;
        }
    }

    runtime.nextEventSerial = eventSerial;
    runtime.nextEventQ32 = calculateEventPositionQ32(channelIndex, eventSerial);

    // GLOBAL means pattern position as well as edge timing is derived from the
    // shared epoch. Otherwise a live CLOCK -> EUCLID/SEQ switch can be on-grid
    // in time while evaluating the wrong pattern step.
    if (channel.common.resetMode == ResetMode::Global) {
        const std::uint8_t cycleLength = channelCycleLength(channelIndex);
        runtime.step = static_cast<std::uint8_t>(eventSerial % cycleLength);
        runtime.displayedStep = eventSerial == 0U
            ? static_cast<std::uint8_t>(0U)
            : static_cast<std::uint8_t>((eventSerial - 1ULL) % cycleLength);
    }
}

void ClockEngine::resetRuntime() {
#ifdef CLOCK_HOST_TEST
    ++resetRuntimeCountForTest_;
#endif
    restartPending_ = true;
    globalScheduleEpochQ32_ = masterPositionQ32_;
    masterBeatPhaseQ32_ = 0U;
    masterBeatSerial_ = 0U;
    masterRemainder_ = 0U;
    masterBeat_ = 1U;
    masterBar_ = 1U;

    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        setGateState(channelIndex, false);
        if (!core::shouldResetChannel(
                configuration_.channels[channelIndex].common.resetMode,
                false)) {
            continue;
        }

        ChannelRuntime& runtime = channelRuntime_[channelIndex];
        runtime.scheduleEpochQ32 = globalScheduleEpochQ32_;
        runtime.step = 0U;
        runtime.displayedStep = 0U;
        runtime.nextEventSerial = 0U;
        runtime.gateOffTick = 0U;
        scheduleChannelFromCurrentPosition(channelIndex, true);
    }
}

void ClockEngine::copyConfigurationUnsafe(const ClockState& state) {
    configuration_.bpm = state.bpm;
    configuration_.beatsPerBar = state.masterMeter.beats;
    configuration_.beatUnit = state.masterMeter.unit;
    configuration_.source = state.source;
    configuration_.syncLossMode = state.externalSync.lossMode;
    configuration_.operatingMode = state.operatingMode;
    configuration_.unifiedHumanizeUs = state.operatingMode == OperatingMode::UnifiedClock
        ? state.unifiedClock.humanizeUs
        : 0U;

    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        configuration_.channels[channelIndex] = resolvePhysicalOutputConfiguration(
            state, channelIndex);
    }
}

}  // namespace clockfw::engine
