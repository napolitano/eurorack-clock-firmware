/**
 * @file clock_engine_sequencer.cpp
 * @brief Sequencer 2.0 pattern/expression integration for the deterministic engine.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "engine/clock_engine.h"

#include <algorithm>

#include "clock_core.h"
#include "config.h"
#include "hal/interrupt_lock.h"

namespace clockfw::engine {
namespace {

std::uint64_t ceilDivide(const std::uint64_t numerator, const std::uint64_t denominator) {
    if (denominator == 0U) {
        return 1U;
    }
    return numerator / denominator + ((numerator % denominator) != 0U ? 1U : 0U);
}

}  // namespace

void ClockEngine::updateSequencerPattern(
    const std::size_t channelIndex,
    const SequencerPatternV2& pattern,
    const bool rescheduleChannel) {
    if (channelIndex >= kChannelCount || !isSequencerPatternValid(pattern)) {
        return;
    }
    hal::InterruptLock interruptLock;
    sequencerPatterns_[channelIndex] = pattern;
    clampSequencerPattern(sequencerPatterns_[channelIndex]);
    sequencerPatternV2Active_[channelIndex] = true;
    ChannelRuntime& runtime = channelRuntime_[channelIndex];
    if (runtime.tieHold) {
        setGateState(channelIndex, false);
    }
    runtime.tieHold = false;
    runtime.ratchetRemaining = 0U;
    if (rescheduleChannel) {
        scheduleChannelFromCurrentPosition(channelIndex);
    }
    synchronizeChannelStepPhase(channelIndex);
}

void ClockEngine::updateSequencerStepMetadata(
    const std::size_t channelIndex,
    const std::array<SequencerStepMetadataWord, kSequencerMaximumSteps>& metadata) {
    if (channelIndex >= kChannelCount) {
        return;
    }
    hal::InterruptLock interruptLock;
    sequencerStepMetadata_[channelIndex] = metadata;
    ChannelRuntime& runtime = channelRuntime_[channelIndex];
    if (runtime.tieHold) {
        setGateState(channelIndex, false);
    }
    runtime.tieHold = false;
    runtime.ratchetRemaining = 0U;
}

void ClockEngine::serviceSequencerRatchets() {
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        ChannelRuntime& runtime = channelRuntime_[channelIndex];
        std::uint8_t guard = 0U;
        while (runtime.ratchetRemaining > 0U &&
               masterPositionQ32_ >= runtime.nextRatchetQ32 && guard++ < 7U) {
            // A bounded ratchet must create a real rising edge. Normal pulse sizing
            // leaves a LOW gap, but force LOW defensively before the sub-event.
            setGateState(channelIndex, false);
            setGateState(channelIndex, true);
            runtime.gateOffTick = schedulerTickCounter_ + runtime.ratchetGateTicks;
            runtime.gateOffScheduled = true;
            --runtime.ratchetRemaining;
            if (runtime.ratchetRemaining > 0U) {
                runtime.nextRatchetQ32 += runtime.ratchetSpacingQ32;
            }
        }
    }
}

void ClockEngine::fireChannelEvent(const std::size_t channelIndex) {
    const ChannelConfig& channel = configuration_.channels[channelIndex];
    ChannelRuntime& runtime = channelRuntime_[channelIndex];

    if (channel.common.mode == ChannelMode::Off) {
        runtime.tieHold = false;
        runtime.ratchetRemaining = 0U;
        setGateState(channelIndex, false);
        return;
    }

    runtime.ratchetRemaining = 0U;
    runtime.displayedStep = runtime.step;
    bool hit = false;
    bool sequencerV2 = false;
    SequencerStepMetadata stepMetadata{};
    const SequencerPatternV2* activePattern = nullptr;

    switch (channel.common.mode) {
        case ChannelMode::Clock:
            hit = true;
            break;
        case ChannelMode::Euclid:
            hit = core::isEuclideanHit(runtime.step, channel.euclid);
            break;
        case ChannelMode::Sequencer:
            if (sequencerPatternV2Active_[channelIndex]) {
                sequencerV2 = true;
                activePattern = &sequencerPatterns_[channelIndex];
                const std::uint32_t seed = 0x53455132UL ^
                    (static_cast<std::uint32_t>(channelIndex + 1U) * 0x9E3779B9UL);
                const SequencerTraversalResult traversal = resolveSequencerTraversal(
                    runtime.nextEventSerial, *activePattern, seed);
                if (traversal.active) {
                    runtime.displayedStep = rotateSequencerStep(traversal.step, *activePattern);
                    hit = sequencerPatternGate(*activePattern, runtime.displayedStep);
                    stepMetadata = unpackSequencerStepMetadata(
                        sequencerStepMetadata_[channelIndex][runtime.displayedStep]);
                }
            } else {
                hit = core::isSequencerHit(runtime.step, channel.sequencer);
            }
            break;
        case ChannelMode::Off:
            return;
    }

    if (channel.common.mode == ChannelMode::Euclid) {
        runtime.step = core::advanceStep(runtime.step, channel.euclid.steps);
    } else if (channel.common.mode == ChannelMode::Sequencer && !sequencerV2) {
        runtime.step = core::advanceStep(runtime.step, channel.sequencer.length);
    } else if (channel.common.mode != ChannelMode::Sequencer) {
        const std::uint8_t localCycleLength =
            channel.clock.meter.beats != 0U ? channel.clock.meter.beats : 1U;
        runtime.step = core::advanceStep(runtime.step, localCycleLength);
    }

    const bool arrivedFromTie = runtime.tieHold;
    runtime.tieHold = false;
    if (channel.common.muted) {
        hit = false;
    } else if (hit) {
        const std::uint8_t probability = sequencerV2 && stepMetadata.probabilityPercent != 0U
            ? stepMetadata.probabilityPercent
            : channel.common.probabilityPercent;
        hit = core::passesProbability(probability, runtime.randomState);
    }

    if (!hit) {
        if (arrivedFromTie) {
            setGateState(channelIndex, false);
        }
        return;
    }

    const std::uint64_t currentEventQ32 = runtime.nextEventQ32;
    const std::uint64_t nextEventQ32 = calculateEventPositionQ32(
        channelIndex, runtime.nextEventSerial + 1U);
    const std::uint64_t q32PerTick = std::max<std::uint64_t>(minimumOutputIntervalQ32(), 1U);
    const std::uint64_t spanQ32 = nextEventQ32 > currentEventQ32
        ? nextEventQ32 - currentEventQ32
        : q32PerTick;
    const std::uint64_t spanTicks64 = std::max<std::uint64_t>(ceilDivide(spanQ32, q32PerTick), 1U);

    std::uint8_t ratchetCount = sequencerV2 ? stepMetadata.ratchetCount : 1U;
    const std::uint64_t maximumDistinctRatchets = std::max<std::uint64_t>(spanTicks64 / 2U, 1U);
    ratchetCount = static_cast<std::uint8_t>(std::min<std::uint64_t>(
        ratchetCount, std::min<std::uint64_t>(maximumDistinctRatchets, kSequencerMaximumRatchetCount)));
    const std::uint64_t subIntervalTicks64 = std::max<std::uint64_t>(spanTicks64 / ratchetCount, 1U);

    const std::uint8_t dutyPercent = sequencerV2
        ? sequencerGateProfileDutyPercent(stepMetadata.gateProfile)
        : 0U;
    std::uint32_t gateTicks = 1U;
    if (dutyPercent != 0U) {
        const std::uint64_t requested = std::max<std::uint64_t>(
            (subIntervalTicks64 * dutyPercent) / 100U, 1U);
        const std::uint64_t maximum = ratchetCount > 1U && subIntervalTicks64 > 1U
            ? subIntervalTicks64 - 1U
            : subIntervalTicks64;
        gateTicks = static_cast<std::uint32_t>(std::min(requested, maximum));
    } else {
        const std::uint16_t gateLengthMs = sequencerV2
            ? sequencerGateProfileMilliseconds(stepMetadata.gateProfile, channel.common.gateLengthMs)
            : channel.common.gateLengthMs;
        const std::uint64_t subIntervalUs = subIntervalTicks64 * config::kSchedulerTickUs;
        gateTicks = core::calculateGatePulseTicks(
            gateLengthMs, config::kSchedulerTickUs, subIntervalUs, 0U);
    }

    bool tieEnabled = false;
    if (sequencerV2 && activePattern != nullptr && stepMetadata.tie && ratchetCount == 1U &&
        activePattern->direction != SequencerPlayDirection::Random) {
        const std::uint32_t seed = 0x53455132UL ^
            (static_cast<std::uint32_t>(channelIndex + 1U) * 0x9E3779B9UL);
        const SequencerTraversalResult nextTraversal = resolveSequencerTraversal(
            runtime.nextEventSerial + 1U, *activePattern, seed);
        if (nextTraversal.active) {
            const std::uint8_t nextStep = rotateSequencerStep(nextTraversal.step, *activePattern);
            const SequencerStepMetadata nextMetadata = unpackSequencerStepMetadata(
                sequencerStepMetadata_[channelIndex][nextStep]);
            tieEnabled = sequencerPatternGate(*activePattern, nextStep) &&
                nextMetadata.ratchetCount == 1U;
        }
    }

    if (arrivedFromTie && ratchetCount > 1U) {
        setGateState(channelIndex, false);
    }
    setGateState(channelIndex, true);

    if (tieEnabled) {
        runtime.tieHold = true;
        runtime.gateOffScheduled = false;
        return;
    }

    runtime.gateOffTick = schedulerTickCounter_ + gateTicks;
    runtime.gateOffScheduled = true;
    if (ratchetCount > 1U) {
        runtime.ratchetRemaining = static_cast<std::uint8_t>(ratchetCount - 1U);
        runtime.ratchetSpacingQ32 = std::max<std::uint64_t>(spanQ32 / ratchetCount, q32PerTick);
        runtime.nextRatchetQ32 = currentEventQ32 + runtime.ratchetSpacingQ32;
        runtime.ratchetGateTicks = gateTicks;
    }
}

}  // namespace clockfw::engine
