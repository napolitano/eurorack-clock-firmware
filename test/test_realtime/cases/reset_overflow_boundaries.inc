/**
 * @file reset_overflow_boundaries.inc
 * @brief Included test cases split from test_realtime/test_main.cpp for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

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
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(501000U, true);
    fixture.inputs.injectResetEdgeForTest(501000U, true);
    fixture.tick(501000U);
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
    CHECK(!fixture.engine.snapshot().externalLocked);

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
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 0U);

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
    CHECK(!fixture.engine.snapshot().externalLocked);
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
    CHECK(!fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);

    // Two clean selected edges are required after continuity loss.
    fixture.inputs.injectSyncEdgeForTest(12001000U, true);
    fixture.sync.processSchedulerTick(12001000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
    fixture.inputs.injectSyncEdgeForTest(12501000U, true);
    fixture.sync.processSchedulerTick(12501000U);
    CHECK(fixture.engine.snapshot().externalLocked);
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
    CHECK(!fixture.engine.snapshot().externalLocked);

    // Duplicate timestamp: period == 0 must be ignored without manufacturing lock.
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(!fixture.engine.snapshot().externalLocked);

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
