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
#include "services/groove_recorder.h"

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


std::vector<std::uint32_t> collectConfiguredCustomGrooveRiseTicks(
    const CustomGroovePattern& pattern,
    const std::uint16_t bpm,
    const RateSettings& rate,
    const std::uint8_t amountPercent,
    const std::uint8_t rotation,
    const std::uint8_t swingPercent,
    const std::size_t count) {
    ClockState state{};
    initializeFactoryDefaults(state);
    state.operatingMode = OperatingMode::Independent;
    state.source = ClockSource::Internal;
    state.bpm = bpm;
    for (auto& configuredChannel : state.channels) configuredChannel.common.mode = ChannelMode::Off;
    auto& channel = state.channels[0];
    channel.common.mode = ChannelMode::Clock;
    channel.common.swingPercent = swingPercent;
    channel.common.groove = {GroovePreset::Custom, amountPercent, rotation, 0U};
    channel.common.probabilityPercent = 100U;
    channel.common.phasePercent = 0U;
    channel.common.gateLengthMs = 1U;
    channel.common.rate = rate;

    hal::GateOutputDriver gates;
    gates.beginDisabled();
    gates.enableOutputStage();
    engine::ClockEngine engine(gates);
    engine.updateCustomGrooveSlot(0U, pattern, false);
    engine.begin(state);
    engine.play();

    // 20 BPM at /4 is the slowest vector below: about 240,000 scheduler
    // ticks per quarter-note event. One million ticks therefore leaves enough
    // headroom to observe three edges even at the slow boundary.
    std::vector<std::uint32_t> ticks;
    for (std::uint32_t tick = 0U; tick < 1000000U && ticks.size() < count; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();
        const bool rose = std::any_of(
            fakefw::writes.begin(), fakefw::writes.end(), [](const fakefw::PinWrite& write) {
                return write.pin == pinmap::kGateChannelPins[0] && write.value == HIGH;
            });
        if (rose) ticks.push_back(tick);
    }
    return ticks;
}

