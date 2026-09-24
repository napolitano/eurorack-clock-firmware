/**
 * @file input_roles.inc
 * @brief Included SYNC behavior cases split for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

void testHeldHighSyncCreatesOnlyOneSelectedEdge() {
    Fixture fixture;
    fixture.begin();
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    const std::uint32_t initial = fixture.sync.filteredBpmMilli();
    for (std::uint32_t now = 1050U; now < 500000U; now += 50000U) {
        fixture.sync.processSchedulerTick(now);
    }
    CHECK_EQ(fixture.sync.filteredBpmMilli(), initial);
}

void testHeldHighSyncAfterLockEventuallyTimesOut() {
    Fixture fixture;
    fixture.state.externalSync.timeoutMs = 200U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    fixture.inputs.injectSyncEdgeForTest(1001000U, true);
    fixture.sync.processSchedulerTick(1001000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    fixture.sync.processSchedulerTick(2000999U);
    CHECK(fixture.engine.snapshot().externalLocked);
    fixture.sync.processSchedulerTick(2001000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
}

void testHeldHighSyncCannotManufactureFallingEdgeClock() {
    Fixture fixture;
    fixture.state.externalSync.edge = SyncEdge::Falling;
    fixture.begin();
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    for (std::uint32_t now = 1050U; now < 500000U; now += 50000U) {
        fixture.sync.processSchedulerTick(now);
    }
    CHECK(!fixture.engine.snapshot().externalLocked);
}


void testSyncRoleCanUsePhysicalInput2() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Off, InputFunction::Sync};
    fixture.begin();
    fixture.engine.stop();

    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectResetEdgeForTest(2000U, false);
    fixture.sync.processSchedulerTick(2000U);
    fixture.inputs.injectResetEdgeForTest(501000U, true);
    fixture.sync.processSchedulerTick(501000U);

    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), 120000U, 1U);
}

void testResetRoleCanUsePhysicalInput1() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Reset, InputFunction::Off};
    fixture.state.externalSync.resetMode = ExternalResetMode::Trigger;
    fixture.begin();
    const std::uint32_t before = fixture.engine.resetRuntimeCountForTest();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK_EQ(fixture.engine.resetRuntimeCountForTest(), before + 1U);
}

void testRunRoleTracksPhysicalLevel() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Run, InputFunction::Off};
    fixture.begin();
    fixture.engine.stop();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(fixture.engine.snapshot().playing);
    TransportState transition = TransportState::Stopped;
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Playing);

    fixture.inputs.injectSyncEdgeForTest(2000U, false);
    fixture.sync.processSchedulerTick(2000U);
    CHECK(!fixture.engine.snapshot().playing);
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Stopped);
}

void testStartAndStopRolesGenerateTransportCommands() {
    Fixture start;
    start.state.inputs = {InputFunction::Start, InputFunction::Off};
    start.begin();
    start.engine.stop();
    start.inputs.injectSyncEdgeForTest(1000U, true);
    start.sync.processSchedulerTick(1000U);
    CHECK(start.engine.snapshot().playing);

    Fixture stop;
    stop.state.inputs = {InputFunction::Stop, InputFunction::Off};
    stop.begin();
    CHECK(stop.engine.snapshot().playing);
    stop.inputs.injectSyncEdgeForTest(1000U, true);
    stop.sync.processSchedulerTick(1000U);
    CHECK(!stop.engine.snapshot().playing);
}

void testRestartRoleResetsPhaseAndStartsTransport() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Restart, InputFunction::Off};
    fixture.begin();
    for (std::uint32_t i = 0U; i < 100U; ++i) {
        fixture.engine.processSchedulerTick();
    }
    const std::uint32_t before = fixture.engine.resetRuntimeCountForTest();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(fixture.engine.snapshot().playing);
    CHECK(fixture.engine.resetRuntimeCountForTest() > before);
}

void testTapRolePreservesCaptureTimestamp() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Tap, InputFunction::Off};
    fixture.begin();

    fixture.inputs.injectSyncEdgeForTest(123456U, true);
    fixture.sync.processSchedulerTick(123456U);
    std::uint32_t timestampUs = 0U;
    CHECK(fixture.sync.consumeTapRequest(timestampUs));
    CHECK_EQ(timestampUs, 123456U);
    CHECK(!fixture.sync.consumeTapRequest(timestampUs));
}

void testOffRoleIgnoresPhysicalEdges() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Off, InputFunction::Off};
    fixture.begin();
    fixture.engine.stop();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(!fixture.engine.snapshot().playing);
    CHECK(!fixture.engine.snapshot().externalLocked);
    std::uint32_t timestampUs = 0U;
    CHECK(!fixture.sync.consumeTapRequest(timestampUs));
}

void testRoleChangeDiscardsQueuedEdgesFromPreviousMeaning() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Sync, InputFunction::Off};
    fixture.begin();
    fixture.engine.stop();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.state.inputs = {InputFunction::Start, InputFunction::Off};
    fixture.sync.updateConfiguration(fixture.state);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(!fixture.engine.snapshot().playing);

    fixture.inputs.injectSyncEdgeForTest(2000U, false);
    fixture.sync.processSchedulerTick(2000U);
    fixture.inputs.injectSyncEdgeForTest(3000U, true);
    fixture.sync.processSchedulerTick(3000U);
    CHECK(fixture.engine.snapshot().playing);
}

void testFirstEdgeAfterBeginIsNotDiscardedForAssignedRole() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Start, InputFunction::Off};
    fixture.begin();
    fixture.engine.stop();

    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(fixture.engine.snapshot().playing);
}

void testRunRoleCanUsePhysicalInput2AndReassertAfterManualTransport() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Off, InputFunction::Run};
    fixture.begin();
    fixture.engine.stop();

    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(fixture.engine.snapshot().playing);
    fixture.sync.processSchedulerTick(1100U);
    CHECK(fixture.engine.snapshot().playing);

    fixture.engine.stop();
    fixture.sync.notifyManualTransportState(TransportState::Stopped);
    fixture.sync.processSchedulerTick(1200U);
    CHECK(fixture.engine.snapshot().playing);
}


void testTransportRolesCanUsePhysicalInput2() {
    struct Case { InputFunction function; bool startsPlaying; bool stopsPlaying; bool producesTap; };
    constexpr Case cases[] = {
        {InputFunction::Start, true, false, false},
        {InputFunction::Stop, false, true, false},
        {InputFunction::Restart, true, false, false},
        {InputFunction::Tap, false, false, true},
    };
    for (const auto& entry : cases) {
        Fixture fixture;
        fixture.state.inputs = {InputFunction::Off, entry.function};
        fixture.begin();
        if (entry.function == InputFunction::Start || entry.function == InputFunction::Tap) {
            fixture.engine.stop();
        }
        const std::uint32_t beforeReset = fixture.engine.resetRuntimeCountForTest();
        fixture.inputs.injectResetEdgeForTest(7777U, true);
        fixture.sync.processSchedulerTick(7777U);
        if (entry.startsPlaying) {
            CHECK(fixture.engine.snapshot().playing);
        }
        if (entry.stopsPlaying) {
            CHECK(!fixture.engine.snapshot().playing);
        }
        if (entry.function == InputFunction::Restart) {
            CHECK(fixture.engine.resetRuntimeCountForTest() > beforeReset);
        }
        std::uint32_t timestampUs = 0U;
        if (entry.producesTap) {
            CHECK(fixture.sync.consumeTapRequest(timestampUs));
            CHECK_EQ(timestampUs, 7777U);
        } else {
            CHECK(!fixture.sync.consumeTapRequest(timestampUs));
        }
    }
}

void testTransportRolesIgnoreFallingAndContinuityMarkers() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Start, InputFunction::Off};
    fixture.begin();
    fixture.engine.stop();
    fixture.inputs.injectSyncEdgeForTest(1000U, false);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(!fixture.engine.snapshot().playing);

    // Overflow/continuity marker without a positive edge must not synthesize START.
    for (std::uint32_t i = 0U; i < 64U; ++i) {
        fixture.inputs.injectSyncEdgeForTest(2000U + i, false);
    }
    fixture.sync.processSchedulerTick(3000U);
    CHECK(!fixture.engine.snapshot().playing);
}

void testResetGateRoleTracksBothLevelsOnPhysicalInput2() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Off, InputFunction::Reset};
    fixture.state.externalSync.resetMode = ExternalResetMode::Gate;
    fixture.begin();
    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(fixture.engine.snapshot().externalResetHeld);
    fixture.inputs.injectResetEdgeForTest(2000U, false);
    fixture.sync.processSchedulerTick(2000U);
    CHECK(!fixture.engine.snapshot().externalResetHeld);
}

void testResetRoleRemovalReleasesAppliedGate() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Off, InputFunction::Reset};
    fixture.state.externalSync.resetMode = ExternalResetMode::Gate;
    fixture.begin();
    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(fixture.engine.snapshot().externalResetHeld);

    fixture.state.inputs = {InputFunction::Off, InputFunction::Off};
    fixture.sync.updateConfiguration(fixture.state);
    fixture.sync.processSchedulerTick(2000U);
    CHECK(!fixture.engine.snapshot().externalResetHeld);
}

void testRunRoleNoAssignmentAndStableLevelAreNoOps() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Off, InputFunction::Off};
    fixture.begin();
    fixture.sync.processSchedulerTick(1000U);
    fixture.sync.processSchedulerTick(1100U);
    CHECK(fixture.engine.snapshot().playing);

    Fixture run;
    run.state.inputs = {InputFunction::Run, InputFunction::Off};
    run.begin();
    run.inputs.injectSyncEdgeForTest(2000U, true);
    run.sync.processSchedulerTick(2000U);
    CHECK(run.engine.snapshot().playing);
    run.sync.processSchedulerTick(2100U);
    CHECK(run.engine.snapshot().playing);
}

void testFillRoleIsDrainedButDoesNothing() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Fill, InputFunction::Off};
    fixture.begin();
    fixture.engine.stop();
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(!fixture.engine.snapshot().playing);
    CHECK(!fixture.engine.snapshot().externalLocked);
}


void testFillRoleOnPhysicalInput2IsDrainedButDoesNothing() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Off, InputFunction::Fill};
    fixture.begin();
    fixture.engine.stop();
    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(!fixture.engine.snapshot().playing);
    CHECK(!fixture.engine.snapshot().externalLocked);
}

void testInput2OnlyRoleChangeInvalidatesQueuedMeaning() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Off, InputFunction::Sync};
    fixture.begin();
    fixture.engine.stop();
    fixture.inputs.injectResetEdgeForTest(1000U, true);
    fixture.state.inputs = {InputFunction::Off, InputFunction::Start};
    fixture.sync.updateConfiguration(fixture.state);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(!fixture.engine.snapshot().playing);
    fixture.inputs.injectResetEdgeForTest(2000U, false);
    fixture.sync.processSchedulerTick(2000U);
    fixture.inputs.injectResetEdgeForTest(3000U, true);
    fixture.sync.processSchedulerTick(3000U);
    CHECK(fixture.engine.snapshot().playing);
}

void testSelectingRunIsTransportNeutralUntilPhysicalLevelChanges() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Off, InputFunction::Reset};
    fixture.begin();
    fixture.engine.stop();

    // Model the comparator already being HIGH while INPUT 1 is still OFF.
    // Selecting RUN in Settings must not reinterpret that standing level as PLAY.
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    TransportState transition = TransportState::Playing;
    CHECK(!fixture.sync.consumeTransportTransition(transition));

    fixture.state.inputs = {InputFunction::Run, InputFunction::Reset};
    fixture.sync.updateConfiguration(fixture.state);
    fixture.sync.processSchedulerTick(1100U);
    CHECK(!fixture.engine.snapshot().playing);
    CHECK(!fixture.sync.consumeTransportTransition(transition));

    // RUN becomes authoritative only after a real post-assignment level change.
    fixture.inputs.injectSyncEdgeForTest(2000U, false);
    fixture.sync.processSchedulerTick(2000U);
    CHECK(!fixture.engine.snapshot().playing);
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Stopped);

    fixture.inputs.injectSyncEdgeForTest(3000U, true);
    fixture.sync.processSchedulerTick(3000U);
    CHECK(fixture.engine.snapshot().playing);
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Playing);
}

void testPersistedRunDoesNotBreakBootStopOnStandingHighLevel() {
    Fixture fixture;
    fixture.state.inputs = {InputFunction::Run, InputFunction::Off};

    // The physical capture latch can already be HIGH before the controller is
    // initialized. Boot must remain STOP until a later physical RUN transition.
    fixture.inputs.injectSyncEdgeForTest(500U, true);
    fixture.engine.begin(fixture.state);
    fixture.engine.stop();
    fixture.sync.begin(fixture.state);
    fixture.sync.processSchedulerTick(600U);

    CHECK(!fixture.engine.snapshot().playing);
    TransportState transition = TransportState::Playing;
    CHECK(!fixture.sync.consumeTransportTransition(transition));

    fixture.inputs.injectSyncEdgeForTest(1000U, false);
    fixture.sync.processSchedulerTick(1000U);
    CHECK(!fixture.engine.snapshot().playing);
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Stopped);

    fixture.inputs.injectSyncEdgeForTest(1500U, true);
    fixture.sync.processSchedulerTick(1500U);
    CHECK(fixture.engine.snapshot().playing);
    CHECK(fixture.sync.consumeTransportTransition(transition));
    CHECK_EQ(transition, TransportState::Playing);
}

void testExternalInputActivitySequenceTracksSyncAndReset() {
    hal::ExternalInputCapture inputs;
    CHECK_EQ(inputs.activitySequence(), 0U);

    inputs.injectSyncEdgeForTest(1000U, true);
    CHECK_EQ(inputs.activitySequence(), 1U);
    inputs.injectSyncEdgeForTest(2000U, false);
    CHECK_EQ(inputs.activitySequence(), 2U);

    inputs.injectResetEdgeForTest(3000U, true);
    CHECK_EQ(inputs.activitySequence(), 3U);
    inputs.injectResetEdgeForTest(4000U, false);
    CHECK_EQ(inputs.activitySequence(), 4U);
}
