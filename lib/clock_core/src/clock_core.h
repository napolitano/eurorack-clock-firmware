/**
 * @file clock_core.h
 * @brief Hardware-independent timing, rhythm, and pattern mathematics.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

#include "domain/clock_types.h"

namespace clockfw::core {

/** Q32 fixed-point representation of exactly one musical beat. */
inline constexpr std::uint64_t kQ32One = 1ULL << 32U;

/**
 * @brief Calculates the greatest common divisor of two unsigned integers.
 * @return The greatest common divisor, or 1 when both inputs are zero.
 */
std::uint32_t greatestCommonDivisor(std::uint32_t first, std::uint32_t second);

/**
 * @brief Resolves one channel's integer and rational rate into a reduced fraction.
 * @param settings Shared channel settings containing divide/multiply and ratio values.
 * @param numerator Receives the reduced rate numerator.
 * @param denominator Receives the reduced rate denominator.
 */
void calculateEffectiveRate(
    const CommonChannelSettings& settings,
    std::uint32_t& numerator,
    std::uint32_t& denominator);

/**
 * @brief Resolves CLOCK mode rate including local/master meter unit relationship.
 * @param settings Shared channel settings.
 * @param localBeatUnit Local CLOCK channel beat unit.
 * @param masterBeatUnit Master beat unit.
 * @param numerator Receives the reduced rate numerator.
 * @param denominator Receives the reduced rate denominator.
 */
void calculateEffectiveClockRate(
    const CommonChannelSettings& settings,
    std::uint8_t localBeatUnit,
    std::uint8_t masterBeatUnit,
    std::uint32_t& numerator,
    std::uint32_t& denominator);

/**
 * @brief Resolves EUCLID/SEQ rate against a sixteenth-note x1 pattern grid.
 * @param settings Shared channel rate settings.
 * @param masterBeatUnit Master meter unit used by the Q32 timeline.
 * @param numerator Receives the reduced rate numerator.
 * @param denominator Receives the reduced rate denominator.
 *
 * At x1 a pattern step is one sixteenth note. Therefore a 16-step pattern
 * spans one 4/4 bar at the master tempo. DIV/MULT and rational ratios are
 * applied on top of that musical grid.
 */
void calculateEffectivePatternRate(
    const CommonChannelSettings& settings,
    std::uint8_t masterBeatUnit,
    std::uint32_t& numerator,
    std::uint32_t& denominator);

/**
 * @brief Returns the next Q32 channel interval while preserving division remainder.
 * @param numerator Rate numerator; zero is treated as one.
 * @param denominator Rate denominator; zero is treated as one.
 * @param remainder Persistent integer remainder carried into the next interval.
 * @return Next interval in Q32 beats, never zero.
 */
std::uint64_t calculateNextIntervalQ32(
    std::uint32_t numerator,
    std::uint32_t denominator,
    volatile std::uint32_t& remainder);

/**
 * @brief Returns one scheduler-tick increment of the master Q32 timeline.
 * @param bpm Master tempo in beats per minute.
 * @param beatUnit Master meter unit.
 * @param schedulerFrequencyHz Scheduler interrupt frequency.
 * @param remainder Persistent division remainder preventing long-term drift.
 * @return Q32 master-position increment for one scheduler tick.
 */
std::uint64_t calculateMasterIncrementQ32(
    std::uint16_t bpm,
    std::uint8_t beatUnit,
    std::uint32_t schedulerFrequencyHz,
    volatile std::uint64_t& remainder);

/**
 * @brief Returns one scheduler-tick increment from a milli-BPM tempo.
 * @param bpmMilli Master tempo in thousandths of a BPM.
 * @param beatUnit Master meter unit.
 * @param schedulerFrequencyHz Scheduler interrupt frequency.
 * @param remainder Persistent division remainder preventing long-term drift.
 * @return Q32 master-position increment for one scheduler tick.
 *
 * This variant is used by the external-sync path so the filtered input tempo
 * can retain sub-BPM precision without introducing floating-point arithmetic.
 */
std::uint64_t calculateMasterIncrementMilliBpmQ32(
    std::uint32_t bpmMilli,
    std::uint8_t beatUnit,
    std::uint32_t schedulerFrequencyHz,
    volatile std::uint64_t& remainder);

/**
 * @brief Applies alternating swing to one ideal interval.
 * @param baseIntervalQ32 Ideal interval in Q32 beats.
 * @param swingPercent Swing amount in the inclusive range 0..50; larger values are clamped.
 * @param longInterval True for the elongated interval, false for the shortened interval.
 * @return Swing-adjusted non-zero interval.
 */
std::uint64_t applySwing(
    std::uint64_t baseIntervalQ32,
    std::uint8_t swingPercent,
    bool longInterval);

/**
 * @brief Converts a percentage phase setting into a Q32 interval offset.
 * @param intervalQ32 Local ideal interval.
 * @param phasePercent Phase percentage, clamped to 99.
 * @return Q32 phase offset.
 */
std::uint64_t calculatePhaseOffsetQ32(std::uint64_t intervalQ32, std::uint8_t phasePercent);

/** @brief Advances a xorshift32 pseudo-random state by one deterministic sample. */
std::uint32_t nextXorshift32(std::uint32_t value);

/**
 * @brief Applies a trigger probability without using floating-point arithmetic.
 * @param probabilityPercent Probability in percent.
 * @param randomState Mutable deterministic random state.
 * @return True when the trigger passes the probability gate.
 */
bool passesProbability(std::uint8_t probabilityPercent, std::uint32_t& randomState);

/**
 * @brief Converts gate length to scheduler ticks and prevents overlap with the next edge.
 * @param gateLengthMs Requested gate length in milliseconds.
 * @param schedulerTickUs Scheduler quantum in microseconds.
 * @param baseIntervalUs Ideal local interval in microseconds.
 * @param swingPercent Swing amount used to derive the shortest possible interval.
 * @return Safe gate duration in scheduler ticks, always at least one tick.
 */
std::uint32_t calculateGatePulseTicks(
    std::uint16_t gateLengthMs,
    std::uint32_t schedulerTickUs,
    std::uint64_t baseIntervalUs,
    std::uint8_t swingPercent);

/** @brief Returns the next wrapped step index for a pattern of the specified length. */
std::uint8_t advanceStep(std::uint8_t currentStep, std::uint8_t patternLength);

/** @brief Returns a bit mask containing exactly the requested number of low-order pattern bits. */
std::uint64_t patternMask(std::uint8_t patternLength);

/** @brief Clears pattern bits outside the active pattern length. */
std::uint64_t clampPattern(std::uint64_t pattern, std::uint8_t patternLength);

/** @brief Inverts only the active portion of a binary gate pattern. */
std::uint64_t invertPattern(std::uint64_t pattern, std::uint8_t patternLength);

/** @brief Builds a 1010... or 0101... alternating pattern with the requested length. */
std::uint64_t alternatingPattern(std::uint8_t patternLength, bool firstStepOn = true);

/** @brief Returns whether the requested Euclidean step contains a hit. */
bool isEuclideanHit(std::uint8_t step, const EuclidSettings& settings);

/** @brief Returns whether the requested sequencer step contains an active gate. */
bool isSequencerHit(std::uint8_t step, const SequencerSettings& settings);

/** @brief Returns whether a channel should be reset for the requested global-reset operation. */
bool shouldResetChannel(ResetMode resetMode, bool includeFreeRunningChannels);

}  // namespace clockfw::core