std::vector<std::uint32_t> collectCustomGrooveRiseTicks(
    const CustomGroovePattern& pattern,
    const std::uint8_t swingPercent,
    const std::size_t count) {
    return collectConfiguredCustomGrooveRiseTicks(
        pattern,
        120U,
        RateSettings{ClockRatioMode::Multiply, 1U, 1U, 1U},
        100U,
        0U,
        swingPercent,
        count);
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

void testCustomGrooveLookupSupportsSignedOffsetsAmountAndRotation() {
    CustomGroovePattern pattern{};
    pattern.length = 4U;
    pattern.offsets256[0] = -64;
    pattern.offsets256[1] = 32;
    pattern.offsets256[2] = 96;
    pattern.offsets256[3] = -120;
    TEST_ASSERT_EQUAL(-64, customGrooveOffset256(pattern, 0U, 100U, 0U));
    TEST_ASSERT_EQUAL(48, customGrooveOffset256(pattern, 2U, 50U, 0U));
    TEST_ASSERT_EQUAL(32, customGrooveOffset256(pattern, 0U, 100U, 1U));
}

void testCustomGrooveRejectsInvalidLengthAndOffset() {
    CustomGroovePattern pattern{};
    pattern.length = 0U;
    TEST_ASSERT_FALSE(isCustomGroovePatternValid(pattern));
    pattern.length = 4U;
    pattern.offsets256[1] = 121;
    TEST_ASSERT_FALSE(isCustomGroovePatternValid(pattern));
}

void testCustomGrooveSignedExtremesRemainMonotonicWithSwing() {
    CustomGroovePattern pattern{};
    pattern.length = 4U;
    pattern.offsets256[0] = 0;
    pattern.offsets256[1] = 120;
    pattern.offsets256[2] = -120;
    pattern.offsets256[3] = 0;
    const auto ticks = collectCustomGrooveRiseTicks(pattern, 50U, 8U);
    TEST_ASSERT_EQUAL_UINT32(8U, static_cast<std::uint32_t>(ticks.size()));
    for (std::size_t index = 1U; index < ticks.size(); ++index) {
        TEST_ASSERT_TRUE(ticks[index] > ticks[index - 1U]);
    }
}

void testCustomGroovePreviewCanBeAppliedAndClearedWithoutStateMutation() {
    ClockState state{};
    initializeFactoryDefaults(state);
    state.operatingMode = OperatingMode::Independent;
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.groove = {GroovePreset::Off, 100U, 0U, 0U};
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    gates.enableOutputStage();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    CustomGroovePattern pattern{};
    pattern.length = 4U;
    pattern.offsets256[1] = 64;
    engine.setCustomGroovePreview(0U, pattern, 100U, 0U, false);
    TEST_ASSERT_EQUAL(GroovePreset::Off, state.channels[0].common.groove.preset);
    engine.clearCustomGroovePreview(0U, false);
    TEST_ASSERT_EQUAL(GroovePreset::Off, state.channels[0].common.groove.preset);
}



void testCustomGrooveLookupSweepsAllLengthsStepsAndAmounts() {
    // Exhaust every supported Custom Groove length and amount. The pattern
    // deliberately spans negative, zero, and positive offsets so this sweep
    // locks the signed scaling contract without depending on scheduler state.
    for (std::uint8_t length = 1U; length <= kCustomGrooveMaximumSteps; ++length) {
        CustomGroovePattern pattern{};
        pattern.length = length;
        for (std::uint8_t step = 0U; step < length; ++step) {
            const std::int16_t centered = static_cast<std::int16_t>((step % 9U) * 30U) - 120;
            pattern.offsets256[step] = static_cast<std::int8_t>(centered);
        }
        TEST_ASSERT_TRUE(isCustomGroovePatternValid(pattern));

        for (std::uint8_t amount = 0U; amount <= 100U; ++amount) {
            for (std::uint8_t step = 0U; step < length; ++step) {
                const auto expected = static_cast<std::int16_t>(
                    (static_cast<std::int32_t>(pattern.offsets256[step]) * amount) / 100);
                TEST_ASSERT_EQUAL(
                    expected,
                    customGrooveOffset256(pattern, step, amount, 0U));
            }
        }

        // Rotation is orthogonal to amount scaling. Exercise the two boundary
        // origins for every legal length, including the one-step degenerate grid.
        TEST_ASSERT_EQUAL(
            pattern.offsets256[0],
            customGrooveOffset256(pattern, 0U, 100U, 0U));
        const std::uint8_t last = static_cast<std::uint8_t>(length - 1U);
        TEST_ASSERT_EQUAL(
            pattern.offsets256[last],
            customGrooveOffset256(pattern, 0U, 100U, last));
    }
}

void testCustomGrooveEngineMatrixAcrossBpmRatesGridsAndAmounts() {
    constexpr std::array<std::uint16_t, 3U> kBpms{{20U, 120U, 999U}};
    constexpr std::array<RateSettings, 5U> kRates{{
        {ClockRatioMode::Divide, 4U, 1U, 1U},
        {ClockRatioMode::Multiply, 1U, 1U, 1U},
        {ClockRatioMode::Multiply, 4U, 1U, 1U},
        {ClockRatioMode::Multiply, 1U, 3U, 2U},
        {ClockRatioMode::Multiply, 1U, 5U, 4U},
    }};
    constexpr std::array<std::uint8_t, 4U> kGridLengths{{1U, 4U, 16U, 64U}};
    constexpr std::array<std::uint8_t, 3U> kAmounts{{0U, 50U, 100U}};

    for (const std::uint16_t bpm : kBpms) {
        for (const RateSettings& rate : kRates) {
            for (const std::uint8_t length : kGridLengths) {
                CustomGroovePattern pattern{};
                pattern.length = length;
                for (std::uint8_t step = 0U; step < length; ++step) {
                    // A repeating signed shape stresses both early and late events.
                    constexpr std::array<std::int8_t, 4U> kShape{{-96, 48, 120, -32}};
                    pattern.offsets256[step] = kShape[step % kShape.size()];
                }
                for (const std::uint8_t amount : kAmounts) {
                    const auto ticks = collectConfiguredCustomGrooveRiseTicks(
                        pattern, bpm, rate, amount, 0U, 0U, 3U);
                    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<std::uint32_t>(ticks.size()));
                    TEST_ASSERT_TRUE(ticks[1] > ticks[0]);
                    TEST_ASSERT_TRUE(ticks[2] > ticks[1]);

                    // Amount zero must reduce Custom Groove to the straight timing path
                    // for the same BPM and rate. A neutral one-step pattern provides the
                    // independent reference without changing any other configuration.
                    if (amount == 0U) {
                        CustomGroovePattern neutral{};
                        neutral.length = 1U;
                        neutral.offsets256[0] = 0;
                        const auto straight = collectConfiguredCustomGrooveRiseTicks(
                            neutral, bpm, rate, 100U, 0U, 0U, 3U);
                        TEST_ASSERT_EQUAL_UINT32(ticks[0], straight[0]);
                        TEST_ASSERT_EQUAL_UINT32(ticks[1], straight[1]);
                        TEST_ASSERT_EQUAL_UINT32(ticks[2], straight[2]);
                    }
                }
            }
        }
    }
}

