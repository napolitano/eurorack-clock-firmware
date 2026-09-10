/**
 * @file test_main.cpp
 * @brief Atomic native behavioral tests for external SYNC, input amplitude modeling, jitter, and loss handling.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdint>
#include <initializer_list>
#include <iostream>
#include <unity.h>
#include <Arduino.h>

#include "config.h"
#include "domain/clock_types.h"
#include "domain/default_configuration.h"
#include "engine/clock_engine.h"
#include "hal/external_input_capture.h"
#include "hal/gate_output_driver.h"
#include "services/external_sync_controller.h"
#include "sync_frontend_model.h"

using namespace clockfw;

void setUp() {}
void tearDown() {}

namespace {

std::uint32_t checks = 0U;

#define CHECK(condition) do { ++checks; TEST_ASSERT_TRUE(condition); } while (false)
#define CHECK_EQ(actual, expected) do { ++checks; TEST_ASSERT_TRUE((actual) == (expected)); } while (false)
#define CHECK_NEAR(actual, expected, tolerance) do { \
    ++checks; \
    TEST_ASSERT_TRUE(std::fabs(static_cast<double>(actual) - static_cast<double>(expected)) <= static_cast<double>(tolerance)); \
} while (false)

ClockState makeState() {
    ClockState state{};
    initializeFactoryDefaults(state);
    state.operatingMode = OperatingMode::Independent;
    state.source = ClockSource::External;
    state.externalSync.pulsesPerQuarterNote = 1U;
    state.externalSync.edge = SyncEdge::Rising;
    state.externalSync.glitchFilterUs = 1U;
    state.externalSync.timeoutMs = 1500U;
    for (ChannelConfig& channel : state.channels) {
        channel.common.mode = ChannelMode::Off;
    }
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.probabilityPercent = 100U;
    return state;
}

struct Fixture final {
    hal::GateOutputDriver gates;
    engine::ClockEngine engine;
    hal::ExternalInputCapture inputs;
    services::ExternalSyncController sync;
    ClockState state;

    Fixture() : engine(gates), sync(inputs, engine), state(makeState()) {
        fakefw::resetArduino();
        gates.beginDisabled();
        gates.enableOutputStage();
    }

    void begin() {
        engine.begin(state);
        sync.begin(state);
        engine.play();
    }
};

std::uint32_t expectedBpmMilli(const std::uint32_t periodUs, const std::uint8_t ppqn = 1U) {
    if (periodUs == 0U || ppqn == 0U) {
        return 0U;
    }
    const std::uint64_t raw = 60000000000ULL /
        (static_cast<std::uint64_t>(periodUs) * static_cast<std::uint64_t>(ppqn));
    return static_cast<std::uint32_t>(std::min<std::uint64_t>(
        raw,
        static_cast<std::uint64_t>(config::kSupportedMaximumBpm) * 1000ULL));
}

void injectRisingPulse(Fixture& fixture, const std::uint32_t risingUs, const std::uint32_t widthUs = 1000U) {
    fixture.inputs.injectSyncEdgeForTest(risingUs, true);
    fixture.sync.processSchedulerTick(risingUs);
    fixture.inputs.injectSyncEdgeForTest(risingUs + widthUs, false);
    fixture.sync.processSchedulerTick(risingUs + widthUs);
}

void injectFallingPulse(Fixture& fixture, const std::uint32_t fallingUs, const std::uint32_t widthUs = 1000U) {
    fixture.inputs.injectSyncEdgeForTest(fallingUs - widthUs, true);
    fixture.sync.processSchedulerTick(fallingUs - widthUs);
    fixture.inputs.injectSyncEdgeForTest(fallingUs, false);
    fixture.sync.processSchedulerTick(fallingUs);
}

void acquireRising(Fixture& fixture, const std::uint32_t periodUs) {
    injectRisingPulse(fixture, 1000U);
    injectRisingPulse(fixture, 1000U + periodUs);
}

void feedPeriods(Fixture& fixture, const std::initializer_list<std::uint32_t> periods) {
    std::uint32_t timestamp = 1000U;
    injectRisingPulse(fixture, timestamp);
    for (const std::uint32_t period : periods) {
        timestamp += period;
        injectRisingPulse(fixture, timestamp);
    }
}

void feedAlternatingPeriods(
    Fixture& fixture,
    const std::uint32_t firstPeriodUs,
    const std::uint32_t secondPeriodUs,
    const std::uint32_t count) {
    std::uint32_t timestamp = 1000U;
    injectRisingPulse(fixture, timestamp);
    for (std::uint32_t index = 0U; index < count; ++index) {
        timestamp += (index & 1U) == 0U ? firstPeriodUs : secondPeriodUs;
        injectRisingPulse(fixture, timestamp);
    }
}

bool applyAnalogLevel(
    Fixture& fixture,
    testsupport::SyncFrontendModel& frontend,
    const std::uint32_t timestampUs,
    const double voltageV) {
    const bool before = frontend.outputHigh();
    const bool after = frontend.sample(voltageV);
    if (before != after) {
        fixture.inputs.injectSyncEdgeForTest(timestampUs, after);
        fixture.sync.processSchedulerTick(timestampUs);
    }
    return after;
}

void assertAnalogClockLocks(const double amplitudeV, const std::uint16_t bpm) {
    Fixture fixture;
    fixture.state.bpm = 120U;
    fixture.begin();
    testsupport::SyncFrontendModel frontend;
    const std::uint32_t periodUs = 60000000U / bpm;
    const std::uint32_t widthUs = std::min<std::uint32_t>(10000U, periodUs / 4U);
    const std::uint32_t firstUs = 1000U;

    CHECK(!applyAnalogLevel(fixture, frontend, firstUs - 1U, 0.0));
    CHECK(applyAnalogLevel(fixture, frontend, firstUs, amplitudeV));
    CHECK(!applyAnalogLevel(fixture, frontend, firstUs + widthUs, 0.0));
    CHECK(applyAnalogLevel(fixture, frontend, firstUs + periodUs, amplitudeV));
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), expectedBpmMilli(periodUs), 2U);
}

void assertExactTempo(const std::uint16_t bpm) {
    Fixture fixture;
    fixture.begin();
    const std::uint32_t periodUs = 60000000U / bpm;
    acquireRising(fixture, periodUs);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), expectedBpmMilli(periodUs), 2U);
}

void assertPpqnTempo(const std::uint8_t ppqn) {
    Fixture fixture;
    fixture.state.externalSync.pulsesPerQuarterNote = ppqn;
    fixture.begin();
    const std::uint32_t periodUs = 500000U / ppqn;
    acquireRising(fixture, periodUs);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), expectedBpmMilli(periodUs, ppqn), 5U);
}

void assertJitterBand(
    const std::array<std::int32_t, 8U>& offsetsUs,
    const std::uint32_t minimumBpmMilli,
    const std::uint32_t maximumBpmMilli) {
    Fixture fixture;
    fixture.state.externalSync.glitchFilterUs = 1U;
    fixture.begin();
    std::uint32_t timestamp = 1000U;
    injectRisingPulse(fixture, timestamp);
    for (std::uint32_t pulse = 0U; pulse < 256U; ++pulse) {
        const std::int32_t signedInterval = 500000 + offsetsUs[pulse % offsetsUs.size()];
        CHECK(signedInterval > 0);
        timestamp += static_cast<std::uint32_t>(signedInterval);
        injectRisingPulse(fixture, timestamp);
    }
    const std::uint32_t filtered = fixture.sync.filteredBpmMilli();
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK(filtered >= minimumBpmMilli);
    CHECK(filtered <= maximumBpmMilli);
}

// -------------------------------------------------------------------------
// Nominal analogue-front-end contract. These are model tests, not HIL proof.
// -------------------------------------------------------------------------

void testSyncFrontendReferenceDividerIsNominal161mV() {
    CHECK_NEAR(testsupport::SyncFrontendModel::referenceVoltageV(), 0.16129, 0.00002);
}

void testSyncFrontendNominalRisingThresholdIsAbout1p79V() {
    CHECK_NEAR(testsupport::SyncFrontendModel::risingThresholdV(), 1.7903, 0.001);
}

void testSyncFrontendNominalFallingThresholdIsAbout1p46V() {
    CHECK_NEAR(testsupport::SyncFrontendModel::fallingThresholdV(), 1.4603, 0.001);
}

void testSyncFrontendNominalHysteresisIsAbout330mV() {
    const double width = testsupport::SyncFrontendModel::risingThresholdV() -
        testsupport::SyncFrontendModel::fallingThresholdV();
    CHECK_NEAR(width, 0.3300, 0.001);
}

void testSyncFrontendZeroVoltsRemainsLow() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(0.0));
}

void testSyncFrontendOneVoltRemainsLow() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(1.0));
}

void testSyncFrontend1400mVRemainsLowFromLowState() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(1.4));
}

void testSyncFrontend1500mVRetainsLowStateInsideHysteresis() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(1.5));
}

void testSyncFrontend1500mVRetainsHighStateInsideHysteresis() {
    testsupport::SyncFrontendModel model;
    model.setOutputHighForTest(true);
    CHECK(model.sample(1.5));
}

void testSyncFrontend1600mVRetainsLowStateInsideHysteresis() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(1.6));
}

void testSyncFrontend1600mVRetainsHighStateInsideHysteresis() {
    testsupport::SyncFrontendModel model;
    model.setOutputHighForTest(true);
    CHECK(model.sample(1.6));
}

void testSyncFrontend1900mVRisesFromLowState() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(1.9));
}

void testSyncFrontend1300mVFallsFromHighState() {
    testsupport::SyncFrontendModel model;
    model.setOutputHighForTest(true);
    CHECK(!model.sample(1.3));
}

void testSyncFrontend2500mVIsNominallyHigh() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(2.5));
}

void testSyncFrontend3000mVIsNominallyHigh() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(3.0));
}

void testSyncFrontend4000mVIsNominallyHigh() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(4.0));
}

void testSyncFrontend5000mVIsNominallyHigh() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(5.0));
}

// End-to-end nominal voltage -> comparator model -> digital capture -> estimator.
void testAnalog2500mVAt20BpmLocksEngine() { assertAnalogClockLocks(2.5, 20U); }
void testAnalog2500mVAt120BpmLocksEngine() { assertAnalogClockLocks(2.5, 120U); }
void testAnalog2500mVAt999BpmLocksEngine() { assertAnalogClockLocks(2.5, 999U); }
void testAnalog3000mVAt20BpmLocksEngine() { assertAnalogClockLocks(3.0, 20U); }
void testAnalog3000mVAt120BpmLocksEngine() { assertAnalogClockLocks(3.0, 120U); }
void testAnalog3000mVAt999BpmLocksEngine() { assertAnalogClockLocks(3.0, 999U); }
void testAnalog4000mVAt20BpmLocksEngine() { assertAnalogClockLocks(4.0, 20U); }
void testAnalog4000mVAt120BpmLocksEngine() { assertAnalogClockLocks(4.0, 120U); }
void testAnalog4000mVAt999BpmLocksEngine() { assertAnalogClockLocks(4.0, 999U); }

// -------------------------------------------------------------------------
// Atomic external-tempo and PPQN behavior.
// -------------------------------------------------------------------------

void testExternal1BpmBoundaryIsAccepted() { assertExactTempo(1U); }
void testExternal20BpmIsAccepted() { assertExactTempo(20U); }
void testExternal30BpmIsAccepted() { assertExactTempo(30U); }
void testExternal60BpmIsAccepted() { assertExactTempo(60U); }
void testExternal120BpmIsAccepted() { assertExactTempo(120U); }
void testExternal240BpmIsAccepted() { assertExactTempo(240U); }
void testExternal300BpmIsAccepted() { assertExactTempo(300U); }
void testExternal600BpmIsAccepted() { assertExactTempo(600U); }
void testExternal900BpmIsAccepted() { assertExactTempo(900U); }
void testExternal999BpmBoundaryIsAccepted() { assertExactTempo(999U); }

void testExternal120BpmAt2PpqnIsAccepted() { assertPpqnTempo(2U); }
void testExternal120BpmAt4PpqnIsAccepted() { assertPpqnTempo(4U); }
void testExternal120BpmAt24PpqnIsAccepted() { assertPpqnTempo(24U); }

void testFallingEdgeClockLocksAt120Bpm() {
    Fixture fixture;
    fixture.state.externalSync.edge = SyncEdge::Falling;
    fixture.begin();
    injectFallingPulse(fixture, 2000U);
    injectFallingPulse(fixture, 502000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
}

void testWrongPolarityTransitionsDoNotEstablishPeriod() {
    Fixture fixture;
    fixture.state.externalSync.edge = SyncEdge::Rising;
    fixture.begin();
    fixture.inputs.injectSyncEdgeForTest(1000U, false);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(501000U, false);
    fixture.sync.processSchedulerTick(501000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 0U);
}

// -------------------------------------------------------------------------
// Glitch rejection and malformed edge streams.
// -------------------------------------------------------------------------

void testDuplicateTimestampDoesNotChangeFilteredTempo() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(501000U, true);
    fixture.sync.processSchedulerTick(501000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void testTenMicrosecondGlitchIsRejected() {
    Fixture fixture;
    fixture.state.externalSync.glitchFilterUs = 1000U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(501010U, true);
    fixture.sync.processSchedulerTick(501010U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void test999MicrosecondGlitchIsRejectedBy1000usFilter() {
    Fixture fixture;
    fixture.state.externalSync.glitchFilterUs = 1000U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(501999U, true);
    fixture.sync.processSchedulerTick(501999U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void test1000MicrosecondBoundaryIsRejectedByTechnicalMaxFloor() {
    Fixture fixture;
    fixture.state.externalSync.pulsesPerQuarterNote = 24U;
    fixture.state.externalSync.glitchFilterUs = 1000U;
    fixture.begin();
    acquireRising(fixture, 20833U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(21833U + 1000U, true);
    fixture.sync.processSchedulerTick(21833U + 1000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void testConfiguredGlitchFloorBoundaryIsAcceptedWhenTechnicallyValid() {
    Fixture fixture;
    fixture.state.externalSync.pulsesPerQuarterNote = 24U;
    fixture.state.externalSync.glitchFilterUs = 5000U;
    fixture.begin();
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(6000U, true);
    fixture.sync.processSchedulerTick(6000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 500000U);
}

void testGlitchStormCannotDragStable120BpmEstimator() {
    Fixture fixture;
    fixture.state.externalSync.glitchFilterUs = 1000U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t stable = fixture.sync.filteredBpmMilli();
    for (std::uint32_t offset = 10U; offset < 1000U; offset += 10U) {
        fixture.inputs.injectSyncEdgeForTest(501000U + offset, true);
        fixture.sync.processSchedulerTick(501000U + offset);
    }
    CHECK_EQ(fixture.sync.filteredBpmMilli(), stable);
    CHECK(fixture.engine.snapshot().externalLocked);
}

void testOppositePolarityNoiseCannotChangeRisingEdgePeriod() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t stable = fixture.sync.filteredBpmMilli();
    for (std::uint32_t index = 0U; index < 20U; ++index) {
        fixture.inputs.injectSyncEdgeForTest(510000U + index * 1000U, false);
        fixture.sync.processSchedulerTick(510000U + index * 1000U);
    }
    CHECK_EQ(fixture.sync.filteredBpmMilli(), stable);
}

// -------------------------------------------------------------------------
// Jitter and deliberately unstable clocks. The contract is bounded, stable
// behavior, not pretending that a wildly varying source has one exact tempo.
// -------------------------------------------------------------------------

void testPlusMinus50usInputJitterRemainsTightlyBounded() {
    assertJitterBand({{-50, 50, -25, 25, -10, 10, 0, 0}}, 119900U, 120100U);
}

void testPlusMinus500usInputJitterRemainsBounded() {
    assertJitterBand({{-500, 500, -250, 250, -100, 100, 0, 0}}, 119000U, 121000U);
}

void testPlusMinus2msInputJitterRemainsBounded() {
    assertJitterBand({{-2000, 2000, -1500, 1500, -750, 750, 0, 0}}, 118000U, 122000U);
}

void testPlusMinus10msInputJitterRemainsLocked() {
    assertJitterBand({{-10000, 10000, -8000, 8000, -4000, 4000, 0, 0}}, 114000U, 126000U);
}

void testPlusMinus20PercentInputJitterRemainsLocked() {
    assertJitterBand({{-100000, 100000, -75000, 75000, -50000, 50000, -25000, 25000}}, 95000U, 155000U);
}

void testAlternating400And600msPeriodsRemainLocked() {
    Fixture fixture;
    fixture.begin();
    feedAlternatingPeriods(fixture, 400000U, 600000U, 128U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK(fixture.sync.filteredBpmMilli() >= 100000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 150000U);
}

void testAlternating250And750msPeriodsRemainLocked() {
    Fixture fixture;
    fixture.begin();
    feedAlternatingPeriods(fixture, 250000U, 750000U, 128U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK(fixture.sync.filteredBpmMilli() >= 80000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 240000U);
}

void testChaoticValidPeriodSequenceNeverProducesOutOfRangeTempo() {
    Fixture fixture;
    fixture.begin();
    constexpr std::array<std::uint32_t, 16U> periods{{
        300000U, 690000U, 410000U, 555000U, 350000U, 620000U, 480000U, 700000U,
        325000U, 590000U, 450000U, 675000U, 375000U, 530000U, 495000U, 640000U}};
    std::uint32_t timestamp = 1000U;
    injectRisingPulse(fixture, timestamp);
    for (std::uint32_t cycle = 0U; cycle < 32U; ++cycle) {
        for (const std::uint32_t period : periods) {
            timestamp += period;
            injectRisingPulse(fixture, timestamp);
            CHECK(fixture.engine.snapshot().externalLocked);
            CHECK(fixture.sync.filteredBpmMilli() >= 1000U);
            CHECK(fixture.sync.filteredBpmMilli() <= 999000U);
        }
    }
}

void testAbrupt60To180BpmChangeConvergesWithoutUnlock() {
    Fixture fixture;
    fixture.begin();
    feedPeriods(fixture, {1000000U, 1000000U, 1000000U, 1000000U});
    std::uint32_t timestamp = 4001000U;
    for (std::uint32_t i = 0U; i < 28U; ++i) {
        timestamp += 333333U;
        injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
    }
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), 180000U, 1000U);
}

void testAbrupt180To60BpmChangeConvergesWithoutUnlock() {
    Fixture fixture;
    fixture.begin();
    feedPeriods(fixture, {333333U, 333333U, 333333U, 333333U});
    std::uint32_t timestamp = 1334332U;
    for (std::uint32_t i = 0U; i < 28U; ++i) {
        timestamp += 1000000U;
        injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
    }
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), 60000U, 1000U);
}


void testLinearAccelerationFrom60To180BpmStaysLocked() {
    Fixture fixture; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    std::uint32_t previousFiltered = 0U;
    for (std::uint32_t i = 0U; i < 80U; ++i) {
        const std::uint32_t bpm = 60U + (120U * i) / 79U;
        timestamp += 60000000U / bpm;
        injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
        const std::uint32_t filtered = fixture.sync.filteredBpmMilli();
        CHECK(filtered >= 1000U && filtered <= 999000U);
        if (i > 12U) CHECK(filtered + 5000U >= previousFiltered);
        previousFiltered = filtered;
    }
    CHECK(fixture.sync.filteredBpmMilli() >= 165000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 185000U);
}

void testLinearDecelerationFrom180To60BpmStaysLocked() {
    Fixture fixture; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    std::uint32_t previousFiltered = 1000000U;
    for (std::uint32_t i = 0U; i < 80U; ++i) {
        const std::uint32_t bpm = 180U - (120U * i) / 79U;
        timestamp += 60000000U / bpm;
        injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
        const std::uint32_t filtered = fixture.sync.filteredBpmMilli();
        CHECK(filtered >= 1000U && filtered <= 999000U);
        if (i > 12U) CHECK(filtered <= previousFiltered + 5000U);
        previousFiltered = filtered;
    }
    CHECK(fixture.sync.filteredBpmMilli() >= 55000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 70000U);
}

void testSlowTempoDriftAround120BpmRemainsLockedAndBounded() {
    Fixture fixture; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    for (std::uint32_t i = 0U; i < 200U; ++i) {
        const std::int32_t offset = static_cast<std::int32_t>(i % 41U) - 20;
        const std::uint32_t bpm = static_cast<std::uint32_t>(120 + offset / 4);
        timestamp += 60000000U / bpm; injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
        CHECK(fixture.sync.filteredBpmMilli() >= 110000U);
        CHECK(fixture.sync.filteredBpmMilli() <= 130000U);
    }
}

void testRepeatedTempoStepsDoNotLoseLock() {
    Fixture fixture; fixture.begin();
    constexpr std::array<std::uint32_t, 8U> bpms{{120U, 90U, 150U, 60U, 180U, 75U, 200U, 120U}};
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    for (const std::uint32_t bpm : bpms) {
        for (std::uint32_t n = 0U; n < 16U; ++n) {
            timestamp += 60000000U / bpm; injectRisingPulse(fixture, timestamp);
            CHECK(fixture.engine.snapshot().externalLocked);
            CHECK(fixture.sync.filteredBpmMilli() >= 1000U);
            CHECK(fixture.sync.filteredBpmMilli() <= 999000U);
        }
    }
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), 120000U, 5000U);
}

void testAccelerationWithAlternatingJitterRemainsLocked() {
    Fixture fixture; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    for (std::uint32_t i = 0U; i < 96U; ++i) {
        const std::uint32_t bpm = 80U + (80U * i) / 95U;
        const std::uint32_t nominal = 60000000U / bpm;
        const std::int32_t jitter = (i & 1U) == 0U ? -2000 : 2000;
        const std::int32_t interval = static_cast<std::int32_t>(nominal) + jitter;
        CHECK(interval > 0); timestamp += static_cast<std::uint32_t>(interval);
        injectRisingPulse(fixture, timestamp); CHECK(fixture.engine.snapshot().externalLocked);
    }
    CHECK(fixture.sync.filteredBpmMilli() >= 145000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 170000U);
}

void testTwentyFourPpqnTempoRampRemainsLocked() {
    Fixture fixture; fixture.state.externalSync.pulsesPerQuarterNote = 24U; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    for (std::uint32_t i = 0U; i < 120U; ++i) {
        const std::uint32_t bpm = 90U + (60U * i) / 119U;
        timestamp += 60000000U / (bpm * 24U); injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
        CHECK(fixture.sync.filteredBpmMilli() >= 1000U);
        CHECK(fixture.sync.filteredBpmMilli() <= 999000U);
    }
    CHECK(fixture.sync.filteredBpmMilli() >= 135000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 160000U);
}

// -------------------------------------------------------------------------
// Lock loss, fallback policies, range boundaries, wrap and continuity.
// -------------------------------------------------------------------------

void testAdaptiveTimeoutKeepsLockOneMicrosecondBeforeBoundary() {
    Fixture fixture;
    fixture.state.externalSync.timeoutMs = 200U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    fixture.sync.processSchedulerTick(1500999U);
    CHECK(fixture.engine.snapshot().externalLocked);
}

void testAdaptiveTimeoutDropsLockExactlyAtBoundary() {
    Fixture fixture;
    fixture.state.externalSync.timeoutMs = 200U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    fixture.sync.processSchedulerTick(1501000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
}

void testStopLossModeFreezesEngineAfterTimeout() {
    Fixture fixture;
    fixture.state.externalSync.lossMode = SyncLossMode::Stop;
    fixture.state.externalSync.timeoutMs = 200U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    fixture.sync.processSchedulerTick(1501000U);
    const auto before = fixture.engine.snapshot().masterPositionQ32;
    for (std::uint32_t i = 0U; i < 1000U; ++i) fixture.engine.processSchedulerTick();
    CHECK_EQ(fixture.engine.snapshot().masterPositionQ32, before);
}

void testFreewheelLossModeContinuesAtLastExternalTempo() {
    Fixture fixture;
    fixture.state.externalSync.lossMode = SyncLossMode::Freewheel;
    fixture.state.externalSync.timeoutMs = 200U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    fixture.sync.processSchedulerTick(1501000U);
    const auto before = fixture.engine.snapshot().masterPositionQ32;
    for (std::uint32_t i = 0U; i < 1000U; ++i) fixture.engine.processSchedulerTick();
    CHECK(fixture.engine.snapshot().masterPositionQ32 > before);
}

void testInternalLossModeFallsBackToConfiguredInternalTempo() {
    Fixture fixture;
    fixture.state.bpm = 90U;
    fixture.state.externalSync.lossMode = SyncLossMode::Internal;
    fixture.state.externalSync.timeoutMs = 200U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    fixture.sync.processSchedulerTick(1501000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
    const auto before = fixture.engine.snapshot().masterPositionQ32;
    for (std::uint32_t i = 0U; i < 1000U; ++i) fixture.engine.processSchedulerTick();
    CHECK(fixture.engine.snapshot().masterPositionQ32 > before);
}

void testAutoSourceFallsBackAfterExternalLoss() {
    Fixture fixture;
    fixture.state.source = ClockSource::Auto;
    fixture.state.bpm = 90U;
    fixture.state.externalSync.timeoutMs = 200U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    fixture.sync.processSchedulerTick(1501000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
    const auto before = fixture.engine.snapshot().masterPositionQ32;
    for (std::uint32_t i = 0U; i < 1000U; ++i) fixture.engine.processSchedulerTick();
    CHECK(fixture.engine.snapshot().masterPositionQ32 > before);
}

void testFasterThan999BpmPulseIsIgnoredAfterLock() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 60060U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(61060U + 50000U, true);
    fixture.sync.processSchedulerTick(61060U + 50000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
    CHECK(fixture.engine.snapshot().externalLocked);
}

void testSlowerThan1BpmGapStartsFreshAcquisition() {
    Fixture fixture;
    fixture.state.bpm = 123U;
    fixture.begin();
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(60002000U, true);
    fixture.sync.processSchedulerTick(60002000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 123000U);
}

void testTimestampWrapMaintains120BpmPeriod() {
    Fixture fixture;
    fixture.begin();
    const std::uint32_t first = 0xFFF90000U;
    fixture.inputs.injectSyncEdgeForTest(first, true);
    fixture.sync.processSchedulerTick(first);
    const std::uint32_t second = first + 500000U;
    fixture.inputs.injectSyncEdgeForTest(second, true);
    fixture.sync.processSchedulerTick(second);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
}

void testExplicitClearRequiresFreshPeriodBeforeTempoUpdate() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
    fixture.sync.clearExternalLock();
    CHECK(!fixture.engine.snapshot().externalLocked);
    fixture.inputs.injectSyncEdgeForTest(2000000U, true);
    fixture.sync.processSchedulerTick(2000000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
    fixture.inputs.injectSyncEdgeForTest(3000000U, true);
    fixture.sync.processSchedulerTick(3000000U);
    CHECK(fixture.sync.filteredBpmMilli() < 120000U);
}

void testQueueOverflowForcesFreshContinuityEpoch() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    for (std::uint32_t i = 0U; i < 40U; ++i) {
        fixture.inputs.injectSyncEdgeForTest(600000U + i * 2000U, (i & 1U) == 0U);
    }
    CHECK(fixture.inputs.droppedSyncEdges() > 0U);
    fixture.sync.processSchedulerTick(700000U);

    fixture.inputs.injectSyncEdgeForTest(2000000U, true);
    fixture.sync.processSchedulerTick(2000000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(2500000U, true);
    fixture.sync.processSchedulerTick(2500000U);
    CHECK(fixture.sync.filteredBpmMilli() >= 119000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 121000U);
    CHECK(before <= 999000U);
}


void testRuntimePpqnChangeRestartsAcquisitionWithoutBogusTempo() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    ClockState updated = fixture.state;
    updated.externalSync.pulsesPerQuarterNote = 24U;
    fixture.sync.updateConfiguration(updated);
    CHECK(!fixture.engine.snapshot().externalLocked);

    fixture.inputs.injectSyncEdgeForTest(700000U, true);
    fixture.sync.processSchedulerTick(700000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    fixture.inputs.injectSyncEdgeForTest(720833U, true);
    fixture.sync.processSchedulerTick(720833U);
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), 120000U, 20U);
}

void testRuntimeSelectedEdgeChangeRestartsAcquisition() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    ClockState updated = fixture.state;
    updated.externalSync.edge = SyncEdge::Falling;
    fixture.sync.updateConfiguration(updated);
    CHECK(!fixture.engine.snapshot().externalLocked);

    injectFallingPulse(fixture, 700000U);
    injectFallingPulse(fixture, 1200000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
}

void testRuntimeGlitchFilterChangePreservesValidLockAndEstimator() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();

    ClockState updated = fixture.state;
    updated.externalSync.glitchFilterUs = 1000U;
    fixture.sync.updateConfiguration(updated);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void testRuntimeTimeoutChangePreservesValidLockAndEstimator() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();

    ClockState updated = fixture.state;
    updated.externalSync.timeoutMs = 3000U;
    fixture.sync.updateConfiguration(updated);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void testRuntimeLossModeChangePreservesValidLockAndEstimator() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();

    ClockState updated = fixture.state;
    updated.externalSync.lossMode = SyncLossMode::Stop;
    fixture.sync.updateConfiguration(updated);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(testSyncFrontendReferenceDividerIsNominal161mV);
    RUN_TEST(testSyncFrontendNominalRisingThresholdIsAbout1p79V);
    RUN_TEST(testSyncFrontendNominalFallingThresholdIsAbout1p46V);
    RUN_TEST(testSyncFrontendNominalHysteresisIsAbout330mV);
    RUN_TEST(testSyncFrontendZeroVoltsRemainsLow);
    RUN_TEST(testSyncFrontendOneVoltRemainsLow);
    RUN_TEST(testSyncFrontend1400mVRemainsLowFromLowState);
    RUN_TEST(testSyncFrontend1500mVRetainsLowStateInsideHysteresis);
    RUN_TEST(testSyncFrontend1500mVRetainsHighStateInsideHysteresis);
    RUN_TEST(testSyncFrontend1600mVRetainsLowStateInsideHysteresis);
    RUN_TEST(testSyncFrontend1600mVRetainsHighStateInsideHysteresis);
    RUN_TEST(testSyncFrontend1900mVRisesFromLowState);
    RUN_TEST(testSyncFrontend1300mVFallsFromHighState);
    RUN_TEST(testSyncFrontend2500mVIsNominallyHigh);
    RUN_TEST(testSyncFrontend3000mVIsNominallyHigh);
    RUN_TEST(testSyncFrontend4000mVIsNominallyHigh);
    RUN_TEST(testSyncFrontend5000mVIsNominallyHigh);
    RUN_TEST(testAnalog2500mVAt20BpmLocksEngine);
    RUN_TEST(testAnalog2500mVAt120BpmLocksEngine);
    RUN_TEST(testAnalog2500mVAt999BpmLocksEngine);
    RUN_TEST(testAnalog3000mVAt20BpmLocksEngine);
    RUN_TEST(testAnalog3000mVAt120BpmLocksEngine);
    RUN_TEST(testAnalog3000mVAt999BpmLocksEngine);
    RUN_TEST(testAnalog4000mVAt20BpmLocksEngine);
    RUN_TEST(testAnalog4000mVAt120BpmLocksEngine);
    RUN_TEST(testAnalog4000mVAt999BpmLocksEngine);
    RUN_TEST(testExternal1BpmBoundaryIsAccepted);
    RUN_TEST(testExternal20BpmIsAccepted);
    RUN_TEST(testExternal30BpmIsAccepted);
    RUN_TEST(testExternal60BpmIsAccepted);
    RUN_TEST(testExternal120BpmIsAccepted);
    RUN_TEST(testExternal240BpmIsAccepted);
    RUN_TEST(testExternal300BpmIsAccepted);
    RUN_TEST(testExternal600BpmIsAccepted);
    RUN_TEST(testExternal900BpmIsAccepted);
    RUN_TEST(testExternal999BpmBoundaryIsAccepted);
    RUN_TEST(testExternal120BpmAt2PpqnIsAccepted);
    RUN_TEST(testExternal120BpmAt4PpqnIsAccepted);
    RUN_TEST(testExternal120BpmAt24PpqnIsAccepted);
    RUN_TEST(testFallingEdgeClockLocksAt120Bpm);
    RUN_TEST(testWrongPolarityTransitionsDoNotEstablishPeriod);
    RUN_TEST(testDuplicateTimestampDoesNotChangeFilteredTempo);
    RUN_TEST(testTenMicrosecondGlitchIsRejected);
    RUN_TEST(test999MicrosecondGlitchIsRejectedBy1000usFilter);
    RUN_TEST(test1000MicrosecondBoundaryIsRejectedByTechnicalMaxFloor);
    RUN_TEST(testConfiguredGlitchFloorBoundaryIsAcceptedWhenTechnicallyValid);
    RUN_TEST(testGlitchStormCannotDragStable120BpmEstimator);
    RUN_TEST(testOppositePolarityNoiseCannotChangeRisingEdgePeriod);
    RUN_TEST(testPlusMinus50usInputJitterRemainsTightlyBounded);
    RUN_TEST(testPlusMinus500usInputJitterRemainsBounded);
    RUN_TEST(testPlusMinus2msInputJitterRemainsBounded);
    RUN_TEST(testPlusMinus10msInputJitterRemainsLocked);
    RUN_TEST(testPlusMinus20PercentInputJitterRemainsLocked);
    RUN_TEST(testAlternating400And600msPeriodsRemainLocked);
    RUN_TEST(testAlternating250And750msPeriodsRemainLocked);
    RUN_TEST(testChaoticValidPeriodSequenceNeverProducesOutOfRangeTempo);
    RUN_TEST(testAbrupt60To180BpmChangeConvergesWithoutUnlock);
    RUN_TEST(testAbrupt180To60BpmChangeConvergesWithoutUnlock);
    RUN_TEST(testLinearAccelerationFrom60To180BpmStaysLocked);
    RUN_TEST(testLinearDecelerationFrom180To60BpmStaysLocked);
    RUN_TEST(testSlowTempoDriftAround120BpmRemainsLockedAndBounded);
    RUN_TEST(testRepeatedTempoStepsDoNotLoseLock);
    RUN_TEST(testAccelerationWithAlternatingJitterRemainsLocked);
    RUN_TEST(testTwentyFourPpqnTempoRampRemainsLocked);
    RUN_TEST(testAdaptiveTimeoutKeepsLockOneMicrosecondBeforeBoundary);
    RUN_TEST(testAdaptiveTimeoutDropsLockExactlyAtBoundary);
    RUN_TEST(testStopLossModeFreezesEngineAfterTimeout);
    RUN_TEST(testFreewheelLossModeContinuesAtLastExternalTempo);
    RUN_TEST(testInternalLossModeFallsBackToConfiguredInternalTempo);
    RUN_TEST(testAutoSourceFallsBackAfterExternalLoss);
    RUN_TEST(testFasterThan999BpmPulseIsIgnoredAfterLock);
    RUN_TEST(testSlowerThan1BpmGapStartsFreshAcquisition);
    RUN_TEST(testTimestampWrapMaintains120BpmPeriod);
    RUN_TEST(testExplicitClearRequiresFreshPeriodBeforeTempoUpdate);
    RUN_TEST(testQueueOverflowForcesFreshContinuityEpoch);
    RUN_TEST(testRuntimePpqnChangeRestartsAcquisitionWithoutBogusTempo);
    RUN_TEST(testRuntimeSelectedEdgeChangeRestartsAcquisition);
    RUN_TEST(testRuntimeGlitchFilterChangePreservesValidLockAndEstimator);
    RUN_TEST(testRuntimeTimeoutChangePreservesValidLockAndEstimator);
    RUN_TEST(testRuntimeLossModeChangePreservesValidLockAndEstimator);
    std::cout << "SYNC behavior assertions: " << checks << "\n";
    return UNITY_END();
}
