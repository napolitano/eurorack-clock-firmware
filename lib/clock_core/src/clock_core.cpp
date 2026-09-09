/**
 * @file clock_core.cpp
 * @brief Hardware-independent timing, rhythm, and pattern mathematics.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "clock_core.h"

namespace clockfw::core {

std::uint32_t greatestCommonDivisor(std::uint32_t first, std::uint32_t second) {
    while (second != 0U) {
        const std::uint32_t remainder = first % second;
        first = second;
        second = remainder;
    }
    return first != 0U ? first : 1U;
}

void calculateEffectiveRate(
    const CommonChannelSettings& settings,
    std::uint32_t& numerator,
    std::uint32_t& denominator) {
    numerator = settings.rate.numerator != 0U ? settings.rate.numerator : 1U;
    denominator = settings.rate.denominator != 0U ? settings.rate.denominator : 1U;

    const std::uint32_t integerFactor = settings.rate.factor != 0U ? settings.rate.factor : 1U;
    if (settings.rate.mode == ClockRatioMode::Multiply) {
        numerator *= integerFactor;
    } else {
        denominator *= integerFactor;
    }

    const std::uint32_t divisor = greatestCommonDivisor(numerator, denominator);
    numerator /= divisor;
    denominator /= divisor;
}

void calculateEffectiveClockRate(
    const CommonChannelSettings& settings,
    const std::uint8_t localBeatUnit,
    const std::uint8_t masterBeatUnit,
    std::uint32_t& numerator,
    std::uint32_t& denominator) {
    calculateEffectiveRate(settings, numerator, denominator);

    numerator *= localBeatUnit != 0U ? localBeatUnit : 4U;
    denominator *= masterBeatUnit != 0U ? masterBeatUnit : 4U;

    const std::uint32_t divisor = greatestCommonDivisor(numerator, denominator);
    numerator /= divisor;
    denominator /= divisor;
}

void calculateEffectivePatternRate(
    const CommonChannelSettings& settings,
    const std::uint8_t masterBeatUnit,
    std::uint32_t& numerator,
    std::uint32_t& denominator) {
    calculateEffectiveRate(settings, numerator, denominator);

    // The pattern engine's x1 grid is a sixteenth note. The master Q32
    // timeline represents one current meter beat, so convert 1/16 note into
    // master-beat units without using floating point.
    numerator *= 16U;
    denominator *= masterBeatUnit != 0U ? masterBeatUnit : 4U;

    const std::uint32_t divisor = greatestCommonDivisor(numerator, denominator);
    numerator /= divisor;
    denominator /= divisor;
}

std::uint64_t calculateNextIntervalQ32(
    std::uint32_t numerator,
    std::uint32_t denominator,
    volatile std::uint32_t& remainder) {
    if (numerator == 0U) {
        numerator = 1U;
    }
    if (denominator == 0U) {
        denominator = 1U;
    }

    const std::uint64_t scaledNumerator =
        static_cast<std::uint64_t>(denominator) * kQ32One + remainder;
    const std::uint64_t interval = scaledNumerator / numerator;
    remainder = static_cast<std::uint32_t>(scaledNumerator % numerator);

    // denominator is normalized to at least one, so scaledNumerator is at least
    // 2^32 while numerator is at most UINT32_MAX. The quotient is therefore
    // guaranteed to be at least one.
    return interval;
}

std::uint64_t calculateMasterIncrementQ32(
    const std::uint16_t bpm,
    std::uint8_t beatUnit,
    std::uint32_t schedulerFrequencyHz,
    volatile std::uint64_t& remainder) {
    if (bpm == 0U) {
        remainder = 0U;
        return 0U;
    }
    if (beatUnit == 0U) {
        beatUnit = 4U;
    }
    if (schedulerFrequencyHz == 0U) {
        schedulerFrequencyHz = 1U;
    }

    const std::uint64_t scaledNumerator =
        static_cast<std::uint64_t>(bpm) * beatUnit * kQ32One + remainder;
    const std::uint64_t denominator = 60ULL * 4ULL * schedulerFrequencyHz;
    const std::uint64_t increment = scaledNumerator / denominator;
    remainder = scaledNumerator % denominator;
    return increment;
}

std::uint64_t calculateMasterIncrementMilliBpmQ32(
    const std::uint32_t bpmMilli,
    std::uint8_t beatUnit,
    std::uint32_t schedulerFrequencyHz,
    volatile std::uint64_t& remainder) {
    if (bpmMilli == 0U) {
        remainder = 0U;
        return 0U;
    }
    if (beatUnit == 0U) {
        beatUnit = 4U;
    }
    if (schedulerFrequencyHz == 0U) {
        schedulerFrequencyHz = 1U;
    }

    const std::uint64_t scaledNumerator =
        static_cast<std::uint64_t>(bpmMilli) * beatUnit * kQ32One + remainder;
    const std::uint64_t denominator = 60ULL * 4ULL * schedulerFrequencyHz * 1000ULL;
    const std::uint64_t increment = scaledNumerator / denominator;
    remainder = scaledNumerator % denominator;
    return increment;
}

std::uint64_t applySwing(
    const std::uint64_t baseIntervalQ32,
    std::uint8_t swingPercent,
    const bool longInterval) {
    if (swingPercent == 0U || baseIntervalQ32 <= 1U) {
        return baseIntervalQ32;
    }
    if (swingPercent > 50U) {
        swingPercent = 50U;
    }

    const std::uint64_t swingDelta = (baseIntervalQ32 * swingPercent) / 100ULL;
    return longInterval ? baseIntervalQ32 + swingDelta : baseIntervalQ32 - swingDelta;
}

std::uint64_t calculatePhaseOffsetQ32(
    const std::uint64_t intervalQ32,
    std::uint8_t phasePercent) {
    if (phasePercent > 99U) {
        phasePercent = 99U;
    }
    return (intervalQ32 * phasePercent) / 100ULL;
}

std::uint32_t nextXorshift32(std::uint32_t value) {
    value ^= value << 13U;
    value ^= value >> 17U;
    value ^= value << 5U;
    return value;
}

bool passesProbability(
    const std::uint8_t probabilityPercent,
    std::uint32_t& randomState) {
    if (probabilityPercent == 0U) {
        return false;
    }
    if (probabilityPercent >= 100U) {
        return true;
    }
    if (randomState == 0U) {
        randomState = 0x6D2B79F5U;
    }

    randomState = nextXorshift32(randomState);
    return (randomState % 100U) < probabilityPercent;
}

std::uint32_t calculateGatePulseTicks(
    const std::uint16_t gateLengthMs,
    std::uint32_t schedulerTickUs,
    const std::uint64_t baseIntervalUs,
    std::uint8_t swingPercent) {
    if (schedulerTickUs == 0U) {
        schedulerTickUs = 1U;
    }
    if (swingPercent > 50U) {
        swingPercent = 50U;
    }

    std::uint64_t requestedTicks64 =
        (static_cast<std::uint64_t>(gateLengthMs) * 1000ULL + schedulerTickUs - 1U) /
        schedulerTickUs;
    if (requestedTicks64 == 0U) {
        requestedTicks64 = 1U;
    }
    // gateLengthMs is uint16_t, so even at a 1 us scheduler quantum the
    // requested duration is below UINT32_MAX.
    const std::uint32_t requestedTicks = static_cast<std::uint32_t>(requestedTicks64);

    // A gate may consume at most half of the shortest swing-adjusted interval.
    const std::uint64_t shortestIntervalUs =
        (baseIntervalUs * (100ULL - swingPercent)) / 100ULL;
    std::uint64_t maximumTicks64 = (shortestIntervalUs / 2ULL) / schedulerTickUs;
    if (maximumTicks64 == 0U) {
        maximumTicks64 = 1U;
    }

    const std::uint32_t maximumTicks = maximumTicks64 > 0xFFFFFFFFULL
        ? 0xFFFFFFFFU
        : static_cast<std::uint32_t>(maximumTicks64);
    return requestedTicks > maximumTicks ? maximumTicks : requestedTicks;
}

std::uint8_t advanceStep(const std::uint8_t currentStep, const std::uint8_t patternLength) {
    const std::uint8_t safeLength = patternLength != 0U ? patternLength : 1U;
    return static_cast<std::uint8_t>(
        (static_cast<std::uint16_t>(currentStep) + 1U) % safeLength);
}

std::uint64_t patternMask(const std::uint8_t patternLength) {
    if (patternLength == 0U) {
        return 0ULL;
    }
    if (patternLength >= 64U) {
        return ~0ULL;
    }
    return (1ULL << patternLength) - 1ULL;
}

std::uint64_t clampPattern(const std::uint64_t pattern, const std::uint8_t patternLength) {
    return pattern & patternMask(patternLength);
}

std::uint64_t invertPattern(const std::uint64_t pattern, const std::uint8_t patternLength) {
    return (~pattern) & patternMask(patternLength);
}

std::uint64_t alternatingPattern(const std::uint8_t patternLength, const bool firstStepOn) {
    const std::uint64_t basePattern =
        firstStepOn ? 0x5555555555555555ULL : 0xAAAAAAAAAAAAAAAAULL;
    return basePattern & patternMask(patternLength);
}

bool isEuclideanHit(const std::uint8_t step, const EuclidSettings& settings) {
    if (settings.steps == 0U || settings.hits == 0U) {
        return false;
    }

    const std::uint8_t boundedHits = settings.hits > settings.steps
        ? settings.steps
        : settings.hits;
    const std::uint8_t rotatedPosition = static_cast<std::uint8_t>(
        (step + settings.rotation) % settings.steps);
    return static_cast<std::uint16_t>(rotatedPosition) * boundedHits % settings.steps < boundedHits;
}

bool isSequencerHit(const std::uint8_t step, const SequencerSettings& settings) {
    if (settings.length == 0U) {
        return false;
    }

    const std::uint8_t rotatedPosition = static_cast<std::uint8_t>(
        (step + settings.rotation) % settings.length);
    return ((settings.pattern >> rotatedPosition) & 1ULL) != 0ULL;
}

bool shouldResetChannel(const ResetMode resetMode, const bool includeFreeRunningChannels) {
    return includeFreeRunningChannels || resetMode == ResetMode::Global;
}

}  // namespace clockfw::core