void testCustomGrooveDefensiveAndRescheduleBranches() {
    ClockState state{};
    initializeFactoryDefaults(state);
    state.operatingMode = OperatingMode::Independent;
    for (auto& configuredChannel : state.channels) configuredChannel.common.mode = ChannelMode::Off;
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.groove = {GroovePreset::Custom, 100U, 0U, 0U};
    state.channels[1].common.mode = ChannelMode::Clock;
    state.channels[1].common.groove = {GroovePreset::Custom, 100U, 0U, 1U};

    hal::GateOutputDriver gates;
    gates.beginDisabled();
    gates.enableOutputStage();
    engine::ClockEngine engine(gates);
    engine.begin(state);

    CustomGroovePattern pattern{};
    pattern.length = 4U;
    pattern.offsets256[0] = -120;
    pattern.offsets256[1] = 120;

    CustomGroovePattern invalid = pattern;
    invalid.length = 0U;
    engine.updateCustomGrooveSlot(kCustomGrooveSlotCount, pattern, true);
    engine.updateCustomGrooveSlot(0U, invalid, true);
    engine.updateCustomGrooveSlot(0U, pattern, false);
    engine.updateCustomGrooveSlot(0U, pattern, true);

    engine.setCustomGroovePreview(kChannelCount, pattern, 100U, 0U, true);
    engine.setCustomGroovePreview(0U, invalid, 100U, 0U, true);
    engine.setCustomGroovePreview(0U, pattern, 101U, 0U, true);
    engine.setCustomGroovePreview(0U, pattern, 100U, pattern.length, true);
    engine.setCustomGroovePreview(0U, pattern, 100U, 0U, false);
    engine.setCustomGroovePreview(0U, pattern, 100U, 0U, true);

    engine.clearCustomGroovePreview(kChannelCount, true);
    engine.clearCustomGroovePreview(0U, false);
    engine.clearCustomGroovePreview(0U, true);

    TEST_ASSERT_EQUAL(0, customGrooveOffset256(invalid, 0U, 100U, 0U));
    TEST_ASSERT_EQUAL(0, customGrooveOffset256(pattern, 0U, 0U, 0U));
    TEST_ASSERT_EQUAL(-120, customGrooveOffset256(pattern, 4U, 255U, 4U));

    invalid = pattern;
    invalid.length = static_cast<std::uint8_t>(kCustomGrooveMaximumSteps + 1U);
    TEST_ASSERT_FALSE(isCustomGroovePatternValid(invalid));
    invalid = pattern;
    invalid.offsets256[1] = -121;
    TEST_ASSERT_FALSE(isCustomGroovePatternValid(invalid));
}

