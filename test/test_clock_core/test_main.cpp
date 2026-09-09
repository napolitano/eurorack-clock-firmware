/**
 * @file test_main.cpp
 * @brief Native unit tests for hardware-independent clock core mathematics.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <unity.h>
#include "clock_core.h"

using namespace clockfw;
using namespace clockfw::core;

void setUp() {}
void tearDown() {}

static CommonChannelSettings baseSettings() {
    CommonChannelSettings s{};
    s.mode = ChannelMode::Clock;
    s.rate.mode = ClockRatioMode::Multiply;
    s.rate.factor = 1;
    s.rate.numerator = 1;
    s.rate.denominator = 1;
    return s;
}

void test_gcd_edge_cases() {
    TEST_ASSERT_EQUAL_UINT32(1, greatestCommonDivisor(0, 0));
    TEST_ASSERT_EQUAL_UINT32(7, greatestCommonDivisor(0, 7));
    TEST_ASSERT_EQUAL_UINT32(7, greatestCommonDivisor(7, 0));
    TEST_ASSERT_EQUAL_UINT32(6, greatestCommonDivisor(54, 24));
    TEST_ASSERT_EQUAL_UINT32(1, greatestCommonDivisor(17, 13));
}

void test_effective_rate_defaults_zero_fields_to_one() {
    CommonChannelSettings s = baseSettings();
    s.rate.factor = 0;
    s.rate.numerator = 0;
    s.rate.denominator = 0;

    uint32_t p = 0, q = 0;
    calculateEffectiveRate(s, p, q);
    TEST_ASSERT_EQUAL_UINT32(1, p);
    TEST_ASSERT_EQUAL_UINT32(1, q);
}

void test_effective_rate_divide_normalizes_rational() {
    CommonChannelSettings s = baseSettings();
    s.rate.mode = ClockRatioMode::Divide;
    s.rate.factor = 2;
    s.rate.numerator = 3;
    s.rate.denominator = 2;

    uint32_t p = 0, q = 0;
    calculateEffectiveRate(s, p, q);
    TEST_ASSERT_EQUAL_UINT32(3, p);
    TEST_ASSERT_EQUAL_UINT32(4, q);
}

void test_effective_rate_multiply_normalizes_rational() {
    CommonChannelSettings s = baseSettings();
    s.rate.mode = ClockRatioMode::Multiply;
    s.rate.factor = 6;
    s.rate.numerator = 2;
    s.rate.denominator = 3;

    uint32_t p = 0, q = 0;
    calculateEffectiveRate(s, p, q);
    TEST_ASSERT_EQUAL_UINT32(4, p);
    TEST_ASSERT_EQUAL_UINT32(1, q);
}

void test_clock_meter_changes_effective_rate() {
    CommonChannelSettings s = baseSettings();
    uint32_t p = 0, q = 0;

    calculateEffectiveClockRate(s, 8, 4, p, q);
    TEST_ASSERT_EQUAL_UINT32(2, p);
    TEST_ASSERT_EQUAL_UINT32(1, q);

    calculateEffectiveClockRate(s, 4, 8, p, q);
    TEST_ASSERT_EQUAL_UINT32(1, p);
    TEST_ASSERT_EQUAL_UINT32(2, q);
}

void test_clock_meter_zero_units_fall_back_to_quarter_note() {
    CommonChannelSettings s = baseSettings();
    uint32_t p = 0, q = 0;
    calculateEffectiveClockRate(s, 0, 0, p, q);
    TEST_ASSERT_EQUAL_UINT32(1, p);
    TEST_ASSERT_EQUAL_UINT32(1, q);
}


void test_pattern_rate_uses_sixteenth_note_grid() {
    CommonChannelSettings s = baseSettings();
    uint32_t p = 0, q = 0;

    // In 4/4, x1 pattern rate is four steps per quarter-note beat.
    calculateEffectivePatternRate(s, 4, p, q);
    TEST_ASSERT_EQUAL_UINT32(4, p);
    TEST_ASSERT_EQUAL_UINT32(1, q);

    // In 7/8, a fixed sixteenth-note grid is two steps per eighth-note beat.
    calculateEffectivePatternRate(s, 8, p, q);
    TEST_ASSERT_EQUAL_UINT32(2, p);
    TEST_ASSERT_EQUAL_UINT32(1, q);

    // A zero beat unit uses the safe quarter-note fallback.
    calculateEffectivePatternRate(s, 0, p, q);
    TEST_ASSERT_EQUAL_UINT32(4, p);
    TEST_ASSERT_EQUAL_UINT32(1, q);
}

void test_rational_accumulator_3_2_has_no_long_term_drift() {
    uint32_t remainder = 0;
    uint64_t sum = 0;
    for (int i = 0; i < 3; ++i) sum += calculateNextIntervalQ32(3, 2, remainder);

    TEST_ASSERT_EQUAL_UINT64(2ULL * kQ32One, sum);
    TEST_ASSERT_EQUAL_UINT32(0, remainder);
}

void test_rational_accumulator_is_exact_for_small_ratios() {
    for (uint32_t p = 1; p <= 32; ++p) {
        for (uint32_t q = 1; q <= 32; ++q) {
            uint32_t remainder = 0;
            uint64_t sum = 0;
            for (uint32_t i = 0; i < p; ++i) sum += calculateNextIntervalQ32(p, q, remainder);
            TEST_ASSERT_EQUAL_UINT64(static_cast<uint64_t>(q) * kQ32One, sum);
            TEST_ASSERT_EQUAL_UINT32(0, remainder);
        }
    }
}

void test_rational_accumulator_zero_inputs_are_safe() {
    uint32_t remainder = 0;
    TEST_ASSERT_EQUAL_UINT64(kQ32One, calculateNextIntervalQ32(0, 0, remainder));
    TEST_ASSERT_EQUAL_UINT32(0, remainder);
}

void test_master_accumulator_120_bpm_is_exact_at_20khz() {
    uint64_t remainder = 0;
    uint64_t sum = 0;
    for (uint32_t i = 0; i < 10000; ++i) {
        sum += calculateMasterIncrementQ32(120, 4, 20000, remainder);
    }
    TEST_ASSERT_EQUAL_UINT64(kQ32One, sum);
    TEST_ASSERT_EQUAL_UINT64(0, remainder);
}

void test_master_accumulator_respects_beat_unit() {
    uint64_t remainder = 0;
    uint64_t sum = 0;
    // At 120 BPM an eighth-note beat reaches one Q32 beat after 0.25 s = 5000 ticks.
    for (uint32_t i = 0; i < 5000; ++i) {
        sum += calculateMasterIncrementQ32(120, 8, 20000, remainder);
    }
    TEST_ASSERT_EQUAL_UINT64(kQ32One, sum);
    TEST_ASSERT_EQUAL_UINT64(0, remainder);
}

void test_master_accumulator_is_exact_across_supported_bpm_range() {
    constexpr uint32_t schedulerFrequencyHz = 20000U;
    constexpr uint32_t simulatedTicks = schedulerFrequencyHz;

    for (uint16_t bpm = 1U; bpm <= 999U; ++bpm) {
        uint64_t remainder = 0U;
        uint64_t accumulatedQ32 = 0U;
        for (uint32_t tick = 0U; tick < simulatedTicks; ++tick) {
            accumulatedQ32 += calculateMasterIncrementQ32(
                bpm, 4U, schedulerFrequencyHz, remainder);
        }

        const uint64_t exactNumerator =
            static_cast<uint64_t>(bpm) * kQ32One * simulatedTicks;
        const uint64_t exactDenominator = 60ULL * schedulerFrequencyHz;
        const uint64_t accumulatorDenominator = 60ULL * 4ULL * schedulerFrequencyHz;
        TEST_ASSERT_EQUAL_UINT64(exactNumerator / exactDenominator, accumulatedQ32);
        TEST_ASSERT_TRUE(remainder < accumulatorDenominator);
    }
}

void test_master_accumulator_zero_inputs_are_safe() {
    uint64_t remainder = 1234;
    TEST_ASSERT_EQUAL_UINT64(0, calculateMasterIncrementQ32(0, 4, 20000, remainder));
    TEST_ASSERT_EQUAL_UINT64(0, remainder);

    // Zero beat unit falls back to quarter-note; zero engine rate is coerced to one tick/s.
    const uint64_t inc = calculateMasterIncrementQ32(60, 0, 0, remainder);
    TEST_ASSERT_EQUAL_UINT64(kQ32One, inc);
    TEST_ASSERT_EQUAL_UINT64(0, remainder);
}

void test_swing_zero_is_identity() {
    TEST_ASSERT_EQUAL_UINT64(kQ32One, applySwing(kQ32One, 0, true));
    TEST_ASSERT_EQUAL_UINT64(kQ32One, applySwing(kQ32One, 0, false));
}

void test_swing_pair_preserves_total_duration() {
    for (uint8_t swing = 1; swing <= 50; ++swing) {
        const uint64_t longInterval = applySwing(kQ32One, swing, true);
        const uint64_t shortInterval = applySwing(kQ32One, swing, false);
        TEST_ASSERT_EQUAL_UINT64(2ULL * kQ32One, longInterval + shortInterval);
        TEST_ASSERT_TRUE(longInterval > kQ32One);
        TEST_ASSERT_TRUE(shortInterval < kQ32One);
    }
}

void test_swing_invariants_hold_across_practical_intervals() {
    for (uint64_t interval = 2U; interval <= 1024U; ++interval) {
        for (uint8_t swing = 0U; swing <= 50U; ++swing) {
            const uint64_t longInterval = applySwing(interval, swing, true);
            const uint64_t shortInterval = applySwing(interval, swing, false);
            TEST_ASSERT_EQUAL_UINT64(2ULL * interval, longInterval + shortInterval);
            TEST_ASSERT_TRUE(longInterval >= interval);
            TEST_ASSERT_TRUE(shortInterval <= interval);
            TEST_ASSERT_TRUE(shortInterval >= 1U);
        }
    }
}

void test_swing_clamps_above_50_percent() {
    TEST_ASSERT_EQUAL_UINT64(applySwing(kQ32One, 50, true), applySwing(kQ32One, 99, true));
    TEST_ASSERT_EQUAL_UINT64(applySwing(kQ32One, 50, false), applySwing(kQ32One, 99, false));
}

void test_swing_tiny_interval_stays_nonzero() {
    TEST_ASSERT_EQUAL_UINT64(1, applySwing(1, 50, true));
    TEST_ASSERT_EQUAL_UINT64(1, applySwing(1, 50, false));
    TEST_ASSERT_TRUE(applySwing(2, 50, false) >= 1);
}

void test_phase_offset_is_relative_and_clamped() {
    TEST_ASSERT_EQUAL_UINT64(0, calculatePhaseOffsetQ32(kQ32One, 0));
    TEST_ASSERT_EQUAL_UINT64(kQ32One / 4, calculatePhaseOffsetQ32(kQ32One, 25));
    TEST_ASSERT_EQUAL_UINT64((kQ32One * 99ULL) / 100ULL, calculatePhaseOffsetQ32(kQ32One, 99));
    TEST_ASSERT_EQUAL_UINT64((kQ32One * 99ULL) / 100ULL, calculatePhaseOffsetQ32(kQ32One, 255));
}

void test_nextXorshift32_known_sequence() {
    uint32_t state = 0x12345678U;
    state = nextXorshift32(state);
    TEST_ASSERT_EQUAL_HEX32(0x87985AA5U, state);
    state = nextXorshift32(state);
    TEST_ASSERT_EQUAL_HEX32(0x155B24A3U, state);
    state = nextXorshift32(state);
    TEST_ASSERT_EQUAL_HEX32(0x4820F4C4U, state);
}

void test_probability_boundaries_do_not_consume_rng() {
    uint32_t state = 0x12345678U;
    TEST_ASSERT_FALSE(passesProbability(0, state));
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, state);
    TEST_ASSERT_TRUE(passesProbability(100, state));
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, state);
    TEST_ASSERT_TRUE(passesProbability(255, state));
    TEST_ASSERT_EQUAL_HEX32(0x12345678U, state);
}

void test_probability_uses_deterministic_rng_sample() {
    uint32_t stateA = 0x12345678U; // first sample ends in 37 modulo 100
    uint32_t stateB = 0x12345678U;
    TEST_ASSERT_FALSE(passesProbability(37, stateA));
    TEST_ASSERT_TRUE(passesProbability(38, stateB));
    TEST_ASSERT_EQUAL_HEX32(0x87985AA5U, stateA);
    TEST_ASSERT_EQUAL_HEX32(0x87985AA5U, stateB);
}

void test_probability_recovers_from_zero_rng_seed() {
    uint32_t state = 0;
    (void)passesProbability(50, state);
    TEST_ASSERT_TRUE(state != 0U);
}

void test_gate_pulse_ticks_round_up_requested_width() {
    TEST_ASSERT_EQUAL_UINT32(200, calculateGatePulseTicks(10, 50, 500000, 0));
    TEST_ASSERT_EQUAL_UINT32(4, calculateGatePulseTicks(1, 333, 500000, 0));
}

void test_gate_pulse_ticks_never_swallow_next_edge() {
    // 10 ms local interval: pulse may occupy at most half = 5 ms = 100 ticks at 50 us.
    TEST_ASSERT_EQUAL_UINT32(100, calculateGatePulseTicks(100, 50, 10000, 0));
    // 50% swing makes the short interval 5 ms; half of that is 2.5 ms = 50 ticks.
    TEST_ASSERT_EQUAL_UINT32(50, calculateGatePulseTicks(100, 50, 10000, 50));
}

void test_gate_pulse_ticks_clamps_invalid_swing_and_tick_zero() {
    TEST_ASSERT_EQUAL_UINT32(
        calculateGatePulseTicks(100, 50, 10000, 50),
        calculateGatePulseTicks(100, 50, 10000, 99));
    TEST_ASSERT_EQUAL_UINT32(1, calculateGatePulseTicks(0, 0, 0, 0));
}


void test_gate_pulse_ticks_handles_extreme_interval_without_overflow() {
    TEST_ASSERT_EQUAL_UINT32(1000U, calculateGatePulseTicks(1U, 1U, UINT64_MAX, 0U));
}

void test_next_step_wraps_and_handles_zero_length() {
    TEST_ASSERT_EQUAL_UINT8(1, advanceStep(0, 4));
    TEST_ASSERT_EQUAL_UINT8(0, advanceStep(3, 4));
    TEST_ASSERT_EQUAL_UINT8(0, advanceStep(0, 1));
    TEST_ASSERT_EQUAL_UINT8(0, advanceStep(255, 0));
}

void test_pattern_masks_cover_boundary_lengths() {
    TEST_ASSERT_EQUAL_UINT64(0ULL, patternMask(0));
    TEST_ASSERT_EQUAL_UINT64(0x1ULL, patternMask(1));
    TEST_ASSERT_EQUAL_UINT64(0xFULL, patternMask(4));
    TEST_ASSERT_EQUAL_UINT64(0x7FFFFFFFFFFFFFFFULL, patternMask(63));
    TEST_ASSERT_EQUAL_UINT64(0xFFFFFFFFFFFFFFFFULL, patternMask(64));
    TEST_ASSERT_EQUAL_UINT64(0xFFFFFFFFFFFFFFFFULL, patternMask(65));
}

void test_pattern_clamp_and_invert_never_leak_hidden_steps() {
    TEST_ASSERT_EQUAL_UINT64(0xFULL, clampPattern(0xFFFFFFFFFFFFFFFFULL, 4));
    TEST_ASSERT_EQUAL_UINT64(0xAULL, invertPattern(0x5ULL, 4));
    TEST_ASSERT_EQUAL_UINT64(0ULL, invertPattern(0xFFFFFFFFFFFFFFFFULL, 4));
    TEST_ASSERT_EQUAL_UINT64(0ULL, invertPattern(0x0ULL, 0));
}

void test_alternating_pattern_respects_length_and_phase() {
    TEST_ASSERT_EQUAL_UINT64(0x15ULL, alternatingPattern(5, true));
    TEST_ASSERT_EQUAL_UINT64(0x0AULL, alternatingPattern(5, false));
    TEST_ASSERT_EQUAL_UINT64(0x5555555555555555ULL, alternatingPattern(64, true));
    TEST_ASSERT_EQUAL_UINT64(0ULL, alternatingPattern(0, true));
}


void test_double_invert_restores_active_pattern() {
    const uint64_t pattern = 0xA55AA55AA55AA55AULL;
    for (uint8_t length = 1; length <= 64; ++length) {
        const uint64_t active = clampPattern(pattern, length);
        const uint64_t twice = invertPattern(invertPattern(active, length), length);
        TEST_ASSERT_EQUAL_UINT64(active, twice);
    }
}

void test_alternating_pattern_has_expected_hit_count_for_all_lengths() {
    for (uint8_t length = 1; length <= 64; ++length) {
        const uint64_t pattern = alternatingPattern(length, true);
        uint8_t count = 0;
        for (uint8_t step = 0; step < length; ++step) {
            if ((pattern >> step) & 1ULL) ++count;
        }
        TEST_ASSERT_EQUAL_UINT8(static_cast<uint8_t>((length + 1U) / 2U), count);
    }
}

void test_euclid_zero_inputs_have_no_hits() {
    EuclidSettings e{};
    e.steps = 0; e.hits = 4; e.rotation = 0;
    TEST_ASSERT_FALSE(isEuclideanHit(0, e));
    e.steps = 16; e.hits = 0;
    TEST_ASSERT_FALSE(isEuclideanHit(0, e));
}

void test_euclid_8_3_expected_pattern() {
    EuclidSettings e{};
    e.steps = 8; e.hits = 3; e.rotation = 0;
    const bool expected[8] = {true,false,false,true,false,false,true,false};
    for (uint8_t step = 0; step < 8; ++step) TEST_ASSERT_EQUAL(expected[step], isEuclideanHit(step, e));
}

void test_euclid_hit_count_is_exact_for_all_supported_sizes() {
    for (uint8_t steps = 1; steps <= 64; ++steps) {
        for (uint8_t hits = 0; hits <= steps; ++hits) {
            EuclidSettings e{};
            e.steps = steps; e.hits = hits; e.rotation = 0;
            uint8_t count = 0;
            for (uint8_t step = 0; step < steps; ++step) if (isEuclideanHit(step, e)) ++count;
            TEST_ASSERT_EQUAL_UINT8(hits, count);
        }
    }
}

void test_euclid_rotation_preserves_hit_count() {
    EuclidSettings e{};
    e.steps = 13; e.hits = 5;
    for (uint8_t rotation = 0; rotation < e.steps; ++rotation) {
        e.rotation = rotation;
        uint8_t count = 0;
        for (uint8_t step = 0; step < e.steps; ++step) if (isEuclideanHit(step, e)) ++count;
        TEST_ASSERT_EQUAL_UINT8(5, count);
    }
}

void test_euclid_hits_above_steps_clamp_to_full_pattern() {
    EuclidSettings e{};
    e.steps = 7; e.hits = 12; e.rotation = 3;
    for (uint8_t step = 0; step < e.steps; ++step) TEST_ASSERT_TRUE(isEuclideanHit(step, e));
}

void test_sequencer_zero_length_has_no_hits() {
    SequencerSettings s{};
    s.length = 0; s.pattern = ~0ULL;
    TEST_ASSERT_FALSE(isSequencerHit(0, s));
}

void test_sequencer_rotation_and_length() {
    SequencerSettings s{};
    s.length = 4;
    s.rotation = 1;
    s.pattern = 0b0101ULL;

    TEST_ASSERT_FALSE(isSequencerHit(0, s));
    TEST_ASSERT_TRUE(isSequencerHit(1, s));
    TEST_ASSERT_FALSE(isSequencerHit(2, s));
    TEST_ASSERT_TRUE(isSequencerHit(3, s));
}

void test_sequencer_supports_step_64() {
    SequencerSettings s{};
    s.length = 64;
    s.rotation = 0;
    s.pattern = 1ULL << 63;
    TEST_ASSERT_FALSE(isSequencerHit(62, s));
    TEST_ASSERT_TRUE(isSequencerHit(63, s));
}


void test_sequencer_rotation_preserves_hit_count() {
    SequencerSettings s{};
    s.length = 13;
    s.pattern = 0x1295ULL;
    uint8_t baseline = 0;
    s.rotation = 0;
    for (uint8_t step = 0; step < s.length; ++step) if (isSequencerHit(step, s)) ++baseline;

    for (uint8_t rotation = 0; rotation < s.length; ++rotation) {
        s.rotation = rotation;
        uint8_t count = 0;
        for (uint8_t step = 0; step < s.length; ++step) if (isSequencerHit(step, s)) ++count;
        TEST_ASSERT_EQUAL_UINT8(baseline, count);
    }
}

void test_reset_policy_truth_table() {
    TEST_ASSERT_TRUE(shouldResetChannel(ResetMode::Global, false));
    TEST_ASSERT_TRUE(shouldResetChannel(ResetMode::Global, true));
    TEST_ASSERT_FALSE(shouldResetChannel(ResetMode::Free, false));
    TEST_ASSERT_TRUE(shouldResetChannel(ResetMode::Free, true));
}

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(test_gcd_edge_cases);
    RUN_TEST(test_effective_rate_defaults_zero_fields_to_one);
    RUN_TEST(test_effective_rate_divide_normalizes_rational);
    RUN_TEST(test_effective_rate_multiply_normalizes_rational);
    RUN_TEST(test_clock_meter_changes_effective_rate);
    RUN_TEST(test_clock_meter_zero_units_fall_back_to_quarter_note);
    RUN_TEST(test_pattern_rate_uses_sixteenth_note_grid);
    RUN_TEST(test_rational_accumulator_3_2_has_no_long_term_drift);
    RUN_TEST(test_rational_accumulator_is_exact_for_small_ratios);
    RUN_TEST(test_rational_accumulator_zero_inputs_are_safe);
    RUN_TEST(test_master_accumulator_120_bpm_is_exact_at_20khz);
    RUN_TEST(test_master_accumulator_respects_beat_unit);
    RUN_TEST(test_master_accumulator_is_exact_across_supported_bpm_range);
    RUN_TEST(test_master_accumulator_zero_inputs_are_safe);
    RUN_TEST(test_swing_zero_is_identity);
    RUN_TEST(test_swing_pair_preserves_total_duration);
    RUN_TEST(test_swing_invariants_hold_across_practical_intervals);
    RUN_TEST(test_swing_clamps_above_50_percent);
    RUN_TEST(test_swing_tiny_interval_stays_nonzero);
    RUN_TEST(test_phase_offset_is_relative_and_clamped);
    RUN_TEST(test_nextXorshift32_known_sequence);
    RUN_TEST(test_probability_boundaries_do_not_consume_rng);
    RUN_TEST(test_probability_uses_deterministic_rng_sample);
    RUN_TEST(test_probability_recovers_from_zero_rng_seed);
    RUN_TEST(test_gate_pulse_ticks_round_up_requested_width);
    RUN_TEST(test_gate_pulse_ticks_never_swallow_next_edge);
    RUN_TEST(test_gate_pulse_ticks_clamps_invalid_swing_and_tick_zero);
    RUN_TEST(test_gate_pulse_ticks_handles_extreme_interval_without_overflow);
    RUN_TEST(test_next_step_wraps_and_handles_zero_length);
    RUN_TEST(test_pattern_masks_cover_boundary_lengths);
    RUN_TEST(test_pattern_clamp_and_invert_never_leak_hidden_steps);
    RUN_TEST(test_alternating_pattern_respects_length_and_phase);
    RUN_TEST(test_double_invert_restores_active_pattern);
    RUN_TEST(test_alternating_pattern_has_expected_hit_count_for_all_lengths);
    RUN_TEST(test_euclid_zero_inputs_have_no_hits);
    RUN_TEST(test_euclid_8_3_expected_pattern);
    RUN_TEST(test_euclid_hit_count_is_exact_for_all_supported_sizes);
    RUN_TEST(test_euclid_rotation_preserves_hit_count);
    RUN_TEST(test_euclid_hits_above_steps_clamp_to_full_pattern);
    RUN_TEST(test_sequencer_zero_length_has_no_hits);
    RUN_TEST(test_sequencer_rotation_and_length);
    RUN_TEST(test_sequencer_supports_step_64);
    RUN_TEST(test_sequencer_rotation_preserves_hit_count);
    RUN_TEST(test_reset_policy_truth_table);
    return UNITY_END();
}
