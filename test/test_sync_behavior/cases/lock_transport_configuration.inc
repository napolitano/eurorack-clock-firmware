/**
 * @file lock_transport_configuration.inc
 * @brief Included SYNC behavior cases split for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

void testFirstPulseIsAcquisitionOnlyAndDoesNotAdvertiseLock() {
    Fixture fixture;
    fixture.state.source = ClockSource::Auto;
    fixture.begin();
    fixture.engine.stop();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);

    const auto snapshot = fixture.engine.snapshot();
    CHECK(!snapshot.externalLocked);
    CHECK_EQ(snapshot.externalBpmMilli, 0U);
    CHECK(!snapshot.playing);
}

void testSecondValidPulseAcquiresLockAndAutoStartsTransport() {
    Fixture fixture;
    fixture.state.source = ClockSource::Auto;
    fixture.begin();
    fixture.engine.stop();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(501000U, true);
    fixture.sync.processSchedulerTick(501000U);

    const auto snapshot = fixture.engine.snapshot();
    CHECK(snapshot.externalLocked);
    CHECK_NEAR(snapshot.externalBpmMilli, 120000U, 1U);
    CHECK(snapshot.playing);
    TransportState transition = TransportState::Stopped;
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Playing);
}

void testManualStopPreventsExternalReacquisitionFromStartingTransport() {
    Fixture fixture;
    fixture.state.source = ClockSource::Auto;
    fixture.begin();
    fixture.engine.stop();
    fixture.sync.notifyManualTransportState(TransportState::Stopped);

    acquireRising(fixture, 500000U);

    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK(!fixture.engine.snapshot().playing);
    TransportState transition = TransportState::Playing;
    CHECK(!fixture.sync.consumeTransportTransition(transition));

    // INTERNAL may observe and report a valid external clock, but acquisition must
    // never take ownership of transport while the user explicitly selected INTERNAL.
    Fixture internal;
    internal.state.source = ClockSource::Internal;
    internal.begin();
    internal.engine.stop();
    acquireRising(internal, 500000U);
    CHECK(internal.engine.snapshot().externalLocked);
    CHECK(!internal.engine.snapshot().playing);
    CHECK(!internal.sync.consumeTransportTransition(transition));
}

void testStopLossStopsAndReacquisitionRestartsWhenNotManuallyStopped() {
    Fixture fixture;
    fixture.state.source = ClockSource::Auto;
    fixture.state.externalSync.lossMode = SyncLossMode::Stop;
    fixture.state.externalSync.timeoutMs = 200U;
    fixture.begin();
    fixture.engine.stop();

    acquireRising(fixture, 500000U);
    CHECK(fixture.engine.snapshot().playing);
    TransportState transition = TransportState::Stopped;
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Playing);

    fixture.sync.processSchedulerTick(1501000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
    CHECK(!fixture.engine.snapshot().playing);
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Stopped);

    fixture.inputs.injectSyncEdgeForTest(2000000U, true);
    fixture.sync.processSchedulerTick(2000000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
    fixture.inputs.injectSyncEdgeForTest(2500000U, true);
    fixture.sync.processSchedulerTick(2500000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK(fixture.engine.snapshot().playing);
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Playing);

    // A manual PAUSE disarms automatic transport ownership. Sync loss still clears
    // the lock, but it must not enqueue a synthetic STOP or later auto-restart.
    Fixture paused;
    paused.state.source = ClockSource::Auto;
    paused.state.externalSync.lossMode = SyncLossMode::Stop;
    paused.state.externalSync.timeoutMs = 200U;
    paused.begin();
    acquireRising(paused, 500000U);
    CHECK(paused.sync.consumeTransportTransition(transition));
    paused.engine.pause();
    paused.sync.notifyManualTransportState(TransportState::Paused);
    paused.sync.processSchedulerTick(1501000U);
    CHECK(!paused.engine.snapshot().externalLocked);
    CHECK(!paused.engine.snapshot().playing);
    CHECK(!paused.sync.consumeTransportTransition(transition));

    // STOP loss policy is irrelevant while INTERNAL owns the clock source.
    Fixture internal;
    internal.state.source = ClockSource::Internal;
    internal.state.externalSync.lossMode = SyncLossMode::Stop;
    internal.state.externalSync.timeoutMs = 200U;
    internal.begin();
    acquireRising(internal, 500000U);
    CHECK(internal.engine.snapshot().externalLocked);
    internal.sync.processSchedulerTick(1501000U);
    CHECK(!internal.engine.snapshot().externalLocked);
    CHECK(internal.engine.snapshot().playing);
    CHECK(!internal.sync.consumeTransportTransition(transition));
}

void testAutoFreewheelKeepsLastMeasuredExternalTempoAfterLoss() {
    Fixture fixture;
    fixture.state.source = ClockSource::Auto;
    fixture.state.bpm = 60U;
    fixture.state.externalSync.lossMode = SyncLossMode::Freewheel;
    fixture.state.externalSync.timeoutMs = 200U;
    fixture.begin();
    acquireRising(fixture, 500000U);  // 120 BPM
    fixture.sync.processSchedulerTick(1501000U);
    CHECK(!fixture.engine.snapshot().externalLocked);

    const auto before = fixture.engine.snapshot().masterPositionQ32;
    for (std::uint32_t i = 0U; i < 20000U; ++i) {
        fixture.engine.processSchedulerTick();
    }
    const auto after = fixture.engine.snapshot().masterPositionQ32;
    const std::uint64_t delta = after - before;
    CHECK(delta > 0U);
    // 20,000 ticks at 20 kHz = one second. 120 BPM advances about two quarter notes,
    // while the 60-BPM internal fallback would advance only one.
    CHECK(delta > (static_cast<std::uint64_t>(1U) << 32U) + (static_cast<std::uint64_t>(1U) << 31U));
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


void testFactoryDefaultAutoAcquiresExternalSync() {
    ClockState state{};
    initializeFactoryDefaults(state);
    CHECK_EQ(state.source, ClockSource::Auto);

    hal::GateOutputDriver gates;
    engine::ClockEngine engine(gates);
    hal::ExternalInputCapture inputs;
    services::ExternalSyncController sync(inputs, engine);
    fakefw::resetArduino();
    gates.beginDisabled();
    gates.enableOutputStage();
    engine.begin(state);
    sync.begin(state);
    engine.play();

    inputs.injectSyncEdgeForTest(1000U, true);
    sync.processSchedulerTick(1000U);
    inputs.injectSyncEdgeForTest(2000U, false);
    sync.processSchedulerTick(2000U);
    inputs.injectSyncEdgeForTest(501000U, true);
    sync.processSchedulerTick(501000U);

    CHECK(engine.snapshot().externalLocked);
    CHECK_NEAR(engine.snapshot().externalBpmMilli, 120000U, 1U);
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
    acquireRising(fixture, 500000U);
    CHECK(fixture.engine.snapshot().externalLocked);

    // A >60 s period is below the supported 1 BPM floor. If it arrives after a
    // valid lock it must drop that lock and become the first pulse of a new epoch.
    fixture.inputs.injectSyncEdgeForTest(60502000U, true);
    fixture.sync.processSchedulerTick(60502000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    fixture.inputs.injectSyncEdgeForTest(61002000U, true);
    fixture.sync.processSchedulerTick(61002000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), 120000U, 1U);
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
    CHECK(!fixture.engine.snapshot().externalLocked);
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
    CHECK(!fixture.engine.snapshot().externalLocked);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(2500000U, true);
    fixture.sync.processSchedulerTick(2500000U);
    CHECK(fixture.sync.filteredBpmMilli() >= 119000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 121000U);
    CHECK(before <= 999000U);

    // Exercise the same continuity boundary during first-pulse acquisition, before
    // any external lock exists. The overflow marker must discard that partial epoch.
    Fixture acquiring;
    acquiring.begin();
    acquiring.inputs.injectSyncEdgeForTest(1000U, true);
    acquiring.sync.processSchedulerTick(1000U);
    for (std::uint32_t i = 0U; i < 40U; ++i) {
        acquiring.inputs.injectSyncEdgeForTest(2000U + i * 1000U, (i & 1U) == 0U);
    }
    CHECK(acquiring.inputs.droppedSyncEdges() > 0U);
    acquiring.sync.processSchedulerTick(50000U);
    CHECK(!acquiring.engine.snapshot().externalLocked);
    acquiring.inputs.injectSyncEdgeForTest(1000000U, true);
    acquiring.sync.processSchedulerTick(1000000U);
    CHECK(!acquiring.engine.snapshot().externalLocked);
    acquiring.inputs.injectSyncEdgeForTest(1500000U, true);
    acquiring.sync.processSchedulerTick(1500000U);
    CHECK(acquiring.engine.snapshot().externalLocked);
    CHECK_NEAR(acquiring.sync.filteredBpmMilli(), 120000U, 1U);
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
    CHECK(!fixture.engine.snapshot().externalLocked);
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

    // An identical foreground state is an explicit no-op and must not disturb lock.
    fixture.sync.updateConfiguration(fixture.state);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);

    ClockState updated = fixture.state;
    updated.externalSync.timeoutMs = 3000U;
    fixture.sync.updateConfiguration(updated);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}


void testFactoryDefaultSyncSmoothingIsLow() {
    ClockState state{};
    initializeFactoryDefaults(state);
    CHECK_EQ(state.externalSync.smoothing, SyncSmoothing::Low);
}

void testSyncSmoothingOffFollowsNextValidPeriodExactly() {
    Fixture fixture;
    fixture.state.externalSync.smoothing = SyncSmoothing::Off;
    fixture.begin();
    acquireRising(fixture, 500000U);
    injectRisingPulse(fixture, 834333U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 180000U);
}

void testSyncSmoothingLowUsesSeventyFivePercentNewPeriod() {
    Fixture fixture;
    fixture.state.externalSync.smoothing = SyncSmoothing::Low;
    fixture.begin();
    acquireRising(fixture, 500000U);
    injectRisingPulse(fixture, 834333U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 160000U);
}

void testSyncSmoothingMediumUsesHalfNewPeriod() {
    Fixture fixture;
    fixture.state.externalSync.smoothing = SyncSmoothing::Medium;
    fixture.begin();
    acquireRising(fixture, 500000U);
    injectRisingPulse(fixture, 834333U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 143999U);
}

void testSyncSmoothingFullPreservesLegacyTwentyFivePercentNewPeriod() {
    Fixture fixture;
    fixture.state.externalSync.smoothing = SyncSmoothing::Full;
    fixture.begin();
    acquireRising(fixture, 500000U);
    injectRisingPulse(fixture, 834333U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 130909U);
}

void testRuntimeSmoothingChangePreservesLockAndAppliesOnNextPeriod() {
    Fixture fixture;
    fixture.state.externalSync.smoothing = SyncSmoothing::Full;
    fixture.begin();
    acquireRising(fixture, 500000U);

    ClockState updated = fixture.state;
    updated.externalSync.smoothing = SyncSmoothing::Off;
    fixture.sync.updateConfiguration(updated);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    injectRisingPulse(fixture, 834333U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 180000U);
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