void testGrooveRecorderCountInAndOneShotCapture() {
    services::GrooveRecorder recorder;
    CustomGroovePattern pattern{};
    pattern.length = 4U;
    constexpr std::uint64_t step = core::kQ32One / 4ULL;

    recorder.reset(services::GrooveRecordMode::OneShot, 2U);
    recorder.start(10ULL * core::kQ32One, step, pattern.length);
    auto view = recorder.view();
    TEST_ASSERT_EQUAL(services::GrooveRecordState::PreCount, view.state);
    TEST_ASSERT_EQUAL_UINT8(2U, view.countInRemaining);
    TEST_ASSERT_FALSE(recorder.capture(10ULL * core::kQ32One, pattern));

    TEST_ASSERT_TRUE(recorder.service(11ULL * core::kQ32One, pattern.length));
    TEST_ASSERT_EQUAL_UINT8(1U, recorder.view().countInRemaining);
    TEST_ASSERT_TRUE(recorder.service(12ULL * core::kQ32One, pattern.length));
    TEST_ASSERT_EQUAL(services::GrooveRecordState::Recording, recorder.view().state);

    TEST_ASSERT_TRUE(recorder.capture(12ULL * core::kQ32One, pattern));
    TEST_ASSERT_EQUAL(0, pattern.offsets256[0]);
    TEST_ASSERT_TRUE((recorder.view().capturedMask & 0x1ULL) != 0ULL);

    // A tap one eighth of a step late becomes +32 / 256 of the nominal step.
    TEST_ASSERT_TRUE(recorder.capture(12ULL * core::kQ32One + step + step / 8ULL, pattern));
    TEST_ASSERT_EQUAL(32, pattern.offsets256[1]);
    TEST_ASSERT_TRUE((recorder.view().capturedMask & 0x2ULL) != 0ULL);

    TEST_ASSERT_TRUE(recorder.service(12ULL * core::kQ32One + 4ULL * step, pattern.length));
    TEST_ASSERT_EQUAL(services::GrooveRecordState::Ready, recorder.view().state);
    TEST_ASSERT_EQUAL_UINT8(0U, recorder.view().playheadStep);
}

void testGrooveRecorderEndlessWrapsAndOverwritesTappedStepOnly() {
    services::GrooveRecorder recorder;
    CustomGroovePattern pattern{};
    pattern.length = 4U;
    pattern.offsets256[2] = 17;
    constexpr std::uint64_t step = core::kQ32One / 4ULL;
    constexpr std::uint64_t start = 3ULL * core::kQ32One;

    recorder.reset(services::GrooveRecordMode::Endless, 0U);
    recorder.start(start, step, pattern.length);
    TEST_ASSERT_EQUAL(services::GrooveRecordState::Recording, recorder.view().state);

    TEST_ASSERT_TRUE(recorder.capture(start + step / 8ULL, pattern));
    TEST_ASSERT_EQUAL(32, pattern.offsets256[0]);
    TEST_ASSERT_EQUAL(17, pattern.offsets256[2]);

    // The next loop targets step 0 again and replaces only that step.
    TEST_ASSERT_TRUE(recorder.capture(start + 4ULL * step - step / 8ULL, pattern));
    TEST_ASSERT_EQUAL(-32, pattern.offsets256[0]);
    TEST_ASSERT_EQUAL(17, pattern.offsets256[2]);
    TEST_ASSERT_EQUAL(services::GrooveRecordState::Recording, recorder.view().state);

    recorder.service(start + 5ULL * step + step / 2ULL, pattern.length);
    TEST_ASSERT_EQUAL_UINT8(1U, recorder.view().playheadStep);
    TEST_ASSERT_TRUE(recorder.view().playheadPhase256 >= 127U);
    recorder.stop();
    TEST_ASSERT_EQUAL_UINT8(0U, recorder.view().playheadStep);
}

