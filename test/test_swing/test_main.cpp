/**
 * @file test_main.cpp
 * @brief Atomic native tests for swing mathematics and observable engine timing.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <vector>
#include <unity.h>
#include <Arduino.h>

#include "clock_core.h"
#include "config.h"
#include "domain/clock_types.h"
#include "domain/default_configuration.h"
#include "domain/groove_catalog.h"
#include "engine/clock_engine.h"
#include "hal/gate_output_driver.h"
#include "pin_map.h"

using namespace clockfw;

void setUp() { fakefw::resetArduino(); }
void tearDown() {}

namespace {

std::vector<std::uint32_t> collectRiseTicks(std::uint8_t swingPercent, std::size_t count) {
    ClockState state{};
    initializeFactoryDefaults(state);
    state.operatingMode = OperatingMode::Independent;
    state.source = ClockSource::Internal;
    state.bpm = 120U;
    for (auto& channel : state.channels) channel.common.mode = ChannelMode::Off;
    auto& channel = state.channels[0];
    channel.common.mode = ChannelMode::Clock;
    channel.common.swingPercent = swingPercent;
    channel.common.probabilityPercent = 100U;
    channel.common.phasePercent = 0U;
    channel.common.gateLengthMs = 1U;
    channel.common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    channel.clock.meter = {4U, 4U};

    hal::GateOutputDriver gates;
    gates.beginDisabled();
    gates.enableOutputStage();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    engine.play();

    std::vector<std::uint32_t> ticks;
    for (std::uint32_t tick = 0U; tick < 60000U && ticks.size() < count; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();
        const bool rose = std::any_of(fakefw::writes.begin(), fakefw::writes.end(), [](const fakefw::PinWrite& w) {
            return w.pin == pinmap::kGateChannelPins[0] && w.value == HIGH;
        });
        if (rose) ticks.push_back(tick);
    }
    return ticks;
}


std::vector<std::uint32_t> collectGrooveRiseTicks(
    GroovePreset preset,
    std::uint8_t amountPercent,
    std::uint8_t rotation,
    std::uint8_t swingPercent,
    std::size_t count) {
    ClockState state{};
    initializeFactoryDefaults(state);
    state.operatingMode = OperatingMode::Independent;
    state.source = ClockSource::Internal;
    state.bpm = 120U;
    for (auto& channel : state.channels) channel.common.mode = ChannelMode::Off;
    auto& channel = state.channels[0];
    channel.common.mode = ChannelMode::Clock;
    channel.common.swingPercent = swingPercent;
    channel.common.groove = {preset, amountPercent, rotation};
    channel.common.probabilityPercent = 100U;
    channel.common.phasePercent = 0U;
    channel.common.gateLengthMs = 1U;
    channel.common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    channel.clock.meter = {4U, 4U};

    hal::GateOutputDriver gates;
    gates.beginDisabled();
    gates.enableOutputStage();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    engine.play();

    std::vector<std::uint32_t> ticks;
    for (std::uint32_t tick = 0U; tick < 100000U && ticks.size() < count; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();
        const bool rose = std::any_of(fakefw::writes.begin(), fakefw::writes.end(), [](const fakefw::PinWrite& w) {
            return w.pin == pinmap::kGateChannelPins[0] && w.value == HIGH;
        });
        if (rose) ticks.push_back(tick);
    }
    return ticks;
}

void assertObservedPair(std::uint8_t swing, std::uint32_t expectedLong, std::uint32_t expectedShort) {
    const auto ticks = collectRiseTicks(swing, 4U);
    TEST_ASSERT_EQUAL_UINT32(4U, static_cast<std::uint32_t>(ticks.size()));
    const std::uint32_t first = ticks[1] - ticks[0];
    const std::uint32_t second = ticks[2] - ticks[1];
    TEST_ASSERT_TRUE(first >= expectedLong - 1U && first <= expectedLong + 1U);
    TEST_ASSERT_TRUE(second >= expectedShort - 1U && second <= expectedShort + 1U);
    TEST_ASSERT_TRUE((first + second) >= 19999U && (first + second) <= 20001U);
}

void testSwingZeroLongIsIdentity() { TEST_ASSERT_EQUAL_UINT64(1000U, core::applySwing(1000U, 0U, true)); }
void testSwingZeroShortIsIdentity() { TEST_ASSERT_EQUAL_UINT64(1000U, core::applySwing(1000U, 0U, false)); }
void testSwingTenLongIs110Percent() { TEST_ASSERT_EQUAL_UINT64(1100U, core::applySwing(1000U, 10U, true)); }
void testSwingTenShortIs90Percent() { TEST_ASSERT_EQUAL_UINT64(900U, core::applySwing(1000U, 10U, false)); }
void testSwingTwentyFiveLongIs125Percent() { TEST_ASSERT_EQUAL_UINT64(1250U, core::applySwing(1000U, 25U, true)); }
void testSwingTwentyFiveShortIs75Percent() { TEST_ASSERT_EQUAL_UINT64(750U, core::applySwing(1000U, 25U, false)); }
void testSwingFiftyLongIs150Percent() { TEST_ASSERT_EQUAL_UINT64(1500U, core::applySwing(1000U, 50U, true)); }
void testSwingFiftyShortIs50Percent() { TEST_ASSERT_EQUAL_UINT64(500U, core::applySwing(1000U, 50U, false)); }
void testSwingAboveFiftyClampsLong() { TEST_ASSERT_EQUAL_UINT64(core::applySwing(1000U, 50U, true), core::applySwing(1000U, 255U, true)); }
void testSwingAboveFiftyClampsShort() { TEST_ASSERT_EQUAL_UINT64(core::applySwing(1000U, 50U, false), core::applySwing(1000U, 255U, false)); }
void testSwingPairConservesAtOnePercent() { TEST_ASSERT_EQUAL_UINT64(2000U, core::applySwing(1000U, 1U, true) + core::applySwing(1000U, 1U, false)); }
void testSwingPairConservesAtThirtyThreePercent() { TEST_ASSERT_EQUAL_UINT64(2000U, core::applySwing(1000U, 33U, true) + core::applySwing(1000U, 33U, false)); }
void testSwingPairConservesAtFiftyPercent() { TEST_ASSERT_EQUAL_UINT64(2000U, core::applySwing(1000U, 50U, true) + core::applySwing(1000U, 50U, false)); }
void testSwingOneTickCannotCollapse() { TEST_ASSERT_EQUAL_UINT64(1U, core::applySwing(1U, 50U, false)); }
void testSwingTwoTicksKeepsShortNonzero() { TEST_ASSERT_TRUE(core::applySwing(2U, 50U, false) >= 1U); }
void testSwingLongIntervalMonotonicWithAmount() {
    std::uint64_t previous = core::applySwing(10000U, 0U, true);
    for (std::uint8_t amount = 1U; amount <= 50U; ++amount) {
        const auto current = core::applySwing(10000U, amount, true);
        TEST_ASSERT_TRUE(current >= previous);
        previous = current;
    }
}
void testSwingShortIntervalMonotonicWithAmount() {
    std::uint64_t previous = core::applySwing(10000U, 0U, false);
    for (std::uint8_t amount = 1U; amount <= 50U; ++amount) {
        const auto current = core::applySwing(10000U, amount, false);
        TEST_ASSERT_TRUE(current <= previous);
        previous = current;
    }
}
void testEngineStraightProducesEqualQuarterIntervals() { assertObservedPair(0U, 10000U, 10000U); }
void testEngineTenPercentProduces11000And9000Ticks() { assertObservedPair(10U, 11000U, 9000U); }
void testEngineTwentyFivePercentProduces12500And7500Ticks() { assertObservedPair(25U, 12500U, 7500U); }
void testEngineFiftyPercentProduces15000And5000Ticks() { assertObservedPair(50U, 15000U, 5000U); }
void testEngineSwingPatternRepeatsDeterministically() {
    const auto ticks = collectRiseTicks(20U, 6U);
    TEST_ASSERT_EQUAL_UINT32(6U, static_cast<std::uint32_t>(ticks.size()));
    TEST_ASSERT_TRUE((ticks[1] - ticks[0] > ticks[3] - ticks[2] ? (ticks[1] - ticks[0]) - (ticks[3] - ticks[2]) : (ticks[3] - ticks[2]) - (ticks[1] - ticks[0])) <= 1U);
    TEST_ASSERT_TRUE((ticks[2] - ticks[1] > ticks[4] - ticks[3] ? (ticks[2] - ticks[1]) - (ticks[4] - ticks[3]) : (ticks[4] - ticks[3]) - (ticks[2] - ticks[1])) <= 1U);
}

void testGrooveOffMatchesStraightTiming() {
    const auto straight = collectRiseTicks(0U, 5U);
    const auto grooveOff = collectGrooveRiseTicks(GroovePreset::Off, 100U, 0U, 0U, 5U);
    TEST_ASSERT_EQUAL_UINT32(straight.size(), grooveOff.size());
    for (std::size_t i = 0U; i < straight.size(); ++i) TEST_ASSERT_EQUAL_UINT32(straight[i], grooveOff[i]);
}
void testGrooveSwing54DelaysEverySecondEvent() {
    const auto ticks = collectGrooveRiseTicks(GroovePreset::Swing54, 100U, 0U, 0U, 4U);
    TEST_ASSERT_EQUAL_UINT32(4U, static_cast<std::uint32_t>(ticks.size()));
    TEST_ASSERT_TRUE((ticks[1] - ticks[0]) >= 10799U && (ticks[1] - ticks[0]) <= 10801U);
    TEST_ASSERT_TRUE((ticks[2] - ticks[1]) >= 9199U && (ticks[2] - ticks[1]) <= 9201U);
}
void testGrooveAmountScalesDeterministically() {
    const auto full = collectGrooveRiseTicks(GroovePreset::Swing54, 100U, 0U, 0U, 3U);
    const auto half = collectGrooveRiseTicks(GroovePreset::Swing54, 50U, 0U, 0U, 3U);
    TEST_ASSERT_TRUE((full[1] - full[0]) >= 10799U && (full[1] - full[0]) <= 10801U);
    TEST_ASSERT_TRUE((half[1] - half[0]) >= 10399U && (half[1] - half[0]) <= 10401U);
}
void testGrooveRotationMovesPatternOrigin() {
    const auto normal = collectGrooveRiseTicks(GroovePreset::Swing54, 100U, 0U, 0U, 4U);
    const auto rotated = collectGrooveRiseTicks(GroovePreset::Swing54, 100U, 1U, 0U, 4U);
    const auto normalShort = normal[2] - normal[1];
    const auto rotatedShort = rotated[1] - rotated[0];
    const auto normalLong = normal[1] - normal[0];
    const auto rotatedLong = rotated[2] - rotated[1];
    TEST_ASSERT_TRUE(normalShort > rotatedShort ? normalShort - rotatedShort <= 1U : rotatedShort - normalShort <= 1U);
    TEST_ASSERT_TRUE(normalLong > rotatedLong ? normalLong - rotatedLong <= 1U : rotatedLong - normalLong <= 1U);
}
void testPocketGrooveIsRepeatable() {
    const auto first = collectGrooveRiseTicks(GroovePreset::PocketC, 73U, 5U, 0U, 10U);
    const auto second = collectGrooveRiseTicks(GroovePreset::PocketC, 73U, 5U, 0U, 10U);
    TEST_ASSERT_EQUAL_UINT32(first.size(), second.size());
    for (std::size_t i = 0U; i < first.size(); ++i) TEST_ASSERT_EQUAL_UINT32(first[i], second[i]);
}
void testCombinedSwingAndGrooveRemainMonotonic() {
    const auto ticks = collectGrooveRiseTicks(GroovePreset::Swing66, 100U, 0U, 50U, 8U);
    TEST_ASSERT_EQUAL_UINT32(8U, static_cast<std::uint32_t>(ticks.size()));
    for (std::size_t i = 1U; i < ticks.size(); ++i) TEST_ASSERT_TRUE(ticks[i] > ticks[i - 1U]);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(testSwingZeroLongIsIdentity);
    RUN_TEST(testSwingZeroShortIsIdentity);
    RUN_TEST(testSwingTenLongIs110Percent);
    RUN_TEST(testSwingTenShortIs90Percent);
    RUN_TEST(testSwingTwentyFiveLongIs125Percent);
    RUN_TEST(testSwingTwentyFiveShortIs75Percent);
    RUN_TEST(testSwingFiftyLongIs150Percent);
    RUN_TEST(testSwingFiftyShortIs50Percent);
    RUN_TEST(testSwingAboveFiftyClampsLong);
    RUN_TEST(testSwingAboveFiftyClampsShort);
    RUN_TEST(testSwingPairConservesAtOnePercent);
    RUN_TEST(testSwingPairConservesAtThirtyThreePercent);
    RUN_TEST(testSwingPairConservesAtFiftyPercent);
    RUN_TEST(testSwingOneTickCannotCollapse);
    RUN_TEST(testSwingTwoTicksKeepsShortNonzero);
    RUN_TEST(testSwingLongIntervalMonotonicWithAmount);
    RUN_TEST(testSwingShortIntervalMonotonicWithAmount);
    RUN_TEST(testEngineStraightProducesEqualQuarterIntervals);
    RUN_TEST(testEngineTenPercentProduces11000And9000Ticks);
    RUN_TEST(testEngineTwentyFivePercentProduces12500And7500Ticks);
    RUN_TEST(testEngineFiftyPercentProduces15000And5000Ticks);
    RUN_TEST(testEngineSwingPatternRepeatsDeterministically);
    RUN_TEST(testGrooveOffMatchesStraightTiming);
    RUN_TEST(testGrooveSwing54DelaysEverySecondEvent);
    RUN_TEST(testGrooveAmountScalesDeterministically);
    RUN_TEST(testGrooveRotationMovesPatternOrigin);
    RUN_TEST(testPocketGrooveIsRepeatable);
    RUN_TEST(testCombinedSwingAndGrooveRemainMonotonic);
    return UNITY_END();
}
