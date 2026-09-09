/**
 * @file clock_engine_timing.cpp
 * @brief Timing-rate calculations and scheduler-resolution limiting for ClockEngine.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "engine/clock_engine.h"

#include <algorithm>

#include "clock_core.h"
#include "config.h"

namespace clockfw::engine {

std::uint32_t ClockEngine::effectiveBpmMilli() const {
    std::uint32_t bpmMilli = static_cast<std::uint32_t>(configuration_.bpm) * 1000U;
    if (configuration_.source == ClockSource::External) {
        if ((externalLocked_ || configuration_.syncLossMode == SyncLossMode::Freewheel) &&
            externalBpmMilli_ != 0U) {
            bpmMilli = externalBpmMilli_;
        }
    } else if (configuration_.source == ClockSource::Auto && externalLocked_ &&
               externalBpmMilli_ != 0U) {
        bpmMilli = externalBpmMilli_;
    }
    return bpmMilli;
}

std::uint64_t ClockEngine::minimumOutputIntervalQ32() const {
    const std::uint32_t bpmMilli = effectiveBpmMilli();
    if (bpmMilli == 0U) {
        return 1U;
    }
    const std::uint8_t beatUnit = configuration_.beatUnit != 0U ? configuration_.beatUnit : 4U;
    const std::uint64_t numerator =
        static_cast<std::uint64_t>(bpmMilli) * beatUnit * core::kQ32One;
    const std::uint64_t denominator =
        60ULL * 4ULL * config::kSchedulerFrequencyHz * 1000ULL;
    return (numerator + denominator - 1ULL) / denominator;
}

void ClockEngine::resolveChannelRate(
    const std::size_t channelIndex,
    std::uint32_t& numerator,
    std::uint32_t& denominator) const {
    const ChannelConfig& channel = configuration_.channels[channelIndex];
    numerator = 1U;
    denominator = 1U;

    if (channel.common.mode == ChannelMode::Clock) {
        core::calculateEffectiveClockRate(
            channel.common,
            channel.clock.meter.unit,
            configuration_.beatUnit,
            numerator,
            denominator);
    } else if (channel.common.mode == ChannelMode::Euclid ||
               channel.common.mode == ChannelMode::Sequencer) {
        core::calculateEffectivePatternRate(
            channel.common,
            configuration_.beatUnit,
            numerator,
            denominator);
    }
}

std::uint64_t ClockEngine::calculateNominalBaseIntervalQ32(
    const std::size_t channelIndex) const {
    std::uint32_t numerator = 1U;
    std::uint32_t denominator = 1U;
    resolveChannelRate(channelIndex, numerator, denominator);

    volatile std::uint32_t ignoredRemainder = 0U;
    const std::uint64_t interval =
        core::calculateNextIntervalQ32(numerator, denominator, ignoredRemainder);
    return std::max(interval, minimumOutputIntervalQ32());
}

std::uint64_t ClockEngine::calculateBaseEventOffsetQ32(
    const std::size_t channelIndex,
    const std::uint64_t eventSerial) const {
    if (eventSerial == 0U) {
        return 0U;
    }

    std::uint32_t numerator = 1U;
    std::uint32_t denominator = 1U;
    resolveChannelRate(channelIndex, numerator, denominator);
    if (numerator == 0U) {
        numerator = 1U;
    }

    const std::uint64_t scaledInterval =
        static_cast<std::uint64_t>(denominator) * core::kQ32One;
    const std::uint64_t quotient = scaledInterval / numerator;
    const std::uint64_t remainder = scaledInterval % numerator;
    const std::uint64_t minimumInterval = minimumOutputIntervalQ32();

    // Once the requested rate exceeds the scheduler's physical edge resolution,
    // the effective grid is deliberately clamped to one scheduler interval.
    if (quotient < minimumInterval) {
        if (eventSerial > UINT64_MAX / minimumInterval) {
            return UINT64_MAX;
        }
        return eventSerial * minimumInterval;
    }

    // floor(serial * scaledInterval / numerator), evaluated without requiring
    // a 128-bit multiply. Splitting serial into full numerator-sized groups
    // keeps every intermediate bounded by the final representable result.
    if (quotient != 0U && eventSerial > UINT64_MAX / quotient) {
        return UINT64_MAX;
    }
    const std::uint64_t whole = eventSerial * quotient;
    const std::uint64_t groups = eventSerial / numerator;
    const std::uint64_t tail = eventSerial % numerator;
    if (remainder != 0U && groups > (UINT64_MAX - whole) / remainder) {
        return UINT64_MAX;
    }
    const std::uint64_t groupedRemainder = groups * remainder;
    const std::uint64_t tailRemainder = (tail * remainder) / numerator;
    if (tailRemainder > UINT64_MAX - whole - groupedRemainder) {
        return UINT64_MAX;
    }
    return whole + groupedRemainder + tailRemainder;
}

std::uint64_t ClockEngine::calculateEventPositionQ32(
    const std::size_t channelIndex,
    const std::uint64_t eventSerial) const {
    const ChannelRuntime& runtime = channelRuntime_[channelIndex];
    const std::uint64_t baseIntervalQ32 = calculateNominalBaseIntervalQ32(channelIndex);
    const std::uint64_t phaseOffsetQ32 = core::calculatePhaseOffsetQ32(
        baseIntervalQ32,
        configuration_.channels[channelIndex].common.phasePercent);
    const std::uint64_t baseOffsetQ32 = calculateBaseEventOffsetQ32(channelIndex, eventSerial);
    const std::uint64_t swingOffsetQ32 = (eventSerial & 1ULL) != 0ULL
        ? calculateSwingDeltaQ32(channelIndex, baseIntervalQ32)
        : 0U;

    if (runtime.scheduleEpochQ32 > UINT64_MAX - phaseOffsetQ32) {
        return UINT64_MAX;
    }
    const std::uint64_t phasePositionQ32 = runtime.scheduleEpochQ32 + phaseOffsetQ32;
    if (baseOffsetQ32 > UINT64_MAX - phasePositionQ32) {
        return UINT64_MAX;
    }
    const std::uint64_t nominalPositionQ32 = phasePositionQ32 + baseOffsetQ32;
    if (swingOffsetQ32 > UINT64_MAX - nominalPositionQ32) {
        return UINT64_MAX;
    }
    const std::uint64_t swungPositionQ32 = nominalPositionQ32 + swingOffsetQ32;
    const std::int64_t humanizeOffsetQ32 = calculateHumanizeOffsetQ32(channelIndex, eventSerial);
    if (humanizeOffsetQ32 >= 0) {
        const std::uint64_t delayQ32 = static_cast<std::uint64_t>(humanizeOffsetQ32);
        return delayQ32 > UINT64_MAX - swungPositionQ32
            ? UINT64_MAX
            : swungPositionQ32 + delayQ32;
    }

    const std::uint64_t advanceQ32 = static_cast<std::uint64_t>(-humanizeOffsetQ32);
    const std::uint64_t earliestPositionQ32 = phasePositionQ32;
    return advanceQ32 >= swungPositionQ32 - earliestPositionQ32
        ? earliestPositionQ32
        : swungPositionQ32 - advanceQ32;
}

std::uint64_t ClockEngine::calculateBaseIntervalUs(const std::size_t channelIndex) const {
    std::uint32_t numerator = 1U;
    std::uint32_t denominator = 1U;
    resolveChannelRate(channelIndex, numerator, denominator);

    std::uint32_t bpmMilli = effectiveBpmMilli();
    if (bpmMilli == 0U) {
        bpmMilli = 120000U;
    }
    const std::uint8_t beatUnit = configuration_.beatUnit != 0U ? configuration_.beatUnit : 4U;
    const std::uint64_t divisor =
        static_cast<std::uint64_t>(bpmMilli) * beatUnit * numerator;
    if (divisor == 0U) {
        return config::kSchedulerTickUs;
    }
    const std::uint64_t intervalUs =
        (60000000000ULL * 4ULL * denominator) / divisor;
    return std::max<std::uint64_t>(intervalUs, config::kSchedulerTickUs);
}



std::uint16_t ClockEngine::effectiveHumanizeUs(const std::size_t channelIndex) const {
    if (configuration_.operatingMode != OperatingMode::UnifiedClock ||
        configuration_.unifiedHumanizeUs == 0U || channelIndex >= kChannelCount) {
        return 0U;
    }

    const ChannelConfig& channel = configuration_.channels[channelIndex];
    const std::uint64_t baseIntervalUs = calculateBaseIntervalUs(channelIndex);
    const std::uint8_t swingPercent = std::min<std::uint8_t>(channel.common.swingPercent, 50U);
    const std::uint64_t shortestSwingIntervalUs =
        (baseIntervalUs * (100ULL - swingPercent)) / 100ULL;
    if (shortestSwingIntervalUs <= config::kSchedulerTickUs) {
        return 0U;
    }

    // Adjacent events may receive opposite signed offsets. Reserve one complete
    // scheduler quantum between them even at the shortest swung interval.
    const std::uint64_t maximumSafeUs =
        (shortestSwingIntervalUs - config::kSchedulerTickUs) / 2ULL;
    const std::uint64_t quantizedSafeUs =
        (maximumSafeUs / config::kSchedulerTickUs) * config::kSchedulerTickUs;
    const std::uint64_t configuredUs = std::min<std::uint64_t>(
        configuration_.unifiedHumanizeUs, config::kMaximumHumanizeUs);
    return static_cast<std::uint16_t>(std::min(configuredUs, quantizedSafeUs));
}

std::int64_t ClockEngine::calculateHumanizeOffsetQ32(
    const std::size_t channelIndex,
    const std::uint64_t eventSerial) const {
    // Keep the shared restart/downbeat exact. Humanization begins with the next edge.
    if (eventSerial == 0U) {
        return 0;
    }
    const std::uint16_t humanizeUs = effectiveHumanizeUs(channelIndex);
    const std::uint32_t maximumTicks = humanizeUs / config::kSchedulerTickUs;
    if (maximumTicks == 0U) {
        return 0;
    }

    // Stateless hash: rescheduling the same event reproduces exactly the same
    // per-output offset, so live UI edits cannot advance a hidden RNG stream.
    std::uint32_t hash = static_cast<std::uint32_t>(eventSerial) ^
        static_cast<std::uint32_t>(eventSerial >> 32U) ^
        (static_cast<std::uint32_t>(channelIndex + 1U) * 0x9E3779B9U);
    hash ^= hash >> 16U;
    hash *= 0x7FEB352DU;
    hash ^= hash >> 15U;
    hash *= 0x846CA68BU;
    hash ^= hash >> 16U;

    const std::uint32_t span = maximumTicks * 2U + 1U;
    const std::int32_t signedTicks = static_cast<std::int32_t>(hash % span) -
        static_cast<std::int32_t>(maximumTicks);
    const std::int64_t tickQ32 = static_cast<std::int64_t>(minimumOutputIntervalQ32());
    return static_cast<std::int64_t>(signedTicks) * tickQ32;
}

std::uint8_t ClockEngine::channelCycleLength(const std::size_t channelIndex) const {
    const ChannelConfig& channel = configuration_.channels[channelIndex];
    if (channel.common.mode == ChannelMode::Euclid) {
        return channel.euclid.steps != 0U ? channel.euclid.steps : 1U;
    }
    if (channel.common.mode == ChannelMode::Sequencer) {
        return channel.sequencer.length != 0U ? channel.sequencer.length : 1U;
    }
    if (channel.common.mode == ChannelMode::Clock) {
        return channel.clock.meter.beats != 0U ? channel.clock.meter.beats : 1U;
    }
    return 1U;
}

void ClockEngine::synchronizeChannelStepPhase(const std::size_t channelIndex) {
    ChannelRuntime& runtime = channelRuntime_[channelIndex];
    const std::uint8_t cycleLength = channelCycleLength(channelIndex);
    if (configuration_.channels[channelIndex].common.resetMode == ResetMode::Global) {
        runtime.step = static_cast<std::uint8_t>(runtime.nextEventSerial % cycleLength);
        runtime.displayedStep = runtime.nextEventSerial == 0U
            ? static_cast<std::uint8_t>(0U)
            : static_cast<std::uint8_t>((runtime.nextEventSerial - 1ULL) % cycleLength);
        return;
    }

    runtime.step = static_cast<std::uint8_t>(runtime.step % cycleLength);
    runtime.displayedStep = static_cast<std::uint8_t>(runtime.displayedStep % cycleLength);
}

std::uint64_t ClockEngine::calculateSwingDeltaQ32(
    const std::size_t channelIndex,
    const std::uint64_t baseIntervalQ32) const {
    const std::uint64_t minimumIntervalQ32 = minimumOutputIntervalQ32();
    std::uint8_t swingPercent = configuration_.channels[channelIndex].common.swingPercent;
    if (swingPercent > 50U) {
        swingPercent = 50U;
    }
    if (swingPercent == 0U || baseIntervalQ32 <= minimumIntervalQ32) {
        return 0U;
    }
    const std::uint64_t requestedDelta = (baseIntervalQ32 * swingPercent) / 100ULL;
    return std::min(requestedDelta, baseIntervalQ32 - minimumIntervalQ32);
}


}  // namespace clockfw::engine