void testGrooveRecorderGuardsClampAndClearCaptureState() {
    services::GrooveRecorder recorder;
    CustomGroovePattern pattern{};
    pattern.length = 1U;
    recorder.reset(services::GrooveRecordMode::OneShot, 255U, UINT64_MAX);
    TEST_ASSERT_EQUAL_UINT8(64U, recorder.view().countInBeats);
    recorder.setCountInBeats(255U);
    TEST_ASSERT_EQUAL_UINT8(64U, recorder.view().countInBeats);

    recorder.start(0U, 0U, pattern.length);
    TEST_ASSERT_EQUAL(services::GrooveRecordState::Ready, recorder.view().state);
    recorder.start(0U, core::kQ32One, 0U);
    TEST_ASSERT_EQUAL(services::GrooveRecordState::Ready, recorder.view().state);
    recorder.reset(services::GrooveRecordMode::OneShot, 0U);
    recorder.start(core::kQ32One, core::kQ32One, pattern.length);
    TEST_ASSERT_FALSE(recorder.capture(0U, pattern));

    CustomGroovePattern invalid{};
    invalid.length = 0U;
    TEST_ASSERT_FALSE(recorder.capture(core::kQ32One, invalid));
    TEST_ASSERT_TRUE(recorder.capture(core::kQ32One, pattern));
    TEST_ASSERT_TRUE(recorder.view().capturedMask != 0ULL);
    recorder.clearCaptured();
    TEST_ASSERT_EQUAL_UINT64(0ULL, recorder.view().capturedMask);
}

void testGrooveRecordEngineCaptureHelpersUseSchedulerResolution() {
    ClockState state{};
    initializeFactoryDefaults(state);
    state.operatingMode = OperatingMode::Independent;
    state.source = ClockSource::Internal;
    state.bpm = 120U;
    for (auto& channel : state.channels) channel.common.mode = ChannelMode::Off;
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};

    hal::GateOutputDriver gates;
    gates.beginDisabled();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    engine.play();
    for (std::uint32_t tick = 0U; tick < 10'000U; ++tick) {
        engine.processSchedulerTick();
    }

    const auto snapshot = engine.snapshot();
    TEST_ASSERT_TRUE(snapshot.masterPositionQ32 > 0U);
    TEST_ASSERT_TRUE(engine.nominalIntervalQ32(0U) > 0U);
    TEST_ASSERT_EQUAL_UINT64(0U, engine.nominalIntervalQ32(kChannelCount));

    constexpr std::uint32_t nowUs = 500'000U;
    TEST_ASSERT_EQUAL_UINT64(
        snapshot.masterPositionQ32,
        engine.estimateMasterPositionAtUs(nowUs, nowUs));
    const std::uint64_t compensated25ms = engine.estimateMasterPositionAtUs(nowUs - 25'000U, nowUs);
    TEST_ASSERT_TRUE(compensated25ms < snapshot.masterPositionQ32);

    // Stale timestamps are deliberately capped at 100 ms rather than being
    // interpreted as an arbitrarily large musical rewind.
    const std::uint64_t compensated100ms = engine.estimateMasterPositionAtUs(nowUs - 100'000U, nowUs);
    const std::uint64_t compensated200ms = engine.estimateMasterPositionAtUs(nowUs - 200'000U, nowUs);
    TEST_ASSERT_EQUAL_UINT64(compensated100ms, compensated200ms);
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
    RUN_TEST(testCustomGrooveLookupSupportsSignedOffsetsAmountAndRotation);
    RUN_TEST(testCustomGrooveRejectsInvalidLengthAndOffset);
    RUN_TEST(testCustomGrooveSignedExtremesRemainMonotonicWithSwing);
    RUN_TEST(testCustomGroovePreviewCanBeAppliedAndClearedWithoutStateMutation);
    RUN_TEST(testCustomGrooveLookupSweepsAllLengthsStepsAndAmounts);
    RUN_TEST(testCustomGrooveEngineMatrixAcrossBpmRatesGridsAndAmounts);
    RUN_TEST(testCustomGrooveDefensiveAndRescheduleBranches);
    RUN_TEST(testGrooveRecorderCountInAndOneShotCapture);
    RUN_TEST(testGrooveRecorderEndlessWrapsAndOverwritesTappedStepOnly);
    RUN_TEST(testGrooveRecorderGuardsClampAndClearCaptureState);
    RUN_TEST(testGrooveRecordEngineCaptureHelpersUseSchedulerResolution);
    return UNITY_END();
}
