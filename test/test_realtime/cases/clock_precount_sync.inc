/**
 * @file clock_precount_sync.inc
 * @brief Included test cases split from test_realtime/test_main.cpp for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

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

void testInternalPreCountSuppressesGatesAndStartsFromPhaseZero() {
    Fixture fixture;
    fixture.state.source = ClockSource::Internal;
    fixture.state.bpm = 120U;
    fixture.state.preCountSteps = 2U;
    fixture.begin();

    auto snapshot = fixture.engine.snapshot();
    CHECK(snapshot.preCountActive);
    CHECK_EQ(snapshot.preCountRemaining, 2U);
    CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], LOW);

    constexpr std::uint32_t kTicksPerBeatAt120 = config::kSchedulerFrequencyHz / 2U;
    runTicks(fixture.engine, kTicksPerBeatAt120);
    snapshot = fixture.engine.snapshot();
    CHECK(snapshot.preCountActive);
    CHECK_EQ(snapshot.preCountRemaining, 1U);
    CHECK_EQ(snapshot.masterPositionQ32, 0U);
    CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], LOW);

    runTicks(fixture.engine, kTicksPerBeatAt120);
    snapshot = fixture.engine.snapshot();
    CHECK(!snapshot.preCountActive);
    CHECK_EQ(snapshot.preCountRemaining, 0U);
    CHECK_EQ(snapshot.masterPositionQ32, 0U);
    CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], LOW);

    fixture.engine.processSchedulerTick();
    CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], HIGH);
}

void testPreCountPauseResumeContinuesInsteadOfRestarting() {
    Fixture fixture;
    fixture.state.source = ClockSource::Internal;
    fixture.state.bpm = 120U;
    fixture.state.preCountSteps = 2U;
    fixture.begin();

    constexpr std::uint32_t kTicksPerBeatAt120 = config::kSchedulerFrequencyHz / 2U;
    runTicks(fixture.engine, kTicksPerBeatAt120);
    CHECK_EQ(fixture.engine.snapshot().preCountRemaining, 1U);
    fixture.engine.pause();
    runTicks(fixture.engine, kTicksPerBeatAt120 * 2U);
    CHECK_EQ(fixture.engine.snapshot().preCountRemaining, 1U);
    fixture.engine.play();
    runTicks(fixture.engine, kTicksPerBeatAt120);
    CHECK(!fixture.engine.snapshot().preCountActive);
}

void testPreCountStopRestartsFullCountOnNextPlay() {
    Fixture fixture;
    fixture.state.source = ClockSource::Internal;
    fixture.state.bpm = 120U;
    fixture.state.preCountSteps = 3U;
    fixture.begin();

    constexpr std::uint32_t kTicksPerBeatAt120 = config::kSchedulerFrequencyHz / 2U;
    runTicks(fixture.engine, kTicksPerBeatAt120);
    CHECK_EQ(fixture.engine.snapshot().preCountRemaining, 2U);
    fixture.engine.stop();
    CHECK(!fixture.engine.snapshot().preCountActive);
    fixture.engine.play();
    CHECK(fixture.engine.snapshot().preCountActive);
    CHECK_EQ(fixture.engine.snapshot().preCountRemaining, 3U);
}

void testExternalPreCountFollowsMasterMeterBeatUnitAcrossPpqn() {
    struct Case final {
        std::uint8_t beatUnit;
        std::uint8_t ppqn;
        std::uint8_t pulsesPerMasterBeat;
    };

    // External Pre-Count counts the configured master-meter beat, not a hard-coded
    // quarter note. These cases cover half-, quarter- and eighth-note master beats
    // plus the high-resolution 24-PPQN path.
    const std::array<Case, 4U> cases{{
        {2U, 4U, 8U},
        {4U, 4U, 4U},
        {8U, 4U, 2U},
        {8U, 24U, 12U},
    }};

    for (const Case item : cases) {
        Fixture fixture;
        fixture.state.source = ClockSource::External;
        fixture.state.externalSync.lossMode = SyncLossMode::Freewheel;
        fixture.state.masterMeter = {4U, item.beatUnit};
        fixture.state.preCountSteps = 2U;
        fixture.begin();

        for (std::uint8_t pulse = 1U; pulse < item.pulsesPerMasterBeat; ++pulse) {
            fixture.engine.acceptExternalPulse(120000U, item.ppqn);
        }
        CHECK(fixture.engine.snapshot().preCountActive);
        CHECK_EQ(fixture.engine.snapshot().preCountRemaining, 2U);

        // Once external pulses own the count, scheduler interpolation must not also
        // advance it. Otherwise sparse PPQN sources would double-count progress.
        runTicks(fixture.engine, config::kSchedulerFrequencyHz);
        CHECK_EQ(fixture.engine.snapshot().preCountRemaining, 2U);

        fixture.engine.acceptExternalPulse(120000U, item.ppqn);
        CHECK(fixture.engine.snapshot().preCountActive);
        CHECK_EQ(fixture.engine.snapshot().preCountRemaining, 1U);
        CHECK_EQ(fixture.engine.snapshot().masterPositionQ32, 0U);
        CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], LOW);

        for (std::uint8_t pulse = 0U; pulse < item.pulsesPerMasterBeat; ++pulse) {
            fixture.engine.acceptExternalPulse(120000U, item.ppqn);
        }
        CHECK(!fixture.engine.snapshot().preCountActive);
        CHECK_EQ(fixture.engine.snapshot().preCountRemaining, 0U);
        CHECK_EQ(fixture.engine.snapshot().masterPositionQ32, 0U);
        CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], LOW);
        fixture.engine.processSchedulerTick();
        CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], HIGH);
    }

    // A sparse 1-PPQN source spans multiple master beats when the meter unit is
    // smaller than a quarter note. One quarter-note edge must therefore be able to
    // consume more than one Pre-Count beat without losing phase.
    Fixture sixteenthFixture;
    sixteenthFixture.state.source = ClockSource::External;
    sixteenthFixture.state.externalSync.lossMode = SyncLossMode::Freewheel;
    sixteenthFixture.state.masterMeter = {4U, 16U};
    sixteenthFixture.state.preCountSteps = 5U;
    sixteenthFixture.begin();

    sixteenthFixture.engine.acceptExternalPulse(120000U, 1U);
    CHECK(sixteenthFixture.engine.snapshot().preCountActive);
    CHECK_EQ(sixteenthFixture.engine.snapshot().preCountRemaining, 1U);
    CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[0]], LOW);

    sixteenthFixture.engine.acceptExternalPulse(120000U, 1U);
    CHECK(!sixteenthFixture.engine.snapshot().preCountActive);
    CHECK_EQ(sixteenthFixture.engine.snapshot().preCountRemaining, 0U);
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
    CHECK(!fixture.engine.snapshot().externalLocked);
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
        CHECK(!fixture.engine.snapshot().externalLocked);
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
        CHECK(!fixture.engine.snapshot().externalLocked);
        fixture.sync.processSchedulerTick(1000U + periodUs - 1U);
        CHECK(!fixture.engine.snapshot().externalLocked);
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
