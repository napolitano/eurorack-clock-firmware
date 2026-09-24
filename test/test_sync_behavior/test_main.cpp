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

#include "cases/frontend_estimator.inc"
#include "cases/lock_transport_configuration.inc"

}  // namespace

#include "cases/input_roles.inc"

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
    RUN_TEST(testFirstPulseIsAcquisitionOnlyAndDoesNotAdvertiseLock);
    RUN_TEST(testSecondValidPulseAcquiresLockAndAutoStartsTransport);
    RUN_TEST(testManualStopPreventsExternalReacquisitionFromStartingTransport);
    RUN_TEST(testStopLossStopsAndReacquisitionRestartsWhenNotManuallyStopped);
    RUN_TEST(testAutoFreewheelKeepsLastMeasuredExternalTempoAfterLoss);
    RUN_TEST(testAdaptiveTimeoutKeepsLockOneMicrosecondBeforeBoundary);
    RUN_TEST(testAdaptiveTimeoutDropsLockExactlyAtBoundary);
    RUN_TEST(testStopLossModeFreezesEngineAfterTimeout);
    RUN_TEST(testFreewheelLossModeContinuesAtLastExternalTempo);
    RUN_TEST(testInternalLossModeFallsBackToConfiguredInternalTempo);
    RUN_TEST(testFactoryDefaultAutoAcquiresExternalSync);
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
    RUN_TEST(testFactoryDefaultSyncSmoothingIsLow);
    RUN_TEST(testSyncSmoothingOffFollowsNextValidPeriodExactly);
    RUN_TEST(testSyncSmoothingLowUsesSeventyFivePercentNewPeriod);
    RUN_TEST(testSyncSmoothingMediumUsesHalfNewPeriod);
    RUN_TEST(testSyncSmoothingFullPreservesLegacyTwentyFivePercentNewPeriod);
    RUN_TEST(testRuntimeSmoothingChangePreservesLockAndAppliesOnNextPeriod);
    RUN_TEST(testHeldHighSyncCreatesOnlyOneSelectedEdge);
    RUN_TEST(testHeldHighSyncAfterLockEventuallyTimesOut);
    RUN_TEST(testHeldHighSyncCannotManufactureFallingEdgeClock);
    RUN_TEST(testSyncRoleCanUsePhysicalInput2);
    RUN_TEST(testResetRoleCanUsePhysicalInput1);
    RUN_TEST(testRunRoleTracksPhysicalLevel);
    RUN_TEST(testStartAndStopRolesGenerateTransportCommands);
    RUN_TEST(testRestartRoleResetsPhaseAndStartsTransport);
    RUN_TEST(testTapRolePreservesCaptureTimestamp);
    RUN_TEST(testOffRoleIgnoresPhysicalEdges);
    RUN_TEST(testRoleChangeDiscardsQueuedEdgesFromPreviousMeaning);
    RUN_TEST(testFirstEdgeAfterBeginIsNotDiscardedForAssignedRole);
    RUN_TEST(testRunRoleCanUsePhysicalInput2AndReassertAfterManualTransport);
    RUN_TEST(testTransportRolesCanUsePhysicalInput2);
    RUN_TEST(testTransportRolesIgnoreFallingAndContinuityMarkers);
    RUN_TEST(testResetGateRoleTracksBothLevelsOnPhysicalInput2);
    RUN_TEST(testResetRoleRemovalReleasesAppliedGate);
    RUN_TEST(testRunRoleNoAssignmentAndStableLevelAreNoOps);
    RUN_TEST(testFillRoleIsDrainedButDoesNothing);
    RUN_TEST(testFillRoleOnPhysicalInput2IsDrainedButDoesNothing);
    RUN_TEST(testInput2OnlyRoleChangeInvalidatesQueuedMeaning);
    RUN_TEST(testSelectingRunIsTransportNeutralUntilPhysicalLevelChanges);
    RUN_TEST(testPersistedRunDoesNotBreakBootStopOnStandingHighLevel);
    RUN_TEST(testExternalInputActivitySequenceTracksSyncAndReset);
    std::cout << "SYNC behavior assertions: " << checks << "\n";
    return UNITY_END();
}
