/**
 * @file groove_application.inc
 * @brief Included test cases split from test_host_firmware/test_main.cpp for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

void testCustomGrooveEditorControllerWorkflow() {
    resetFakes();
    prepareDisplaySuccess();
    hal::OledDisplay display;
    CHECK(display.begin());

    ClockState state = makeDefaultState();
    state.operatingMode = OperatingMode::UnifiedClock;
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::PersistentStateService persistentState(storage);
    persistentState.begin();
    services::CustomGrooveStore grooveStore(storage);
    ui::UiRenderer renderer(display, persistentState, grooveStore);
    ui::UiController controller(state, engine, renderer, persistentState, grooveStore);
    std::uint32_t now = 100U;

    auto openUnifiedEditor = [&]() {
        controllerOpenSettingsChord(controller, now);
        controllerTurn(controller, 1, now); // CHANNEL SETTINGS
        controllerShortPress(controller, now);
        CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedClock);
        controllerTurn(controller, 1, now); // TIMING
        controllerShortPress(controller, now);
        controllerTurn(controller, 4, now); // GROOVE
        controllerShortPress(controller, now);
        CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);
        controllerTurn(controller, 4, now); // EDITOR
        controllerShortPress(controller, now);
        CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveEditor);
    };

    openUnifiedEditor();
    CHECK_EQ(controller.navigation().grooveDraft.length, 16U);
    CHECK_EQ(controller.navigation().grooveCursor, 0U);

    // Fine and coarse marker movement cover both signed clamp ends.
    controllerTurn(controller, 127, now);
    CHECK_EQ(controller.navigation().grooveDraft.offsets256[0], kCustomGrooveMaximumOffset256);
    hal::ControlSample sample{};
    sample.tapButton = heldButton();
    sample.encoderDelta = -127;
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().grooveDraft.offsets256[0], kCustomGrooveMinimumOffset256);

    // TAP advances the marker. START+TURN is consumed as zoom and must not toggle transport.
    controllerTapAt(controller, now + 2U);
    now += 3U;
    CHECK_EQ(controller.navigation().grooveCursor, 1U);
    sample = {};
    sample.transportButton = heldButton();
    sample.encoderDelta = 1;
    controller.processControls(sample, now++);
    CHECK(controller.navigation().grooveZoomSteps != 0U);
    sample = {};
    sample.transportButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(state.transport, TransportState::Stopped);

    // Long press opens the normal Groove menu. Exercise ZOOM and LENGTH edits.
    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::GrooveEditorMenu);
    controllerTurn(controller, 2, now); // ZOOM
    controllerShortPress(controller, now);
    CHECK(controller.navigation().editing);
    controllerTurn(controller, 1, now);
    controllerTurn(controller, -1, now);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now); // LENGTH
    controllerShortPress(controller, now);
    controllerTurn(controller, -127, now);
    CHECK_EQ(controller.navigation().grooveDraft.length, 1U);
    CHECK_EQ(controller.navigation().grooveCursor, 0U);
    controllerTurn(controller, 127, now);
    CHECK_EQ(controller.navigation().grooveDraft.length, kCustomGrooveMaximumSteps);
    controllerShortPress(controller, now);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveEditor);

    // SAVE into a new slot. A generated musical name is present immediately and
    // encoder long-press commits it without forcing the user through 16 characters.
    controller.seedGeneratedNames(0x12345678U);
    controllerLongPress(controller, now);
    controllerShortPress(controller, now); // SAVE
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    controllerShortPress(controller, now); // empty slot 0 -> name entry
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveNameEntry);
    CHECK(controller.navigation().grooveNameBuffer[0] != ' ');
    CHECK(controller.navigation().grooveNameBuffer[0] != '\0');
    controllerTurn(controller, 1, now); // generated default remains editable
    controllerLongPress(controller, now); // explicit HOLD TO SAVE contract
    CHECK(grooveStore.exists(0U));
    char savedGrooveName[services::CustomGrooveStore::kNameLength + 1U]{};
    grooveStore.name(0U, savedGrooveName, sizeof(savedGrooveName));
    CHECK(savedGrooveName[0] != '\0');
    CHECK_EQ(state.unifiedClock.groove.preset, GroovePreset::Custom);
    CHECK_EQ(state.unifiedClock.groove.customSlot, 0U);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);
    CHECK_EQ(controller.navigation().cursor, 4U);

    // Normal Groove settings expose LOAD directly; loading activates the stored
    // Custom Groove without first entering the graphical editor.
    state.unifiedClock.groove = {GroovePreset::Off, 100U, 0U, 0U};
    engine.updateConfiguration(state, true);
    controllerTurn(controller, -1, now); // LOAD
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    controllerShortPress(controller, now); // slot 0
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);
    CHECK_EQ(state.unifiedClock.groove.preset, GroovePreset::Custom);
    CHECK_EQ(state.unifiedClock.groove.customSlot, 0U);

    // Reopening an active Custom Groove must load the stored slot into the draft.
    controllerTurn(controller, 1, now); // EDITOR
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveEditor);
    CHECK_EQ(controller.navigation().grooveDraft.length, kCustomGrooveMaximumSteps);

    // Dirty LOAD first cancels, then confirms discard and reaches the slot list.
    controllerTurn(controller, 1, now);
    controllerLongPress(controller, now);
    controllerTurn(controller, 1, now); // LOAD
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveDiscardConfirm);
    controllerShortPress(controller, now); // NO
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveEditor);
    controllerLongPress(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now); // YES
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);

    // Failed load remains in the list; successful load returns to the editor and clears dirty state.
    controllerTurn(controller, 9, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    controllerTurn(controller, -9, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveEditor);
    controllerReset(controller, now); // clean editor returns immediately
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);

    // Dirty BACK confirmation covers both NO and YES paths.
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveDiscardConfirm);
    controllerReset(controller, now); // cancel with BACK
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveEditor);
    controllerReset(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now); // YES -> restore original settings
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);
    CHECK_EQ(state.unifiedClock.groove.preset, GroovePreset::Custom);

    // Overwrite flow: NO keeps slot selection; YES reuses the durable name and commits.
    controllerShortPress(controller, now);
    controllerLongPress(controller, now);
    controllerShortPress(controller, now); // SAVE
    controllerShortPress(controller, now); // existing slot -> overwrite confirm
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveOverwriteConfirm);
    controllerShortPress(controller, now); // NO
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    controllerReset(controller, now); // slots -> editor
    controllerLongPress(controller, now);
    controllerShortPress(controller, now);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now); // YES
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);

    // Rename an occupied Custom Groove from the normal Groove page. The name
    // editor reuses the existing name rather than generating a new one. The UI
    // accepts the mutation into RAM immediately; durable Flash commit is serviced
    // separately once transport timing allows it.
    CHECK(grooveStore.service(true));
    char nameBeforeRename[services::CustomGrooveStore::kNameLength + 1U]{};
    grooveStore.name(0U, nameBeforeRename, sizeof(nameBeforeRename));
    controllerTurn(controller, 2, now); // RENAME
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    controllerShortPress(controller, now); // slot 0
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveNameEntry);
    CHECK_EQ(controller.navigation().grooveSlotAction, ui::GrooveSlotAction::Rename);
    CHECK(controller.navigation().grooveNameBuffer[0] == nameBeforeRename[0]);
    controllerTurn(controller, 1, now);
    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    char pendingRenamed[services::CustomGrooveStore::kNameLength + 1U]{};
    grooveStore.name(0U, pendingRenamed, sizeof(pendingRenamed));
    CHECK(std::strcmp(nameBeforeRename, pendingRenamed) != 0);

    hal::PersistentStorage::powerLossBeforeCommitForTest();
    CHECK(!grooveStore.service(true));
    services::CustomGrooveStore durableAfterFailedRename(storage);
    char durableName[services::CustomGrooveStore::kNameLength + 1U]{};
    durableAfterFailedRename.name(0U, durableName, sizeof(durableName));
    CHECK(std::strcmp(nameBeforeRename, durableName) == 0);
    CHECK(grooveStore.service(true));
    char nameAfterRename[services::CustomGrooveStore::kNameLength + 1U]{};
    grooveStore.name(0U, nameAfterRename, sizeof(nameAfterRename));
    CHECK(std::strcmp(nameBeforeRename, nameAfterRename) != 0);

    // Delete requires an explicit YES. NO must be a true no-op.
    CustomGroovePattern disposable{};
    disposable.length = 4U;
    disposable.offsets256[1] = 32;
    CHECK(grooveStore.save(1U, "DELETE ME", disposable));
    controllerReset(controller, now); // RENAME slot list -> Groove
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);
    CHECK_EQ(controller.navigation().cursor, 6U);
    controllerTurn(controller, 1, now); // DELETE
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    controllerTurn(controller, 1, now); // slot 1
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveDeleteConfirm);
    controllerShortPress(controller, now); // NO
    CHECK(grooveStore.exists(1U));
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    controllerShortPress(controller, now); // slot 1 again
    controllerTurn(controller, 1, now); // YES
    controllerShortPress(controller, now);
    CHECK(!grooveStore.exists(1U));
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    controllerReset(controller, now); // DELETE slot list -> Groove

    // Independent mode exercises the per-channel preview/update/restore branches.
    state.operatingMode = OperatingMode::Independent;
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.groove = {GroovePreset::Custom, 75U, 0U, 0U};
    engine.updateConfiguration(state, true);
    controllerReset(controller, now); // Groove -> ChannelTiming
    controllerReset(controller, now); // -> Channel
    controllerReset(controller, now); // -> Root
    controllerReset(controller, now); // -> Performance
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now); // Channel
    controllerTurn(controller, 1, now); // Timing
    controllerShortPress(controller, now);
    controllerTurn(controller, 4, now); // Groove
    controllerShortPress(controller, now);
    controllerTurn(controller, 4, now); // Editor
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveEditor);
    controllerTurn(controller, -1, now);
    controllerReset(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);
    CHECK_EQ(state.channels[0].common.groove.preset, GroovePreset::Custom);
}

void testCustomGrooveRecorderControllerWorkflow() {
    resetFakes();
    prepareDisplaySuccess();
    hal::OledDisplay display;
    CHECK(display.begin());

    ClockState state = makeDefaultState();
    state.operatingMode = OperatingMode::UnifiedClock;
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::PersistentStateService persistentState(storage);
    persistentState.begin();
    services::CustomGrooveStore grooveStore(storage);
    ui::UiRenderer renderer(display, persistentState, grooveStore);
    ui::UiController controller(state, engine, renderer, persistentState, grooveStore);
    std::uint32_t now = 100U;

    // SETTINGS -> CHANNEL SETTINGS -> TIMING -> GROOVE -> RECORD.
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    controllerTurn(controller, 4, now);
    controllerShortPress(controller, now);
    controllerTurn(controller, 5, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveRecorder);
    CHECK_EQ(controller.navigation().grooveRecordCountInBeats, 4U);
    CHECK_EQ(controller.navigation().grooveRecordMode, ui::GrooveRecordMode::OneShot);
    CHECK_EQ(controller.navigation().grooveRecordState, ui::GrooveRecordState::Ready);

    // Default Count-In is a recorder-local four-beat pre-roll. PLAY starts the
    // global clock if needed, then enters PRECOUNT; PLAY again cancels only the
    // recorder and rewinds its playhead.
    hal::ControlSample sample{};
    sample.transportButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(state.transport, TransportState::Playing);
    CHECK_EQ(controller.navigation().grooveRecordState, ui::GrooveRecordState::PreCount);
    CHECK_EQ(controller.navigation().grooveRecordCountInRemaining, 4U);
    sample = {};
    sample.transportButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().grooveRecordState, ui::GrooveRecordState::Ready);
    CHECK_EQ(controller.navigation().grooveRecordPlayheadStep, 0U);

    // Record menu: switch to ENDLESS, turn Count-In off and use a four-step pattern.
    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::GrooveRecordMenu);
    controllerShortPress(controller, now); // MODE edit
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().grooveRecordMode, ui::GrooveRecordMode::Endless);
    controllerTurn(controller, 1, now); // COUNT IN
    controllerShortPress(controller, now);
    controllerTurn(controller, -4, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().grooveRecordCountInBeats, 0U);
    controllerTurn(controller, 1, now); // LENGTH
    controllerShortPress(controller, now);
    controllerTurn(controller, -12, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().grooveDraft.length, 4U);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveRecorder);

    // PLAY starts recording and, from STOP, starts only the global musical clock.
    sample = {};
    sample.transportButton = releasedEdge();
    const std::uint32_t playNowUs = now * 1000U;
    controller.processControls(sample, now++, playNowUs);
    CHECK_EQ(state.transport, TransportState::Playing);
    CHECK_EQ(controller.navigation().grooveRecordState, ui::GrooveRecordState::Recording);

    // TAP is a record event here, not Tap Tempo. Capture straight step 1 at the
    // current engine position using the high-resolution physical edge timestamp.
    const std::uint16_t bpmBeforeTap = state.bpm;
    sample = {};
    sample.tapButton = {hal::ButtonEdge::Pressed, true, 1'000'000U};
    controller.processControls(sample, now++, 1'000'000U);
    CHECK_EQ(state.bpm, bpmBeforeTap);
    CHECK((controller.navigation().grooveRecordCapturedMask & 0x1ULL) != 0ULL);
    CHECK_EQ(controller.navigation().grooveDraft.offsets256[0], 0);

    // Advance slightly beyond the next nominal grid point and capture another
    // marker. The recorded offset must become visible in the shared draft.
    const std::uint64_t intervalQ32 = engine.nominalIntervalQ32(0U);
    const std::uint64_t targetQ32 = intervalQ32 + intervalQ32 / 8ULL;
    std::uint32_t guard = 0U;
    while (engine.snapshot().masterPositionQ32 < targetQ32 && guard++ < 20'000U) {
        engine.processSchedulerTick();
    }
    CHECK(guard < 20'000U);
    sample = {};
    sample.tapButton = {hal::ButtonEdge::Pressed, true, 2'000'000U};
    controller.processControls(sample, now++, 2'000'000U);
    CHECK((controller.navigation().grooveRecordCapturedMask & 0x2ULL) != 0ULL);
    CHECK(controller.navigation().grooveDraft.offsets256[1] > 0);

    // BACK stops capture before presenting the normal dirty-draft confirmation.
    // Cancelling that dialog returns to RECORD in READY; recording never runs
    // invisibly behind the modal screen.
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveDiscardConfirm);
    CHECK_EQ(controller.navigation().grooveRecordState, ui::GrooveRecordState::Ready);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveRecorder);
    CHECK_EQ(controller.navigation().grooveRecordState, ui::GrooveRecordState::Ready);

    sample = {};
    sample.transportButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().grooveRecordState, ui::GrooveRecordState::Recording);

    // PLAY+TURN is consumed by zoom and must not stop the active recorder.
    const std::uint8_t zoomBefore = controller.navigation().grooveZoomSteps;
    sample = {};
    sample.transportButton = heldButton();
    sample.encoderDelta = 1;
    controller.processControls(sample, now++);
    sample = {};
    sample.transportButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK(controller.navigation().grooveZoomSteps != zoomBefore);
    CHECK_EQ(controller.navigation().grooveRecordState, ui::GrooveRecordState::Recording);

    // PLAY alone stops recording and rewinds the recorder playhead without
    // stopping the global transport that supplies the clock.
    sample = {};
    sample.transportButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().grooveRecordState, ui::GrooveRecordState::Ready);
    CHECK_EQ(controller.navigation().grooveRecordPlayheadStep, 0U);
    CHECK_EQ(controller.navigation().grooveRecordPlayheadPhase256, 0U);
    CHECK_EQ(state.transport, TransportState::Playing);

    // SAVE from the Record menu uses the same durable Custom Groove store as
    // the manual editor. This is the direct recorder save path requested by the UI.
    const auto capturedDraft = controller.navigation().grooveDraft;
    controllerLongPress(controller, now);
    controllerTurn(controller, 4, now); // SAVE
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveSlots);
    controllerShortPress(controller, now); // slot 0 -> generated name
    controllerLongPress(controller, now);
    CHECK(grooveStore.exists(0U));
    CustomGroovePattern stored{};
    CHECK(grooveStore.load(0U, stored));
    CHECK_EQ(stored.length, capturedDraft.length);
    CHECK_EQ(stored.offsets256[0], capturedDraft.offsets256[0]);
    CHECK_EQ(stored.offsets256[1], capturedDraft.offsets256[1]);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);
    CHECK_EQ(controller.navigation().cursor, 5U); // RECORD

    // Reopening RECORD loads the stored pattern. EDITOR then receives the exact
    // same draft for normal marker-by-marker correction.
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveRecorder);
    CHECK_EQ(controller.navigation().grooveDraft.offsets256[1], capturedDraft.offsets256[1]);
    controllerLongPress(controller, now);
    controllerTurn(controller, 5, now); // EDITOR
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::GrooveEditor);
    CHECK_EQ(controller.navigation().grooveDraft.length, capturedDraft.length);
    CHECK_EQ(controller.navigation().grooveDraft.offsets256[0], capturedDraft.offsets256[0]);
    CHECK_EQ(controller.navigation().grooveDraft.offsets256[1], capturedDraft.offsets256[1]);
}


void testExternalTapTimestampTranslationSurvivesMicrosecondWrap() {
    constexpr std::uint32_t edgeBeforeWrapUs = 0xFFFFF000U;
    constexpr std::uint32_t nowAfterWrapUs = 0x00001000U;
    constexpr std::uint32_t ageUs = nowAfterWrapUs - edgeBeforeWrapUs;
    static_assert(ageUs == 8192U);

    const std::uint32_t nowMs = 4'304'000U;
    const std::uint32_t translated = app::ClockApplication::translateExternalTapTimestampMsForTest(
        nowMs, nowAfterWrapUs, edgeBeforeWrapUs);
    CHECK_EQ(translated, nowMs - ageUs / 1000U);

    // A fresh post-wrap edge must map close to the current millisecond epoch,
    // not to the ~0..4,295,000 ms epoch produced by raw edgeUs/1000 truncation.
    const std::uint32_t recentEdgeUs = nowAfterWrapUs - 500'000U;
    const std::uint32_t recent = app::ClockApplication::translateExternalTapTimestampMsForTest(
        nowMs, nowAfterWrapUs, recentEdgeUs);
    CHECK_EQ(recent, nowMs - 500U);
}

void testApplicationAndEntryPoints() {
#if !CLOCK_DISPLAY_USE_SPI
    resetFakes(); // failure path, abort infinite safe halt via fake delay exception
    fakefw::throwOnDelay=true;
    try { app::ClockApplication failed; failed.begin(); CHECK(false); } catch(const std::runtime_error&) { CHECK(true); }
#endif
    resetFakes(); prepareDisplaySuccess(); fakefw::delayAdvanceOverrideMs = 333U;
    app::ClockApplication application; application.begin(); CHECK(fakefw::nowMs>=config::kBootDurationMs); application.runOnce();
    resetFakes(); prepareDisplaySuccess(); fakefw::delayAdvanceOverrideMs = 333U;
    ClockState stoppedState=makeDefaultState(); stoppedState.transport=TransportState::Stopped;
    app::ClockApplication stoppedApplication; stoppedApplication.begin(stoppedState); stoppedApplication.runOnce();
    CHECK(HardwareTimer::lastInstance!=nullptr); HardwareTimer::lastInstance->fire();

    // Regression: a persisted PLAY state is remembered as metadata, but power-up
    // must still enter STOP and must never emit a gate until the user explicitly plays.
    resetFakes();
    {
        hal::PersistentStorage seedStorage;
        services::PersistentStateService seedState(seedStorage);
        seedState.begin();
        seedState.requestTransportState(TransportState::Playing, 10U);
        seedState.service(10U + config::kPersistenceCommitDelayMs);
        CHECK(seedState.hasStoredTransportState());
        CHECK_EQ(seedState.storedTransportState(), TransportState::Playing);
    }
    prepareDisplaySuccess();
    fakefw::delayAdvanceOverrideMs = 333U;
    app::ClockApplication persistedPlayApplication;
    persistedPlayApplication.begin();
    CHECK(HardwareTimer::lastInstance != nullptr);

    // Confirm that begin() did not erase or silently rewrite the historical state.
    {
        hal::PersistentStorage verifyStorage;
        services::PersistentStateService verifyState(verifyStorage);
        verifyState.begin();
        CHECK_EQ(verifyState.storedTransportState(), TransportState::Playing);
    }

    fakefw::writes.clear();
    for (std::uint32_t tick = 0U; tick < 25000U; ++tick) {
        HardwareTimer::lastInstance->fire();
    }
    for (const std::uint32_t gatePin : pinmap::kGateChannelPins) {
        CHECK_EQ(fakefw::pinValues[gatePin], LOW);
        const bool emittedHighGate = std::any_of(
            fakefw::writes.begin(),
            fakefw::writes.end(),
            [gatePin](const fakefw::PinWrite& write) {
                return write.pin == gatePin && write.value == HIGH;
            });
        CHECK(!emittedHighGate);
    }

    // Framework glue functions use the translation-unit static application.
    resetFakes(); prepareDisplaySuccess(); setup(); loop(); CHECK(HardwareTimer::lastInstance!=nullptr);
}
