/**
 * @file ui_games.inc
 * @brief Included test cases split from test_host_firmware/test_main.cpp for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

void testUiControllerFlows() {
    resetFakes();
    prepareDisplaySuccess();
    hal::OledDisplay display;
    CHECK(display.begin());

    ClockState state = makeDefaultState();
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    hal::PersistentStorage storage;
    hal::PersistentStorage::resetForTest();
    services::PersistentStateService persistentState(storage);
    persistentState.begin();
    services::CustomGrooveStore grooveStore(storage);
    services::SequencerPatternStore sequencerStore(storage);
    services::SequencerStepStore sequencerStepStore(storage);
    CHECK(sequencerStore.begin());
    CHECK(sequencerStepStore.begin());
    ui::UiRenderer renderer(
        display, persistentState, grooveStore, sequencerStore, sequencerStepStore);
    ui::UiController controller(
        state, engine, renderer, persistentState, grooveStore, sequencerStore, sequencerStepStore);
    std::uint32_t now = 100U;

    controller.invalidate();
    controller.serviceRendering(now);
    controller.invalidate();
    controller.serviceRendering(now + 1U); // refresh throttle
    controller.serviceRendering(now + config::kDisplayRefreshMinimumMs + 1U);

    const std::uint16_t originalBpm = state.bpm;
    controllerTurn(controller, 1, now);
    CHECK_EQ(state.bpm, originalBpm + 1U);
    controllerTurn(controller, -1, now);

    // A short encoder push opens the compact eight-channel overview. Turning
    // only moves the cursor; a second short push commits the channel and returns.
    const std::uint8_t overviewStart = controller.navigation().selectedChannel;
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    controllerTurn(controller, 2, now);
    CHECK_EQ(
        controller.navigation().cursor,
        static_cast<std::uint8_t>((overviewStart + 2U) % kChannelCount));
    CHECK_EQ(controller.navigation().selectedChannel, overviewStart);
    const std::uint8_t committedOverviewChannel = controller.navigation().cursor;
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    CHECK_EQ(controller.navigation().selectedChannel, committedOverviewChannel);

    // Channel settings are deliberately long-push only from Performance or overview.
    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);
    const std::uint8_t longPushChannel = controller.navigation().cursor;
    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(controller.navigation().selectedChannel, longPushChannel);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // TAP + Turn inside the overview opens the horizontal mode carousel.
    // Releasing TAP now requests a mode change; changing mode always requires
    // an explicit YES before the new configuration is applied.
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    hal::ControlSample sample{};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    sample.encoderDelta = 1; // CLOCK -> EUCLID
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeSelect);
    CHECK_EQ(controller.navigation().cursor, 3U);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    CHECK_EQ(
        state.channels[controller.navigation().selectedChannel].common.mode,
        ChannelMode::Clock);
    controllerConfirmModeYes(controller, now);
    CHECK_EQ(state.operatingMode, OperatingMode::Independent);
    CHECK_EQ(
        state.channels[controller.navigation().selectedChannel].common.mode,
        ChannelMode::Euclid);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Euclid);
    CHECK_EQ(controller.navigation().cursor, 0U);

    // Selecting the already active mode needs no destructive-change confirmation.
    controllerReset(controller, now);
    controllerReset(controller, now);
    controllerShortPress(controller, now);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    sample.encoderDelta = 1; // EUCLID -> SEQ candidate
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    controllerShortPress(controller, now); // Default NO
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeSelect);
    CHECK_EQ(state.channels[controller.navigation().selectedChannel].common.mode, ChannelMode::Euclid);
    controllerReset(controller, now); // ModeSelect -> overview
    controllerReset(controller, now); // overview -> performance

    // Return to Performance, then select the global one-clock-to-eight mode.
    controllerReset(controller, now); // Euclid -> Channel
    controllerReset(controller, now); // Channel -> Performance
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    sample.encoderDelta = 3; // EUCLID -> ONE CLOCK
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    controllerConfirmModeYes(controller, now);
    CHECK_EQ(state.operatingMode, OperatingMode::UnifiedClock);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedClock);
    CHECK_EQ(controller.navigation().cursor, 1U);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // In a global mode the overview is informational rather than a fake channel
    // selector. Short push returns; long push enters the relevant global menu.
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    const std::uint8_t channelBeforeGlobalTurn = controller.navigation().selectedChannel;
    controllerTurn(controller, 1, now);
    CHECK_EQ(controller.navigation().selectedChannel, channelBeforeGlobalTurn);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    controllerShortPress(controller, now);
    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedClock);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // Switching back to ordinary CLOCK confirms first and then returns directly
    // to the performance screen as requested by the mode workflow.
    controllerShortPress(controller, now);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    sample.encoderDelta = 2; // ONE CLOCK -> CLOCK
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    controllerConfirmModeYes(controller, now);
    CHECK_EQ(state.operatingMode, OperatingMode::Independent);
    CHECK_EQ(
        state.channels[controller.navigation().selectedChannel].common.mode,
        ChannelMode::Clock);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // TAP + encoder push is the only gesture for the combined settings tree.
    // It must not leak a tap-tempo event or a second encoder action on release.
    const std::uint16_t bpmBeforeChord = state.bpm;
    controllerOpenSettingsChord(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Root);
    controllerShortPress(controller, now);  // GENERAL SETTINGS
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::General);

    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);  // INPUTS
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::InputAssignments);
    CHECK_EQ(controller.navigation().cursor, 0U);
    controllerTurn(controller, 2, now);
    controllerShortPress(controller, now);  // CONFIG
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Sync);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::InputAssignments);
    CHECK_EQ(controller.navigation().cursor, 2U);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::General);
    CHECK_EQ(controller.navigation().cursor, 1U);

    controllerTurn(controller, 3, now);
    controllerShortPress(controller, now);  // HARDWARE
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Hardware);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::General);
    CHECK_EQ(controller.navigation().cursor, 4U);

    controllerTurn(controller, -1, now);
    controllerShortPress(controller, now);  // DIAGNOSTICS
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Diagnostics);
    controllerShortPress(controller, now);  // INPUTS
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::DiagnosticsInputs);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Diagnostics);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);  // OUTPUTS
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::DiagnosticsOutputs);
    controllerReset(controller, now);
    controllerReset(controller, now);
    controllerReset(controller, now);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    controllerOpenSettingsChord(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Root);
    CHECK_EQ(state.bpm, bpmBeforeChord);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // Exercise all six root actions, including SUPPORT CLOCK.
    for (std::uint8_t rootRow = 0U; rootRow < 6U; ++rootRow) {
        controllerOpenSettingsChord(controller, now);
        controllerTurn(controller, static_cast<std::int8_t>(rootRow), now);
        controllerShortPress(controller, now);
        if (rootRow == 5U) {
            CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Root);
        } else {
            CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
            if (rootRow == 0U) {
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::General);
            } else if (rootRow == 1U) {
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
            } else if (rootRow == 2U) {
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Preferences);
            } else if (rootRow == 3U) {
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Support);
                controllerShortPress(controller, now); // GitHub Sponsors -> informational popover
                CHECK_EQ(controller.navigation().screen, ui::Screen::InformationPopover);
                CHECK(std::strcmp(controller.navigation().informationPopoverTitle.data(), "GITHUB SPONSORS") == 0);
                controllerShortPress(controller, now);
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Support);
            } else {
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Info);
                controllerShortPress(controller, now); // short PRODUCT fits; no popover
                CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Info);
                CHECK(!controller.navigation().editing);
            }
            controllerReset(controller, now); // child -> Root
        }
        controllerReset(controller, now); // Root -> Performance
        CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    }

    // PRESETS owns factory templates as well as load/save operations.
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 2, now);     // PRESETS
    controllerShortPress(controller, now);
    controllerTurn(controller, 3, now);     // Templates
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Templates);
    controllerTurn(controller, 5, now);
    controllerTurn(controller, -5, now);
    state.channels[0].common.mode = ChannelMode::Off;
    controllerShortPress(controller, now); // apply ALL MASTER template
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    CHECK_EQ(state.channels[0].common.mode, ChannelMode::Clock);

    // PRESETS: save a complete named preset, then verify occupied slots ask
    // before overwrite and that NO is the safe default.
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 2, now);     // PRESETS
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Preferences);
    controllerTurn(controller, 2, now);     // Save preset
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::PresetSlots);
    controllerShortPress(controller, now);  // empty slot 1 -> name entry
    CHECK_EQ(controller.navigation().screen, ui::Screen::NameEntry);
    controllerTurn(controller, 1, now);     // rotate character band
    for (std::uint8_t character = 0U;
         character < services::PersistentStateService::kPresetNameLength;
         ++character) {
        controllerShortPress(controller, now);
    }
    CHECK_EQ(controller.navigation().screen, ui::Screen::PresetSlots);
    persistentState.service(now + config::kPersistenceCommitDelayMs, true);
    CHECK(persistentState.presetExists(0U));

    // Selecting the same occupied slot for SAVE must not jump straight to name entry.
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::OverwriteConfirm);
    CHECK_EQ(controller.navigation().cursor, 0U); // NO
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::PresetSlots);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now); // YES
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::NameEntry);
    controllerReset(controller, now); // cancel name entry
    controllerReset(controller, now); // slots -> Preferences
    controllerReset(controller, now); // Preferences -> Root
    controllerReset(controller, now); // Root -> Performance

    // Load the saved slot.
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 2, now);     // PRESETS
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);     // Load preset
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::PresetSlots);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // The saved ALL MASTER preset correctly restores ONE CLOCK. Switch to the
    // independent channel architecture for the remaining per-channel UI tests.
    state.operatingMode = OperatingMode::Independent;
    engine.updateConfiguration(state, true);

    // The selected-channel page is a compact mode-aware hierarchy. OFF has
    // only MODE; active modes expose the three logical groups in priority order.
    std::uint8_t selectedChannel = controller.navigation().selectedChannel;
    state.channels[selectedChannel].common.mode = ChannelMode::Off;
    engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now); // Channel
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Channel, ChannelMode::Off), 1U);
    controllerShortPress(controller, now); // MODE
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeSelect);
    CHECK_EQ(controller.navigation().cursor, 5U);
    controllerTurn(controller, -3, now); // OFF -> CLOCK
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    controllerConfirmModeYes(controller, now);
    CHECK_EQ(state.channels[selectedChannel].common.mode, ChannelMode::Clock);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Channel, ChannelMode::Clock), 4U);

    // CLOCK exposes TIMING, CLOCK and OUTPUT as one-level group pages.
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now); // Channel
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now); // TIMING
    CHECK(std::strcmp(ui::buildMenuRow(ui::SettingsPage::Channel,1U,selectedChannel,state).label,"TIMING") == 0);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::ChannelTiming);
    CHECK(std::strcmp(ui::buildMenuRow(ui::SettingsPage::ChannelTiming,0U,selectedChannel,state).label,"DIV/MULT") == 0);
    controllerShortPress(controller, now);
    CHECK(controller.navigation().editing);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    CHECK(!controller.navigation().editing);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(controller.navigation().cursor, 1U);
    controllerTurn(controller, 1, now); // CLOCK
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Clock);
    controllerShortPress(controller, now); // METER BEATS
    CHECK(controller.navigation().editing);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(controller.navigation().cursor, 2U);
    controllerReset(controller, now); // Channel -> Root
    controllerReset(controller, now); // Root -> Performance

    // EUCLID puts its mode group first, then TIMING, then OUTPUT. Groove remains
    // inside TIMING and BACK restores both subgroup and root cursors.
    state.channels[selectedChannel].common.mode = ChannelMode::Euclid;
    engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now); // Channel
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK(std::strcmp(ui::buildMenuRow(ui::SettingsPage::Channel,1U,selectedChannel,state).label,"EUCLID") == 0);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Euclid);
    CHECK(std::strcmp(ui::buildMenuRow(ui::SettingsPage::Euclid,0U,selectedChannel,state).label,"STEPS") == 0);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(controller.navigation().cursor, 1U);
    controllerTurn(controller, 1, now); // TIMING
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::ChannelTiming);
    controllerTurn(controller, 4, now); // GROOVE
    CHECK_EQ(controller.navigation().scrollOffset, 0U);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::ChannelTiming);
    CHECK_EQ(controller.navigation().cursor, 4U);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(controller.navigation().cursor, 2U);
    controllerReset(controller, now);
    controllerReset(controller, now);

    // Sequencer 2.0 exposes the active P1..P8 slot, 1..128 length,
    // direction, loop mode and a direct 8-step editor entry. Long-push from
    // the editor opens pattern operations and BACK returns to the editor.
    state.channels[selectedChannel].common.mode = ChannelMode::Sequencer;
    engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now); // Channel
    CHECK(std::strcmp(ui::buildMenuRow(ui::SettingsPage::Channel,1U,selectedChannel,state).label,"SEQUENCER") == 0);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Sequencer);
    CHECK(std::strcmp(
        ui::buildSequencerV2MenuRow(0U, sequencerStore.activeSlot(selectedChannel),
                                    sequencerStore.pattern(selectedChannel, sequencerStore.activeSlot(selectedChannel))).label,
        "EDITOR") == 0);

    controllerTurn(controller, 1, now);     // PATTERN
    controllerShortPress(controller, now); // edit PATTERN
    controllerTurn(controller, 1, now);     // P1 -> P2
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStore.activeSlot(selectedChannel), 1U);
    controllerShortPress(controller, now);
    controllerTurn(controller, -100, now);  // clamp to P1
    controllerTurn(controller, -1, now);    // no-op at lower boundary
    controllerTurn(controller, 100, now);   // clamp to P8
    controllerTurn(controller, 1, now);     // no-op at upper boundary
    controllerTurn(controller, -6, now);    // restore P2
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStore.activeSlot(selectedChannel), 1U);

    controllerTurn(controller, 1, now);     // LENGTH
    controllerShortPress(controller, now);
    controllerTurn(controller, 112, now);   // 16 -> 128
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStore.pattern(selectedChannel, 1U).length, 128U);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);     // no-op at 128
    controllerTurn(controller, -127, now);  // clamp to 1
    controllerTurn(controller, -1, now);    // no-op at 1
    controllerTurn(controller, 127, now);   // restore 128
    controllerShortPress(controller, now);

    controllerTurn(controller, 2, now);     // DIRECTION
    controllerShortPress(controller, now);
    controllerTurn(controller, 2, now);     // FORWARD -> PINGPONG
    controllerShortPress(controller, now);
    CHECK_EQ(
        sequencerStore.pattern(selectedChannel, 1U).direction,
        SequencerPlayDirection::PingPong);
    controllerShortPress(controller, now);
    controllerTurn(controller, 10, now);    // clamp to RANDOM
    controllerTurn(controller, 1, now);     // no-op at RANDOM
    controllerTurn(controller, -10, now);   // clamp to FORWARD
    controllerTurn(controller, -1, now);    // no-op at FORWARD
    controllerTurn(controller, 2, now);     // restore PINGPONG
    controllerShortPress(controller, now);

    controllerTurn(controller, 1, now);     // LOOP
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);     // LOOP -> ONCE
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStore.pattern(selectedChannel, 1U).loopMode, SequencerLoopMode::Once);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);     // no-op at ONCE
    controllerTurn(controller, -10, now);   // clamp to LOOP
    controllerTurn(controller, -1, now);    // no-op at LOOP
    controllerTurn(controller, 1, now);     // restore ONCE
    controllerShortPress(controller, now);

    controllerTurn(controller, -5, now);    // EDITOR
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::SequencerEditor);
    controllerTurn(controller, 5, now);
    controllerShortPress(controller, now);
    CHECK(sequencerPatternGate(sequencerStore.pattern(selectedChannel, 1U), 5U));

    // TAP + encoder push opens per-step expression without consuming a page-back action.
    controllerTapPushChord(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::SequencerStepEditor);
    CHECK_EQ(controller.navigation().cursor, 0U);

    controllerTurn(controller, 1, now);     // PROB
    controllerShortPress(controller, now);
    controllerTurn(controller, 75, now);    // DEFAULT -> 75%
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStepStore.metadata(selectedChannel, 1U, 5U).probabilityPercent, 75U);
    controllerShortPress(controller, now);
    controllerTurn(controller, 100, now);   // clamp at 100%
    controllerTurn(controller, 1, now);     // no-op at upper boundary
    controllerTurn(controller, -127, now);  // clamp at inherited/default 0%
    controllerTurn(controller, -1, now);    // no-op at lower boundary
    controllerTurn(controller, 75, now);    // restore representative explicit value
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStepStore.metadata(selectedChannel, 1U, 5U).probabilityPercent, 75U);

    controllerTurn(controller, 1, now);     // LENGTH
    controllerShortPress(controller, now);
    controllerTurn(controller, 6, now);     // DEFAULT -> 50% duty
    controllerShortPress(controller, now);
    CHECK_EQ(
        sequencerStepStore.metadata(selectedChannel, 1U, 5U).gateProfile,
        SequencerGateProfile::Duty50);
    controllerShortPress(controller, now);
    controllerTurn(controller, 40, now);    // clamp at longest gate profile
    controllerTurn(controller, 1, now);     // no-op at upper boundary
    controllerTurn(controller, -40, now);   // clamp back at DEFAULT
    controllerTurn(controller, -1, now);    // no-op at lower boundary
    controllerTurn(controller, 6, now);     // restore 50% duty
    controllerShortPress(controller, now);

    controllerTurn(controller, 1, now);     // RATCHET
    controllerShortPress(controller, now);
    controllerTurn(controller, 2, now);     // 1 -> 3
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStepStore.metadata(selectedChannel, 1U, 5U).ratchetCount, 3U);
    controllerShortPress(controller, now);
    controllerTurn(controller, 20, now);    // clamp at 8 substeps
    CHECK_EQ(sequencerStepStore.metadata(selectedChannel, 1U, 5U).ratchetCount, 8U);
    controllerTurn(controller, 1, now);     // no-op at upper boundary
    controllerTurn(controller, -20, now);   // clamp at 1
    CHECK_EQ(sequencerStepStore.metadata(selectedChannel, 1U, 5U).ratchetCount, 1U);
    controllerTurn(controller, -1, now);    // no-op at lower boundary
    controllerTurn(controller, 2, now);     // restore 3
    controllerShortPress(controller, now);

    controllerTurn(controller, 1, now);     // TIE
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);     // OFF -> ON; mutually excludes ratchet
    controllerShortPress(controller, now);
    CHECK(sequencerStepStore.metadata(selectedChannel, 1U, 5U).tie);
    CHECK_EQ(sequencerStepStore.metadata(selectedChannel, 1U, 5U).ratchetCount, 1U);
    controllerShortPress(controller, now);
    controllerTurn(controller, -1, now);    // ON -> OFF
    CHECK(!sequencerStepStore.metadata(selectedChannel, 1U, 5U).tie);
    controllerTurn(controller, 1, now);     // OFF -> ON
    CHECK(sequencerStepStore.metadata(selectedChannel, 1U, 5U).tie);
    controllerShortPress(controller, now);

    controllerTurn(controller, -1, now);    // RATCHET
    controllerShortPress(controller, now);
    controllerTurn(controller, 2, now);     // 1 -> 3; clears tie
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStepStore.metadata(selectedChannel, 1U, 5U).ratchetCount, 3U);
    CHECK(!sequencerStepStore.metadata(selectedChannel, 1U, 5U).tie);

    controllerReset(controller, now);       // Step detail -> editor
    CHECK_EQ(controller.navigation().screen, ui::Screen::SequencerEditor);
    CHECK_EQ(controller.navigation().sequencerCursor, 5U);

    // Crossing an eight-step viewport boundary with the encoder updates the
    // editor page directly. PLAY remains transport while editing and must not
    // steal viewport navigation.
    controllerTurn(controller, 120, now);
    CHECK_EQ(controller.navigation().sequencerPage, 15U);
    CHECK_EQ(controller.navigation().sequencerCursor, 5U);
    sample = {};
    sample.transportButton = pressedEdge();
    controller.processControls(sample, now++);
    CHECK(engine.snapshot().playing);
    CHECK_EQ(controller.navigation().sequencerPage, 15U);
    sample = {};
    sample.transportButton = pressedEdge();
    controller.processControls(sample, now++);
    CHECK(!engine.snapshot().playing);
    CHECK_EQ(controller.navigation().sequencerPage, 15U);
    controllerTurn(controller, -120, now);
    CHECK_EQ(controller.navigation().sequencerPage, 0U);
    CHECK_EQ(controller.navigation().sequencerCursor, 5U);

    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::SequencerPattern);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::SequencerPattern), "PATTERN") == 0);
    controllerShortPress(controller, now); // INVERT
    CHECK(!sequencerPatternGate(sequencerStore.pattern(selectedChannel, 1U), 5U));

    controllerTurn(controller, 1, now);     // CLEAR
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStore.pattern(selectedChannel, 1U).gates[0], 0ULL);
    CHECK_EQ(sequencerStepStore.metadata(selectedChannel, 1U, 5U).ratchetCount, 1U);

    controllerTurn(controller, 1, now);     // FILL alternating
    controllerShortPress(controller, now);
    CHECK(sequencerPatternGate(sequencerStore.pattern(selectedChannel, 1U), 0U));
    CHECK(!sequencerPatternGate(sequencerStore.pattern(selectedChannel, 1U), 1U));

    controllerTurn(controller, 1, now);     // COPY
    controllerShortPress(controller, now);
    controllerTurn(controller, -2, now);    // CLEAR again
    controllerShortPress(controller, now);
    CHECK_EQ(sequencerStore.pattern(selectedChannel, 1U).gates[0], 0ULL);
    controllerTurn(controller, 3, now);     // PASTE
    controllerShortPress(controller, now);
    CHECK(sequencerPatternGate(sequencerStore.pattern(selectedChannel, 1U), 0U));
    CHECK(!sequencerPatternGate(sequencerStore.pattern(selectedChannel, 1U), 1U));

    controllerReset(controller, now);       // Pattern ops -> editor
    CHECK_EQ(controller.navigation().screen, ui::Screen::SequencerEditor);
    controllerReset(controller, now);       // Editor -> Sequencer settings
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Sequencer);
    CHECK_EQ(controller.navigation().cursor, 0U);
    controllerReset(controller, now);       // Sequencer -> Channel
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(controller.navigation().cursor, 1U);
    controllerReset(controller, now);       // Channel -> Root
    controllerReset(controller, now);       // Root -> Performance

    // Defensive Sequencer 2.0 controller paths are part of the product contract:
    // an unattached store, an invalid channel, or a cursor beyond pattern length
    // must be harmless rather than dereference stale persistence state.
    auto& sequencerNav = ui::UiControllerTestAccess::navigation(controller);
    auto* const attachedPatternStore = ui::UiControllerTestAccess::patternStore(controller);
    auto* const attachedStepStore = ui::UiControllerTestAccess::stepStore(controller);
    const std::uint8_t savedChannel = sequencerNav.selectedChannel;

    ui::UiControllerTestAccess::patternStore(controller) = nullptr;
    CHECK(ui::UiControllerTestAccess::activePattern(controller) == nullptr);
    ui::UiControllerTestAccess::synchronizePattern(controller, false);
    ui::UiControllerTestAccess::synchronizeMetadata(controller);
    ui::UiControllerTestAccess::openStepEditor(controller);
    CHECK_EQ(ui::UiControllerTestAccess::activeMetadata(controller).ratchetCount, 1U);
    ui::UiControllerTestAccess::adjustPattern(controller, 1, now++);
    ui::UiControllerTestAccess::adjustMetadata(controller, 1, now++);
    ui::UiControllerTestAccess::toggleStep(controller, now++);
    CHECK(!ui::UiControllerTestAccess::command(controller, 0U, now++));
    ui::UiControllerTestAccess::patternStore(controller) = attachedPatternStore;

    ui::UiControllerTestAccess::stepStore(controller) = nullptr;
    ui::UiControllerTestAccess::synchronizeMetadata(controller);
    ui::UiControllerTestAccess::openStepEditor(controller);
    CHECK_EQ(ui::UiControllerTestAccess::activeMetadata(controller).ratchetCount, 1U);
    ui::UiControllerTestAccess::adjustMetadata(controller, 1, now++);
    ui::UiControllerTestAccess::stepStore(controller) = attachedStepStore;

    sequencerNav.selectedChannel = kChannelCount;
    CHECK(ui::UiControllerTestAccess::activePattern(controller) == nullptr);
    ui::UiControllerTestAccess::synchronizePattern(controller, true);
    ui::UiControllerTestAccess::synchronizeMetadata(controller);
    ui::UiControllerTestAccess::openStepEditor(controller);
    CHECK_EQ(ui::UiControllerTestAccess::activeMetadata(controller).ratchetCount, 1U);
    ui::UiControllerTestAccess::adjustPattern(controller, 1, now++);
    ui::UiControllerTestAccess::adjustMetadata(controller, 1, now++);
    ui::UiControllerTestAccess::toggleStep(controller, now++);
    CHECK(!ui::UiControllerTestAccess::command(controller, 0U, now++));
    sequencerNav.selectedChannel = savedChannel;

    SequencerPatternV2 defensivePattern = sequencerStore.pattern(savedChannel, 1U);
    defensivePattern.length = 128U;
    defensivePattern.rotation = 127U;
    CHECK(sequencerStore.updatePattern(savedChannel, 1U, defensivePattern, now++));
    (void)sequencerStore.setActiveSlot(savedChannel, 1U, now++);
    sequencerNav.cursor = 2U;
    ui::UiControllerTestAccess::adjustPattern(controller, -127, now++);
    CHECK_EQ(sequencerStore.pattern(savedChannel, 1U).length, 1U);
    CHECK_EQ(sequencerStore.pattern(savedChannel, 1U).rotation, 0U);
    sequencerNav.cursor = 3U;
    ui::UiControllerTestAccess::adjustPattern(controller, 1, now++);
    sequencerNav.cursor = 6U;
    ui::UiControllerTestAccess::adjustPattern(controller, 1, now++);
    ui::UiControllerTestAccess::adjustPattern(controller, 0, now++);

    sequencerNav.sequencerPage = 1U;
    sequencerNav.sequencerCursor = 0U;
    ui::UiControllerTestAccess::openStepEditor(controller);
    ui::UiControllerTestAccess::adjustMetadata(controller, 1, now++);
    ui::UiControllerTestAccess::toggleStep(controller, now++);
    sequencerNav.sequencerPage = 0U;
    sequencerNav.sequencerCursor = 0U;
    sequencerNav.cursor = 0U;
    ui::UiControllerTestAccess::adjustMetadata(controller, 1, now++);
    sequencerNav.cursor = 5U;
    ui::UiControllerTestAccess::adjustMetadata(controller, 1, now++);

    defensivePattern = sequencerStore.pattern(savedChannel, 1U);
    defensivePattern.direction = SequencerPlayDirection::Random;
    CHECK(sequencerStore.updatePattern(savedChannel, 1U, defensivePattern, now++));
    sequencerNav.cursor = 4U;
    ui::UiControllerTestAccess::adjustMetadata(controller, 1, now++);

    ui::UiControllerTestAccess::clearSequencerClipboard(controller);
    CHECK(!ui::UiControllerTestAccess::command(controller, 4U, now++));
    CHECK(!ui::UiControllerTestAccess::command(controller, 99U, now++));

    // Restore a regular long pattern for the rest of the UI-flow regression.
    defensivePattern = sequencerStore.pattern(savedChannel, 1U);
    defensivePattern.length = 128U;
    defensivePattern.rotation = 0U;
    defensivePattern.direction = SequencerPlayDirection::PingPong;
    CHECK(sequencerStore.updatePattern(savedChannel, 1U, defensivePattern, now++));
    ui::UiControllerTestAccess::synchronizePattern(controller, true);
    sequencerNav.cursor = 0U;

    // TAP no longer has a long-press shortcut. Holding it without turning must
    // never leave Performance; mode-specific parameters are reached through menus.
    state.channels[selectedChannel].common.mode = ChannelMode::Euclid;
    engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    controller.processControls(sample, now + 1500U);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now + 1501U);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    now += 1502U;

    // Transport and tap tempo remain dedicated only on Performance.
    sample = {};
    sample.transportButton = pressedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(state.transport, TransportState::Playing);
    sample.transportButton = pressedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(state.transport, TransportState::Paused);
    controllerReset(controller, now);
    CHECK_EQ(state.transport, TransportState::Stopped);

    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, 1000U);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, 1001U);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, 1500U);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, 1501U);
    CHECK_EQ(state.bpm, 120U);

    // Rendering invalidation reacts to external-lock changes and visible pattern steps.
    state.source = ClockSource::Internal;
    selectedChannel = controller.navigation().selectedChannel;
    state.channels[selectedChannel].common.mode = ChannelMode::Euclid;
    engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
    controller.invalidate();
    controller.serviceRendering(now + config::kDisplayRefreshMinimumMs + 1U);
    state.channels[selectedChannel].common.mode = ChannelMode::Sequencer;
    state.channels[selectedChannel].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[selectedChannel].sequencer.length = 16U;
    engine.updateConfiguration(state, true);
    engine.play();
    controller.invalidate();
    controller.serviceRendering(now + config::kDisplayRefreshMinimumMs + 2U);
    engine.setExternalLock(true, 120000U);
    controller.serviceRendering(now + (2U * config::kDisplayRefreshMinimumMs) + 2U);
    runEngineTicks(engine, 20001U);
    CHECK(engine.snapshot().channelStep[selectedChannel] != 0U);
    controller.serviceRendering(now + (3U * config::kDisplayRefreshMinimumMs) + 3U);

    // FACTORY RESET is deliberately buried as the final INFO entry. NO is the
    // safe default; YES clears the complete persistent image and returns to the
    // documented factory state in STOP.
    game::ArcadeLeaderboardStore resetLeaderboard(storage, game::ArcadeGameId::PixelRaid);
    CHECK(resetLeaderboard.markStarted());
    const std::array<char, 4U> resetInitials{{'R','S','T','\0'}};
    CHECK_EQ(resetLeaderboard.insertAndSave(9876U, resetInitials), 0);
    CHECK(persistentState.presetExists(0U));
    state.bpm = 321U;
    state.device.encoderDirectionReversed = true;
    engine.updateConfiguration(state, true);

    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 4, now); // INFO
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Info);
    controllerTurn(controller, 5, now); // FACTORY RESET, final INFO item
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::FactoryResetConfirm);
    CHECK_EQ(controller.navigation().cursor, 0U); // NO
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Info);
    CHECK_EQ(controller.navigation().cursor, 5U);
    CHECK(persistentState.presetExists(0U));
    CHECK(resetLeaderboard.hasPersistentRecord());

    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::FactoryResetConfirm);
    controllerTurn(controller, 1, now); // YES
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    CHECK_EQ(state.transport, TransportState::Stopped);
    CHECK_EQ(state.bpm, defaults::kMasterBpm);
    CHECK_EQ(state.operatingMode, defaults::kOperatingMode);
    CHECK_EQ(state.device.encoderDirectionReversed, defaults::kEncoderDirectionReversed);
    CHECK(!persistentState.presetExists(0U));
    CHECK(!resetLeaderboard.hasPersistentRecord());
    ClockState reloadedFactory{};
    CHECK(persistentState.restoreCurrentState(reloadedFactory));
    CHECK_EQ(reloadedFactory.bpm, defaults::kMasterBpm);
    CHECK_EQ(reloadedFactory.transport, TransportState::Stopped);

    // Defensive invalid-screen and no-op paths.
    auto& mutableNavigation = const_cast<ui::NavigationState&>(controller.navigation());
    mutableNavigation.screen = static_cast<ui::Screen>(99U);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    sample = {};
    controller.processControls(sample, now++);
    controller.serviceRendering(now);
}

void testHighScoreResetAppearsAfterStartAndClearsSafely() {
    resetFakes();
    prepareDisplaySuccess();
    hal::OledDisplay display;
    CHECK(display.begin());
    ClockState state = makeDefaultState();
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::PersistentStateService persistentState(storage);
    persistentState.begin();
    game::ArcadeLeaderboardStore leaderboard(storage, game::ArcadeGameId::PixelRaid);
    ui::UiRenderer renderer(display, persistentState);
    ui::UiController controller(state, engine, renderer, persistentState, &leaderboard);
    std::uint32_t now = 100U;

    controllerOpenSettingsChord(controller, now);
    CHECK(!controller.navigation().highScoreResetAvailable);
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Root, ChannelMode::Clock, false), 6U);
    controllerReset(controller, now);

    CHECK(leaderboard.markStarted());
    const std::array<char,4U> initials{{'A','X','L','\0'}};
    CHECK_EQ(leaderboard.insertAndSave(4321U, initials), 0);
    state.transport = TransportState::Playing;
    engine.updateConfiguration(state, true);

    controllerOpenSettingsChord(controller, now);
    CHECK(controller.navigation().highScoreResetAvailable);
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Root, ChannelMode::Clock, true), 7U);
    controllerTurn(controller, 5, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::HighScoreClearConfirm);
    CHECK_EQ(controller.navigation().cursor, 0U);

    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    CHECK_EQ(state.transport, TransportState::Stopped);
    CHECK(leaderboard.hasPersistentRecord());
    CHECK_EQ(leaderboard.load().count, 0U);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Root);
    CHECK(controller.navigation().highScoreResetAvailable);
}

void testEasterEggGameAndHighScore() {
    resetFakes();
    prepareDisplaySuccess();

    hal::PersistentStorage storage;
    game::EasterEggScoreStore scoreStore(storage);
    const game::HighScoreEntry emptyScore = scoreStore.load();
    CHECK_EQ(emptyScore.score, 0U);
    CHECK_EQ(emptyScore.initials[0], 'A');

    game::HighScoreEntry score{};
    score.score = 1234U;
    score.initials = {{'A', 'X', 'L', '\0'}};
    CHECK(scoreStore.save(score));
    const game::HighScoreEntry loadedScore = scoreStore.load();
    CHECK_EQ(loadedScore.score, 1234U);
    CHECK_EQ(loadedScore.initials[0], 'A');
    CHECK_EQ(loadedScore.initials[1], 'X');
    CHECK_EQ(loadedScore.initials[2], 'L');

    // Game-specific score slots must be independent while preserving the historical Pixel Raid offset.
    game::EasterEggScoreStore formulaScoreSlot(storage, game::EasterEggScoreId::Formula1);
    CHECK_EQ(formulaScoreSlot.load().score, 0U);
    game::HighScoreEntry formulaScore{};
    formulaScore.score = 2468U;
    formulaScore.initials = {{'F', 'O', 'M', '\0'}};
    CHECK(formulaScoreSlot.save(formulaScore));
    CHECK_EQ(formulaScoreSlot.load().score, 2468U);
    CHECK_EQ(scoreStore.load().score, 1234U);
    game::EasterEggScoreStore eggJourneyScoreSlot(storage, game::EasterEggScoreId::EggJourney);
    CHECK_EQ(eggJourneyScoreSlot.load().score, 0U);
    game::HighScoreEntry eggJourneyScore{};
    eggJourneyScore.score = 1357U;
    eggJourneyScore.initials = {{'E', 'G', 'G', '\0'}};
    CHECK(eggJourneyScoreSlot.save(eggJourneyScore));
    CHECK_EQ(eggJourneyScoreSlot.load().score, 1357U);
    CHECK_EQ(formulaScoreSlot.load().score, 2468U);
    CHECK_EQ(scoreStore.load().score, 1234U);

    game::HighScoreEntry invalidScore = score;
    invalidScore.initials[0] = '?';
    CHECK(!scoreStore.save(invalidScore));
    invalidScore = score; invalidScore.initials[1] = '?'; CHECK(!scoreStore.save(invalidScore));
    invalidScore = score; invalidScore.initials[2] = '?'; CHECK(!scoreStore.save(invalidScore));
    hal::PersistentStorage::failNextReadForTest();
    CHECK_EQ(scoreStore.load().score, 0U);

    // Corrupt record metadata and initials to exercise every validation stage.
    std::array<std::uint8_t, 16U> rawScore{};
    CHECK(storage.readBytes(3072U, rawScore.data(), rawScore.size()));
    for (const std::size_t corruptIndex : std::array<std::size_t, 5U>{{0U, 1U, 2U, 3U, 4U}}) {
        auto corrupt = rawScore;
        corrupt[corruptIndex] ^= 0x01U;
        CHECK(storage.writeBytes(3072U, corrupt.data(), corrupt.size()));
        CHECK_EQ(scoreStore.load().score, 0U);
        CHECK(storage.writeBytes(3072U, rawScore.data(), rawScore.size()));
    }
    auto badCrc = rawScore; badCrc[15] ^= 0x01U; CHECK(storage.writeBytes(3072U, badCrc.data(), badCrc.size())); CHECK_EQ(scoreStore.load().score, 0U);
    CHECK(storage.writeBytes(3072U, rawScore.data(), rawScore.size()));
    for (const std::size_t initialIndex : std::array<std::size_t, 3U>{{9U, 10U, 11U}}) {
        auto corrupt = rawScore; corrupt[initialIndex] = static_cast<std::uint8_t>('?');
        // Recompute is intentionally skipped: this still validates CRC rejection before initials.
        CHECK(storage.writeBytes(3072U, corrupt.data(), corrupt.size()));
        CHECK_EQ(scoreStore.load().score, 0U);
        CHECK(storage.writeBytes(3072U, rawScore.data(), rawScore.size()));
    }

    // New Top-100 stores migrate the historical single-score slots without erasing them.
    game::ArcadeLeaderboardStore pixelLeaderboard(storage, game::ArcadeGameId::PixelRaid);
    game::ArcadeLeaderboardStore formulaLeaderboard(storage, game::ArcadeGameId::Formula1);
    game::ArcadeLeaderboardStore breakoutLeaderboard(storage, game::ArcadeGameId::Breakout);
    game::ArcadeLeaderboardStore eggLeaderboard(storage, game::ArcadeGameId::EggJourney);
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
    CHECK_EQ(formulaLeaderboard.load().entries[0].score, 2468U);
    CHECK_EQ(eggLeaderboard.load().entries[0].score, 1357U);
    const std::array<char, 4U> topName{{'T','O','P','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(5000U, topName), 0);
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 5000U);
    CHECK_EQ(pixelLeaderboard.load().entries[1].score, 1234U);
    CHECK_EQ(formulaLeaderboard.load().entries[0].score, 2468U);
    CHECK_EQ(breakoutLeaderboard.load().count, 0U);

    // Leaderboard durable-record rejection is tested field-by-field so header,
    // CRC, initials, ordering, read failure, and write failure cannot silently regress.
    constexpr std::size_t kLeaderboardRecordBytes = 812U;
    constexpr std::size_t kPixelLeaderboardOffset = 4096U;
    std::array<std::uint8_t, kLeaderboardRecordBytes> pixelRecord{};
    CHECK(storage.readBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));
    for (const std::size_t headerIndex : std::array<std::size_t, 6U>{{0U, 1U, 2U, 3U, 4U, 5U}}) {
        auto corrupt = pixelRecord;
        corrupt[headerIndex] = headerIndex == 5U ? 101U : static_cast<std::uint8_t>(corrupt[headerIndex] ^ 0x5AU);
        CHECK(storage.writeBytes(kPixelLeaderboardOffset, corrupt.data(), corrupt.size()));
        CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
        CHECK(storage.writeBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));
    }
    auto corruptLeaderboardCrc = pixelRecord;
    corruptLeaderboardCrc.back() ^= 0x01U;
    CHECK(storage.writeBytes(kPixelLeaderboardOffset, corruptLeaderboardCrc.data(), corruptLeaderboardCrc.size()));
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
    CHECK(storage.writeBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));

    for (const std::size_t initialByte : std::array<std::size_t, 3U>{{12U, 13U, 14U}}) {
        auto corrupt = pixelRecord;
        corrupt[initialByte] = static_cast<std::uint8_t>('?');
        writeTestUint32Le(corrupt.data() + kLeaderboardRecordBytes - 4U,
            testCrc32(corrupt.data(), kLeaderboardRecordBytes - 4U));
        CHECK(storage.writeBytes(kPixelLeaderboardOffset, corrupt.data(), corrupt.size()));
        CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
        CHECK(storage.writeBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));
    }
    auto unsorted = pixelRecord;
    writeTestUint32Le(unsorted.data() + 16U, 6000U);
    writeTestUint32Le(unsorted.data() + kLeaderboardRecordBytes - 4U,
        testCrc32(unsorted.data(), kLeaderboardRecordBytes - 4U));
    CHECK(storage.writeBytes(kPixelLeaderboardOffset, unsorted.data(), unsorted.size()));
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
    CHECK(storage.writeBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));

    hal::PersistentStorage::failNextReadForTest();
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
    std::array<char, 4U> invalidArcadeInitials{{'?','A','A','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, invalidArcadeInitials), -1);
    invalidArcadeInitials = {{'A','?','A','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, invalidArcadeInitials), -1);
    invalidArcadeInitials = {{'A','A','?','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, invalidArcadeInitials), -1);
    invalidArcadeInitials = {{'[','A','A','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, invalidArcadeInitials), -1);
    CHECK_EQ(pixelLeaderboard.insertAndSave(0U, topName), -1);
    hal::PersistentStorage::failNextWriteForTest();
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, topName), -1);

    game::LeaderboardTable fullTable{};
    fullTable.count = 100U;
    for (std::size_t index = 0U; index < fullTable.entries.size(); ++index) {
        fullTable.entries[index].score = static_cast<std::uint32_t>(10000U - index * 50U);
    }
    CHECK_EQ(pixelLeaderboard.qualifyingRank(10001U, fullTable), 0);
    CHECK_EQ(pixelLeaderboard.qualifyingRank(9999U, fullTable), 1);
    CHECK_EQ(pixelLeaderboard.qualifyingRank(5051U, fullTable), 99);
    CHECK_EQ(pixelLeaderboard.qualifyingRank(100U, fullTable), -1);
    CHECK_EQ(pixelLeaderboard.qualifyingRank(0U, fullTable), -1);

    hal::OledDisplay display;
    CHECK(display.begin());
    hal::ControlPanel controls;
    controls.begin();
    hal::GateOutputDriver gates;
    gates.beginDisabled();

    // Every Easter-egg intro is user-controlled: it must never time out into gameplay.
    const std::array<game::ArcadeTitle, 5U> arcadeTitles{{
        game::ArcadeTitle::PixelRaid, game::ArcadeTitle::Formula1, game::ArcadeTitle::Breakout,
        game::ArcadeTitle::EggJourney, game::ArcadeTitle::Beatknecht}};
    for (const game::ArcadeTitle title : arcadeTitles) {
        game::ArcadeShell introShell(display, title, title == game::ArcadeTitle::Beatknecht ? nullptr : &pixelLeaderboard);
        introShell.begin(100U);
        introShell.render(100U);
        introShell.render(10'100U);
        hal::ControlSample idleIntroControls{};
        CHECK(introShell.update(idleIntroControls, 60'100U) == game::ArcadeShell::Action::None);
        CHECK(introShell.screen() == game::ArcadeShell::Screen::Intro);
        idleIntroControls.transportButton = pressedEdge();
        if (title == game::ArcadeTitle::Beatknecht) {
            CHECK(introShell.update(idleIntroControls, 60'101U) == game::ArcadeShell::Action::StartRun);
            CHECK(introShell.screen() == game::ArcadeShell::Screen::Playing);
        } else {
            CHECK(introShell.update(idleIntroControls, 60'101U) == game::ArcadeShell::Action::None);
            CHECK(introShell.screen() == game::ArcadeShell::Screen::Intro);
        }
    }

    // Shared arcade shell: individual intro -> play -> qualifying initials -> Top 100 -> restart.
    game::ArcadeShell shell(display, game::ArcadeTitle::PixelRaid, &pixelLeaderboard);
    shell.begin(100U);
    shell.render(900U);
    CHECK(shell.screen() == game::ArcadeShell::Screen::Intro);
    hal::ControlSample shellControls{};
    shellControls.tapButton = pressedEdge();
    CHECK(shell.update(shellControls, 901U) == game::ArcadeShell::Action::StartRun);
    CHECK(shell.playing());
    shell.finishRun(9000U);
    CHECK(shell.screen() == game::ArcadeShell::Screen::NameEntry);
    shellControls = {};
    shellControls.encoderDelta = -1;
    CHECK(shell.update(shellControls, 902U) == game::ArcadeShell::Action::None);
    shell.render(903U);
    shellControls.encoderDelta = 2;
    CHECK(shell.update(shellControls, 904U) == game::ArcadeShell::Action::None);
    shell.render(905U);
    for (int letter = 0; letter < 3; ++letter) {
        shellControls = {};
        shellControls.encoderButton = pressedEdge();
        (void)shell.update(shellControls, 910U + static_cast<std::uint32_t>(letter));
    }
    CHECK(shell.screen() == game::ArcadeShell::Screen::Leaderboard);
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 9000U);
    shell.render(920U);
    shellControls = {};
    shellControls.resetButton = pressedEdge();
    CHECK(shell.update(shellControls, 930U) == game::ArcadeShell::Action::RestartRun);
    CHECK(shell.playing());

    const std::array<char, 4U> aaa{{'A','A','A','\0'}};
    CHECK(pixelLeaderboard.insertAndSave(4500U, aaa) >= 0);
    CHECK(pixelLeaderboard.insertAndSave(4000U, aaa) >= 0);
    CHECK(pixelLeaderboard.insertAndSave(3500U, aaa) >= 0);
    CHECK(pixelLeaderboard.insertAndSave(3000U, aaa) >= 0);
    game::ArcadeShell scrollShell(display, game::ArcadeTitle::PixelRaid, &pixelLeaderboard);
    scrollShell.begin(940U);
    scrollShell.forceLeaderboardForTest(0U, -1);
    scrollShell.render(941U);
    const auto firstLeaderboardFrame = display.framebufferForTest();
    shellControls = {};
    shellControls.encoderDelta = 1;
    CHECK(scrollShell.update(shellControls, 942U) == game::ArcadeShell::Action::None);
    scrollShell.render(943U);
    CHECK(firstLeaderboardFrame != display.framebufferForTest());
    shellControls.encoderDelta = -100;
    CHECK(scrollShell.update(shellControls, 944U) == game::ArcadeShell::Action::None);
    shellControls.encoderDelta = 100;
    CHECK(scrollShell.update(shellControls, 945U) == game::ArcadeShell::Action::None);

    // Non-qualifying/empty and no-leaderboard paths remain stable and renderable.
    game::ArcadeShell emptyShell(display, game::ArcadeTitle::Breakout, &breakoutLeaderboard);
    emptyShell.begin(950U);
    hal::ControlSample emptyControls{};
    emptyControls.encoderButton = pressedEdge();
    CHECK(emptyShell.update(emptyControls, 951U) == game::ArcadeShell::Action::StartRun);
    emptyShell.finishRun(0U);
    CHECK(emptyShell.screen() == game::ArcadeShell::Screen::Leaderboard);
    emptyShell.render(952U);
    emptyControls = {};
    emptyControls.encoderDelta = 1;
    CHECK(emptyShell.update(emptyControls, 953U) == game::ArcadeShell::Action::None);

    game::ArcadeShell noStoreShell(display, static_cast<game::ArcadeTitle>(99U), nullptr);
    noStoreShell.begin(960U);
    noStoreShell.render(961U);
    noStoreShell.finishRun(123U);
    CHECK(noStoreShell.screen() == game::ArcadeShell::Screen::Intro);
    emptyControls = {};
    emptyControls.encoderButton = pressedEdge();
    CHECK(noStoreShell.update(emptyControls, 962U) == game::ArcadeShell::Action::StartRun);
    CHECK_EQ(noStoreShell.highScore(), 0U);

    game::PixelRaidGame pixelRaid(display, controls, gates, pixelLeaderboard);

    // Host run executes the real reset/render path without entering a blocking game loop.
    pixelRaid.run();
    game::PixelRaidGameTestAccess::reset(pixelRaid);
    game::PixelRaidGameTestAccess::render(pixelRaid);
    CHECK(game::PixelRaidGameTestAccess::chooseEnemyShotX(pixelRaid) >= 0);
    CHECK(game::PixelRaidGameTestAccess::hitAlien(pixelRaid, 7, 13));
    CHECK(!game::PixelRaidGameTestAccess::hitAlien(pixelRaid, 127, 63));

    hal::ControlSample sample{};
    sample.encoderDelta = 1;
    sample.tapButton = pressedEdge();
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 1000U);
    // A second TAP while the player projectile is active must not spawn another shot.
    sample = {}; sample.tapButton = pressedEdge();
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 1010U);
    sample = {};
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 2000U);
    game::PixelRaidGameTestAccess::forcePlayerShot(pixelRaid, 7, 15);
    game::PixelRaidGameTestAccess::clearAliensAndAdvance(pixelRaid, 3000U);

    // Exercise inactive/expired/missed shot and alien-motion branches.
    game::PixelRaidGameTestAccess::setPlayerShot(pixelRaid, false, 0, 0);
    game::PixelRaidGameTestAccess::updatePlayerShot(pixelRaid);
    game::PixelRaidGameTestAccess::setPlayerShot(pixelRaid, true, 120, 10);
    game::PixelRaidGameTestAccess::updatePlayerShot(pixelRaid);
    game::PixelRaidGameTestAccess::setPlayerShot(pixelRaid, true, 120, 30);
    game::PixelRaidGameTestAccess::updatePlayerShot(pixelRaid);
    game::PixelRaidGameTestAccess::setEnemyShot(pixelRaid, false, 0, 0, 4000U);
    game::PixelRaidGameTestAccess::updateEnemyShot(pixelRaid, 4500U);
    game::PixelRaidGameTestAccess::updateEnemyShot(pixelRaid, 6000U);
    game::PixelRaidGameTestAccess::setEnemyShot(pixelRaid, true, 0, 63);
    game::PixelRaidGameTestAccess::updateEnemyShot(pixelRaid, 6100U);
    game::PixelRaidGameTestAccess::setEnemyShot(pixelRaid, true, 0, 20);
    game::PixelRaidGameTestAccess::updateEnemyShot(pixelRaid, 6200U);
    game::PixelRaidGameTestAccess::setAlienMotion(pixelRaid, 7, 1, 7000U);
    game::PixelRaidGameTestAccess::updateAliens(pixelRaid, 7100U);
    game::PixelRaidGameTestAccess::setAlienMotion(pixelRaid, 120, 1, 0U);
    game::PixelRaidGameTestAccess::updateAliens(pixelRaid, 8000U);
    game::PixelRaidGameTestAccess::setAlienMotion(pixelRaid, 20, 1, 0U);
    game::PixelRaidGameTestAccess::updateAliens(pixelRaid, 9000U);
    game::PixelRaidGameTestAccess::setAlienAlive(pixelRaid, 0U, false);
    game::PixelRaidGameTestAccess::setPlayerShot(pixelRaid, true, 60, 30);
    game::PixelRaidGameTestAccess::setEnemyShot(pixelRaid, true, 10, 30);
    game::PixelRaidGameTestAccess::render(pixelRaid);
    game::PixelRaidGameTestAccess::clearAliens(pixelRaid);
    CHECK_EQ(game::PixelRaidGameTestAccess::chooseEnemyShotX(pixelRaid), 64);

    game::PixelRaidGameTestAccess::flashLifeLost(pixelRaid);
    for (const auto pin : pinmap::kGateChannelPins) CHECK_EQ(fakefw::pinValues[pin], LOW);
    for (const std::uint32_t pin : pinmap::kGateChannelPins) CHECK_EQ(fakefw::pinValues[pin], LOW);

    CHECK(game::PixelRaidGameTestAccess::confirmExit(pixelRaid));

    game::PixelRaidGameTestAccess::reset(pixelRaid);
    game::PixelRaidGameTestAccess::forceEnemyHit(pixelRaid, 2U, 10U, 4000U);
    CHECK_EQ(game::PixelRaidGameTestAccess::lives(pixelRaid), 1U);
    game::PixelRaidGameTestAccess::forceEnemyHit(pixelRaid, 1U, 10U, 5000U);
    CHECK(game::PixelRaidGameTestAccess::gameOver(pixelRaid));
    game::PixelRaidGameTestAccess::render(pixelRaid);
    sample = {};
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 5500U);
    CHECK(game::PixelRaidGameTestAccess::gameOver(pixelRaid));

    // Final death is handed to the shared arcade shell; gameplay no longer self-restarts.
    sample = {};
    sample.tapButton = pressedEdge();
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 6000U);
    CHECK(game::PixelRaidGameTestAccess::gameOver(pixelRaid));

    // Alternative compile-time Easter eggs share the same safe boot-only HAL boundary.
    game::Formula1Game formula1(display, controls, gates, formulaLeaderboard);
    formula1.run();
    CHECK(std::any_of(display.framebufferForTest().begin(), display.framebufferForTest().end(), [](const std::uint8_t value) { return value != 0U; }));
    for (const auto pin : pinmap::kGateChannelPins) CHECK_EQ(fakefw::pinValues[pin], LOW);

    // Formula 1 regression: world motion must stay encoder-playable, curves must alter the frame,
    // and lane dashes must never escape above the horizon into the central sky region.
    game::Formula1GameTestAccess::reset(formula1);
    hal::ControlSample formulaControls{};
    for (std::uint32_t nowMs = 55U; nowMs <= 5500U; nowMs += 55U) {
        game::Formula1GameTestAccess::update(formula1, formulaControls, nowMs);
    }
    CHECK(game::Formula1GameTestAccess::speed(formula1) <= 184U);
    CHECK(game::Formula1GameTestAccess::distance(formula1) > 250U);
    CHECK(game::Formula1GameTestAccess::distance(formula1) < 650U);
    game::Formula1GameTestAccess::setDistance(formula1, 0U);
    game::Formula1GameTestAccess::render(formula1);
    const auto straightFormulaFrame = display.framebufferForTest();
    game::Formula1GameTestAccess::setDistance(formula1, 700U);
    game::Formula1GameTestAccess::render(formula1);
    CHECK(display.framebufferForTest() != straightFormulaFrame);
    const auto& curveFrame = display.framebufferForTest();
    bool centralSkyIsClear = true;
    for (std::int16_t y = 4; y < 12; ++y) {
        for (std::int16_t x = 40; x < 96; ++x) {
            const std::size_t index = static_cast<std::size_t>(x) + static_cast<std::size_t>(y / 8) * static_cast<std::size_t>(hal::OledDisplay::kWidth);
            const std::uint8_t mask = static_cast<std::uint8_t>(1U << static_cast<std::uint8_t>(y & 7));
            if ((curveFrame[index] & mask) != 0U) centralSkyIsClear = false;
        }
    }
    CHECK(centralSkyIsClear);

    game::Formula1GameTestAccess::reset(formula1);
    game::Formula1GameTestAccess::forceCrash(formula1, 1000U);
    CHECK(game::Formula1GameTestAccess::crashed(formula1));
    CHECK_EQ(game::Formula1GameTestAccess::crashesRemaining(formula1), 2U);
    game::Formula1GameTestAccess::render(formula1);
    const auto crashFrame = display.framebufferForTest();
    game::Formula1GameTestAccess::update(formula1, formulaControls, 1900U);
    CHECK(!game::Formula1GameTestAccess::crashed(formula1));
    game::Formula1GameTestAccess::finishScore(formula1, 4321U);
    CHECK_EQ(formulaLeaderboard.qualifyingRank(4321U, formulaLeaderboard.load()), 0);
    CHECK(std::any_of(crashFrame.begin(), crashFrame.end(), [](const std::uint8_t value) { return value != 0U; }));

    game::BreakoutGame breakout(display, controls, gates, breakoutLeaderboard);
    breakout.run();
    CHECK(std::any_of(display.framebufferForTest().begin(), display.framebufferForTest().end(), [](const std::uint8_t value) { return value != 0U; }));
    for (const auto pin : pinmap::kGateChannelPins) CHECK_EQ(fakefw::pinValues[pin], LOW);

    // Breakout paddle zones must produce different rebound angles, and modifiers change the bat geometry.
    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setPaddleX(breakout, 50);
    game::BreakoutGameTestAccess::setBall(breakout, 52, 56, 1, 1);
    game::BreakoutGameTestAccess::update(breakout, formulaControls, 35U);
    const std::int8_t leftBounceX = game::BreakoutGameTestAccess::velocityX(breakout);
    const std::int8_t leftBounceY = game::BreakoutGameTestAccess::velocityY(breakout);
    game::BreakoutGameTestAccess::setBall(breakout, 68, 56, -1, 1);
    game::BreakoutGameTestAccess::update(breakout, formulaControls, 70U);
    CHECK_NE(game::BreakoutGameTestAccess::velocityX(breakout), leftBounceX);
    CHECK(leftBounceY < 0);
    CHECK(game::BreakoutGameTestAccess::velocityY(breakout) < 0);
    game::BreakoutGameTestAccess::applySmall(breakout, 100U);
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 12);
    game::BreakoutGameTestAccess::applyLarge(breakout, 200U);
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 30);
    game::BreakoutGameTestAccess::applyFast(breakout, 300U);
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 20);

    // Breakout deterministic edge matrix: launch/debounce, all walls, brick hit,
    // falling extras, lost balls, level clear, game-over guard, and render variants.
    game::BreakoutGameTestAccess::reset(breakout);
    hal::ControlSample breakoutControls{};
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 10U);
    CHECK(!game::BreakoutGameTestAccess::launched(breakout));
    breakoutControls.encoderDelta = -50;
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 11U);
    CHECK(game::BreakoutGameTestAccess::ballX(breakout) >= 2);
    breakoutControls = {};
    breakoutControls.tapButton = pressedEdge();
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 12U);
    CHECK(game::BreakoutGameTestAccess::launched(breakout));

    game::BreakoutGameTestAccess::setBall(breakout, 2, 30, -2, -1);
    game::BreakoutGameTestAccess::substep(breakout, 40U, true, false);
    CHECK(game::BreakoutGameTestAccess::velocityX(breakout) > 0);
    game::BreakoutGameTestAccess::setBall(breakout, 125, 30, 2, -1);
    game::BreakoutGameTestAccess::substep(breakout, 41U, true, false);
    CHECK(game::BreakoutGameTestAccess::velocityX(breakout) < 0);
    game::BreakoutGameTestAccess::setBall(breakout, 30, 2, 1, -2);
    game::BreakoutGameTestAccess::substep(breakout, 42U, false, true);
    CHECK(game::BreakoutGameTestAccess::velocityY(breakout) > 0);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::clearBricks(breakout);
    game::BreakoutGameTestAccess::setBrick(breakout, 0U, true);
    game::BreakoutGameTestAccess::setBall(breakout, 5, 5, 1, 1);
    game::BreakoutGameTestAccess::substep(breakout, 43U, false, false);
    CHECK(!game::BreakoutGameTestAccess::brick(breakout, 0U));
    CHECK_EQ(game::BreakoutGameTestAccess::score(breakout), 10U);

    // Exercise deterministic modifier spawn suppression and all three selector values.
    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setFalling(breakout, 1U, 10, 10, true);
    game::BreakoutGameTestAccess::maybeSpawn(breakout, 20, 20);
    CHECK(game::BreakoutGameTestAccess::fallingActive(breakout));
    std::array<bool, 4U> seenExtra{};
    for (std::uint32_t seed = 1U; seed < 20000U && !(seenExtra[1] && seenExtra[2] && seenExtra[3]); ++seed) {
        game::BreakoutGameTestAccess::reset(breakout);
        game::BreakoutGameTestAccess::setRandomState(breakout, seed);
        game::BreakoutGameTestAccess::maybeSpawn(breakout, 64, 20);
        if (game::BreakoutGameTestAccess::fallingActive(breakout)) {
            const std::uint8_t type = game::BreakoutGameTestAccess::fallingType(breakout);
            if (type < seenExtra.size()) seenExtra[type] = true;
        }
    }
    CHECK(seenExtra[1] && seenExtra[2] && seenExtra[3]);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setPaddleX(breakout, 50);
    game::BreakoutGameTestAccess::setFalling(breakout, 2U, 55, 56, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 1000U);
    CHECK(!game::BreakoutGameTestAccess::fallingActive(breakout));
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 30);
    game::BreakoutGameTestAccess::setFalling(breakout, 3U, 5, 63, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 1001U);
    CHECK(!game::BreakoutGameTestAccess::fallingActive(breakout));
    game::BreakoutGameTestAccess::updateExtra(breakout, 9001U);
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 20);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setLives(breakout, 2U);
    game::BreakoutGameTestAccess::setBall(breakout, 20, 64, 1, 2);
    game::BreakoutGameTestAccess::substep(breakout, 1100U, false, false);
    CHECK_EQ(game::BreakoutGameTestAccess::lives(breakout), 1U);
    CHECK(!game::BreakoutGameTestAccess::gameOver(breakout));
    game::BreakoutGameTestAccess::setLives(breakout, 1U);
    game::BreakoutGameTestAccess::setBall(breakout, 20, 64, 1, 2);
    game::BreakoutGameTestAccess::substep(breakout, 1101U, false, false);
    CHECK_EQ(game::BreakoutGameTestAccess::lives(breakout), 0U);
    CHECK(game::BreakoutGameTestAccess::gameOver(breakout));
    breakoutControls = {};
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 1200U);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::clearBricks(breakout);
    game::BreakoutGameTestAccess::setLevel(breakout, 98U);
    game::BreakoutGameTestAccess::setBall(breakout, 64, 40, 1, -1, true);
    game::BreakoutGameTestAccess::setLastPhysicsAt(breakout, 0U);
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 2000U);
    CHECK_EQ(game::BreakoutGameTestAccess::level(breakout), 99U);
    CHECK(!game::BreakoutGameTestAccess::launched(breakout));
    game::BreakoutGameTestAccess::clearBricks(breakout);
    game::BreakoutGameTestAccess::setBall(breakout, 64, 40, 1, -1, true);
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 2040U);
    CHECK_EQ(game::BreakoutGameTestAccess::level(breakout), 99U);

    for (std::uint8_t type = 1U; type <= 3U; ++type) {
        game::BreakoutGameTestAccess::reset(breakout);
        game::BreakoutGameTestAccess::setFalling(breakout, type, 64, 30, true);
        game::BreakoutGameTestAccess::render(breakout);
        game::BreakoutGameTestAccess::setBall(breakout, 64, 40, 1, -1, true);
        game::BreakoutGameTestAccess::render(breakout);
    }

    // Remaining collision/control branches: paddle near-misses, already-launched
    // encoder motion, zero-life underflow guard, substep ratios, and empty render.
    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setPaddleX(breakout, 50);
    game::BreakoutGameTestAccess::setFalling(breakout, 1U, 49, 55, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 3000U);
    CHECK(game::BreakoutGameTestAccess::fallingActive(breakout));
    game::BreakoutGameTestAccess::setFalling(breakout, 1U, 71, 55, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 3001U);
    CHECK(game::BreakoutGameTestAccess::fallingActive(breakout));
    game::BreakoutGameTestAccess::setFalling(breakout, 1U, 5, 63, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 3002U);
    CHECK(!game::BreakoutGameTestAccess::fallingActive(breakout));

    game::BreakoutGameTestAccess::setBall(breakout, 55, 56, 1, 1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3008U, false, false);
    game::BreakoutGameTestAccess::setBall(breakout, 55, 61, 1, 1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3009U, false, false);
    game::BreakoutGameTestAccess::setBall(breakout, 55, 55, 1, -1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3010U, false, false);
    game::BreakoutGameTestAccess::setBall(breakout, 49, 55, 1, 1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3011U, false, false);
    game::BreakoutGameTestAccess::setBall(breakout, 71, 55, 1, 1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3012U, false, false);

    breakoutControls = {};
    breakoutControls.encoderDelta = 1;
    const std::int16_t launchedBallBefore = game::BreakoutGameTestAccess::ballX(breakout);
    game::BreakoutGameTestAccess::setLastPhysicsAt(breakout, 3013U);
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 3013U);
    CHECK_EQ(game::BreakoutGameTestAccess::ballX(breakout), launchedBallBefore);
    game::BreakoutGameTestAccess::setBall(breakout, 20, 64, 1, 2, true);
    game::BreakoutGameTestAccess::setLives(breakout, 0U);
    game::BreakoutGameTestAccess::substep(breakout, 3014U, false, false);
    CHECK_EQ(game::BreakoutGameTestAccess::lives(breakout), 0U);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setLastPhysicsAt(breakout, 0U);
    game::BreakoutGameTestAccess::update(breakout, {}, 40U);
    CHECK(!game::BreakoutGameTestAccess::launched(breakout));
    game::BreakoutGameTestAccess::setBall(breakout, 80, 40, 2, 1, true);
    game::BreakoutGameTestAccess::setLastPhysicsAt(breakout, 0U);
    game::BreakoutGameTestAccess::update(breakout, {}, 40U);
    game::BreakoutGameTestAccess::setBall(breakout, 80, 40, 1, 2, true);
    game::BreakoutGameTestAccess::update(breakout, {}, 80U);
    game::BreakoutGameTestAccess::clearBricks(breakout);
    game::BreakoutGameTestAccess::render(breakout);

    // BEATKNECHT: TAP cycles styles, encoder adjusts tempo, all eight
    // gate rows are rendered, and the step engine drives real gate source pins.
    game::Beatknecht drummer(display, controls, gates);
    drummer.run();
    CHECK(std::any_of(display.framebufferForTest().begin(), display.framebufferForTest().end(), [](const std::uint8_t value) { return value != 0U; }));
    game::BeatknechtTestAccess::reset(drummer, 0U);
    CHECK_EQ(game::BeatknechtTestAccess::bpm(drummer), 128U);
    hal::ControlSample drumControls{};
    drumControls.tapButton = pressedEdge();
    game::BeatknechtTestAccess::update(drummer, drumControls, 1U);
    CHECK_EQ(game::BeatknechtTestAccess::style(drummer), 1U);
    CHECK_EQ(game::BeatknechtTestAccess::bpm(drummer), 128U);
    drumControls = {};
    drumControls.encoderDelta = 6;
    game::BeatknechtTestAccess::update(drummer, drumControls, 2U);
    CHECK_EQ(game::BeatknechtTestAccess::bpm(drummer), 134U);
    game::BeatknechtTestAccess::reset(drummer, 0U);
    drumControls = {};
    drumControls.transportButton = pressedEdge();
    game::BeatknechtTestAccess::update(drummer, drumControls, 100U);
    CHECK_EQ(game::BeatknechtTestAccess::transport(drummer), TransportState::Playing);
    CHECK_EQ(game::BeatknechtTestAccess::step(drummer), 0U);
    CHECK_EQ(fakefw::pinValues[pinmap::kChannel1GateLedPin], HIGH);
    drumControls = {}; drumControls.transportButton = pressedEdge();
    game::BeatknechtTestAccess::update(drummer, drumControls, 150U);
    CHECK_EQ(game::BeatknechtTestAccess::transport(drummer), TransportState::Paused);
    CHECK_EQ(fakefw::pinValues[pinmap::kChannel1GateLedPin], LOW);
    drumControls = {}; drumControls.resetButton = pressedEdge();
    game::BeatknechtTestAccess::update(drummer, drumControls, 200U);
    CHECK_EQ(game::BeatknechtTestAccess::transport(drummer), TransportState::Stopped);
    for (const auto pin : pinmap::kGateChannelPins) CHECK_EQ(fakefw::pinValues[pin], LOW);

    // Egg Journey: the terrain auto-scrolls, encoder changes the egg's screen position,
    // TAP jumps, failures consume three lives, TAP RETRY requires a fresh press, and the
    // high score owns its own slot.
    game::EggJourneyGame eggJourney(display, controls, gates, eggLeaderboard);
    eggJourney.run();
    CHECK(std::any_of(display.framebufferForTest().begin(), display.framebufferForTest().end(), [](const std::uint8_t value) { return value != 0U; }));
    for (const auto pin : pinmap::kGateChannelPins) CHECK_EQ(fakefw::pinValues[pin], LOW);
    game::EggJourneyGameTestAccess::reset(eggJourney);
    const std::int32_t cameraBefore = game::EggJourneyGameTestAccess::cameraX(eggJourney);
    const std::int16_t screenBefore = game::EggJourneyGameTestAccess::screenX(eggJourney);
    hal::ControlSample moonControls{};
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 45U);
    CHECK(game::EggJourneyGameTestAccess::cameraX(eggJourney) > cameraBefore);
    CHECK_EQ(game::EggJourneyGameTestAccess::screenX(eggJourney), screenBefore);
    game::EggJourneyGameTestAccess::reset(eggJourney);
    game::EggJourneyGameTestAccess::setWorldX(eggJourney, 3700);
    const std::int32_t advancedCameraBefore = game::EggJourneyGameTestAccess::cameraX(eggJourney);
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 45U);
    CHECK(game::EggJourneyGameTestAccess::cameraX(eggJourney) - advancedCameraBefore >= 2);
    game::EggJourneyGameTestAccess::reset(eggJourney);
    moonControls = {};
    moonControls.encoderDelta = 2;
    moonControls.tapButton = pressedEdge();
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 90U);
    CHECK(game::EggJourneyGameTestAccess::screenX(eggJourney) > screenBefore);
    CHECK(game::EggJourneyGameTestAccess::jumpHeight(eggJourney) > 0);
    game::EggJourneyGameTestAccess::spawnAsteroid(eggJourney, 1000U);
    CHECK(game::EggJourneyGameTestAccess::asteroidActive(eggJourney));
    game::EggJourneyGameTestAccess::render(eggJourney);
    const auto moonActionFrame = display.framebufferForTest();
    CHECK(std::any_of(moonActionFrame.begin(), moonActionFrame.end(), [](const std::uint8_t value) { return value != 0U; }));

    game::EggJourneyGameTestAccess::reset(eggJourney);
    CHECK_EQ(game::EggJourneyGameTestAccess::lives(eggJourney), 3U);
    CHECK(game::EggJourneyGameTestAccess::craterDepth(eggJourney, -18) >= 3);
    game::EggJourneyGameTestAccess::setWorldX(eggJourney, -18);
    moonControls = {};
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 45U);
    CHECK(!game::EggJourneyGameTestAccess::gameOver(eggJourney));
    CHECK(game::EggJourneyGameTestAccess::awaitingRetry(eggJourney));
    CHECK_EQ(game::EggJourneyGameTestAccess::lives(eggJourney), 2U);
    game::EggJourneyGameTestAccess::render(eggJourney);
    const auto brokenEggFrame = display.framebufferForTest();

    // TAP RETRY is explicitly release-armed, then accepts a new press.
    moonControls = {};
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 46U);
    moonControls.tapButton = pressedEdge();
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 47U);
    CHECK(!game::EggJourneyGameTestAccess::awaitingRetry(eggJourney));
    CHECK(!game::EggJourneyGameTestAccess::gameOver(eggJourney));
    CHECK_EQ(game::EggJourneyGameTestAccess::lives(eggJourney), 2U);

    game::EggJourneyGameTestAccess::forceFailure(eggJourney, true);
    CHECK_EQ(game::EggJourneyGameTestAccess::lives(eggJourney), 1U);
    CHECK(game::EggJourneyGameTestAccess::awaitingRetry(eggJourney));
    moonControls = {};
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 48U);
    moonControls.tapButton = pressedEdge();
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 49U);
    CHECK(!game::EggJourneyGameTestAccess::awaitingRetry(eggJourney));

    game::EggJourneyGameTestAccess::forceFailure(eggJourney, true);
    CHECK_EQ(game::EggJourneyGameTestAccess::lives(eggJourney), 0U);
    CHECK(game::EggJourneyGameTestAccess::gameOver(eggJourney));
    game::EggJourneyGameTestAccess::render(eggJourney);
    CHECK(display.framebufferForTest() != brokenEggFrame);

    // Final GAME OVER is handed to the shared arcade results flow, not the retry state.
    moonControls = {};
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 50U);
    moonControls.tapButton = pressedEdge();
    game::EggJourneyGameTestAccess::update(eggJourney, moonControls, 51U);
    CHECK(game::EggJourneyGameTestAccess::gameOver(eggJourney));
    CHECK_EQ(game::EggJourneyGameTestAccess::lives(eggJourney), 0U);

    game::EggJourneyGameTestAccess::reset(eggJourney);
    game::EggJourneyGameTestAccess::setScore(eggJourney, 7777U);
    CHECK_EQ(eggLeaderboard.qualifyingRank(7777U, eggLeaderboard.load()), 0);
    CHECK_EQ(formulaLeaderboard.load().entries[0].score, 2468U);
    CHECK_EQ(scoreStore.load().score, 1234U);

    const std::array<char, 4U> ids{{'I','D','S','\0'}};
    CHECK(formulaLeaderboard.insertAndSave(8000U, ids) >= 0);
    CHECK(breakoutLeaderboard.insertAndSave(8000U, ids) >= 0);
    CHECK(eggLeaderboard.insertAndSave(8000U, ids) >= 0);
}
