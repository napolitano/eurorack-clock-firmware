/**
 * @file rendering_ui.inc
 * @brief Included test cases split from test_host_firmware/test_main.cpp for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

void testRenderEveryScreenAndState() {
    resetFakes(); prepareDisplaySuccess(); hal::OledDisplay display; CHECK(display.begin());
    ClockState state=makeDefaultState();
    hal::GateOutputDriver gates; gates.beginDisabled(); engine::ClockEngine engine(gates); engine.begin(state); engine.play();
    hal::PersistentStorage storage; hal::PersistentStorage::resetForTest(); services::PersistentStateService persistentState(storage); persistentState.begin();
    services::CustomGrooveStore customGrooveStore(storage);
    ui::UiRenderer renderer(display, persistentState, customGrooveStore); ui::NavigationState nav{};

    for(auto transport:{TransportState::Playing,TransportState::Paused,TransportState::Stopped}){
        state.transport=transport;
        for(auto mode:{ChannelMode::Off,ChannelMode::Clock,ChannelMode::Euclid,ChannelMode::Sequencer}){
            state.channels[0].common.mode=mode; state.channels[0].common.swingPercent=18U;
            for(std::uint64_t phase:{0ULL,core::kQ32One/3ULL,core::kQ32One/2ULL,core::kQ32One*7ULL/8ULL}){
                auto snap=engine.snapshot(); snap.masterBeatPhaseQ32=phase; nav.screen=ui::Screen::Performance; nav.selectedChannel=0U; renderer.render(state,nav,snap);
            }
        }
    }
    nav.screen=ui::Screen::Performance; renderer.render(state,nav,engine.snapshot());

    // Render every master/slave/lock combination used by the compact status line.
    for (const auto source : {ClockSource::Internal, ClockSource::External, ClockSource::Auto}) {
        for (const bool locked : {false, true}) {
            state.source = source;
            auto statusSnapshot = engine.snapshot();
            statusSnapshot.externalLocked = locked;
            renderer.render(state, nav, statusSnapshot);
        }
    }

    // Exercise CLOCK performance-rate formatting and the One Clock humanize icon.
    state.source = ClockSource::Internal;
    state.operatingMode = OperatingMode::Independent;
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 2U, 1U, 1U};
    renderer.render(state, nav, engine.snapshot());
    state.channels[0].common.rate = {ClockRatioMode::Divide, 2U, 1U, 1U};
    renderer.render(state, nav, engine.snapshot());
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 1U, 3U, 2U};
    renderer.render(state, nav, engine.snapshot());
    state.operatingMode = OperatingMode::UnifiedClock;
    state.unifiedClock.humanizeUs = 500U;
    state.unifiedClock.rate = {ClockRatioMode::Multiply, 2U, 1U, 1U};
    renderer.render(state, nav, engine.snapshot());
    state.unifiedClock.humanizeUs = 0U;
    state.operatingMode = OperatingMode::Independent;

    // Exercise defensive and partial-block pattern rendering directly.
    ui::PatternStripRenderer patternStrip(display);
    patternStrip.drawEuclidPattern(EuclidSettings{0U, 0U, 0U}, 0U, 52);
    patternStrip.drawEuclidPattern(EuclidSettings{64U, 32U, 63U}, 63U, 52);
    patternStrip.drawSequencerPlaybackBlock(SequencerSettings{0U, 0U, 0ULL}, 0U, 52);
    patternStrip.drawSequencerPlaybackBlock(SequencerSettings{20U, 0U, 0xAAAAULL}, 18U, 52);
    patternStrip.drawSequencerBlockIndicator(16U, 0U);
    patternStrip.drawSequencerBlockIndicator(17U, 16U);
    patternStrip.drawSequencerBlockIndicator(64U, 255U);

    nav.screen=ui::Screen::ChannelQuickSelect;
    state.operatingMode = OperatingMode::Independent;
    for (std::uint8_t channelIndex = 0U; channelIndex < 8U; ++channelIndex) {
        state.channels[channelIndex].common.mode = static_cast<ChannelMode>(channelIndex % 4U);
        nav.cursor = channelIndex;
        renderer.render(state, nav, engine.snapshot());
    }
    state.operatingMode = OperatingMode::UnifiedClock;
    renderer.render(state, nav, engine.snapshot());
    state.operatingMode = OperatingMode::DividerBank;
    renderer.render(state, nav, engine.snapshot());
    nav.screen = ui::Screen::Performance;
    for (const auto bank : {DividerBank::PowersOfTwo, DividerBank::Integers, DividerBank::Primes}) {
        state.dividerBank.bank = bank;
        renderer.render(state, nav, engine.snapshot());
    }
    state.operatingMode = OperatingMode::UnifiedClock;
    renderer.render(state, nav, engine.snapshot());
    state.operatingMode = OperatingMode::Independent;
    nav.screen = ui::Screen::ModeSelect;
    for (std::size_t modeIndex = 0U; modeIndex < ui::kModeFunctions.size(); ++modeIndex) {
        nav.cursor = static_cast<std::uint8_t>(modeIndex);
        renderer.render(state, nav, engine.snapshot());
    }

    // The carousel keeps the active item centered, inverts only that center
    // slot, and leaves its neighbors unframed on the black background.
    nav.cursor = ui::modeFunctionIndexForState(state, 0U);
    renderer.render(state, nav, engine.snapshot());
    const auto modeCarouselClockFrame = display.framebufferForTest();
    const auto modePixelSet = [&](const auto& frame, const int x, const int y) {
        const std::size_t index = static_cast<std::size_t>(x) +
            static_cast<std::size_t>(y / 8) * static_cast<std::size_t>(hal::OledDisplay::kWidth);
        const std::uint8_t mask = static_cast<std::uint8_t>(1U << (y & 7));
        return (frame[index] & mask) != 0U;
    };
    CHECK(modePixelSet(modeCarouselClockFrame, 43, 14));
    CHECK(modePixelSet(modeCarouselClockFrame, 84, 53));
    CHECK(!modePixelSet(modeCarouselClockFrame, 42, 14));
    CHECK(!modePixelSet(modeCarouselClockFrame, 85, 14));

    nav.cursor = static_cast<std::uint8_t>(
        (nav.cursor + 1U) % ui::kModeFunctions.size());
    renderer.render(state, nav, engine.snapshot());
    const auto modeCarouselNextFrame = display.framebufferForTest();
    CHECK(modeCarouselNextFrame != modeCarouselClockFrame);
    CHECK(modePixelSet(modeCarouselNextFrame, 43, 14));
    CHECK(modePixelSet(modeCarouselNextFrame, 84, 53));

    nav.screen=ui::Screen::ModeChangeConfirm;
    for (const ui::ModeFunction modeFunction : ui::kModeFunctions) {
        nav.pendingModeFunction = modeFunction;
        for (std::uint8_t choice=0U; choice<2U; ++choice) {
            nav.cursor=choice;
            renderer.render(state,nav,engine.snapshot());
        }
    }
    nav.pendingModeFunction = static_cast<ui::ModeFunction>(99U);
    renderer.render(state,nav,engine.snapshot());
    nav.screen=ui::Screen::SequencerEditor; state.channels[0].sequencer={20U,0U,0xAAAAULL}; for(std::uint8_t page=0;page<4U;++page){nav.sequencerPage=page;nav.sequencerCursor=7U;renderer.render(state,nav,engine.snapshot());}

    // Sequencer 2.0 editor visual contract: quarter/beat boundaries are solid,
    // intermediate steps are dotted, edit selection is independent from the
    // playback triangle, expression symbols stay compact, and the bottom page
    // indicator follows the editor viewport rather than the playback page.
    ui::ChannelNavigationRenderer sequencerRenderer(display);
    SequencerPatternV2 editorPattern{};
    editorPattern.length = 128U;
    editorPattern.gates = {{0xFFULL, 0ULL}};
    std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> editorMetadata{};
    SequencerStepMetadata expression{};
    expression.probabilityPercent = 75U;
    editorMetadata[0U] = packSequencerStepMetadata(expression);
    expression = SequencerStepMetadata{};
    expression.gateProfile = SequencerGateProfile::Duty50;
    editorMetadata[1U] = packSequencerStepMetadata(expression);
    expression = SequencerStepMetadata{};
    expression.ratchetCount = 4U;
    editorMetadata[2U] = packSequencerStepMetadata(expression);
    expression.ratchetCount = 8U;
    editorMetadata[3U] = packSequencerStepMetadata(expression);
    expression = SequencerStepMetadata{};
    expression.tie = true;
    editorMetadata[4U] = packSequencerStepMetadata(expression);
    nav.sequencerPage = 0U;
    nav.sequencerCursor = 0U;
    auto editorSnapshot = engine.snapshot();
    editorSnapshot.playing = true;
    editorSnapshot.channelStep[0] = 3U;
    sequencerRenderer.renderSequencerEditor(
        state, nav, editorSnapshot, &editorPattern, 0U, &editorMetadata);
    const auto sequencerEditorFrame = display.framebufferForTest();
    CHECK(modePixelSet(sequencerEditorFrame, 4, 16));   // solid beat boundary
    CHECK(modePixelSet(sequencerEditorFrame, 19, 15));  // dotted step boundary
    CHECK(!modePixelSet(sequencerEditorFrame, 19, 16));
    CHECK(modePixelSet(sequencerEditorFrame, 19, 17));
    CHECK(modePixelSet(sequencerEditorFrame, 5, 20));   // edit-cursor outline
    CHECK(modePixelSet(sequencerEditorFrame, 56, 48));  // playback triangle tip
    CHECK(modePixelSet(sequencerEditorFrame, 53, 43));  // ratchet 8 row 1
    CHECK(modePixelSet(sequencerEditorFrame, 53, 46));  // ratchet 8 row 2

    for (const auto direction : {
             SequencerPlayDirection::Forward,
             SequencerPlayDirection::Reverse,
             SequencerPlayDirection::PingPong,
             SequencerPlayDirection::Random}) {
        editorPattern.direction = direction;
        for (const auto loopMode : {SequencerLoopMode::Loop, SequencerLoopMode::Once}) {
            editorPattern.loopMode = loopMode;
            sequencerRenderer.renderSequencerEditor(
                state, nav, editorSnapshot, &editorPattern, 0U, &editorMetadata);
        }
    }

    nav.sequencerPage = 5U;
    nav.sequencerCursor = 0U;
    editorSnapshot.playing = false;
    editorSnapshot.channelStep[0] = 0U;
    sequencerRenderer.renderSequencerEditor(
        state, nav, editorSnapshot, &editorPattern, 0U, &editorMetadata);
    const auto pageFiveFrame = display.framebufferForTest();
    CHECK(modePixelSet(pageFiveFrame, 40, 62));
    CHECK(!modePixelSet(pageFiveFrame, 0, 62));
    CHECK(modePixelSet(pageFiveFrame, 0, 63));

    // Step-expression editor rendering must cover both inherited/default and
    // explicit values, both fixed-ms and duty gate profiles, RANDOM tie
    // unavailability, and the selected/editing value inversion contract.
    nav.sequencerPage = 0U;
    nav.sequencerCursor = 0U;
    nav.editing = false;
    SequencerStepMetadata stepEditorMetadata{};
    editorPattern.direction = SequencerPlayDirection::Forward;
    editorPattern.gates = {{0x01ULL, 0ULL}};
    for (std::uint8_t row = 0U; row < 5U; ++row) {
        nav.cursor = row;
        sequencerRenderer.renderSequencerStepEditor(
            state, nav, editorPattern, 0U, stepEditorMetadata);
    }

    stepEditorMetadata.probabilityPercent = 64U;
    stepEditorMetadata.gateProfile = SequencerGateProfile::Duty75;
    stepEditorMetadata.ratchetCount = 8U;
    stepEditorMetadata.tie = true;
    for (std::uint8_t row = 1U; row < 5U; ++row) {
        nav.cursor = row;
        nav.editing = true;
        sequencerRenderer.renderSequencerStepEditor(
            state, nav, editorPattern, 0U, stepEditorMetadata);
    }

    stepEditorMetadata.gateProfile = SequencerGateProfile::Trigger10Ms;
    editorPattern.direction = SequencerPlayDirection::Random;
    editorPattern.gates = {{0ULL, 0ULL}};
    nav.cursor = 4U;
    nav.editing = false;
    sequencerRenderer.renderSequencerStepEditor(
        state, nav, editorPattern, 0U, stepEditorMetadata);

    // Defensive/fallback rendering remains defined when only the legacy
    // sequencer state is available and when playback lies outside the edited
    // viewport. These are product fallbacks, not unreachable coverage stubs.
    state.channels[0].sequencer.length = 0U;
    state.channels[0].sequencer.rotation = 63U;
    nav.sequencerPage = 0U;
    nav.sequencerCursor = 0U;
    editorSnapshot.playing = true;
    editorSnapshot.channelStep[0] = 120U;
    sequencerRenderer.renderSequencerEditor(
        state, nav, editorSnapshot, nullptr, 0U, nullptr);
    state.channels[0].sequencer.length = 8U;
    state.channels[0].sequencer.rotation = 0U;
    sequencerRenderer.renderSequencerEditor(
        state, nav, editorSnapshot, nullptr, 0U, nullptr);

    state.operatingMode = OperatingMode::UnifiedClock;
    sequencerRenderer.renderChannelQuickSelect(state, nav);
    state.operatingMode = OperatingMode::DividerBank;
    sequencerRenderer.renderChannelQuickSelect(state, nav);
    state.operatingMode = OperatingMode::Independent;

    nav.screen=ui::Screen::Settings;
    const ui::SettingsPage pages[]={ui::SettingsPage::Root,ui::SettingsPage::General,ui::SettingsPage::InputAssignments,ui::SettingsPage::Hardware,ui::SettingsPage::Master,ui::SettingsPage::Sync,ui::SettingsPage::Preferences,ui::SettingsPage::Screensaver,ui::SettingsPage::Info,ui::SettingsPage::Licenses,ui::SettingsPage::Updates,ui::SettingsPage::Channel,ui::SettingsPage::ChannelTiming,ui::SettingsPage::ChannelOutput,ui::SettingsPage::Clock,ui::SettingsPage::Euclid,ui::SettingsPage::Sequencer,ui::SettingsPage::SequencerPattern,ui::SettingsPage::UnifiedClock,ui::SettingsPage::UnifiedTiming,ui::SettingsPage::UnifiedOutput,ui::SettingsPage::Groove,ui::SettingsPage::GrooveEditorMenu,ui::SettingsPage::GrooveRecordMenu,ui::SettingsPage::DividerBank};
    for(auto page:pages){nav.settingsPage=page;const auto count=ui::settingsPageItemCount(page);for(std::uint8_t row=0;row<count;++row){nav.cursor=row;nav.scrollOffset=row>4U?static_cast<std::uint8_t>(row-4U):0U;nav.editing=(row&1U)!=0U;renderer.render(state,nav,engine.snapshot());}}
    ui::DiagnosticSnapshot diagnostics{};
    diagnostics.syncHigh = true;
    diagnostics.resetHigh = false;
    diagnostics.outputs = {{true,false,true,false,false,true,false,true}};
    nav.settingsPage = ui::SettingsPage::DiagnosticsInputs;
    renderer.render(state, nav, engine.snapshot(), diagnostics);
    const auto inputsFrame = display.framebufferForTest();
    nav.settingsPage = ui::SettingsPage::DiagnosticsOutputs;
    renderer.render(state, nav, engine.snapshot(), diagnostics);
    const auto outputsFrame = display.framebufferForTest();
    CHECK(inputsFrame != outputsFrame);
    nav.settingsPage=ui::SettingsPage::Channel;
    for(const auto mode:{ChannelMode::Off,ChannelMode::Clock,ChannelMode::Euclid,ChannelMode::Sequencer}){state.channels[0].common.mode=mode;const auto count=ui::settingsPageItemCount(ui::SettingsPage::Channel,mode);for(std::uint8_t row=0;row<count;++row){nav.cursor=row;nav.scrollOffset=row>4U?static_cast<std::uint8_t>(row-4U):0U;nav.editing=(row&1U)!=0U;renderer.render(state,nav,engine.snapshot());}}
    nav.screen=ui::Screen::Templates; for(std::uint8_t i=0;i<services::TemplateService::kTemplateCount;++i){nav.cursor=i;nav.scrollOffset=i>3U?static_cast<std::uint8_t>(i-3U):0U;renderer.render(state,nav,engine.snapshot());}
    nav.screen=ui::Screen::PresetSlots; for(auto action:{ui::PresetSlotAction::Load,ui::PresetSlotAction::Save}){nav.presetSlotAction=action;for(std::uint8_t i=0;i<8U;++i){nav.cursor=i;nav.scrollOffset=i>4U?static_cast<std::uint8_t>(i-4U):0U;renderer.render(state,nav,engine.snapshot());}}
    nav.screen=ui::Screen::OverwriteConfirm; nav.selectedPresetSlot=0U; for(std::uint8_t choice=0U;choice<2U;++choice){nav.cursor=choice;renderer.render(state,nav,engine.snapshot());}
    nav.screen=ui::Screen::HighScoreClearConfirm; for(std::uint8_t choice=0U;choice<2U;++choice){nav.cursor=choice;renderer.render(state,nav,engine.snapshot());}
    nav.screen=ui::Screen::FactoryResetConfirm; for(std::uint8_t choice=0U;choice<2U;++choice){nav.cursor=choice;renderer.render(state,nav,engine.snapshot());}
    nav.screen=ui::Screen::NameEntry; nav.presetNameBuffer.fill(' '); nav.presetNameBuffer.back()='\0'; nav.presetNameBuffer[0]='A'; for(std::uint8_t i=0;i<16U;++i){nav.nameCharacterIndex=i;renderer.render(state,nav,engine.snapshot());}

    // Custom Groove graphical/editor surfaces use the production 128x64 renderer.
    nav.grooveDraft = CustomGroovePattern{};
    nav.grooveDraft.length = 16U;
    nav.grooveDraft.offsets256[1] = -64;
    nav.grooveDraft.offsets256[2] = 64;
    nav.grooveCursor = 2U;
    nav.grooveZoomSteps = 0U;
    nav.screen = ui::Screen::GrooveEditor;
    renderer.render(state, nav, engine.snapshot());
    const auto grooveFitFrame = display.framebufferForTest();

    // Dedicated editors use the same title row and separator contract. The
    // Custom Groove grid must begin below that reserved header area and keep
    // the bottom status row independent from the timing canvas.
    CHECK(std::strcmp(text::get(text::TextId::GrooveEditorTitle), "GROOVE EDITOR") == 0);
    const auto groovePixelSet = [&](const int x, const int y) {
        const std::size_t index = static_cast<std::size_t>(x) +
            static_cast<std::size_t>(y / 8) * static_cast<std::size_t>(hal::OledDisplay::kWidth);
        const std::uint8_t mask = static_cast<std::uint8_t>(1U << (y & 7));
        return (grooveFitFrame[index] & mask) != 0U;
    };
    std::size_t grooveTitlePixels = 0U;
    std::size_t grooveRulePixels = 0U;
    std::size_t grooveStatusPixels = 0U;
    for (int y = 0; y < 8; ++y) {
        for (int x = 0; x < hal::OledDisplay::kWidth; ++x) {
            if (groovePixelSet(x, y)) ++grooveTitlePixels;
        }
    }
    for (int x = 0; x < hal::OledDisplay::kWidth; ++x) {
        if (groovePixelSet(x, 9)) ++grooveRulePixels;
    }
    for (int y = 55; y < hal::OledDisplay::kHeight; ++y) {
        for (int x = 0; x < hal::OledDisplay::kWidth; ++x) {
            if (groovePixelSet(x, y)) ++grooveStatusPixels;
        }
    }
    CHECK(grooveTitlePixels > 20U);
    CHECK_EQ(grooveRulePixels, static_cast<std::size_t>(hal::OledDisplay::kWidth));
    CHECK(!groovePixelSet(4, 18));
    CHECK(groovePixelSet(4, 20));
    CHECK(grooveStatusPixels > 0U);

    // An unselected marker gets a one-pixel quiet zone around the diamond.
    // This prevents a solid beat guide from visually touching the diagonal
    // edges and reading as a line through the hollow marker.
    CHECK(groovePixelSet(4, 30));  // grid above the backdrop
    CHECK(!groovePixelSet(4, 31)); // quiet-zone margin
    CHECK(groovePixelSet(4, 32));  // diamond outline
    CHECK(!groovePixelSet(4, 35)); // no grid inside the hollow diamond
    CHECK(groovePixelSet(4, 38));  // diamond outline
    CHECK(!groovePixelSet(4, 39)); // quiet-zone margin
    CHECK(groovePixelSet(4, 40));  // grid resumes below the backdrop

    nav.grooveZoomSteps = 4U;
    renderer.render(state, nav, engine.snapshot());
    CHECK(display.framebufferForTest() != grooveFitFrame);
    nav.grooveDraft.length = 1U;
    nav.grooveCursor = 0U;
    nav.grooveZoomSteps = 4U;
    renderer.render(state, nav, engine.snapshot());

    // Groove Record reuses the same timing grid, adds a moving playhead, and
    // draws only captured markers. Marker quiet zones must remain above the
    // playhead layer so captured timing stays visually unambiguous.
    nav.grooveDraft = CustomGroovePattern{};
    nav.grooveDraft.length = 4U;
    nav.grooveZoomSteps = 4U;
    nav.grooveCursor = 0U;
    nav.grooveRecordCapturedMask = (1ULL << 1U);
    nav.grooveRecordState = ui::GrooveRecordState::Recording;
    nav.grooveRecordPlayheadStep = 0U;
    nav.grooveRecordPlayheadPhase256 = 128U;
    nav.screen = ui::Screen::GrooveRecorder;
    renderer.render(state, nav, engine.snapshot());
    const auto grooveRecordFrame = display.framebufferForTest();
    CHECK(std::strcmp(text::get(text::TextId::GrooveRecordTitle), "GROOVE RECORD") == 0);
    CHECK(modePixelSet(grooveRecordFrame, 23, 25)); // playhead between steps 1 and 2
    CHECK(modePixelSet(grooveRecordFrame, 43, 32)); // captured diamond outline
    CHECK(!modePixelSet(grooveRecordFrame, 43, 35)); // playhead/grid cannot pierce marker

    nav.grooveRecordState = ui::GrooveRecordState::PreCount;
    nav.grooveRecordCountInRemaining = 3U;
    nav.grooveRecordPlayheadStep = 2U;
    nav.grooveRecordPlayheadPhase256 = 64U;
    renderer.render(state, nav, engine.snapshot());
    CHECK(display.framebufferForTest() != grooveRecordFrame);
    nav.grooveRecordState = ui::GrooveRecordState::Ready;
    nav.grooveRecordPlayheadStep = 0U;
    nav.grooveRecordPlayheadPhase256 = 0U;
    renderer.render(state, nav, engine.snapshot());
    CHECK(modePixelSet(display.framebufferForTest(), 4, 25));

    CustomGroovePattern storedGroove{};
    storedGroove.length = 8U;
    storedGroove.offsets256[3] = 24;
    CHECK(customGrooveStore.save(0U, "CUSTOM A", storedGroove));
    nav.screen = ui::Screen::GrooveSlots;
    nav.scrollOffset = 0U;
    nav.cursor = 0U;
    for (const auto action : {
             ui::GrooveSlotAction::LoadEditor,
             ui::GrooveSlotAction::Activate,
             ui::GrooveSlotAction::Save,
             ui::GrooveSlotAction::Rename,
             ui::GrooveSlotAction::Delete}) {
        nav.grooveSlotAction = action;
        renderer.render(state, nav, engine.snapshot());
    }
    nav.scrollOffset = 8U;
    nav.cursor = 9U;
    renderer.render(state, nav, engine.snapshot());
    for (const auto screen : {
             ui::Screen::GrooveOverwriteConfirm,
             ui::Screen::GrooveDeleteConfirm,
             ui::Screen::GrooveDiscardConfirm}) {
        nav.screen = screen;
        for (std::uint8_t choice = 0U; choice < 2U; ++choice) {
            nav.cursor = choice;
            renderer.render(state, nav, engine.snapshot());
        }
    }
    nav.screen = ui::Screen::GrooveNameEntry;
    nav.grooveNameBuffer.fill(' ');
    nav.grooveNameBuffer.back() = '\0';
    nav.grooveNameBuffer[0] = 'A';
    for (std::uint8_t character = 0U; character < 16U; ++character) {
        nav.nameCharacterIndex = character;
        renderer.render(state, nav, engine.snapshot());
    }
    nav.screen = ui::Screen::InformationPopover;
    std::snprintf(nav.informationPopoverTitle.data(), nav.informationPopoverTitle.size(), "%s", "AUTHOR");
    std::snprintf(nav.informationPopoverValue.data(), nav.informationPopoverValue.size(), "%s", "AXEL NAPOLITANO");
    renderer.render(state, nav, engine.snapshot());
    nav.cursor=5U; nav.scrollOffset=5U; renderer.render(state,nav,engine.snapshot());
    renderer.renderBootScreen(0U); renderer.renderBootScreen(config::kBootDurationMs/2U); renderer.renderBootScreen(config::kBootDurationMs+100U);
    nav.screen=static_cast<ui::Screen>(99U); renderer.render(state,nav,engine.snapshot());
}

void controllerShortPress(ui::UiController& controller,std::uint32_t& now){hal::ControlSample s{};s.encoderButton=pressedEdge();controller.processControls(s,now++);s={};s.encoderButton=releasedEdge();controller.processControls(s,now++);}
void controllerLongPress(ui::UiController& controller,std::uint32_t& now){hal::ControlSample s{};s.encoderButton=pressedEdge();controller.processControls(s,now++);s={};s.encoderButton=heldButton();now += config::kEncoderLongPressMs;controller.processControls(s,now++);s={};s.encoderButton=releasedEdge();controller.processControls(s,now++);}
void controllerTapPushChord(ui::UiController& controller,std::uint32_t& now){
    hal::ControlSample s{};
    s.tapButton=pressedEdge();
    controller.processControls(s,now++);
    s={};
    s.tapButton=heldButton();
    s.encoderButton=pressedEdge();
    controller.processControls(s,now++);
    s={};
    s.tapButton=heldButton();
    s.encoderButton=releasedEdge();
    controller.processControls(s,now++);
    s={};
    s.tapButton=releasedEdge();
    controller.processControls(s,now++);
}

void controllerOpenSettingsChord(ui::UiController& controller,std::uint32_t& now){
    hal::ControlSample s{};
    s.tapButton=pressedEdge();
    controller.processControls(s,now++);
    s={};
    s.tapButton=heldButton();
    s.encoderButton=pressedEdge();
    controller.processControls(s,now++);
    s={};
    s.tapButton=heldButton();
    s.encoderButton=releasedEdge();
    controller.processControls(s,now++);
    s={};
    s.tapButton=releasedEdge();
    controller.processControls(s,now++);
}
void controllerReset(ui::UiController& controller,std::uint32_t& now){hal::ControlSample s{};s.resetButton=pressedEdge();controller.processControls(s,now++);}
void controllerTurn(ui::UiController& controller,std::int8_t delta,std::uint32_t& now){hal::ControlSample s{};s.encoderDelta=delta;controller.processControls(s,now++);}
void controllerConfirmModeYes(ui::UiController& controller,std::uint32_t& now){controllerTurn(controller,1,now);controllerShortPress(controller,now);}
void controllerTapAt(ui::UiController& controller,const std::uint32_t releasedAtMs){hal::ControlSample s{};s.tapButton=pressedEdge();controller.processControls(s,releasedAtMs-1U);s={};s.tapButton=releasedEdge();controller.processControls(s,releasedAtMs);}

std::size_t countFramebufferPixels(
    const std::array<std::uint8_t, hal::OledDisplay::kFramebufferSize>& framebuffer,
    const std::int16_t x,
    const std::int16_t y,
    const std::int16_t width,
    const std::int16_t height) {
    std::size_t count = 0U;
    for (std::int16_t row = 0; row < height; ++row) {
        for (std::int16_t column = 0; column < width; ++column) {
            const std::int16_t pixelX = static_cast<std::int16_t>(x + column);
            const std::int16_t pixelY = static_cast<std::int16_t>(y + row);
            const std::size_t index = static_cast<std::size_t>(pixelX) +
                static_cast<std::size_t>(pixelY / 8) * static_cast<std::size_t>(hal::OledDisplay::kWidth);
            const std::uint8_t mask = static_cast<std::uint8_t>(1U << (pixelY & 7));
            if ((framebuffer[index] & mask) != 0U) {
                ++count;
            }
        }
    }
    return count;
}



void testHierarchicalSettingsRenderDistinctRootAndGroupPages() {
    resetFakes();
    prepareDisplaySuccess();

    ClockState state = makeDefaultState();
    state.operatingMode = OperatingMode::Independent;
    state.channels[0].common.mode = ChannelMode::Clock;

    hal::OledDisplay display;
    CHECK(display.begin());
    ui::SettingsRenderer renderer(display);
    ui::NavigationState navigation{};
    navigation.screen = ui::Screen::Settings;
    navigation.selectedChannel = 0U;

    navigation.settingsPage = ui::SettingsPage::Channel;
    navigation.cursor = 1U;
    renderer.renderSettings(state, navigation);
    const auto rootFrame = display.framebufferForTest();

    navigation.settingsPage = ui::SettingsPage::ChannelTiming;
    navigation.cursor = 0U;
    renderer.renderSettings(state, navigation);
    const auto timingFrame = display.framebufferForTest();
    CHECK(rootFrame != timingFrame);

    navigation.settingsPage = ui::SettingsPage::ChannelOutput;
    navigation.cursor = 0U;
    renderer.renderSettings(state, navigation);
    const auto outputFrame = display.framebufferForTest();
    CHECK(timingFrame != outputFrame);

    navigation.settingsPage = ui::SettingsPage::Clock;
    renderer.renderSettings(state, navigation);
    const auto clockFrame = display.framebufferForTest();
    CHECK(outputFrame != clockFrame);
}


void testSettingsSelectionUsesFullRowInversionWithoutLeftCursor() {
    resetFakes();
    prepareDisplaySuccess();
    ClockState state = makeDefaultState();
    hal::OledDisplay display;
    CHECK(display.begin());
    ui::SettingsRenderer renderer(display);
    ui::NavigationState navigation{};
    navigation.screen = ui::Screen::Settings;
    navigation.settingsPage = ui::SettingsPage::General;
    navigation.cursor = 0U;
    renderer.renderSettings(state, navigation);
    const auto selectedFrame = display.framebufferForTest();

    // Row 0 background occupies x=0..123 at y=10..18; the next row has no
    // dedicated left cursor. This locks the recovered horizontal gutter.
    CHECK(countFramebufferPixels(selectedFrame, 0, 10, 2, 9) > 0U);
    CHECK_EQ(countFramebufferPixels(selectedFrame, 0, 20, 2, 9), 0U);

    navigation.editing = true;
    renderer.renderSettings(state, navigation);
    CHECK(display.framebufferForTest() != selectedFrame);
}

void testReadOnlyInformationOverflowUsesPopoverWithoutEditingSemantics() {
    resetFakes();
    prepareDisplaySuccess();
    ClockState state = makeDefaultState();
    hal::OledDisplay display;
    CHECK(display.begin());

    ui::SettingsRenderer settingsRenderer(display);
    ui::NavigationState navigation{};
    navigation.screen = ui::Screen::Settings;
    navigation.settingsPage = ui::SettingsPage::Info;
    navigation.cursor = 0U; // Keep AUTHOR visible but not inverted.
    settingsRenderer.renderSettings(state, navigation);
    const auto authorFrame = display.framebufferForTest();

    // The full value is wider than the space remaining after the AUTHOR label.
    // The rendered row must preserve the configured gap instead of allowing the
    // informational value to collide with its label.
    const ui::MenuRow authorRow = ui::buildMenuRow(ui::SettingsPage::Info, 2U, 0U, state);
    const auto authorLabelBounds = display.measureText(authorRow.label, 0, 0);
    const auto fullAuthorBounds = display.measureText(authorRow.value, 0, 0);
    const std::int16_t authorLabelRight = static_cast<std::int16_t>(2 + authorLabelBounds.width);
    const std::int16_t availableAuthorWidth = static_cast<std::int16_t>(121 - authorLabelRight - 3);
    CHECK(static_cast<std::int16_t>(fullAuthorBounds.width) > availableAuthorWidth);
    CHECK_EQ(countFramebufferPixels(authorFrame, authorLabelRight, 31, 3, 7), 0U);

    hal::GateOutputDriver gates;
    gates.beginDisabled();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::PersistentStateService persistentState(storage);
    persistentState.begin();
    ui::UiRenderer renderer(display, persistentState);
    ui::UiController controller(state, engine, renderer, persistentState);
    std::uint32_t now = 100U;
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 4, now); // INFO
    controllerShortPress(controller, now);
    controllerShortPress(controller, now); // PRODUCT fits and is not expandable in practice
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Info);
    controllerTurn(controller, 2, now); // AUTHOR
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::InformationPopover);
    CHECK(std::strcmp(controller.navigation().informationPopoverTitle.data(), "AUTHOR") == 0);
    CHECK(std::strcmp(controller.navigation().informationPopoverValue.data(), "AXEL NAPOLITANO") == 0);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Info);
    CHECK(!controller.navigation().editing);
}

void testPerformanceRendererShowsActiveGrooveAtModeSpecificPosition() {
    resetFakes();
    prepareDisplaySuccess();

    ClockState state = makeDefaultState();
    state.transport = TransportState::Playing;
    ui::NavigationState navigation{};
    engine::EngineSnapshot snapshot{};

    hal::OledDisplay display;
    CHECK(display.begin());
    ui::PerformanceRenderer renderer(display);

    // Plain CLOCK: the active preset belongs in the far lower-left status area.
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.groove = {GroovePreset::PocketA, 100U, 0U};
    renderer.render(state, navigation, snapshot);
    const auto activeClockFrame = display.framebufferForTest();
    CHECK(countFramebufferPixels(activeClockFrame, 0, 54, 80, 10) > 0U);

    state.channels[0].common.groove.amountPercent = 0U;
    renderer.render(state, navigation, snapshot);
    const auto disabledClockFrame = display.framebufferForTest();
    CHECK(countFramebufferPixels(activeClockFrame, 0, 54, 80, 10) >
          countFramebufferPixels(disabledClockFrame, 0, 54, 80, 10));

    // EUCLID/SEQ keep the bottom row for the pattern strip, so Groove sits above it.
    state.channels[0].common.mode = ChannelMode::Euclid;
    state.channels[0].common.groove = {GroovePreset::PocketB, 100U, 0U};
    renderer.render(state, navigation, snapshot);
    const auto activeEuclidFrame = display.framebufferForTest();
    CHECK(countFramebufferPixels(activeEuclidFrame, 0, 42, 80, 9) > 0U);

    state.channels[0].common.groove.preset = GroovePreset::Off;
    renderer.render(state, navigation, snapshot);
    const auto disabledEuclidFrame = display.framebufferForTest();
    CHECK(countFramebufferPixels(activeEuclidFrame, 0, 42, 80, 9) >
          countFramebufferPixels(disabledEuclidFrame, 0, 42, 80, 9));

    state.channels[0].common.mode = ChannelMode::Sequencer;
    state.channels[0].common.groove = {GroovePreset::PocketC, 100U, 0U};
    renderer.render(state, navigation, snapshot);
    CHECK(countFramebufferPixels(display.framebufferForTest(), 0, 42, 80, 9) > 0U);

    // One Clock uses its global Groove assignment in the same lower-left CLOCK position.
    state.operatingMode = OperatingMode::UnifiedClock;
    state.unifiedClock.groove = {GroovePreset::Swing58, 100U, 0U};
    renderer.render(state, navigation, snapshot);
    CHECK(countFramebufferPixels(display.framebufferForTest(), 0, 54, 80, 10) > 0U);

    // A stored Custom Groove must expose its real user name on Performance, not
    // collapse every library entry to the generic CUSTOM label.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage grooveStorage;
    services::CustomGrooveStore grooveStore(grooveStorage);
    CustomGroovePattern pattern{};
    pattern.length = 16U;
    CHECK(grooveStore.save(0U, "VELVET PUSH", pattern));
    state.unifiedClock.groove = {GroovePreset::Custom, 100U, 0U, 0U};
    renderer.render(state, navigation, snapshot);
    const auto genericCustomFrame = display.framebufferForTest();
    ui::PerformanceRenderer namedRenderer(display, &grooveStore);
    namedRenderer.render(state, navigation, snapshot);
    const auto namedCustomFrame = display.framebufferForTest();
    CHECK(namedCustomFrame != genericCustomFrame);
    CHECK(countFramebufferPixels(namedCustomFrame, 0, 54, 110, 10) >
          countFramebufferPixels(genericCustomFrame, 0, 54, 110, 10));
}

void testPerformanceRendererShowsStaticPreCountPopoverAndMeterProgress() {
    resetFakes(); prepareDisplaySuccess();

    ClockState state = makeDefaultState();
    state.transport = TransportState::Playing;
    state.preCountSteps = 8U;
    state.masterMeter.beats = 4U;
    ui::NavigationState navigation{};
    engine::EngineSnapshot snapshot{};

    hal::OledDisplay display;
    CHECK(display.begin());
    ui::PerformanceRenderer renderer(display);

    renderer.render(state, navigation, snapshot);
    const auto normalFrame = display.framebufferForTest();

    snapshot.preCountActive = true;
    snapshot.preCountRemaining = 8U;
    snapshot.preCountPhaseQ32 = 0U;
    renderer.render(state, navigation, snapshot);
    const auto firstBeatFrame = display.framebufferForTest();
    CHECK(firstBeatFrame != normalFrame);

    // The popover is deliberately static within one beat: engine phase changes must
    // not alter the framebuffer anymore.
    snapshot.preCountPhaseQ32 = (core::kQ32One * 3ULL) / 4ULL;
    renderer.render(state, navigation, snapshot);
    CHECK(display.framebufferForTest() == firstBeatFrame);

    // 4/4 uses one filled active-step cell plus three outlines; the filled marker
    // moves to the next fixed meter position on the next count beat.
    constexpr std::int16_t kStepRowY = 52;
    const auto beatOneFrame = display.framebufferForTest();
    CHECK(countFramebufferPixels(beatOneFrame, 49, kStepRowY, 6, 4) >
          countFramebufferPixels(beatOneFrame, 57, kStepRowY, 6, 4));

    snapshot.preCountRemaining = 7U;
    renderer.render(state, navigation, snapshot);
    const auto secondBeatFrame = display.framebufferForTest();
    CHECK(secondBeatFrame != firstBeatFrame);
    CHECK(countFramebufferPixels(secondBeatFrame, 57, kStepRowY, 6, 4) >
          countFramebufferPixels(beatOneFrame, 57, kStepRowY, 6, 4));
    CHECK(countFramebufferPixels(secondBeatFrame, 49, kStepRowY, 6, 4) <
          countFramebufferPixels(beatOneFrame, 49, kStepRowY, 6, 4));

    // The row must remain representable at the supported maximum 16-beat meter.
    state.masterMeter.beats = 16U;
    renderer.render(state, navigation, snapshot);
    const auto sixteenBeatFrame = display.framebufferForTest();
    CHECK(countFramebufferPixels(sixteenBeatFrame, 37, 51, 54, 7) > 0U);

    state.masterMeter.beats = 4U;
    snapshot.preCountActive = false;
    renderer.render(state, navigation, snapshot);
    CHECK(display.framebufferForTest() == normalFrame);

    // Defensive snapshot contract: an active flag with no remaining count must not
    // draw a stale popover.
    snapshot.preCountActive = true;
    snapshot.preCountRemaining = 0U;
    renderer.render(state, navigation, snapshot);
    CHECK(display.framebufferForTest() == normalFrame);
}

void testPreCountPopoverRedrawsAtBeatBoundariesAndClearsOnCompletion() {
    resetFakes();
    prepareDisplaySuccess();

    hal::OledDisplay display;
    CHECK(display.begin());
    ClockState state = makeDefaultState();
    state.preCountSteps = 2U;
    state.transport = TransportState::Stopped;

    hal::GateOutputDriver gates;
    gates.beginDisabled();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::PersistentStateService persistence(storage);
    persistence.begin();
    ui::UiRenderer renderer(display, persistence);
    ui::UiController controller(state, engine, renderer, persistence);

    std::uint32_t nowMs = 1000U;
    controller.invalidate();
    controller.serviceRendering(nowMs);

    hal::ControlSample sample{};
    sample.transportButton = pressedEdge();
    controller.processControls(sample, ++nowMs);
    CHECK_EQ(state.transport, TransportState::Playing);
    CHECK(engine.snapshot().preCountActive);

    nowMs += config::kDisplayRefreshMinimumMs + 1U;
    controller.serviceRendering(nowMs);
    const auto initialCountFrame = display.framebufferForTest();

    const std::uint32_t ticksPerBeat = static_cast<std::uint32_t>(
        (60ULL * config::kSchedulerFrequencyHz) / state.bpm);
    runEngineTicks(engine, ticksPerBeat / 2U);
    nowMs += 300U;

    // No beat boundary occurred, so the static popover must not request a new frame.
    controller.serviceRendering(nowMs);
    CHECK(display.framebufferForTest() == initialCountFrame);

    runEngineTicks(engine, ticksPerBeat / 2U);
    CHECK(engine.snapshot().preCountRemaining == 1U);
    nowMs += config::kDisplayRefreshMinimumMs + 1U;
    controller.serviceRendering(nowMs);
    const auto secondCountFrame = display.framebufferForTest();
    CHECK(secondCountFrame != initialCountFrame);

    // PAUSE leaves the current count beat frozen. Scheduler ticks cannot advance it.
    sample = {};
    sample.transportButton = pressedEdge();
    controller.processControls(sample, ++nowMs);
    CHECK_EQ(state.transport, TransportState::Paused);
    nowMs += config::kDisplayRefreshMinimumMs + 1U;
    controller.serviceRendering(nowMs);
    const auto pausedFrame = display.framebufferForTest();
    runEngineTicks(engine, ticksPerBeat);
    nowMs += 300U;
    controller.serviceRendering(nowMs);
    CHECK(display.framebufferForTest() == pausedFrame);

    sample = {};
    sample.transportButton = pressedEdge();
    controller.processControls(sample, ++nowMs);
    CHECK_EQ(state.transport, TransportState::Playing);
    runEngineTicks(engine, ticksPerBeat);
    CHECK(!engine.snapshot().preCountActive);

    // Completion is an engine-only state edge. The controller must redraw once more
    // to erase the final popover and return to the normal Performance UI.
    const auto staleOverlayFrame = display.framebufferForTest();
    nowMs += config::kDisplayRefreshMinimumMs + 1U;
    controller.serviceRendering(nowMs);
    CHECK(display.framebufferForTest() != staleOverlayFrame);

    // Rendering must also leave the Performance-only Pre-Count invalidation path
    // cleanly when the user opens ordinary Settings.
    controllerOpenSettingsChord(controller, nowMs);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    nowMs += config::kDisplayRefreshMinimumMs + 1U;
    controller.serviceRendering(nowMs);
}

void testPerformanceRendererShowsMeasuredExternalBpmWhileLockedAndFreewheeling() {
    resetFakes(); prepareDisplaySuccess();

    ClockState actualState = makeDefaultState();
    actualState.source = ClockSource::Auto;
    actualState.bpm = 90U;
    ui::NavigationState navigation{};
    engine::EngineSnapshot lockedSnapshot{};
    lockedSnapshot.externalLocked = true;
    lockedSnapshot.externalBpmMilli = 120000U;

    hal::OledDisplay actualDisplay;
    CHECK(actualDisplay.begin());
    ui::PerformanceRenderer actualRenderer(actualDisplay);
    actualRenderer.render(actualState, navigation, lockedSnapshot);
    const auto lockedExternalFrame = actualDisplay.framebufferForTest();

    ClockState expectedState = actualState;
    expectedState.bpm = 120U;
    hal::OledDisplay expectedDisplay;
    CHECK(expectedDisplay.begin());
    ui::PerformanceRenderer expectedRenderer(expectedDisplay);
    expectedRenderer.render(expectedState, navigation, lockedSnapshot);
    CHECK(lockedExternalFrame == expectedDisplay.framebufferForTest());

    // FREEWHEEL keeps ownership of the last valid external tempo after lock loss.
    // Compare two unlocked frames so the LOCK icon itself cannot affect the assertion.
    engine::EngineSnapshot unlockedSnapshot = lockedSnapshot;
    unlockedSnapshot.externalLocked = false;
    actualState.externalSync.lossMode = SyncLossMode::Freewheel;
    hal::OledDisplay freewheelDisplay;
    CHECK(freewheelDisplay.begin());
    ui::PerformanceRenderer freewheelRenderer(freewheelDisplay);
    freewheelRenderer.render(actualState, navigation, unlockedSnapshot);

    ClockState expectedFreewheelState = actualState;
    expectedFreewheelState.bpm = 120U;
    hal::OledDisplay expectedFreewheelDisplay;
    CHECK(expectedFreewheelDisplay.begin());
    ui::PerformanceRenderer expectedFreewheelRenderer(expectedFreewheelDisplay);
    expectedFreewheelRenderer.render(expectedFreewheelState, navigation, unlockedSnapshot);
    CHECK(freewheelDisplay.framebufferForTest() == expectedFreewheelDisplay.framebufferForTest());

    // INTERNAL loss deliberately hands the display back to the configured fallback BPM.
    actualState.externalSync.lossMode = SyncLossMode::Internal;
    hal::OledDisplay internalLossDisplay;
    CHECK(internalLossDisplay.begin());
    ui::PerformanceRenderer internalLossRenderer(internalLossDisplay);
    internalLossRenderer.render(actualState, navigation, unlockedSnapshot);
    CHECK(freewheelDisplay.framebufferForTest() != internalLossDisplay.framebufferForTest());

    ClockState internalState = actualState;
    internalState.source = ClockSource::Internal;
    internalState.externalSync.lossMode = SyncLossMode::Freewheel;
    hal::OledDisplay internalDisplay;
    CHECK(internalDisplay.begin());
    ui::PerformanceRenderer internalRenderer(internalDisplay);
    internalRenderer.render(internalState, navigation, lockedSnapshot);
    CHECK(lockedExternalFrame != internalDisplay.framebufferForTest());
}

void testTapTempoPreservesClockSourceAndPersistence() {
    for (const ClockSource source : {ClockSource::Internal, ClockSource::External, ClockSource::Auto}) {
        resetFakes();
        prepareDisplaySuccess();
        hal::OledDisplay display;
        CHECK(display.begin());

        ClockState state = makeFactoryState();
        state.source = source;
        state.bpm = 90U;

        hal::GateOutputDriver gates;
        gates.beginDisabled();
        engine::ClockEngine engine(gates);
        engine.begin(state);

        hal::PersistentStorage::resetForTest();
        hal::PersistentStorage storage;
        services::PersistentStateService persistence(storage);
        persistence.begin();
        ui::UiRenderer renderer(display, persistence);
        ui::UiController controller(state, engine, renderer, persistence);

        controllerTapAt(controller, 1000U);
        controllerTapAt(controller, 1500U);

        CHECK_EQ(state.bpm, 120U);
        CHECK_EQ(state.source, source);

        persistence.service(1500U + config::kPersistenceCommitDelayMs);
        ClockState restored{};
        CHECK(persistence.restoreCurrentState(restored));
        CHECK_EQ(restored.bpm, 120U);
        CHECK_EQ(restored.source, source);
    }
}

void testTapIndicatorStartsOnSecondTapAndRestartsEveryFollowingTap() {
    resetFakes(); prepareDisplaySuccess();
    hal::OledDisplay display; CHECK(display.begin());
    ClockState state = makeDefaultState();
    hal::GateOutputDriver gates; gates.beginDisabled();
    engine::ClockEngine engine(gates); engine.begin(state);
    hal::PersistentStorage::resetForTest(); hal::PersistentStorage storage;
    services::PersistentStateService persistence(storage); persistence.begin();
    ui::UiRenderer renderer(display, persistence);
    ui::UiController controller(state, engine, renderer, persistence);

    controllerTapAt(controller, 1000U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 0U);
    controllerTapAt(controller, 1500U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 1U);
    CHECK_EQ(state.bpm, 120U);

    controller.serviceRendering(1580U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 2U);
    controllerTapAt(controller, 2000U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 1U);
    controllerTapAt(controller, 2500U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 1U);
}

void testTapIndicatorRendersFourShrinkingEightPixelFramesThenClears() {
    resetFakes(); prepareDisplaySuccess();
    hal::OledDisplay display; CHECK(display.begin());
    ClockState state = makeDefaultState();
    hal::GateOutputDriver gates; gates.beginDisabled();
    engine::ClockEngine engine(gates); engine.begin(state);
    hal::PersistentStorage::resetForTest(); hal::PersistentStorage storage;
    services::PersistentStateService persistence(storage); persistence.begin();
    ui::UiRenderer renderer(display, persistence);
    ui::UiController controller(state, engine, renderer, persistence);

    controllerTapAt(controller, 1000U);
    controllerTapAt(controller, 1500U);

    display.setFont(hal::DisplayFont::TempoLarge);
    const hal::TextBounds tempoBounds = display.measureText("120", 0, 20);
    const std::int16_t tapX = static_cast<std::int16_t>(hal::OledDisplay::kWidth - 8 - 1);
    const std::int16_t tapY = static_cast<std::int16_t>(20 + (static_cast<int>(tempoBounds.height) - 8) / 2);

    std::array<std::size_t, 4U> pixelCounts{};
    for (std::uint8_t frame = 0U; frame < 4U; ++frame) {
        const std::uint32_t frameTime = 1500U + static_cast<std::uint32_t>(frame) * config::kTapIndicatorFrameDurationMs;
        controller.serviceRendering(frameTime);
        CHECK_EQ(controller.navigation().tapIndicatorFrame, static_cast<std::uint8_t>(frame + 1U));
        pixelCounts[frame] = countFramebufferPixels(display.framebufferForTest(), tapX, tapY, 8, 8);
        CHECK(pixelCounts[frame] > 0U);
        if (frame > 0U) {
            CHECK(pixelCounts[frame] < pixelCounts[frame - 1U]);
        }
    }

    controller.serviceRendering(1500U + 4U * config::kTapIndicatorFrameDurationMs);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 0U);
    controller.serviceRendering(1500U + 4U * config::kTapIndicatorFrameDurationMs + config::kDisplayRefreshMinimumMs);
    CHECK_EQ(countFramebufferPixels(display.framebufferForTest(), tapX, tapY, 8, 8), 0U);
}

void testTapIndicatorSequenceResetsAfterConfiguredMinimumBpmInterval() {
    resetFakes(); prepareDisplaySuccess();
    hal::OledDisplay display; CHECK(display.begin());
    ClockState state = makeDefaultState();
    state.tempoRange.minimumBpm = 20U;
    hal::GateOutputDriver gates; gates.beginDisabled();
    engine::ClockEngine engine(gates); engine.begin(state);
    hal::PersistentStorage::resetForTest(); hal::PersistentStorage storage;
    services::PersistentStateService persistence(storage); persistence.begin();
    ui::UiRenderer renderer(display, persistence);
    ui::UiController controller(state, engine, renderer, persistence);

    controllerTapAt(controller, 1000U);
    controllerTapAt(controller, 1500U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 1U);

    // At 20 BPM exactly 3000 ms is still a valid interval.
    controllerTapAt(controller, 4500U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 1U);

    // One millisecond beyond that boundary starts a fresh sequence: no indicator.
    controllerTapAt(controller, 7501U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 0U);
    controllerTapAt(controller, 8001U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 1U);
}

void testTapIndicatorIsExclusiveToPerformanceTapTempo() {
    resetFakes(); prepareDisplaySuccess();
    hal::OledDisplay display; CHECK(display.begin());
    ClockState state = makeDefaultState();
    hal::GateOutputDriver gates; gates.beginDisabled();
    engine::ClockEngine engine(gates); engine.begin(state);
    hal::PersistentStorage::resetForTest(); hal::PersistentStorage storage;
    services::PersistentStateService persistence(storage); persistence.begin();
    ui::UiRenderer renderer(display, persistence);
    ui::UiController controller(state, engine, renderer, persistence);
    std::uint32_t now = 1000U;

    controllerOpenSettingsChord(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 0U);
    controllerReset(controller, now);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    controllerTapAt(controller, 2000U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 0U);
    controllerTapAt(controller, 2500U);
    CHECK_EQ(controller.navigation().tapIndicatorFrame, 1U);
}


void testScreensaverRenderingAndPolicy() {
    resetFakes();
    prepareDisplaySuccess();
    hal::OledDisplay display;
    CHECK(display.begin());

    // HAL power/contrast commands and public pixel primitive are directly testable.
    display.setPixel(3, 4);
    CHECK(std::any_of(
        display.framebufferForTest().begin(),
        display.framebufferForTest().end(),
        [](const std::uint8_t value) { return value != 0U; }));
    display.setContrast(config::kDisplayDimmedContrast);
    display.setPower(false);
    display.setPower(true);

    ui::ScreensaverRenderer saver(display);
    saver.render(ScreensaverMode::Fractal, 0U);
    const auto fractalFrame = display.framebufferForTest();
    CHECK(std::any_of(fractalFrame.begin(), fractalFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Fractal, 0U); // repeated render must not randomly jump view
    CHECK(display.framebufferForTest() == fractalFrame);
    saver.render(ScreensaverMode::Fractal, 20U); // progressively reveal the selected crop
    const auto grownFractalFrame = display.framebufferForTest();
    CHECK(grownFractalFrame != fractalFrame);
    CHECK(std::count_if(grownFractalFrame.begin(), grownFractalFrame.end(), [](const std::uint8_t value) { return value != 0U; }) >=
          std::count_if(fractalFrame.begin(), fractalFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Fractal, 48U); // next cycle selects another curated crop
    const auto secondFractalCrop = display.framebufferForTest();
    CHECK(secondFractalCrop != fractalFrame);
    CHECK(std::any_of(secondFractalCrop.begin(), secondFractalCrop.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Fractal, 68U); // second crop grows in place
    CHECK(display.framebufferForTest() != secondFractalCrop);
    saver.render(ScreensaverMode::Fractal, 0U); // later activation must not restart with the first crop
    CHECK(display.framebufferForTest() != fractalFrame);

    saver.render(ScreensaverMode::Orbit, 3U);
    const auto orbitFrame = display.framebufferForTest();
    CHECK(std::any_of(orbitFrame.begin(), orbitFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Orbit, 4U);
    CHECK(display.framebufferForTest() != orbitFrame);
    saver.render(ScreensaverMode::Orbit, 35U); // sine-table wrap reproduces frame 3
    CHECK(display.framebufferForTest() == orbitFrame);
    saver.render(ScreensaverMode::Plug, 2U);
    const auto plugFrame = display.framebufferForTest();
    CHECK(std::any_of(plugFrame.begin(), plugFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Plug, 5U);
    CHECK(display.framebufferForTest() != plugFrame);
    saver.render(ScreensaverMode::Clock, 2U);
    const auto clockFrame = display.framebufferForTest();
    CHECK(std::any_of(clockFrame.begin(), clockFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Clock, 5U);
    CHECK(display.framebufferForTest() != clockFrame);
    saver.render(ScreensaverMode::Heartbeat, 2U);
    const auto heartbeatFrame = display.framebufferForTest();
    CHECK(std::any_of(heartbeatFrame.begin(), heartbeatFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Heartbeat, 5U);
    CHECK(display.framebufferForTest() != heartbeatFrame);
    saver.render(ScreensaverMode::Acid, 1U);
    const auto acidFrame = display.framebufferForTest();
    CHECK(std::any_of(acidFrame.begin(), acidFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Acid, 12U);
    CHECK(display.framebufferForTest() != acidFrame);
    saver.render(ScreensaverMode::Spectrum, 1U);
    const auto spectrumFrame = display.framebufferForTest();
    CHECK(std::any_of(spectrumFrame.begin(), spectrumFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Spectrum, 12U);
    CHECK(display.framebufferForTest() != spectrumFrame);
    saver.render(ScreensaverMode::Field, 1U);
    const auto fieldFrame = display.framebufferForTest();
    CHECK(std::any_of(fieldFrame.begin(), fieldFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Field, 24U);
    CHECK(display.framebufferForTest() != fieldFrame);
    for (std::uint32_t frame = 0U; frame <= 12U; ++frame) saver.render(ScreensaverMode::Blox, frame);
    const auto bloxFrame = display.framebufferForTest();
    CHECK(std::any_of(bloxFrame.begin(), bloxFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    for (std::uint32_t frame = 13U; frame <= 40U; ++frame) saver.render(ScreensaverMode::Blox, frame);
    CHECK(display.framebufferForTest() != bloxFrame);
    saver.render(ScreensaverMode::Matrix, 3U);
    const auto matrixFrame = display.framebufferForTest();
    CHECK(std::any_of(matrixFrame.begin(), matrixFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Matrix, 20U);
    CHECK(display.framebufferForTest() != matrixFrame);
    saver.render(ScreensaverMode::CubeCover, 6U);
    const auto cubeFrame = display.framebufferForTest();
    CHECK(std::any_of(cubeFrame.begin(), cubeFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::CubeCover, 70U);
    CHECK(display.framebufferForTest() != cubeFrame);
    saver.render(ScreensaverMode::MakeMusic, 18U);
    const auto makeMusicFrame = display.framebufferForTest();
    CHECK(std::any_of(makeMusicFrame.begin(), makeMusicFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Labyrinth, 40U);
    const auto labyrinthFrame = display.framebufferForTest();
    CHECK(std::any_of(labyrinthFrame.begin(), labyrinthFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Starfield, 7U);
    const auto starfieldFrame = display.framebufferForTest();
    CHECK(std::any_of(starfieldFrame.begin(), starfieldFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    saver.render(ScreensaverMode::Fireworks, 15U);
    const auto fireworksFrame = display.framebufferForTest();
    CHECK(std::any_of(fireworksFrame.begin(), fireworksFrame.end(), [](const std::uint8_t value) { return value != 0U; }));
    const auto beforeNone = display.framebufferForTest();
    saver.render(ScreensaverMode::None, 0U);
    CHECK(display.framebufferForTest() == beforeNone);
    saver.render(static_cast<ScreensaverMode>(99U), 0U);
    CHECK(display.framebufferForTest() == beforeNone);

    // Settings expose readable screensaver names and ordered minute thresholds.
    ClockState state = makeDefaultState();
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    ui::SettingsEditor editor(state, engine);
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Screensaver), 4U);
    auto row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.label, "MODE") == 0);
    CHECK(std::strcmp(row.value, "CLOCK") == 0);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, -1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::None);
    row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.value, "OFF") == 0);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, -1); // OFF is the first entry
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::None);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Clock);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Plug);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 4);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Field);
    row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.value, "FIELD") == 0);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Blox);
    row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.value, "BLOX") == 0);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Matrix);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::CubeCover);
    row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.value, "CUBE COVER") == 0);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Fractal);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Orbit);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::MakeMusic);
    row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.value, "MAKE MUSIC") == 0);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Labyrinth);
    row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.value, "LABYRINTH") == 0);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Starfield);
    row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.value, "STARFIELD") == 0);
    editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Fireworks);
    row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.value, "FIREWORKS") == 0);
    state.display.screensaverMode = static_cast<ScreensaverMode>(99U);
    row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, state);
    CHECK(std::strcmp(row.value, "OFF") == 0);

    state.display = {ScreensaverMode::Fractal, 2U, 5U, 10U};
    editor.adjust(ui::SettingsPage::Screensaver, 1U, 0U, 10);
    CHECK_EQ(state.display.screensaverAfterMinutes, 5U);
    editor.adjust(ui::SettingsPage::Screensaver, 2U, 0U, -10);
    CHECK_EQ(state.display.dimAfterMinutes, 5U);
    editor.adjust(ui::SettingsPage::Screensaver, 3U, 0U, -10);
    CHECK_EQ(state.display.offAfterMinutes, 5U);
    editor.adjust(ui::SettingsPage::Screensaver, 3U, 0U, 127);
    CHECK_EQ(state.display.offAfterMinutes, config::kMaximumScreensaverMinutes);

    // End-to-end inactivity policy: animation -> dim -> off -> immediate wake.
    state = makeDefaultState();
    state.display = {ScreensaverMode::Fractal, 1U, 2U, 3U};
    engine.updateConfiguration(state, true);
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::PersistentStateService persistence(storage);
    persistence.begin();
    ui::UiRenderer renderer(display, persistence);
    hal::ExternalInputCapture externalInputs;
    ui::UiController controller(
        state, engine, renderer, persistence, nullptr, &externalInputs);

    // Screensaver now lives below GENERAL SETTINGS.
    std::uint32_t navigationNow = 10U;
    controllerOpenSettingsChord(controller, navigationNow);
    controllerShortPress(controller, navigationNow); // GENERAL SETTINGS
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::General);
    controllerTurn(controller, 2, navigationNow); // SCREENSAVER
    controllerShortPress(controller, navigationNow);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Screensaver);
    controllerReset(controller, navigationNow); // SCREENSAVER -> GENERAL
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::General);
    controllerReset(controller, navigationNow); // GENERAL -> Root
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Root);
    controllerReset(controller, navigationNow); // Root -> Performance

    hal::ControlSample idle{};
    controller.processControls(idle, 0U);
    controller.serviceRendering(1U);
    controller.serviceRendering(60'001U);  // first fractal frame
    controller.serviceRendering(60'001U + config::kScreensaverFrameIntervalMs); // next frame
    controller.serviceRendering(120'001U); // dim + fractal
    controller.serviceRendering(180'001U); // display off

    hal::ControlSample wake{};
    wake.resetButton = releasedEdge(); // activity without triggering RESET action
    controller.processControls(wake, 180'002U);
    controller.serviceRendering(180'002U + config::kDisplayRefreshMinimumMs);

    // Any external SYNC or RST transition is visible activity: it wakes the
    // stopped module immediately and restarts the inactivity interval.
    controller.serviceRendering(240'003U);
    const auto syncSleeperFrame = display.framebufferForTest();
    externalInputs.injectSyncEdgeForTest(240'004'000U, true);
    controller.serviceRendering(240'004U);
    CHECK(display.framebufferForTest() != syncSleeperFrame);

    controller.serviceRendering(300'005U);
    const auto resetSleeperFrame = display.framebufferForTest();
    externalInputs.injectResetEdgeForTest(300'006'000U, true);
    controller.serviceRendering(300'006U);
    CHECK(display.framebufferForTest() != resetSleeperFrame);

    // MODE 3 deliberately keeps the ordinary STOP screen until DIM/OFF.
    state.display = {ScreensaverMode::None, 1U, 2U, 3U};
    controller.processControls(wake, 320'000U);
    controller.serviceRendering(380'001U);

    // Screensaver logic is disabled while transport is not STOPPED.
    state.transport = TransportState::Playing;
    controller.serviceRendering(500'000U);
}


void testNestedGroupNavigationCoverage() {
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
    ui::UiRenderer renderer(display, persistentState);
    ui::UiController controller(state, engine, renderer, persistentState);
    std::uint32_t now = 20U;

    // ONE CLOCK: exercise both group links, nested Groove return, and root restoration.
    state.operatingMode = OperatingMode::UnifiedClock;
    engine.updateConfiguration(state, true);
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now); // CHANNEL SETTINGS
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedClock);
    controllerTurn(controller, 1, now); // TIMING
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedTiming);
    controllerTurn(controller, 4, now); // GROOVE
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Groove);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedTiming);
    CHECK_EQ(controller.navigation().cursor, 4U);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedClock);
    CHECK_EQ(controller.navigation().cursor, 1U);
    controllerTurn(controller, 1, now); // OUTPUT
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedOutput);
    controllerShortPress(controller, now); // edit GATE
    CHECK(controller.navigation().editing);
    controllerShortPress(controller, now);
    CHECK(!controller.navigation().editing);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedClock);
    CHECK_EQ(controller.navigation().cursor, 2U);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Root);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // Independent CLOCK: OUTPUT is a peer group and BACK restores its root row.
    state.operatingMode = OperatingMode::Independent;
    state.channels[0].common.mode = ChannelMode::Clock;
    engine.updateConfiguration(state, true);
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    controllerTurn(controller, 3, now); // OUTPUT
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::ChannelOutput);
    controllerShortPress(controller, now); // edit PROB
    CHECK(controller.navigation().editing);
    controllerTurn(controller, -1, now);
    controllerShortPress(controller, now);
    CHECK(!controller.navigation().editing);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(controller.navigation().cursor, 3U);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Root);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
}
