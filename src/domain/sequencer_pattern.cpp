/**
 * @file sequencer_pattern.cpp
 * @brief Sequencer 2.0 pattern validation, bit access, and traversal.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "domain/sequencer_pattern.h"

#include <cstddef>

namespace clockfw {
namespace {

bool validDirection(const SequencerPlayDirection direction) {
    return direction == SequencerPlayDirection::Forward ||
        direction == SequencerPlayDirection::Reverse ||
        direction == SequencerPlayDirection::PingPong ||
        direction == SequencerPlayDirection::Random;
}

bool validLoopMode(const SequencerLoopMode mode) {
    return mode == SequencerLoopMode::Loop || mode == SequencerLoopMode::Once;
}

std::uint32_t mixRandomEvent(std::uint64_t eventSerial, std::uint32_t seed) {
    // Small deterministic avalanche hash. It is not cryptographic randomness;
    // it merely provides independent, history-free musical step selection.
    std::uint32_t value = static_cast<std::uint32_t>(eventSerial) ^
        static_cast<std::uint32_t>(eventSerial >> 32U) ^ seed ^ 0x9E3779B9U;
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    value *= 0x846CA68BU;
    value ^= value >> 16U;
    return value;
}

}  // namespace

bool isSequencerPatternValid(const SequencerPatternV2& pattern) {
    return pattern.length >= 1U && pattern.length <= kSequencerMaximumSteps &&
        pattern.rotation < pattern.length && validDirection(pattern.direction) &&
        validLoopMode(pattern.loopMode);
}

bool sequencerPatternGate(const SequencerPatternV2& pattern, const std::uint8_t step) {
    if (step >= kSequencerMaximumSteps) {
        return false;
    }
    const std::size_t wordIndex = static_cast<std::size_t>(step / 64U);
    const std::uint8_t bitIndex = static_cast<std::uint8_t>(step % 64U);
    return ((pattern.gates[wordIndex] >> bitIndex) & 1ULL) != 0ULL;
}

bool setSequencerPatternGate(
    SequencerPatternV2& pattern,
    const std::uint8_t step,
    const bool enabled) {
    if (step >= kSequencerMaximumSteps) {
        return false;
    }
    const std::size_t wordIndex = static_cast<std::size_t>(step / 64U);
    const std::uint8_t bitIndex = static_cast<std::uint8_t>(step % 64U);
    const std::uint64_t mask = 1ULL << bitIndex;
    if (enabled) {
        pattern.gates[wordIndex] |= mask;
    } else {
        pattern.gates[wordIndex] &= ~mask;
    }
    return true;
}

void clampSequencerPattern(SequencerPatternV2& pattern) {
    if (pattern.length == 0U) {
        pattern.gates = {{0ULL, 0ULL}};
        pattern.rotation = 0U;
        return;
    }
    if (pattern.length < 64U) {
        pattern.gates[0] &= (1ULL << pattern.length) - 1ULL;
        pattern.gates[1] = 0ULL;
    } else if (pattern.length == 64U) {
        pattern.gates[1] = 0ULL;
    } else if (pattern.length < kSequencerMaximumSteps) {
        const std::uint8_t upperBits = static_cast<std::uint8_t>(pattern.length - 64U);
        pattern.gates[1] &= (1ULL << upperBits) - 1ULL;
    }
    if (pattern.rotation >= pattern.length) {
        pattern.rotation = 0U;
    }
}

std::uint16_t sequencerTraversalCycleLength(const SequencerPatternV2& pattern) {
    const std::uint16_t length = pattern.length == 0U ? 1U : pattern.length;
    if (pattern.direction == SequencerPlayDirection::PingPong && length > 1U) {
        return static_cast<std::uint16_t>(length * 2U - 2U);
    }
    return length;
}

SequencerTraversalResult resolveSequencerTraversal(
    const std::uint64_t eventSerial,
    const SequencerPatternV2& pattern,
    const std::uint32_t randomSeed) {
    SequencerTraversalResult result{};
    if (!isSequencerPatternValid(pattern)) {
        return result;
    }

    const std::uint16_t cycleLength = sequencerTraversalCycleLength(pattern);
    if (pattern.loopMode == SequencerLoopMode::Once && eventSerial >= cycleLength) {
        return result;
    }

    const std::uint16_t position = static_cast<std::uint16_t>(eventSerial % cycleLength);
    const std::uint16_t length = pattern.length;
    std::uint16_t step = 0U;
    switch (pattern.direction) {
        case SequencerPlayDirection::Forward:
            step = static_cast<std::uint16_t>(position % length);
            break;
        case SequencerPlayDirection::Reverse:
            step = static_cast<std::uint16_t>(length - 1U - (position % length));
            break;
        case SequencerPlayDirection::PingPong:
            step = position < length
                ? position
                : static_cast<std::uint16_t>(cycleLength - position);
            break;
        case SequencerPlayDirection::Random:
            step = static_cast<std::uint16_t>(mixRandomEvent(eventSerial, randomSeed) % length);
            break;
    }

    result.active = true;
    result.step = static_cast<std::uint8_t>(step);
    return result;
}

std::uint8_t rotateSequencerStep(
    const std::uint8_t step,
    const SequencerPatternV2& pattern) {
    if (!isSequencerPatternValid(pattern)) {
        return 0U;
    }
    return static_cast<std::uint8_t>(
        (static_cast<std::uint16_t>(step) + pattern.rotation) % pattern.length);
}

bool sequencerPatternHitForEvent(
    const std::uint64_t eventSerial,
    const SequencerPatternV2& pattern,
    const std::uint32_t randomSeed) {
    const SequencerTraversalResult traversal =
        resolveSequencerTraversal(eventSerial, pattern, randomSeed);
    return traversal.active &&
        sequencerPatternGate(pattern, rotateSequencerStep(traversal.step, pattern));
}

}  // namespace clockfw
