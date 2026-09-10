/**
 * @file test_main.cpp
 * @brief Focused integration and stress tests for CLOCK real-time timing, SYNC, and RST behavior.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <array>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <unity.h>

#include "clock_core.h"
#include "config.h"
#include "defaults.h"
#include "domain/clock_types.h"
#include "domain/default_configuration.h"
#include "engine/clock_engine.h"
#include "hal/external_input_capture.h"
#include "hal/gate_output_driver.h"
#include "pin_map.h"
#include "services/external_sync_controller.h"

using namespace clockfw;

void setUp() {}
void tearDown() {}

namespace {

std::uint32_t checks = 0U;

#define CHECK(condition) do { ++checks; TEST_ASSERT_TRUE(condition); } while (false)
#define CHECK_EQ(actual, expected) do { ++checks; TEST_ASSERT_TRUE((actual) == (expected)); } while (false)

ClockState makeRealtimeState() {
    ClockState state{};
    initializeFactoryDefaults(state);
    state.operatingMode = OperatingMode::Independent;
    state.source = ClockSource::Auto;
    for (ChannelConfig& channel : state.channels) {
        channel.common.mode = ChannelMode::Off;
    }
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.probabilityPercent = 100U;
    state.channels[0].common.resetMode = ResetMode::Global;
    return state;
}

struct Fixture final {
    hal::GateOutputDriver gates;
    engine::ClockEngine engine;
    hal::ExternalInputCapture inputs;
    services::ExternalSyncController sync;
    ClockState state;

    Fixture()
        : engine(gates), sync(inputs, engine), state(makeRealtimeState()) {
        fakefw::resetArduino();
        gates.beginDisabled();
        gates.enableOutputStage();
    }

    void begin() {
        engine.begin(state);
        sync.begin(state);
        engine.play();
    }

    void tick(std::uint32_t nowUs) {
        sync.processSchedulerTick(nowUs);
        engine.processSchedulerTick();
    }
};

void runTicks(engine::ClockEngine& engine, const std::uint32_t count) {
    for (std::uint32_t index = 0U; index < count; ++index) {
        engine.processSchedulerTick();
    }
}

void testMinuteLongMasterTimingHasNoDrift() {
    for (const std::uint16_t bpm : std::array<std::uint16_t, 3U>{{20U, 120U, 999U}}) {
        Fixture fixture;
        fixture.state.source = ClockSource::Internal;
        fixture.state.bpm = bpm;
        fixture.begin();
        constexpr std::uint32_t kTicksPerMinute = config::kSchedulerFrequencyHz * 60U;
        runTicks(fixture.engine, kTicksPerMinute);
        const auto snapshot = fixture.engine.snapshot();
        CHECK_EQ(snapshot.masterPositionQ32, static_cast<std::uint64_t>(bpm) * core::kQ32One);
        CHECK_EQ(snapshot.masterBeatSerial, static_cast<std::uint32_t>(bpm));
    }
}

void testConfiguredGateLengthsReachPhysicalGpio() {
    for (const std::uint16_t gateMs : std::array<std::uint16_t, 7U>{{1U, 2U, 5U, 10U, 20U, 50U, 100U}}) {
        Fixture fixture;
        fixture.state.source = ClockSource::Internal;
        fixture.state.bpm = 120U;
        fixture.state.channels[0].common.gateLengthMs = gateMs;
        fixture.begin();

        fixture.engine.processSchedulerTick();
        CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], HIGH);
        const std::uint32_t gateTicks =
            (static_cast<std::uint32_t>(gateMs) * 1000U) / config::kSchedulerTickUs;
        if (gateTicks > 1U) {
            runTicks(fixture.engine, gateTicks - 1U);
        }
        CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], HIGH);
        fixture.engine.processSchedulerTick();
        CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], LOW);
    }
}

void testRepresentativeExternalTemposAndPpqn() {
    struct Case { std::uint32_t periodUs; std::uint8_t ppqn; };
    const std::array<Case, 12U> cases{{
        {3000000U, 1U}, {1500000U, 2U}, {750000U, 4U}, {125000U, 24U},
        {500000U, 1U}, {250000U, 2U}, {125000U, 4U}, {20833U, 24U},
        {60060U, 1U}, {30030U, 2U}, {15015U, 4U}, {2502U, 24U},
    }};

    for (const Case item : cases) {
        Fixture fixture;
        fixture.state.externalSync.pulsesPerQuarterNote = item.ppqn;
        fixture.state.externalSync.edge = SyncEdge::Rising;
        fixture.state.externalSync.glitchFilterUs = 1U;
        fixture.begin();
        const std::uint32_t first = 1000U;
        fixture.inputs.injectSyncEdgeForTest(first, true);
        fixture.sync.processSchedulerTick(first);
        fixture.inputs.injectSyncEdgeForTest(first + item.periodUs, true);
        fixture.sync.processSchedulerTick(first + item.periodUs);

        const std::uint64_t rawExpected =
            60000000000ULL / (static_cast<std::uint64_t>(item.periodUs) * item.ppqn);
        const std::uint32_t expected = static_cast<std::uint32_t>(
            rawExpected > static_cast<std::uint64_t>(config::kSupportedMaximumBpm) * 1000ULL
                ? static_cast<std::uint64_t>(config::kSupportedMaximumBpm) * 1000ULL
                : rawExpected);
        const auto snapshot = fixture.engine.snapshot();
        CHECK(snapshot.externalLocked);
        CHECK_EQ(snapshot.externalBpmMilli, expected);
        CHECK_EQ(fixture.sync.filteredBpmMilli(), expected);
    }
}

void testFallingEdgeSelection() {
    Fixture fixture;
    fixture.state.externalSync.edge = SyncEdge::Falling;
    fixture.state.externalSync.glitchFilterUs = 1U;
    fixture.begin();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
    fixture.inputs.injectSyncEdgeForTest(2000U, false);
    fixture.sync.processSchedulerTick(2000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    fixture.inputs.injectSyncEdgeForTest(502000U, false);
    fixture.sync.processSchedulerTick(502000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
}

void testGlitchesDoNotCorruptPeriodEstimator() {
    Fixture fixture;
    fixture.state.externalSync.glitchFilterUs = 1000U;
    fixture.begin();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(501000U, true);
    fixture.sync.processSchedulerTick(501000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    fixture.inputs.injectSyncEdgeForTest(501100U, true);
    fixture.sync.processSchedulerTick(501100U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
    fixture.inputs.injectSyncEdgeForTest(1001000U, true);
    fixture.sync.processSchedulerTick(1001000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
}

void testDeterministicJitterRemainsBounded() {
    Fixture fixture;
    fixture.state.externalSync.pulsesPerQuarterNote = 24U;
    fixture.state.externalSync.glitchFilterUs = 100U;
    fixture.state.externalSync.timeoutMs = 2000U;
    fixture.begin();

    std::uint32_t timestamp = 1000U;
    fixture.inputs.injectSyncEdgeForTest(timestamp, true);
    fixture.sync.processSchedulerTick(timestamp);
    constexpr std::array<std::int32_t, 8U> jitter{{-90, 70, -40, 100, -70, 50, -20, 0}};
    for (std::uint32_t pulse = 0U; pulse < 10000U; ++pulse) {
        const std::int32_t interval = 20833 + jitter[pulse % jitter.size()];
        timestamp += static_cast<std::uint32_t>(interval);
        fixture.inputs.injectSyncEdgeForTest(timestamp, true);
        fixture.sync.processSchedulerTick(timestamp);
    }
    const std::uint32_t bpm = fixture.sync.filteredBpmMilli();
    CHECK(bpm >= 119000U);
    CHECK(bpm <= 121000U);
    CHECK(fixture.engine.snapshot().externalLocked);
}

void testSyncTimeoutBoundaryAndLossModes() {
    for (const SyncLossMode lossMode : {SyncLossMode::Stop, SyncLossMode::Freewheel}) {
        Fixture fixture;
        fixture.state.source = ClockSource::External;
        fixture.state.externalSync.lossMode = lossMode;
        fixture.state.externalSync.timeoutMs = 200U;
        fixture.begin();
        fixture.inputs.injectSyncEdgeForTest(1000U, true);
        fixture.sync.processSchedulerTick(1000U);
        CHECK(fixture.engine.snapshot().externalLocked);
        fixture.inputs.injectSyncEdgeForTest(501000U, true);
        fixture.sync.processSchedulerTick(501000U);
        CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

        // The user timeout is a floor. At 120 BPM / 1 PPQN the measured
        // 500-ms period allows one missing pulse, so loss occurs at 1000 ms.
        fixture.sync.processSchedulerTick(1500999U);
        CHECK(fixture.engine.snapshot().externalLocked);
        fixture.sync.processSchedulerTick(1501000U);
        CHECK(!fixture.engine.snapshot().externalLocked);

        const std::uint64_t before = fixture.engine.snapshot().masterPositionQ32;
        runTicks(fixture.engine, 100U);
        const std::uint64_t after = fixture.engine.snapshot().masterPositionQ32;
        if (lossMode == SyncLossMode::Stop) {
            CHECK_EQ(after, before);
        } else {
            CHECK(after > before);
        }
    }
}

void testSlowExternalClockDoesNotTimeoutBeforeSecondPulse() {
    for (const std::uint16_t minimumBpm : std::array<std::uint16_t, 2U>{{1U, 20U}}) {
        Fixture fixture;
        fixture.state.source = ClockSource::External;
        fixture.state.tempoRange.minimumBpm = minimumBpm;
        fixture.state.externalSync.pulsesPerQuarterNote = 1U;
        fixture.state.externalSync.timeoutMs = 1500U;
        fixture.state.externalSync.glitchFilterUs = 1U;
        fixture.begin();

        const std::uint32_t periodUs = 60000000U / minimumBpm;
        fixture.inputs.injectSyncEdgeForTest(1000U, true);
        fixture.sync.processSchedulerTick(1000U);
        CHECK(fixture.engine.snapshot().externalLocked);
        fixture.sync.processSchedulerTick(1000U + periodUs - 1U);
        CHECK(fixture.engine.snapshot().externalLocked);
        fixture.inputs.injectSyncEdgeForTest(1000U + periodUs, true);
        fixture.sync.processSchedulerTick(1000U + periodUs);
        CHECK(fixture.engine.snapshot().externalLocked);
        CHECK_EQ(fixture.sync.filteredBpmMilli(), static_cast<std::uint32_t>(minimumBpm) * 1000U);

        // Once measured, one missing pulse is tolerated; the second missing period
        // reaches the adaptive loss boundary exactly.
        fixture.sync.processSchedulerTick(1000U + periodUs * 3U - 1U);
        CHECK(fixture.engine.snapshot().externalLocked);
        fixture.sync.processSchedulerTick(1000U + periodUs * 3U);
        CHECK(!fixture.engine.snapshot().externalLocked);
    }
}

void testTimestampWraparound() {
    Fixture fixture;
    fixture.state.externalSync.glitchFilterUs = 1U;
    fixture.begin();
    constexpr std::uint32_t first = 0xFFFFFF00U;
    constexpr std::uint32_t period = 500000U;
    const std::uint32_t second = first + period;
    fixture.inputs.injectSyncEdgeForTest(first, true);
    fixture.sync.processSchedulerTick(first);
    fixture.inputs.injectSyncEdgeForTest(second, true);
    fixture.sync.processSchedulerTick(second);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
    CHECK(fixture.engine.snapshot().externalLocked);
}

void testResetTriggerIsEdgeTriggered() {
    Fixture fixture;
    fixture.state.source = ClockSource::Internal;
    fixture.state.externalSync.resetMode = ExternalResetMode::Trigger;
    fixture.begin();
    runTicks(fixture.engine, 137U);
    CHECK(fixture.engine.snapshot().masterBeatPhaseQ32 > 0U);

    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK_EQ(fixture.engine.snapshot().masterBeatPhaseQ32, 0U);
    CHECK(!fixture.engine.snapshot().externalResetHeld);

    runTicks(fixture.engine, 10U);
    const std::uint64_t advanced = fixture.engine.snapshot().masterBeatPhaseQ32;
    CHECK(advanced > 0U);
    fixture.sync.processSchedulerTick(1100U);
    CHECK_EQ(fixture.engine.snapshot().masterBeatPhaseQ32, advanced);
    fixture.inputs.injectResetEdgeForTest(1200U, false);
    fixture.sync.processSchedulerTick(1200U);
    CHECK_EQ(fixture.engine.snapshot().masterBeatPhaseQ32, advanced);
    fixture.inputs.injectResetEdgeForTest(1300U, true);
    fixture.sync.processSchedulerTick(1300U);
    CHECK_EQ(fixture.engine.snapshot().masterBeatPhaseQ32, 0U);
}

void testResetBurstIsCoalescedPerSchedulerQuantum() {
    Fixture fixture;
    fixture.state.source = ClockSource::Internal;
    fixture.state.externalSync.resetMode = ExternalResetMode::Trigger;
    fixture.begin();
    const std::uint32_t before = fixture.engine.resetRuntimeCountForTest();

    // A noisy comparator can queue several transitions before the 20-kHz scheduler
    // gets CPU time. They are indistinguishable inside one 50-us quantum and must
    // not trigger repeated full eight-channel schedule rebuilds.
    for (std::uint32_t index = 0U; index < 6U; ++index) {
        fixture.inputs.injectResetEdgeForTest(1000U + index * 2U, true);
        fixture.inputs.injectResetEdgeForTest(1001U + index * 2U, false);
    }
    fixture.sync.processSchedulerTick(1050U);
    CHECK_EQ(fixture.engine.resetRuntimeCountForTest(), before + 1U);
    fixture.sync.processSchedulerTick(1100U);
    CHECK_EQ(fixture.engine.resetRuntimeCountForTest(), before + 1U);
}

void testResetOverflowStillCoalescesToOneReset() {
    Fixture fixture;
    fixture.state.source = ClockSource::Internal;
    fixture.state.externalSync.resetMode = ExternalResetMode::Trigger;
    fixture.begin();
    const std::uint32_t before = fixture.engine.resetRuntimeCountForTest();

    for (std::uint32_t index = 0U; index < 24U; ++index) {
        fixture.inputs.injectResetEdgeForTest(2000U + index, (index & 1U) == 0U);
    }
    CHECK(fixture.inputs.collapsedResetEdges() > 0U);
    fixture.sync.processSchedulerTick(2100U);
    CHECK_EQ(fixture.engine.resetRuntimeCountForTest(), before + 1U);
}

void testResetGateHoldsAndReleasesScheduler() {
    Fixture fixture;
    fixture.state.source = ClockSource::Internal;
    fixture.state.externalSync.resetMode = ExternalResetMode::Gate;
    fixture.begin();
    runTicks(fixture.engine, 100U);
    const std::uint64_t beforeHold = fixture.engine.snapshot().masterPositionQ32;

    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(fixture.engine.snapshot().externalResetHeld);
    CHECK_EQ(fixture.engine.snapshot().masterBeatPhaseQ32, 0U);
    for (std::uint32_t tick = 0U; tick < 5000U; ++tick) {
        fixture.tick(1000U + tick * config::kSchedulerTickUs);
    }
    CHECK_EQ(fixture.engine.snapshot().masterPositionQ32, beforeHold);
    CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], LOW);

    fixture.inputs.injectResetEdgeForTest(300000U, false);
    fixture.sync.processSchedulerTick(300000U);
    CHECK(!fixture.engine.snapshot().externalResetHeld);
    runTicks(fixture.engine, 10U);
    CHECK(fixture.engine.snapshot().masterPositionQ32 > beforeHold);
}

void testGateModeHonorsAlreadyHighInputAndRuntimeModeChange() {
    Fixture fixture;
    fixture.state.externalSync.resetMode = ExternalResetMode::Gate;
    fixture.inputs.injectResetEdgeForTest(100U, true);
    fixture.begin();
    fixture.sync.processSchedulerTick(100U);
    CHECK(fixture.engine.snapshot().externalResetHeld);

    fixture.state.externalSync.resetMode = ExternalResetMode::Trigger;
    fixture.sync.updateConfiguration(fixture.state);
    fixture.sync.processSchedulerTick(150U);
    CHECK(!fixture.engine.snapshot().externalResetHeld);

    fixture.state.externalSync.resetMode = ExternalResetMode::Gate;
    fixture.sync.updateConfiguration(fixture.state);
    fixture.sync.processSchedulerTick(200U);
    CHECK(fixture.engine.snapshot().externalResetHeld);
}

void testResetWinsWhenSyncArrivesOnSameSchedulerBoundary() {
    Fixture fixture;
    fixture.state.externalSync.resetMode = ExternalResetMode::Gate;
    fixture.begin();
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.tick(1000U);
    const auto snapshot = fixture.engine.snapshot();
    CHECK(snapshot.externalLocked);
    CHECK(snapshot.externalResetHeld);
    CHECK_EQ(snapshot.masterBeatPhaseQ32, 0U);
}

void testPhysicalComparatorIrqPathWhenPinsAreAssigned() {
    if constexpr (pinmap::kExternalSyncSignalPin == pinmap::kUnassignedDigitalPin ||
                  pinmap::kExternalResetSignalPin == pinmap::kUnassignedDigitalPin) {
        CHECK(true);
        return;
    }

    Fixture fixture;
    fixture.state.externalSync.edge = SyncEdge::Rising;
    fixture.state.externalSync.glitchFilterUs = 1U;
    fixture.state.externalSync.resetMode = ExternalResetMode::Gate;
    fakefw::setPin(pinmap::kExternalSyncSignalPin, LOW);
    fakefw::setPin(pinmap::kExternalResetSignalPin, LOW);
    fixture.inputs.begin();
    fixture.begin();

    fakefw::nowUs = 1000U;
    fakefw::setPin(pinmap::kExternalSyncSignalPin, HIGH);
    fixture.sync.processSchedulerTick(fakefw::nowUs);
    CHECK(fixture.engine.snapshot().externalLocked);

    fakefw::nowUs = 501000U;
    fakefw::setPin(pinmap::kExternalSyncSignalPin, LOW);
    fakefw::setPin(pinmap::kExternalSyncSignalPin, HIGH);
    fixture.sync.processSchedulerTick(fakefw::nowUs);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    fakefw::nowUs = 600000U;
    fakefw::setPin(pinmap::kExternalResetSignalPin, HIGH);
    fixture.sync.processSchedulerTick(fakefw::nowUs);
    CHECK(fixture.engine.snapshot().externalResetHeld);
    fakefw::nowUs = 601000U;
    fakefw::setPin(pinmap::kExternalResetSignalPin, LOW);
    fixture.sync.processSchedulerTick(fakefw::nowUs);
    CHECK(!fixture.engine.snapshot().externalResetHeld);
}

void testInputQueuesPublishStableContinuityBoundaryOnOverflow() {
    hal::ExternalInputCapture inputs;
    for (std::uint32_t index = 0U; index < 24U; ++index) {
        inputs.injectSyncEdgeForTest(1000U + index, (index & 1U) != 0U);
    }
    CHECK(inputs.droppedSyncEdges() > 0U);
    hal::ExternalInputEdge edge{};
    std::uint32_t popped = 0U;
    hal::ExternalInputEdge last{};
    while (inputs.popSyncEdge(edge)) {
        last = edge;
        ++popped;
    }
    CHECK_EQ(popped, 16U);
    CHECK_EQ(last.timestampUs, 1015U);
    CHECK(last.high);
    CHECK(last.continuityLost);

    for (std::uint32_t index = 0U; index < 24U; ++index) {
        const bool high = index != 23U;
        inputs.injectResetEdgeForTest(2000U + index, high);
    }
    CHECK(inputs.collapsedResetEdges() > 0U);
    CHECK(!inputs.resetLevelHigh());
    popped = 0U;
    while (inputs.popResetEdge(edge)) {
        last = edge;
        ++popped;
    }
    CHECK_EQ(popped, 16U);
    CHECK_EQ(last.timestampUs, 2015U);
    CHECK(last.high);
    CHECK(last.continuityLost);
}

void testOverflowMarkerCannotCreateCrossGapTempo() {
    Fixture fixture;
    fixture.state.externalSync.edge = SyncEdge::Rising;
    fixture.state.externalSync.glitchFilterUs = 1U;
    fixture.begin();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(501000U, true);
    fixture.sync.processSchedulerTick(501000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    // Fill the CHANGE queue with unselected falling transitions. The first dropped
    // transition is also falling, so continuity loss must be honored before edge
    // polarity is filtered. Later transitions remain dropped until the marker is
    // consumed; they must never appear ahead of the loss boundary.
    for (std::uint32_t index = 0U; index < 24U; ++index) {
        fixture.inputs.injectSyncEdgeForTest(600000U + index, false);
    }
    CHECK(fixture.inputs.droppedSyncEdges() > 0U);
    fixture.sync.processSchedulerTick(700000U);

    // First real edge after loss only re-anchors. It cannot be combined with the
    // pre-overflow 501000-us pulse into a bogus slow tempo estimate.
    fixture.inputs.injectSyncEdgeForTest(5001000U, true);
    fixture.sync.processSchedulerTick(5001000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
    fixture.inputs.injectSyncEdgeForTest(5501000U, true);
    fixture.sync.processSchedulerTick(5501000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
}

void testTechnicalMaximumRejectsImpossibleSyncRate() {
    Fixture fixture;
    fixture.state.externalSync.pulsesPerQuarterNote = 24U;
    fixture.state.externalSync.glitchFilterUs = 0U;
    fixture.begin();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    // 100 us at 24 PPQN would imply 25,000 BPM. Ignore it as an impossible
    // acquisition sample; this also protects the Q32 master increment arithmetic.
    fixture.inputs.injectSyncEdgeForTest(1100U, true);
    fixture.sync.processSchedulerTick(1100U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    // The first physically supported interval is accepted and hard-capped at the
    // documented 999-BPM technical ceiling rather than overflowing downstream math.
    fixture.inputs.injectSyncEdgeForTest(1000U + 2502U, true);
    fixture.sync.processSchedulerTick(1000U + 2502U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 999000U);
}

void testUnchangedSyncConfigurationDoesNotMaskInterrupts() {
    Fixture fixture;
    fixture.begin();
    fakefw::noInterruptCalls = 0U;
    fakefw::interruptCalls = 0U;
    fixture.sync.updateConfiguration(fixture.state);
    CHECK_EQ(fakefw::noInterruptCalls, 0U);
    CHECK_EQ(fakefw::interruptCalls, 0U);

    fixture.state.externalSync.timeoutMs = static_cast<std::uint16_t>(
        fixture.state.externalSync.timeoutMs + 100U);
    fixture.sync.updateConfiguration(fixture.state);
    CHECK_EQ(fakefw::noInterruptCalls, 1U);
    CHECK_EQ(fakefw::interruptCalls, 1U);
}

void testExplicitExternalLockClearAndReacquire() {
    Fixture fixture;
    fixture.state.source = ClockSource::External;
    fixture.state.externalSync.pulsesPerQuarterNote = 1U;
    fixture.state.externalSync.glitchFilterUs = 1U;
    fixture.begin();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(501000U, true);
    fixture.sync.processSchedulerTick(501000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    fixture.sync.clearExternalLock();
    CHECK(!fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    // One fresh edge re-anchors phase without inventing a period from the cable gap.
    fixture.inputs.injectSyncEdgeForTest(5001000U, true);
    fixture.sync.processSchedulerTick(5001000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    // The following clean period resumes normal tempo acquisition.
    fixture.inputs.injectSyncEdgeForTest(5501000U, true);
    fixture.sync.processSchedulerTick(5501000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
}

void testSyncQueueOverflowDoesNotMasqueradeAsTempoDrop() {
    Fixture fixture;
    fixture.state.externalSync.pulsesPerQuarterNote = 1U;
    fixture.state.externalSync.glitchFilterUs = 1U;
    fixture.begin();

    std::uint32_t timestamp = 1000U;
    for (std::uint32_t pulse = 0U; pulse < 24U; ++pulse) {
        fixture.inputs.injectSyncEdgeForTest(timestamp, true);
        timestamp += 500000U;
    }
    CHECK(fixture.inputs.droppedSyncEdges() > 0U);
    fixture.sync.processSchedulerTick(timestamp);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    // The next clean edge reacquires the period without a transient false BPM.
    fixture.inputs.injectSyncEdgeForTest(12001000U, true);
    fixture.sync.processSchedulerTick(12001000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
}

void testExternalSyncConfigurationBoundaryPaths() {
    Fixture fixture;
    fixture.state.source = ClockSource::External;
    fixture.state.tempoRange.minimumBpm = 0U;
    fixture.state.externalSync.pulsesPerQuarterNote = 0U;
    fixture.state.externalSync.glitchFilterUs = 0U;
    fixture.begin();

    // Apply the begin-time dirty state, then change one field at a time so every
    // settingsEqual short-circuit and the no-op/configuration hot path are covered.
    fixture.sync.processSchedulerTick(0U);
    ClockState updated = fixture.state;
    updated.externalSync.edge = SyncEdge::Falling;
    fixture.sync.updateConfiguration(updated);
    fixture.sync.processSchedulerTick(1U);
    updated.externalSync.lossMode = SyncLossMode::Freewheel;
    fixture.sync.updateConfiguration(updated);
    fixture.sync.processSchedulerTick(2U);
    updated.externalSync.glitchFilterUs = 250U;
    fixture.sync.updateConfiguration(updated);
    fixture.sync.processSchedulerTick(3U);
    updated.bpm = 121U;
    fixture.sync.updateConfiguration(updated);
    fixture.sync.processSchedulerTick(4U);
    updated.tempoRange.minimumBpm = 20U;
    fixture.sync.updateConfiguration(updated);
    fixture.sync.processSchedulerTick(5U);

    // Return to rising/zero-PPQN/zero-minimum settings and exercise defensive
    // period paths that validation normally prevents from user configuration.
    updated.externalSync.edge = SyncEdge::Rising;
    updated.externalSync.lossMode = SyncLossMode::Stop;
    updated.externalSync.glitchFilterUs = 0U;
    updated.tempoRange.minimumBpm = 0U;
    fixture.sync.updateConfiguration(updated);
    fixture.sync.processSchedulerTick(6U);

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(fixture.engine.snapshot().externalLocked);

    // Duplicate timestamp: period == 0 must be ignored without corrupting lock.
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(fixture.engine.snapshot().externalLocked);

    // A gap beyond the supported 1-BPM technical floor becomes a fresh
    // acquisition boundary rather than an ultra-slow tempo sample.
    fixture.inputs.injectSyncEdgeForTest(60002000U, true);
    fixture.sync.processSchedulerTick(60002000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 121000U);
}

void testEngineExternalSyncDefensiveConfigurationPaths() {
    {
        Fixture fixture;
        fixture.state.source = ClockSource::Internal;
        fixture.begin();
        fixture.engine.acceptExternalPulseFromIsr(123000U, 0U);
        CHECK(fixture.engine.snapshot().externalLocked);
    }

    {
        Fixture fixture;
        fixture.state.source = ClockSource::External;
        fixture.state.masterMeter.beats = 0U;
        fixture.state.masterMeter.unit = 0U;
        fixture.state.channels[0].common.resetMode = ResetMode::Free;
        fixture.begin();

        // First pulse initializes the external epoch. A zero PPQN is defensively
        // normalized to one; the second pulse also exercises zero meter fallbacks.
        fixture.engine.acceptExternalPulseFromIsr(120000U, 0U);
        fixture.engine.acceptExternalPulseFromIsr(120000U, 0U);
        const auto snapshot = fixture.engine.snapshot();
        CHECK(snapshot.externalLocked);
        CHECK(snapshot.masterBeat >= 1U);
        CHECK(snapshot.masterBar >= 1U);
    }
}

}  // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(testMinuteLongMasterTimingHasNoDrift);
    RUN_TEST(testConfiguredGateLengthsReachPhysicalGpio);
    RUN_TEST(testRepresentativeExternalTemposAndPpqn);
    RUN_TEST(testFallingEdgeSelection);
    RUN_TEST(testGlitchesDoNotCorruptPeriodEstimator);
    RUN_TEST(testDeterministicJitterRemainsBounded);
    RUN_TEST(testSyncTimeoutBoundaryAndLossModes);
    RUN_TEST(testSlowExternalClockDoesNotTimeoutBeforeSecondPulse);
    RUN_TEST(testTimestampWraparound);
    RUN_TEST(testResetTriggerIsEdgeTriggered);
    RUN_TEST(testResetBurstIsCoalescedPerSchedulerQuantum);
    RUN_TEST(testResetOverflowStillCoalescesToOneReset);
    RUN_TEST(testResetGateHoldsAndReleasesScheduler);
    RUN_TEST(testGateModeHonorsAlreadyHighInputAndRuntimeModeChange);
    RUN_TEST(testResetWinsWhenSyncArrivesOnSameSchedulerBoundary);
    RUN_TEST(testPhysicalComparatorIrqPathWhenPinsAreAssigned);
    RUN_TEST(testInputQueuesPublishStableContinuityBoundaryOnOverflow);
    RUN_TEST(testOverflowMarkerCannotCreateCrossGapTempo);
    RUN_TEST(testTechnicalMaximumRejectsImpossibleSyncRate);
    RUN_TEST(testUnchangedSyncConfigurationDoesNotMaskInterrupts);
    RUN_TEST(testExplicitExternalLockClearAndReacquire);
    RUN_TEST(testSyncQueueOverflowDoesNotMasqueradeAsTempoDrop);
    RUN_TEST(testExternalSyncConfigurationBoundaryPaths);
    RUN_TEST(testEngineExternalSyncDefensiveConfigurationPaths);
    std::cout << "Realtime assertions: " << checks << "\n";
    return UNITY_END();
}
