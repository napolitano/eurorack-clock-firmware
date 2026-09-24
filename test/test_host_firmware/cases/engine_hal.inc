/**
 * @file engine_hal.inc
 * @brief Included test cases split from test_host_firmware/test_main.cpp for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

void testMenuModelAndFormatters() {
    ClockState state = makeDefaultState();
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    char buffer[32]{};
    ui::formatRate(state.channels[0].common, buffer, sizeof(buffer)); CHECK(std::strcmp(buffer,"x1")==0);
    state.channels[0].common.rate = {ClockRatioMode::Divide,2U,1U,1U};
    ui::formatRate(state.channels[0].common, buffer, sizeof(buffer)); CHECK(std::strcmp(buffer,"/2")==0);
    state.channels[0].common.rate = {ClockRatioMode::Multiply,1U,3U,2U};
    ui::formatRate(state.channels[0].common, buffer, sizeof(buffer)); CHECK(std::strcmp(buffer,"3:2")==0);
    state.channels[0].common.rate = {ClockRatioMode::Multiply,2U,3U,2U};
    ui::formatRate(state.channels[0].common, buffer, sizeof(buffer)); CHECK(std::strstr(buffer,"3:2")!=nullptr);
    state.channels[0].common.rate = {ClockRatioMode::Divide,1U,1U,2U};
    ui::formatRate(state.channels[0].common, buffer, sizeof(buffer)); CHECK(std::strstr(buffer,"1:2")!=nullptr);

    for (ChannelMode mode : {ChannelMode::Clock, ChannelMode::Euclid, ChannelMode::Sequencer}) {
        state.channels[0].common.mode = mode;
        ui::formatChannelDetail(state.channels[0], buffer, sizeof(buffer)); CHECK(std::strlen(buffer)>0U);
        ui::formatChannelSummary(state.channels[0], buffer, sizeof(buffer)); CHECK(std::strlen(buffer)>0U);
    }
    state.channels[0].common.mode = ChannelMode::Off;
    ui::formatChannelDetail(state.channels[0], buffer, sizeof(buffer)); CHECK_EQ(std::strlen(buffer), 0U);
    ui::formatChannelSummary(state.channels[0], buffer, sizeof(buffer)); CHECK(std::strcmp(buffer, "OFF") == 0);

    const ui::SettingsPage pages[] = {ui::SettingsPage::Root,ui::SettingsPage::General,ui::SettingsPage::InputAssignments,ui::SettingsPage::Hardware,ui::SettingsPage::Diagnostics,ui::SettingsPage::Master,ui::SettingsPage::Sync,ui::SettingsPage::Preferences,ui::SettingsPage::Screensaver,ui::SettingsPage::Info,ui::SettingsPage::Licenses,ui::SettingsPage::Updates,ui::SettingsPage::Channel,ui::SettingsPage::ChannelTiming,ui::SettingsPage::ChannelOutput,ui::SettingsPage::Clock,ui::SettingsPage::Euclid,ui::SettingsPage::Sequencer,ui::SettingsPage::SequencerPattern,ui::SettingsPage::UnifiedClock,ui::SettingsPage::UnifiedTiming,ui::SettingsPage::UnifiedOutput,ui::SettingsPage::Groove,ui::SettingsPage::GrooveEditorMenu,ui::SettingsPage::GrooveRecordMenu,ui::SettingsPage::DividerBank};
    state.source=ClockSource::External; state.externalSync.edge=SyncEdge::Falling; state.externalSync.lossMode=SyncLossMode::Stop;
    state.channels[0].common.resetMode=ResetMode::Free; state.channels[0].common.muted=true;
    for (auto page: pages) {
        CHECK(std::strlen(ui::settingsPageTitle(page))>0U);
        const auto count=ui::settingsPageItemCount(page); CHECK(count>0U);
        for (std::uint8_t row=0; row<count; ++row) {
            const auto menuRow=ui::buildMenuRow(page,row,0U,state);
            CHECK(std::strlen(menuRow.label)>0U);
        }
    }
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::General), "GENERAL SETTINGS") == 0);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::Diagnostics), "DIAGNOSTICS") == 0);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::DiagnosticsInputs), "INPUTS") == 0);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::DiagnosticsOutputs), "OUTPUTS") == 0);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::Master), "CLOCK") == 0);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::Preferences), "PRESETS") == 0);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::Screensaver), "SCREENSAVER") == 0);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::Info), "INFO") == 0);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::Licenses), "LICENSES") == 0);
    CHECK(std::strcmp(ui::settingsPageTitle(ui::SettingsPage::Updates), "UPDATES") == 0);
    const auto generalRoot = ui::buildMenuRow(ui::SettingsPage::Root, 0U, 0U, state);
    const auto channelRoot = ui::buildMenuRow(ui::SettingsPage::Root, 1U, 0U, state);
    const auto presetsRoot = ui::buildMenuRow(ui::SettingsPage::Root, 2U, 0U, state);
    const auto infoRoot = ui::buildMenuRow(ui::SettingsPage::Root, 3U, 0U, state);
    const auto phaseResetRoot = ui::buildMenuRow(ui::SettingsPage::Root, 4U, 0U, state);
    CHECK(std::strcmp(generalRoot.label, "GENERAL SETTINGS") == 0);
    CHECK(std::strcmp(channelRoot.label, "CHANNEL SETTINGS") == 0);
    CHECK(std::strcmp(presetsRoot.label, "PRESETS") == 0);
    CHECK(std::strcmp(infoRoot.label, "INFO") == 0);
    CHECK(std::strcmp(phaseResetRoot.label, "PHASE RESET") == 0);
    const auto generalSync = ui::buildMenuRow(ui::SettingsPage::General, 1U, 0U, state);
    const auto generalSaver = ui::buildMenuRow(ui::SettingsPage::General, 2U, 0U, state);
    const auto generalDiagnostics = ui::buildMenuRow(ui::SettingsPage::General, 3U, 0U, state);
    const auto generalHardware = ui::buildMenuRow(ui::SettingsPage::General, 4U, 0U, state);
    const auto diagnosticInputs = ui::buildMenuRow(ui::SettingsPage::Diagnostics, 0U, 0U, state);
    const auto diagnosticOutputs = ui::buildMenuRow(ui::SettingsPage::Diagnostics, 1U, 0U, state);
    CHECK(std::strcmp(generalSync.label, "INPUTS") == 0);
    CHECK(std::strcmp(generalSaver.label, "SCREENSAVER") == 0);
    CHECK(std::strcmp(generalDiagnostics.label, "DIAGNOSTICS") == 0);
    CHECK(std::strcmp(generalHardware.label, "HARDWARE") == 0);
    CHECK(std::strcmp(diagnosticInputs.label, "INPUTS") == 0);
    CHECK(std::strcmp(diagnosticOutputs.label, "OUTPUTS") == 0);
    const auto infoProduct = ui::buildMenuRow(ui::SettingsPage::Info, 0U, 0U, state);
    CHECK(std::strcmp(infoProduct.value, "Clock") == 0);
    CHECK(infoProduct.expandableInformation);
    const auto infoAuthor = ui::buildMenuRow(ui::SettingsPage::Info, 2U, 0U, state);
    CHECK(std::strcmp(infoAuthor.value, "AXEL NAPOLITANO") == 0);
    CHECK(infoAuthor.expandableInformation);
    const auto infoLicenses = ui::buildMenuRow(ui::SettingsPage::Info, 3U, 0U, state);
    const auto infoUpdates = ui::buildMenuRow(ui::SettingsPage::Info, 4U, 0U, state);
    const auto infoFactoryReset = ui::buildMenuRow(ui::SettingsPage::Info, 5U, 0U, state);
    CHECK(std::strcmp(infoLicenses.label, "LICENSES") == 0);
    CHECK(std::strcmp(infoUpdates.label, "UPDATES") == 0);
    CHECK(std::strcmp(infoFactoryReset.label, "FACTORY RESET") == 0);
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Info), 6U);
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Groove), 8U);
    state.channels[0].common.mode = ChannelMode::Sequencer;
    const auto channelModeRow = ui::buildMenuRow(ui::SettingsPage::Channel, 0U, 0U, state);
    CHECK(std::strcmp(channelModeRow.value, "SEQUENCER") == 0);
    const auto templatePresetRow = ui::buildMenuRow(ui::SettingsPage::Preferences, 3U, 0U, state);
    CHECK(std::strcmp(templatePresetRow.label, "TEMPLATES") == 0);

    CHECK_EQ(ui::settingsPageItemCount(static_cast<ui::SettingsPage>(99)),0U);
    CHECK(std::strlen(ui::settingsPageTitle(static_cast<ui::SettingsPage>(99)))>0U);
    CHECK(!ui::isChannelSettingsPage(ui::SettingsPage::Root));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::Channel));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::ChannelTiming));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::ChannelOutput));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::Clock));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::Euclid));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::Sequencer));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::SequencerPattern));

    state.externalSync.lossMode=SyncLossMode::Freewheel;
    (void)ui::buildMenuRow(ui::SettingsPage::Sync,3U,0U,state);
    state.externalSync.lossMode=SyncLossMode::Internal;
    (void)ui::buildMenuRow(ui::SettingsPage::Sync,3U,0U,state);
    state.externalSync.resetMode=ExternalResetMode::Gate;
    const auto resetGateRow = ui::buildSyncMenuRow(4U, state);
    CHECK(std::strcmp(resetGateRow.value, "GATE") == 0);
    const auto invalidSyncRow = ui::buildSyncMenuRow(99U, state);
    CHECK_EQ(std::strlen(invalidSyncRow.label), 0U);
    state.externalSync.resetMode=ExternalResetMode::Trigger;
    state.channels[0].common.mode=ChannelMode::Clock; (void)ui::buildMenuRow(ui::SettingsPage::Channel,0U,0U,state);
    state.channels[0].common.mode=ChannelMode::Euclid; (void)ui::buildMenuRow(ui::SettingsPage::Channel,0U,0U,state);
    state.channels[0].common.mode=ChannelMode::Sequencer; (void)ui::buildMenuRow(ui::SettingsPage::Channel,0U,0U,state);
    state.channels[0].common.mode=ChannelMode::Off;
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Channel, ChannelMode::Off), 1U);
    const auto offModeRow = ui::buildMenuRow(ui::SettingsPage::Channel,0U,0U,state);
    CHECK(std::strcmp(offModeRow.value, "OFF") == 0);
    state.channels[0].common.muted=false;
    (void)ui::buildMenuRow(static_cast<ui::SettingsPage>(99),0U,0U,state);

    for (std::size_t i=0;i<kRateOptions.size();++i) { state.channels[0].common.rate.mode=kRateOptions[i].mode; state.channels[0].common.rate.factor=kRateOptions[i].factor; CHECK_EQ(ui::findRateOptionIndex(state.channels[0].common),i); }
    state.channels[0].common.rate.factor=99U; CHECK_EQ(ui::findRateOptionIndex(state.channels[0].common),9U);
    for (std::size_t i=0;i<kGateLengthOptionsMs.size();++i) CHECK_EQ(ui::findGateLengthOptionIndex(kGateLengthOptionsMs[i]),i);
    CHECK_EQ(ui::findGateLengthOptionIndex(999U),3U);
    for (std::size_t i=0;i<kBeatUnitOptions.size();++i) CHECK_EQ(ui::findBeatUnitOptionIndex(kBeatUnitOptions[i]),i);
    CHECK_EQ(ui::findBeatUnitOptionIndex(3U),1U);
}

void testHalBasicsAndDisplay() {
    resetFakes();
#if CLOCK_ENFORCE_FINAL_PIN_MAP
    CHECK_EQ(pinmap::kPlayPauseButtonPin, mcu::PA1);
    CHECK_EQ(pinmap::kResetBackButtonPin, mcu::PA2);
    CHECK_EQ(pinmap::kTapTempoButtonPin, mcu::PA3);
    CHECK_EQ(pinmap::kDisplaySpiChipSelectPin, mcu::PA4);
    CHECK_EQ(pinmap::kDisplaySpiClockPin, mcu::PA5);
    CHECK_EQ(pinmap::kDisplaySpiDataPin, mcu::PA7);
    CHECK_EQ(pinmap::kExternalSyncSignalPin, mcu::PA8);
    CHECK_EQ(pinmap::kExternalResetSignalPin, mcu::PA9);
    CHECK_EQ(pinmap::kEncoderPhaseAPin, mcu::PB6);
    CHECK_EQ(pinmap::kEncoderPhaseBPin, mcu::PB7);
    CHECK_EQ(pinmap::kEncoderPushButtonPin, mcu::PB14);
    CHECK_EQ(pinmap::kDisplaySpiDataCommandPin, mcu::PB9);
    CHECK_EQ(pinmap::kDisplayResetPin, mcu::PB15);
    const std::array<std::uint32_t, 8U> expectedGatePins{{mcu::PB0,mcu::PB1,mcu::PB2,mcu::PB5,mcu::PB8,mcu::PB10,mcu::PB12,mcu::PB13}};
    CHECK(pinmap::kGateChannelPins == expectedGatePins);
    CHECK_EQ(pinmap::kGateBufferOutputEnablePin, pinmap::kUnassignedDigitalPin);
#endif
    {
        hal::InterruptLock lock;
        CHECK_EQ(fakefw::noInterruptCalls,1U);
    }
    CHECK_EQ(fakefw::interruptCalls,1U);
    CHECK_EQ(hal::SystemClock::milliseconds(),0U);
    hal::SystemClock::delayMilliseconds(7U); CHECK_EQ(hal::SystemClock::milliseconds(),7U);

    hal::GateOutputDriver gates;
    gates.beginDisabled();
    CHECK(!gates.channelStateHigh(0U));
    gates.setChannelState(0U, true);
    CHECK(!gates.channelStateHigh(0U));
    CHECK_EQ(fakefw::pinValues[pinmap::kChannel1GateLedPin], LOW);
    gates.enableOutputStage();
    gates.setChannelState(0U, true);
    CHECK(gates.channelStateHigh(0U));
    CHECK_EQ(fakefw::pinValues[pinmap::kChannel1GateLedPin], HIGH);
    gates.disableOutputStage();
    CHECK(!gates.channelStateHigh(0U));
    CHECK_EQ(fakefw::pinValues[pinmap::kChannel1GateLedPin], LOW);
    gates.setChannelState(0U,true);
    CHECK_EQ(fakefw::pinValues[pinmap::kChannel1GateLedPin],LOW);
    gates.setChannelState(0U,false); gates.setChannelState(99U,true); gates.setAllChannelsLow();
    for (auto pin: pinmap::kGateChannelPins) CHECK_EQ(fakefw::pinValues[pin],LOW);

    hal::ExternalInputCapture externalInputs;
    externalInputs.begin();
    externalInputs.injectSyncEdgeForTest(101U, true);
    CHECK(externalInputs.syncLevelHigh());
    externalInputs.injectSyncEdgeForTest(102U, false);
    CHECK(!externalInputs.syncLevelHigh());
    externalInputs.injectResetEdgeForTest(103U, true);
    CHECK(externalInputs.resetLevelHigh());
    externalInputs.injectResetEdgeForTest(104U, false);
    CHECK(!externalInputs.resetLevelHigh());

    hal::PeriodicTimer timer;
    bool callbackCalled=false;
    static bool* callbackTarget=nullptr; callbackTarget=&callbackCalled;
    auto callback=[](){ if(callbackTarget) *callbackTarget=true; };
    timer.start(1234U,callback);
    CHECK(HardwareTimer::lastInstance!=nullptr); CHECK_EQ(HardwareTimer::lastInstance->overflow,1234U);
    CHECK_EQ(HardwareTimer::lastInstance->preemptPriority, config::kSchedulerInterruptPreemptPriority);
    CHECK_EQ(HardwareTimer::lastInstance->subPriority, config::kSchedulerInterruptSubPriority);
    HardwareTimer::lastInstance->fire(); CHECK(callbackCalled); timer.stop(); CHECK(HardwareTimer::lastInstance->paused);

    resetFakes(); prepareDisplaySuccess();
    hal::OledDisplay display;
    CHECK_EQ(display.framebufferForTest().size(), hal::OledDisplay::kFramebufferSize);
#if CLOCK_DISPLAY_USE_SPI
    CHECK(display.begin());
    CHECK(SPI.begun);
    CHECK(!SPI.transfers.empty());
    CHECK_EQ(SPI.sclk, pinmap::kDisplaySpiClockPin);
    CHECK_EQ(SPI.mosi, pinmap::kDisplaySpiDataPin);
    CHECK_EQ(fakefw::pinModes[pinmap::kDisplaySpiChipSelectPin], OUTPUT);
    CHECK_EQ(fakefw::pinModes[pinmap::kDisplaySpiDataCommandPin], OUTPUT);
    CHECK_EQ(fakefw::pinModes[pinmap::kDisplayResetPin], OUTPUT);
    CHECK_EQ(fakefw::pinValues[pinmap::kDisplaySpiChipSelectPin], HIGH);
    CHECK_EQ(fakefw::pinValues[pinmap::kDisplayResetPin], HIGH);
    CHECK_EQ(SPI.lastClock, config::kDisplaySpiFrequencyHz);
    bool sawResetLow = false;
    bool sawChipSelectLow = false;
    std::size_t firstCsHigh = fakefw::writes.size();
    std::size_t firstResetLow = fakefw::writes.size();
    std::size_t firstCsLow = fakefw::writes.size();
    for (std::size_t index = 0U; index < fakefw::writes.size(); ++index) {
        const auto& write = fakefw::writes[index];
        if (write.pin == pinmap::kDisplaySpiChipSelectPin && write.value == HIGH && firstCsHigh == fakefw::writes.size()) {
            firstCsHigh = index;
        }
        if (write.pin == pinmap::kDisplayResetPin && write.value == LOW && firstResetLow == fakefw::writes.size()) {
            firstResetLow = index;
            sawResetLow = true;
        }
        if (write.pin == pinmap::kDisplaySpiChipSelectPin && write.value == LOW && firstCsLow == fakefw::writes.size()) {
            firstCsLow = index;
            sawChipSelectLow = true;
        }
    }
    CHECK(sawResetLow);
    CHECK(sawChipSelectLow);
    CHECK(firstCsHigh < firstResetLow);
    CHECK(firstResetLow < firstCsLow);
    CHECK(hal::SystemClock::milliseconds() >= 50U);
#else
    CHECK(display.begin()); CHECK(Wire.begun); CHECK(!Wire.transmissions.empty());
#endif
    const auto framebufferBeforeRotation = display.framebufferForTest();
#if CLOCK_DISPLAY_USE_SPI
    const std::size_t rotationStart = SPI.transfers.size();
    display.setRotation180(true);
    CHECK_EQ(SPI.transfers.size(), rotationStart + 2U);
    CHECK_EQ(SPI.transfers[rotationStart], 0xA1U);
    CHECK_EQ(SPI.transfers[rotationStart + 1U], 0xC8U);
    display.setRotation180(false);
    CHECK_EQ(SPI.transfers[rotationStart + 2U], 0xA1U);
    CHECK_EQ(SPI.transfers[rotationStart + 3U], 0xC8U);
#else
    const std::size_t rotationStart = Wire.transmissions.size();
    display.setRotation180(true);
    CHECK_EQ(Wire.transmissions.size(), rotationStart + 2U);
    CHECK_EQ(Wire.transmissions[rotationStart].bytes.size(), 2U);
    CHECK_EQ(Wire.transmissions[rotationStart].bytes[0], 0x00U);
    CHECK_EQ(Wire.transmissions[rotationStart].bytes[1], 0xA1U);
    CHECK_EQ(Wire.transmissions[rotationStart + 1U].bytes[1], 0xC8U);
    display.setRotation180(false);
    CHECK_EQ(Wire.transmissions[rotationStart + 2U].bytes[1], 0xA1U);
    CHECK_EQ(Wire.transmissions[rotationStart + 3U].bytes[1], 0xC8U);
#endif
    CHECK(display.framebufferForTest() == framebufferBeforeRotation);

    // 180-degree mounting is implemented as a byte-level framebuffer rotation
    // while the OLED controller stays in A1/C8. Verify an asymmetric pair of
    // pixels maps to the exact opposite coordinates without mirroring glyph data.
    display.clear();
    display.setPixel(3, 5);
    display.setPixel(11, 17);
    display.setRotation180(true);
    display.present();
#if !CLOCK_DISPLAY_USE_SPI
    while (display.service()) {}
#endif
    const auto& rotatedPhysical = display.presentedFramebufferForTest();
    const auto pixelSet = [](const auto& fb, int x, int y) {
        const std::size_t index = static_cast<std::size_t>(x) +
            static_cast<std::size_t>(y / 8) * static_cast<std::size_t>(hal::OledDisplay::kWidth);
        return (fb[index] & static_cast<std::uint8_t>(1U << (y & 7))) != 0U;
    };
    CHECK(pixelSet(rotatedPhysical, 124, 58));
    CHECK(pixelSet(rotatedPhysical, 116, 46));
    CHECK(!pixelSet(rotatedPhysical, 3, 5));
    display.setRotation180(false);
    display.clear();
    display.present();
#if !CLOCK_DISPLAY_USE_SPI
    while (display.service()) {}
#endif

    display.clear();
    display.setFont(hal::DisplayFont::Small); display.setTextColor(hal::PixelColor::White);
    auto empty=display.measureText(nullptr,1,2); CHECK_EQ(empty.width,0U); CHECK_EQ(empty.y,2);
    auto small=display.measureText("ABC",3,4); CHECK_EQ(small.width,17U); CHECK_EQ(small.height,7U);
    display.drawText(0,0,nullptr); display.drawText(0,0,"Aa0!"); display.drawCharacter(5,8,'?');
    display.setFont(hal::DisplayFont::Tempo);
    CHECK_EQ(display.measureText(nullptr,0,0).width,0U);
    CHECK_EQ(display.measureText("",0,0).width,0U);
    CHECK(display.measureText("8",0,0).width > 0U); // single glyph: no inter-glyph spacing
    CHECK_EQ(display.measureText("111",0,0).width, display.measureText("888",0,0).width);
    auto large=display.measureText("120",0,30); CHECK_EQ(large.height,hal::font::kTempoGlyphHeight); CHECK_EQ(large.y,30);
    display.drawText(0,30,"0123456789"); display.drawCharacter(60,30,'4'); display.drawCharacter(70,30,'X');
    display.drawHorizontalLine(-2,0,5); display.drawVerticalLine(0,-2,5);
    display.drawHorizontalLine(127,63,5); display.drawVerticalLine(127,63,5);
    display.drawLine(0,0,10,4); display.drawLine(10,10,0,2); display.drawLine(5,5,5,5);
    display.drawRectangle(0,0,10,10); display.drawRectangle(0,0,0,10); display.drawRectangle(0,0,10,0); display.fillRectangle(2,2,5,4); display.fillRectangle(2,2,2,2,hal::PixelColor::Black);
    display.present();

    // An unchanged frame must cause no bus traffic. A one-page change must not
    // retransmit the entire 1 KiB framebuffer. This protects the interactive UI
    // from avoidable I2C/SPI stalls during pattern playback.
    display.clear();
    display.present();
#if CLOCK_DISPLAY_USE_SPI
    const std::size_t unchangedTransferCount = SPI.transfers.size();
    display.present();
    CHECK_EQ(SPI.transfers.size(), unchangedTransferCount);
    display.fillRectangle(0, 63, 1, 1);
    const std::size_t partialTransferStart = SPI.transfers.size();
    display.present();
    const std::size_t partialTransferBytes = SPI.transfers.size() - partialTransferStart;
    CHECK(partialTransferBytes > 0U);
    CHECK(partialTransferBytes < hal::OledDisplay::kFramebufferSize / 2U);
#else
    const std::size_t unchangedTransmissionCount = Wire.transmissions.size();
    display.present();
    CHECK_EQ(Wire.transmissions.size(), unchangedTransmissionCount);
    CHECK(!display.service());
    CHECK_EQ(Wire.transmissions.size(), unchangedTransmissionCount);

    display.fillRectangle(0, 63, 1, 1);
    const std::size_t partialTransmissionStart = Wire.transmissions.size();
    display.present();
    // present() only publishes the newest framebuffer; runtime I2C bus work is deferred.
    CHECK_EQ(Wire.transmissions.size(), partialTransmissionStart);
    std::size_t serviceCalls = 0U;
    while (display.service()) {
        ++serviceCalls;
    }
    CHECK(serviceCalls > 1U);
    CHECK_EQ(Wire.transmissions.size() - partialTransmissionStart, serviceCalls);
    std::size_t partialDataBytes = 0U;
    for (std::size_t index = partialTransmissionStart; index < Wire.transmissions.size(); ++index) {
        const auto& bytes = Wire.transmissions[index].bytes;
        if (!bytes.empty() && bytes.front() == 0x40U) {
            CHECK(bytes.size() <= 25U);  // one control byte + at most 24 display bytes
            partialDataBytes += bytes.size() - 1U;
        }
    }
    CHECK_EQ(partialDataBytes, static_cast<std::size_t>(hal::OledDisplay::kWidth));

    // Latest-frame-wins: replace an in-progress refresh, then verify the display
    // model converges to the newest framebuffer rather than completing stale UI.
    display.clear();
    display.fillRectangle(0, 0, 1, 1);
    display.present();
    CHECK(display.service());  // page-window transaction
    CHECK(display.service());  // first data chunk from the now-stale frame
    display.clear();
    display.fillRectangle(127, 63, 1, 1);
    display.present();
    while (display.service()) {
    }
    const auto& target = display.framebufferForTest();
    const auto& physical = display.presentedFramebufferForTest();
    for (std::size_t index = 0U; index < target.size(); ++index) {
        CHECK_EQ(physical[index], target[index]);
    }

    // I2C NACKs must never advance the software model of the physical display.
    // A failed command or data transaction stays queued and is retried on a
    // later foreground service pass; the clock scheduler is not involved.
#if CLOCK_DISPLAY_I2C_ADDRESS
    constexpr std::uint8_t kTestDisplayAddress = static_cast<std::uint8_t>(CLOCK_DISPLAY_I2C_ADDRESS);
#else
    constexpr std::uint8_t kTestDisplayAddress = 0x3CU;
#endif
    const auto physicalBeforeNack = display.presentedFramebufferForTest();
    display.fillRectangle(3, 3, 1, 1);
    display.present();

    const std::size_t commandNackStart = Wire.transmissions.size();
    Wire.ack[kTestDisplayAddress] = 4U;
    CHECK(display.service());  // page-window command is NACKed
    CHECK_EQ(Wire.transmissions.size(), commandNackStart + 1U);
    CHECK(display.presentedFramebufferForTest() == physicalBeforeNack);

    Wire.ack[kTestDisplayAddress] = 0U;
    CHECK(display.service());  // page-window command now succeeds
    CHECK(display.presentedFramebufferForTest() == physicalBeforeNack);

    const std::size_t dataNackStart = Wire.transmissions.size();
    Wire.ack[kTestDisplayAddress] = 4U;
    CHECK(display.service());  // first data chunk is NACKed
    CHECK_EQ(Wire.transmissions.size(), dataNackStart + 1U);
    CHECK(display.presentedFramebufferForTest() == physicalBeforeNack);

    Wire.ack[kTestDisplayAddress] = 0U;
    while (display.service()) {
    }
    CHECK(display.presentedFramebufferForTest() == display.framebufferForTest());
#endif

    display.setFont(hal::DisplayFont::TempoLarge);
    CHECK_EQ(display.measureText("120",0,0).height, hal::font::kLargeTempoGlyphHeight);
    CHECK(display.measureText("---",0,0).width > 0U);
    display.drawText(20, 20, "120");
    display.drawText(20, 20, "---");

#if !CLOCK_DISPLAY_USE_SPI && !CLOCK_DISPLAY_I2C_ADDRESS
    resetFakes(); Wire.ack[0x3DU]=0U; hal::OledDisplay second; CHECK(second.begin());
    resetFakes(); hal::OledDisplay missing; CHECK(!missing.begin());
#endif
}

void setAllChannelsSimpleClock(ClockState& state) {
    for (auto& channel: state.channels) {
        channel.common.mode=ChannelMode::Clock; channel.common.rate={ClockRatioMode::Multiply,1U,1U,1U};
        channel.common.probabilityPercent=100U; channel.common.muted=false; channel.common.swingPercent=0U; channel.common.phasePercent=0U; channel.common.gateLengthMs=1U;
        channel.clock.meter={4U,4U};
    }
}

void runEngineTicks(engine::ClockEngine& engine, std::uint32_t count) { for(std::uint32_t i=0;i<count;++i) engine.processSchedulerTick(); }

void testEngineAndSettingsEditor() {
    resetFakes(); hal::GateOutputDriver gates; gates.beginDisabled();
    ClockState state=makeDefaultState(); setAllChannelsSimpleClock(state);
    engine::ClockEngine engine(gates); engine.begin(state);
    CHECK(!engine.snapshot().playing); engine.processSchedulerTick(); engine.play(); CHECK(engine.snapshot().playing);
    engine.setExternalLock(true,123456U); CHECK(engine.snapshot().externalLocked); CHECK_EQ(engine.snapshot().externalBpmMilli,123456U);
    runEngineTicks(engine,45050U); CHECK(engine.snapshot().masterBeatSerial>=4U); CHECK(engine.snapshot().masterBar>=2U);
    engine.pause(); CHECK(!engine.snapshot().playing); engine.play();
    engine.updateChannel(99U,state.channels[0],true);
    engine.updateConfiguration(state,false); engine.updateConfiguration(state,true);

    // Cover all event generators and gate outcomes.
    state.channels[0].common.mode=ChannelMode::Euclid; state.channels[0].euclid={4U,2U,0U};
    state.channels[1].common.mode=ChannelMode::Sequencer; state.channels[1].sequencer={4U,0U,0x5ULL};
    state.channels[2].common.mode=ChannelMode::Off;
    state.channels[2].common.muted=false;
    state.channels[3].common.probabilityPercent=0U;
    state.channels[4].common.swingPercent=25U;
    state.channels[5].clock.meter={0U,8U};
    state.channels[6].common.phasePercent=25U;
    state.channels[7].common.rate={ClockRatioMode::Multiply,32U,1U,1U};
    engine.updateConfiguration(state,true); engine.play(); runEngineTicks(engine,25000U);
    CHECK_EQ(fakefw::pinValues[pinmap::kGateChannelPins[2]], LOW);

    state.source=ClockSource::External; state.externalSync.lossMode=SyncLossMode::Stop; engine.updateConfiguration(state,false); engine.setExternalLock(false,0U);
    const auto before=engine.snapshot().masterPositionQ32; runEngineTicks(engine,20U); CHECK_EQ(engine.snapshot().masterPositionQ32,before);
    state.externalSync.lossMode=SyncLossMode::Freewheel; engine.updateConfiguration(state,false); runEngineTicks(engine,20U); CHECK(engine.snapshot().masterPositionQ32>before);
    state.bpm=0U; state.masterMeter.unit=0U; engine.updateConfiguration(state,true); runEngineTicks(engine,10U);
    state.channels[0].common.resetMode = ResetMode::Free; engine.updateConfiguration(state, true); engine.resetGlobalPhase();
    engine.play(); runEngineTicks(engine, 1000U); engine.resetGlobalPhaseFromIsr();
    CHECK(engine.snapshot().playing); CHECK_EQ(engine.snapshot().masterBeat, 1U); CHECK_EQ(engine.snapshot().masterBar, 1U);
    engine.stop(); CHECK(!engine.snapshot().playing);

    // Force rapid one-beat bars to cover the documented 1..999 bar counter wrap.
    state.bpm = 300U; state.masterMeter = {1U, 255U}; state.source = ClockSource::Internal;
    state.channels[0].common.resetMode = ResetMode::Global; engine.updateConfiguration(state, true); engine.play();
    runEngineTicks(engine, 70000U); CHECK(engine.snapshot().masterBar >= 1U && engine.snapshot().masterBar <= 999U);

    ui::SettingsEditor editor(state,engine);
    CHECK(!editor.executeSequencerCommand(0U,7U)); // paste before any copy
    state.bpm = 290U;
    engine.updateMasterTempo(state.bpm);
    state.tempoRange = {20U, 300U};
    fakefw::noInterruptCalls = 0U;
    fakefw::interruptCalls = 0U;
    editor.changeMasterTempo(127); CHECK_EQ(state.bpm,300U);
    CHECK_EQ(fakefw::noInterruptCalls, 1U);
    CHECK_EQ(fakefw::interruptCalls, 1U);
    fakefw::noInterruptCalls = 0U;
    fakefw::interruptCalls = 0U;
    editor.changeMasterTempo(127); CHECK_EQ(state.bpm,300U);
    CHECK_EQ(fakefw::noInterruptCalls, 0U);
    CHECK_EQ(fakefw::interruptCalls, 0U);
    editor.changeMasterTempo(-128); CHECK_EQ(state.bpm,172U);
    // Exercise every editable row and both directions.
    const ui::SettingsPage pages[]={ui::SettingsPage::Master,ui::SettingsPage::Sync,ui::SettingsPage::Channel,ui::SettingsPage::ChannelTiming,ui::SettingsPage::ChannelOutput,ui::SettingsPage::Clock,ui::SettingsPage::Euclid,ui::SettingsPage::Sequencer,ui::SettingsPage::SequencerPattern,ui::SettingsPage::UnifiedTiming,ui::SettingsPage::UnifiedOutput};
    for(auto page:pages){ const auto count=ui::settingsPageItemCount(page); for(std::uint8_t row=0;row<count;++row){ editor.adjust(page,row,0U,1); editor.adjust(page,row,0U,-1); } }
    editor.adjust(ui::SettingsPage::Info,0U,0U,1); editor.adjust(ui::SettingsPage::Root,0U,0U,1); editor.adjust(ui::SettingsPage::Master,0U,99U,1);
    editor.adjust(ui::SettingsPage::Master,99U,0U,1);
    state.externalSync.pulsesPerQuarterNote=3U; editor.adjust(ui::SettingsPage::Sync,1U,0U,1);
    editor.adjust(ui::SettingsPage::Sync,99U,0U,1);
    editor.adjust(ui::SettingsPage::ChannelTiming,99U,0U,1);
    editor.adjust(ui::SettingsPage::ChannelOutput,99U,0U,1);
    editor.adjust(ui::SettingsPage::Clock,99U,0U,1);
    editor.adjust(ui::SettingsPage::Euclid,99U,0U,1);
    editor.adjust(ui::SettingsPage::Sequencer,99U,0U,1);
    editor.adjust(ui::SettingsPage::SequencerPattern,99U,0U,1);
    editor.adjust(ui::SettingsPage::UnifiedTiming,99U,0U,1);
    editor.adjust(ui::SettingsPage::UnifiedOutput,99U,0U,1);
    editor.adjust(ui::SettingsPage::Channel,2U,99U,1);
    state.channels[0].euclid = {16U, 16U, 15U}; editor.adjust(ui::SettingsPage::Euclid,0U,0U,-15);
    state.channels[0].sequencer = {16U, 15U, 0xFFFFULL}; editor.adjust(ui::SettingsPage::Sequencer,1U,0U,-15);
    editor.toggleSequencerStep(0U,0U); editor.toggleSequencerStep(0U,63U); editor.toggleSequencerStep(99U,0U); editor.toggleSequencerStep(0U,64U);
    for(std::uint8_t row=0;row<10U;++row) (void)editor.executeSequencerCommand(0U,row);
    CHECK(!editor.executeSequencerCommand(99U,3U));
}


void testOperatingModesPersistenceAndGlobalEditors() {
    ClockState state = makeDefaultState();

    // Physical output resolution is deliberately independent from UI navigation.
    // Independent mode returns the selected channel unchanged.
    state.channels[3].common.mode = ChannelMode::Euclid;
    state.channels[3].euclid = {13U, 5U, 2U};
    ChannelConfig resolved = engine::resolvePhysicalOutputConfiguration(state, 3U);
    CHECK_EQ(resolved.common.mode, ChannelMode::Euclid);
    CHECK_EQ(resolved.euclid.steps, 13U);
    CHECK_EQ(engine::resolvePhysicalOutputConfiguration(state, 99U).common.mode, ChannelMode::Clock);

    // Unified-clock mode must make all eight physical outputs identical.
    state.operatingMode = OperatingMode::UnifiedClock;
    state.unifiedClock.rate = {ClockRatioMode::Divide, 3U, 5U, 4U};
    state.unifiedClock.swingPercent = 17U;
    state.unifiedClock.gateLengthMs = 20U;
    state.unifiedClock.phasePercent = 23U;
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        resolved = engine::resolvePhysicalOutputConfiguration(state, channelIndex);
        CHECK_EQ(resolved.common.mode, ChannelMode::Clock);
        CHECK_EQ(resolved.common.rate.mode, ClockRatioMode::Divide);
        CHECK_EQ(resolved.common.rate.factor, 3U);
        CHECK_EQ(resolved.common.rate.numerator, 5U);
        CHECK_EQ(resolved.common.rate.denominator, 4U);
        CHECK_EQ(resolved.common.swingPercent, 17U);
        CHECK_EQ(resolved.common.gateLengthMs, 20U);
        CHECK_EQ(resolved.common.phasePercent, 23U);
        CHECK_EQ(resolved.common.probabilityPercent, 100U);
        CHECK_EQ(resolved.common.resetMode, ResetMode::Global);
        CHECK(!resolved.common.muted);
    }

    // Each divider family is verified end-to-end, including the x1 first output.
    constexpr std::uint8_t expectedDivisors[3][kChannelCount] = {
        {1U, 2U, 4U, 8U, 16U, 32U, 64U, 128U},
        {1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U},
        {1U, 2U, 3U, 5U, 7U, 11U, 13U, 17U}};
    state.operatingMode = OperatingMode::DividerBank;
    state.dividerBank.gateLengthMs = 50U;
    for (std::uint8_t bankIndex = 0U; bankIndex < 3U; ++bankIndex) {
        state.dividerBank.bank = static_cast<DividerBank>(bankIndex);
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            resolved = engine::resolvePhysicalOutputConfiguration(state, channelIndex);
            const std::uint8_t divisor = expectedDivisors[bankIndex][channelIndex];
            CHECK_EQ(resolved.common.rate.factor, divisor);
            CHECK_EQ(
                resolved.common.rate.mode,
                divisor == 1U ? ClockRatioMode::Multiply : ClockRatioMode::Divide);
            CHECK_EQ(resolved.common.gateLengthMs, 50U);
            CHECK_EQ(resolved.common.swingPercent, 0U);
        }
    }

    // The graphical mode carousel maps both directions without hidden state.
    state = makeFactoryState();
    CHECK_EQ(state.operatingMode, OperatingMode::UnifiedClock);
    CHECK_EQ(ui::modeFunctionIndexForState(state, 2U), 0U);
    state.operatingMode = OperatingMode::Independent;
    state.channels[2].common.mode = ChannelMode::Off;
    CHECK_EQ(ui::modeFunctionIndexForState(state, 2U), 5U);
    state.channels[2].common.mode = ChannelMode::Clock;
    CHECK_EQ(ui::modeFunctionIndexForState(state, 2U), 2U);
    state.channels[2].common.mode = ChannelMode::Euclid;
    CHECK_EQ(ui::modeFunctionIndexForState(state, 2U), 3U);
    state.channels[2].common.mode = ChannelMode::Sequencer;
    CHECK_EQ(ui::modeFunctionIndexForState(state, 2U), 4U);
    state.operatingMode = OperatingMode::UnifiedClock;
    CHECK_EQ(ui::modeFunctionIndexForState(state, 2U), 0U);
    state.operatingMode = OperatingMode::DividerBank;
    CHECK_EQ(ui::modeFunctionIndexForState(state, 2U), 1U);
    state.operatingMode = OperatingMode::Independent;
    CHECK_EQ(ui::modeFunctionIndexForState(state, 99U), 2U);

    for (const ui::ModeFunction modeFunction : ui::kModeFunctions) {
        ClockState candidate = makeDefaultState();
        ui::applyModeFunction(candidate, 4U, modeFunction);
        if (modeFunction == ui::ModeFunction::UnifiedClock) {
            CHECK_EQ(candidate.operatingMode, OperatingMode::UnifiedClock);
        } else if (modeFunction == ui::ModeFunction::DividerBank) {
            CHECK_EQ(candidate.operatingMode, OperatingMode::DividerBank);
        } else {
            CHECK_EQ(candidate.operatingMode, OperatingMode::Independent);
        }
    }
    ClockState invalidChannel = makeDefaultState();
    ui::applyModeFunction(invalidChannel, 99U, ui::ModeFunction::Euclid);
    CHECK_EQ(invalidChannel.operatingMode, OperatingMode::Independent);

    // The preset-name character band must wrap deterministically and map an
    // unsupported stored byte back to the safe space entry.
    CHECK_EQ(ui::presetname::characterIndex(' '), 0U);
    CHECK_EQ(ui::presetname::characterIndex('Z'), 26U);
    CHECK_EQ(ui::presetname::characterIndex('?'), 0U);
    CHECK_EQ(ui::presetname::characterAtWrapped(0), ' ');
    CHECK_EQ(ui::presetname::characterAtWrapped(-1), '_');
    CHECK_EQ(ui::presetname::characterAtWrapped(static_cast<int>(ui::presetname::kAlphabetLength)), ' ');

    // New global mode state participates in the stable CRC-protected schema.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage persistenceStorage;
    services::PersistentStateService persistence(persistenceStorage);
    persistence.begin();
    ClockState durable = makeDefaultState();
    durable.bpm = 137U;
    durable.operatingMode = OperatingMode::DividerBank;
    durable.unifiedClock.rate = {ClockRatioMode::Multiply, 6U, 7U, 5U};
    durable.unifiedClock.swingPercent = 31U;
    durable.unifiedClock.gateLengthMs = 100U;
    durable.unifiedClock.phasePercent = 44U;
    durable.dividerBank.bank = DividerBank::Primes;
    durable.dividerBank.gateLengthMs = 20U;
    persistence.requestCurrentState(durable, 100U);
    persistence.service(100U + config::kPersistenceCommitDelayMs + 1U, true);
    services::PersistentStateService reloadedPersistence(persistenceStorage);
    reloadedPersistence.begin();
    ClockState restored = makeDefaultState();
    CHECK(reloadedPersistence.restoreCurrentState(restored));
    CHECK_EQ(restored.bpm, 137U);
    CHECK_EQ(restored.operatingMode, OperatingMode::DividerBank);
    CHECK_EQ(restored.unifiedClock.rate.factor, 6U);
    CHECK_EQ(restored.unifiedClock.rate.numerator, 7U);
    CHECK_EQ(restored.unifiedClock.rate.denominator, 5U);
    CHECK_EQ(restored.unifiedClock.swingPercent, 31U);
    CHECK_EQ(restored.unifiedClock.gateLengthMs, 100U);
    CHECK_EQ(restored.unifiedClock.phasePercent, 44U);
    CHECK_EQ(restored.dividerBank.bank, DividerBank::Primes);
    CHECK_EQ(restored.dividerBank.gateLengthMs, 20U);

    // Exercise all editable values on the two new global settings pages.
    resetFakes();
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    state = makeDefaultState();
    engine::ClockEngine clockEngine(gates);
    clockEngine.begin(state);
    ui::SettingsEditor editor(state, clockEngine);
    for (std::uint8_t row = 0U; row < ui::settingsPageItemCount(ui::SettingsPage::UnifiedTiming); ++row) {
        editor.adjust(ui::SettingsPage::UnifiedTiming, row, 0U, 1);
        editor.adjust(ui::SettingsPage::UnifiedTiming, row, 0U, -1);
    }
    for (std::uint8_t row = 0U; row < ui::settingsPageItemCount(ui::SettingsPage::UnifiedOutput); ++row) {
        editor.adjust(ui::SettingsPage::UnifiedOutput, row, 0U, 1);
        editor.adjust(ui::SettingsPage::UnifiedOutput, row, 0U, -1);
    }
    // Also exercise a non-listed ratio so the rate-option fallback branch is covered.
    state.unifiedClock.rate = {ClockRatioMode::Multiply, 99U, 1U, 1U};
    editor.adjust(ui::SettingsPage::UnifiedTiming, 0U, 0U, 1);
    editor.adjust(ui::SettingsPage::UnifiedTiming, 99U, 0U, 1);
    editor.adjust(ui::SettingsPage::UnifiedOutput, 99U, 0U, 1);
    for (std::uint8_t row = 0U; row < ui::settingsPageItemCount(ui::SettingsPage::DividerBank); ++row) {
        editor.adjust(ui::SettingsPage::DividerBank, row, 0U, 1);
        editor.adjust(ui::SettingsPage::DividerBank, row, 0U, -1);
    }
    editor.adjust(ui::SettingsPage::DividerBank, 99U, 0U, 1);
}


void testChannelRescheduleKeepsSharedMusicalEpoch() {
    resetFakes();
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    gates.enableOutputStage();

    ClockState state = makeDefaultState();
    setAllChannelsSimpleClock(state);
    state.bpm = 120U;

    // Three equivalent musical streams: CLOCK emits quarter-note edges while
    // EUCLID and SEQ emit every sixteenth. Their quarter boundaries must coincide.
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[1].common.mode = ChannelMode::Euclid;
    state.channels[1].euclid = {16U, 16U, 0U};
    state.channels[2].common.mode = ChannelMode::Sequencer;
    state.channels[2].sequencer = {16U, 0U, 0xFFFFULL};
    for (std::size_t channelIndex = 0U; channelIndex < 3U; ++channelIndex) {
        state.channels[channelIndex].common.resetMode = ResetMode::Global;
        state.channels[channelIndex].common.swingPercent = 0U;
        state.channels[channelIndex].common.phasePercent = 0U;
        state.channels[channelIndex].common.probabilityPercent = 100U;
        state.channels[channelIndex].common.gateLengthMs = 1U;
    }

    engine::ClockEngine engine(gates);
    engine.begin(state);
    engine.play();

    // Edit the two pattern channels deliberately between grid lines. The old
    // scheduler implementation re-anchored them at this exact instant, causing
    // a permanent visible phase offset from the CLOCK channel.
    runEngineTicks(engine, 3333U);
    engine.updateChannel(1U, state.channels[1], true);
    engine.updateChannel(2U, state.channels[2], true);

    bool foundClockBoundary = false;
    for (std::uint32_t tick = 0U; tick < 8000U && !foundClockBoundary; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();

        const auto wroteHigh = [](const std::uint32_t pin) {
            return std::any_of(
                fakefw::writes.begin(),
                fakefw::writes.end(),
                [pin](const fakefw::PinWrite& write) {
                    return write.pin == pin && write.value == HIGH;
                });
        };

        if (wroteHigh(pinmap::kGateChannelPins[0])) {
            foundClockBoundary = true;
            CHECK(wroteHigh(pinmap::kGateChannelPins[1]));
            CHECK(wroteHigh(pinmap::kGateChannelPins[2]));
        }
    }
    CHECK(foundClockBoundary);
}



void testCrossModeSynchronizationRegressions() {
    const auto wroteHigh = [](const std::uint32_t pin) {
        return std::any_of(
            fakefw::writes.begin(),
            fakefw::writes.end(),
            [pin](const fakefw::PinWrite& write) {
                return write.pin == pin && write.value == HIGH;
            });
    };

    resetFakes();
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    gates.enableOutputStage();

    ClockState state = makeDefaultState();
    for (auto& channel : state.channels) {
        channel.common.mode = ChannelMode::Off;
    }
    state.bpm = 120U;
    state.masterMeter = {4U, 4U};

    // CLOCK x4, EUCLID x1, and SEQ x1 are the same sixteenth-note grid.
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 4U, 1U, 1U};
    state.channels[0].clock.meter = {4U, 4U};
    state.channels[1].common.mode = ChannelMode::Euclid;
    state.channels[1].euclid = {16U, 16U, 0U};
    state.channels[2].common.mode = ChannelMode::Sequencer;
    state.channels[2].sequencer = {16U, 0U, 0xFFFFULL};
    for (std::size_t channelIndex = 0U; channelIndex < 3U; ++channelIndex) {
        state.channels[channelIndex].common.resetMode = ResetMode::Global;
        state.channels[channelIndex].common.gateLengthMs = 1U;
        state.channels[channelIndex].common.probabilityPercent = 100U;
        state.channels[channelIndex].common.phasePercent = 0U;
        state.channels[channelIndex].common.swingPercent = 0U;
    }

    engine::ClockEngine engine(gates);
    engine.begin(state);
    engine.play();

    // Step 1 belongs to the shared downbeat, not one complete local interval later.
    fakefw::writes.clear();
    engine.processSchedulerTick();
    CHECK(wroteHigh(pinmap::kGateChannelPins[0]));
    CHECK(wroteHigh(pinmap::kGateChannelPins[1]));
    CHECK(wroteHigh(pinmap::kGateChannelPins[2]));

    // A small positive phase must move an edge slightly later. It must not jump
    // from "one full period late" to "almost immediate" at PHASE 0 -> 1.
    engine.stop();
    state.channels[0].common.phasePercent = 0U;
    state.channels[1].common.mode = ChannelMode::Clock;
    state.channels[1].common.rate = state.channels[0].common.rate;
    state.channels[1].clock.meter = state.channels[0].clock.meter;
    state.channels[1].common.phasePercent = 1U;
    state.channels[2].common.mode = ChannelMode::Off;
    engine.updateConfiguration(state, true);
    engine.play();
    std::uint32_t phaseZeroTick = 0xFFFFFFFFUL;
    std::uint32_t phaseOneTick = 0xFFFFFFFFUL;
    for (std::uint32_t tick = 0U; tick < 200U; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();
        if (phaseZeroTick == 0xFFFFFFFFUL && wroteHigh(pinmap::kGateChannelPins[0])) {
            phaseZeroTick = tick;
        }
        if (phaseOneTick == 0xFFFFFFFFUL && wroteHigh(pinmap::kGateChannelPins[1])) {
            phaseOneTick = tick;
        }
    }
    CHECK(phaseZeroTick != 0xFFFFFFFFUL);
    CHECK(phaseOneTick != 0xFFFFFFFFUL);
    CHECK(phaseOneTick > phaseZeroTick);
    CHECK(phaseOneTick - phaseZeroTick < 100U);

    // A GLOBAL mode switch must derive the pattern step from the shared epoch.
    // The old implementation kept CLOCK's local step counter, so the new SEQ was
    // edge-aligned but evaluated the wrong bit in the pattern.
    engine.stop();
    for (auto& channel : state.channels) channel.common.mode = ChannelMode::Off;
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[0].clock.meter = {4U, 4U};
    state.channels[0].common.resetMode = ResetMode::Global;
    state.channels[0].common.phasePercent = 0U;
    state.channels[0].common.swingPercent = 0U;
    state.channels[1].common.mode = ChannelMode::Sequencer;
    state.channels[1].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[1].sequencer = {16U, 0U, 0x1ULL};
    state.channels[1].common.resetMode = ResetMode::Global;
    state.channels[1].common.phasePercent = 0U;
    state.channels[1].common.swingPercent = 0U;
    state.channels[1].common.gateLengthMs = 1U;
    state.channels[1].common.probabilityPercent = 100U;
    engine.updateConfiguration(state, true);
    engine.play();
    runEngineTicks(engine, 7000U);

    state.channels[0].common.mode = ChannelMode::Sequencer;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[0].sequencer = {16U, 0U, 0x1ULL};
    state.channels[0].common.gateLengthMs = 1U;
    engine.updateChannel(0U, state.channels[0], true);

    bool sawReferenceHit = false;
    for (std::uint32_t tick = 0U; tick < 40000U && !sawReferenceHit; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();
        const bool switchedHigh = wroteHigh(pinmap::kGateChannelPins[0]);
        const bool referenceHigh = wroteHigh(pinmap::kGateChannelPins[1]);
        if (switchedHigh || referenceHigh) {
            CHECK(switchedHigh == referenceHigh);
        }
        sawReferenceHit = referenceHigh;
    }
    CHECK(sawReferenceHit);

    // Rescheduling only one GLOBAL channel must reconstruct swing parity from
    // the common epoch; otherwise it falls back to the unswung grid.
    engine.stop();
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 4U, 1U, 1U};
    state.channels[0].clock.meter = {4U, 4U};
    state.channels[1].common.mode = ChannelMode::Euclid;
    state.channels[1].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[1].euclid = {16U, 16U, 0U};
    state.channels[2].common.mode = ChannelMode::Sequencer;
    state.channels[2].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[2].sequencer = {16U, 0U, 0xFFFFULL};
    for (std::size_t channelIndex = 0U; channelIndex < 3U; ++channelIndex) {
        state.channels[channelIndex].common.resetMode = ResetMode::Global;
        state.channels[channelIndex].common.swingPercent = 25U;
        state.channels[channelIndex].common.phasePercent = 0U;
        state.channels[channelIndex].common.gateLengthMs = 1U;
        state.channels[channelIndex].common.probabilityPercent = 100U;
    }
    engine.updateConfiguration(state, true);
    engine.play();
    runEngineTicks(engine, 1000U);
    engine.updateChannel(1U, state.channels[1], true);
    engine.updateChannel(2U, state.channels[2], true);

    bool sawSwungBoundary = false;
    for (std::uint32_t tick = 0U; tick < 4000U && !sawSwungBoundary; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();
        const bool clockHigh = wroteHigh(pinmap::kGateChannelPins[0]);
        const bool euclidHigh = wroteHigh(pinmap::kGateChannelPins[1]);
        const bool sequencerHigh = wroteHigh(pinmap::kGateChannelPins[2]);
        if (clockHigh || euclidHigh || sequencerHigh) {
            CHECK(clockHigh == euclidHigh);
            CHECK(clockHigh == sequencerHigh);
        }
        sawSwungBoundary = clockHigh;
    }
    CHECK(sawSwungBoundary);

    // Equivalent sixteenth grids also remain coherent when the master beat is an eighth note.
    engine.stop();
    state.masterMeter = {7U, 8U};
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[0].clock.meter = {14U, 16U};
    state.channels[0].common.swingPercent = 0U;
    state.channels[1].common.mode = ChannelMode::Euclid;
    state.channels[1].common.swingPercent = 0U;
    state.channels[2].common.mode = ChannelMode::Sequencer;
    state.channels[2].common.swingPercent = 0U;
    engine.updateConfiguration(state, true);
    engine.play();
    for (std::uint32_t event = 0U; event < 12U; ++event) {
        bool found = false;
        for (std::uint32_t tick = 0U; tick < 4000U && !found; ++tick) {
            fakefw::writes.clear();
            engine.processSchedulerTick();
            const bool clockHigh = wroteHigh(pinmap::kGateChannelPins[0]);
            const bool euclidHigh = wroteHigh(pinmap::kGateChannelPins[1]);
            const bool sequencerHigh = wroteHigh(pinmap::kGateChannelPins[2]);
            if (clockHigh || euclidHigh || sequencerHigh) {
                CHECK(clockHigh == euclidHigh);
                CHECK(clockHigh == sequencerHigh);
            }
            found = clockHigh;
        }
        CHECK(found);
    }

    // Changing GLOBAL Euclid STEPS or Sequencer LENGTH must change the pattern
    // modulo immediately without detaching pattern phase from the common event serial.
    engine.stop();
    state.masterMeter = {4U, 4U};
    state.source = ClockSource::Internal;
    for (auto& channel : state.channels) channel.common.mode = ChannelMode::Off;
    state.channels[0].common.mode = ChannelMode::Sequencer;
    state.channels[0].sequencer = {7U, 0U, 0x7FULL};
    state.channels[1].common.mode = ChannelMode::Sequencer;
    state.channels[1].sequencer = {16U, 0U, 0xFFFFULL};
    state.channels[2].common.mode = ChannelMode::Euclid;
    state.channels[2].euclid = {16U, 16U, 0U};
    for (std::size_t channelIndex = 0U; channelIndex < 3U; ++channelIndex) {
        state.channels[channelIndex].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
        state.channels[channelIndex].common.resetMode = ResetMode::Global;
        state.channels[channelIndex].common.swingPercent = 0U;
        state.channels[channelIndex].common.phasePercent = 0U;
        state.channels[channelIndex].common.gateLengthMs = 1U;
        state.channels[channelIndex].common.probabilityPercent = 100U;
    }
    engine.updateConfiguration(state, true);
    engine.play();
    runEngineTicks(engine, 14000U);
    state.channels[1].sequencer.length = 7U;
    state.channels[1].sequencer.pattern = 0x7FULL;
    state.channels[2].euclid.steps = 7U;
    state.channels[2].euclid.hits = 7U;
    engine.updateChannel(1U, state.channels[1], false);
    engine.updateChannel(2U, state.channels[2], false);

    bool sawEditedLengthBoundary = false;
    for (std::uint32_t tick = 0U; tick < 4000U && !sawEditedLengthBoundary; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();
        const bool referenceHigh = wroteHigh(pinmap::kGateChannelPins[0]);
        const bool sequencerHigh = wroteHigh(pinmap::kGateChannelPins[1]);
        const bool euclidHigh = wroteHigh(pinmap::kGateChannelPins[2]);
        if (referenceHigh || sequencerHigh || euclidHigh) {
            CHECK(referenceHigh);
            CHECK(sequencerHigh);
            CHECK(euclidHigh);
            const auto snapshot = engine.snapshot();
            CHECK_EQ(snapshot.channelStep[0], snapshot.channelStep[1]);
            CHECK_EQ(snapshot.channelStep[0], snapshot.channelStep[2]);
            CHECK(snapshot.channelStep[1] < 7U);
            CHECK(snapshot.channelStep[2] < 7U);
            sawEditedLengthBoundary = true;
        }
    }
    CHECK(sawEditedLengthBoundary);

    // The same common grid must stay coherent when the master timeline is
    // driven by external pulses. Pulse-phase correction is global and must not
    // make CLOCK, EUCLID, and SEQ disagree about a boundary or pattern step.
    engine.stop();
    state.masterMeter = {4U, 4U};
    state.source = ClockSource::External;
    state.externalSync.lossMode = SyncLossMode::Freewheel;
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 4U, 1U, 1U};
    state.channels[0].clock.meter = {4U, 4U};
    state.channels[1].common.mode = ChannelMode::Euclid;
    state.channels[1].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[1].euclid = {16U, 16U, 0U};
    state.channels[2].common.mode = ChannelMode::Sequencer;
    state.channels[2].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[2].sequencer = {16U, 0U, 0xFFFFULL};
    for (std::size_t channelIndex = 0U; channelIndex < 3U; ++channelIndex) {
        state.channels[channelIndex].common.resetMode = ResetMode::Global;
        state.channels[channelIndex].common.swingPercent = 0U;
        state.channels[channelIndex].common.phasePercent = 0U;
        state.channels[channelIndex].common.gateLengthMs = 1U;
        state.channels[channelIndex].common.probabilityPercent = 100U;
    }
    engine.updateConfiguration(state, true);
    engine.setExternalLock(true, 120000U);
    engine.play();
    engine.acceptExternalPulse(120000U, 1U);

    std::uint32_t externalBoundaries = 0U;
    for (std::uint32_t tick = 0U; tick < 40000U; ++tick) {
        if (tick != 0U && (tick % 10000U) == 0U) {
            engine.acceptExternalPulse(120000U, 1U);
        }
        fakefw::writes.clear();
        engine.processSchedulerTick();
        const bool clockHigh = wroteHigh(pinmap::kGateChannelPins[0]);
        const bool euclidHigh = wroteHigh(pinmap::kGateChannelPins[1]);
        const bool sequencerHigh = wroteHigh(pinmap::kGateChannelPins[2]);
        if (clockHigh || euclidHigh || sequencerHigh) {
            CHECK(clockHigh);
            CHECK(euclidHigh);
            CHECK(sequencerHigh);
            ++externalBoundaries;
        }
    }
    CHECK(externalBoundaries >= 16U);

    // Deterministic cross-mode property matrix. CLOCK with a 1/16 local meter,
    // EUCLID x1, and SEQ x1 are the same mathematical grid. Applying the same
    // rational rate, swing, and phase must therefore produce identical edges
    // before and after individual live reschedules, independent of master beat unit.
    struct RationalGridCase final {
        std::uint8_t masterUnit;
        std::uint8_t numerator;
        std::uint8_t denominator;
        std::uint8_t swing;
        std::uint8_t phase;
    };
    constexpr RationalGridCase gridCases[] = {
        {4U, 3U, 2U, 0U, 0U},
        {4U, 5U, 7U, 25U, 17U},
        {8U, 7U, 5U, 10U, 33U},
        {8U, 11U, 8U, 50U, 1U},
        {16U, 13U, 9U, 25U, 49U},
        {2U, 16U, 3U, 0U, 73U},
    };

    for (const auto& gridCase : gridCases) {
        engine.stop();
        state = makeDefaultState();
        for (auto& channel : state.channels) {
            channel.common.mode = ChannelMode::Off;
        }
        state.bpm = 137U;
        state.masterMeter = {7U, gridCase.masterUnit};
        state.channels[0].common.mode = ChannelMode::Clock;
        state.channels[0].clock.meter = {16U, 16U};
        state.channels[1].common.mode = ChannelMode::Euclid;
        state.channels[1].euclid = {16U, 16U, 0U};
        state.channels[2].common.mode = ChannelMode::Sequencer;
        state.channels[2].sequencer = {16U, 0U, 0xFFFFULL};
        for (std::size_t channelIndex = 0U; channelIndex < 3U; ++channelIndex) {
            auto& common = state.channels[channelIndex].common;
            common.rate = {ClockRatioMode::Multiply, 1U, gridCase.numerator, gridCase.denominator};
            common.resetMode = ResetMode::Global;
            common.swingPercent = gridCase.swing;
            common.phasePercent = gridCase.phase;
            common.gateLengthMs = 1U;
            common.probabilityPercent = 100U;
        }
        engine.updateConfiguration(state, true);
        engine.play();

        // Deliberately reschedule the pattern channels at a non-grid scheduler tick.
        runEngineTicks(engine, 1237U);
        engine.updateChannel(1U, state.channels[1], true);
        engine.updateChannel(2U, state.channels[2], true);

        std::uint32_t matchedEdges = 0U;
        for (std::uint32_t tick = 0U; tick < 50000U && matchedEdges < 4U; ++tick) {
            fakefw::writes.clear();
            engine.processSchedulerTick();
            const bool clockHigh = wroteHigh(pinmap::kGateChannelPins[0]);
            const bool euclidHigh = wroteHigh(pinmap::kGateChannelPins[1]);
            const bool sequencerHigh = wroteHigh(pinmap::kGateChannelPins[2]);
            if (clockHigh || euclidHigh || sequencerHigh) {
                CHECK(clockHigh);
                CHECK(euclidHigh);
                CHECK(sequencerHigh);
                const auto gridSnapshot = engine.snapshot();
                CHECK_EQ(gridSnapshot.channelStep[0], gridSnapshot.channelStep[1]);
                CHECK_EQ(gridSnapshot.channelStep[0], gridSnapshot.channelStep[2]);
                ++matchedEdges;
            }
        }
        CHECK_EQ(matchedEdges, 4U);
    }
}



void testOneClockHumanizeAndTempoLimits() {
    const auto wroteHigh = [](const std::uint32_t pin) {
        return std::any_of(fakefw::writes.begin(), fakefw::writes.end(), [pin](const fakefw::PinWrite& write) {
            return write.pin == pin && write.value == HIGH;
        });
    };

    ClockState state = makeDefaultState();
    CHECK_EQ(state.tempoRange.minimumBpm, 20U);
    CHECK_EQ(state.tempoRange.maximumBpm, 999U);
    CHECK_EQ(config::kSupportedMinimumBpm, 1U);
    CHECK_EQ(config::kSupportedMaximumBpm, 999U);

    resetFakes();
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    gates.enableOutputStage();
    engine::ClockEngine engine(gates);
    engine.begin(state);
    ui::SettingsEditor editor(state, engine);

    // User MIN/MAX constrain manual tempo and follow the current BPM when tightened.
    state.bpm = 120U;
    state.tempoRange = {20U, 999U};
    editor.adjust(ui::SettingsPage::Master, 1U, 0U, 127);
    CHECK_EQ(state.tempoRange.minimumBpm, 147U);
    CHECK_EQ(state.bpm, 147U);
    editor.adjust(ui::SettingsPage::Master, 2U, 0U, -128);
    CHECK_EQ(state.tempoRange.maximumBpm, 871U);
    CHECK_EQ(state.bpm, 147U);
    editor.adjust(ui::SettingsPage::Master, 1U, 0U, -128);
    CHECK_EQ(state.tempoRange.minimumBpm, 19U);
    editor.adjust(ui::SettingsPage::Master, 1U, 0U, -128);
    CHECK_EQ(state.tempoRange.minimumBpm, 1U);
    editor.adjust(ui::SettingsPage::Master, 2U, 0U, 127);
    editor.adjust(ui::SettingsPage::Master, 2U, 0U, 127);
    CHECK(state.tempoRange.maximumBpm <= config::kSupportedMaximumBpm);

    state.tempoRange = {20U, 999U};
    state.bpm = 998U;
    editor.changeMasterTempo(5);
    CHECK_EQ(state.bpm, 999U);
    state.bpm = 21U;
    editor.changeMasterTempo(-5);
    CHECK_EQ(state.bpm, 20U);

    const auto minRow = ui::buildMenuRow(ui::SettingsPage::Master, 1U, 0U, state);
    const auto maxRow = ui::buildMenuRow(ui::SettingsPage::Master, 2U, 0U, state);
    CHECK(std::strcmp(minRow.label, "MIN BPM") == 0);
    CHECK(std::strcmp(minRow.value, "20") == 0);
    CHECK(std::strcmp(maxRow.label, "MAX BPM") == 0);
    CHECK(std::strcmp(maxRow.value, "999") == 0);

    services::TapTempo tap;
    CHECK_EQ(tap.registerTap(1000U, 20U, 999U), 0U);
    CHECK_EQ(tap.registerTap(1060U, 20U, 999U), 999U);

    // External sync is not clamped by the user's manual BPM range.
    state.tempoRange = {90U, 110U};
    state.source = ClockSource::External;
    engine.updateConfiguration(state, false);
    engine.setExternalLock(true, 480000U);
    CHECK(engine.snapshot().externalLocked);
    CHECK_EQ(engine.snapshot().externalBpmMilli, 480000U);

    // Humanize exists at the timing boundary only in One Clock (UnifiedClock).
    state = makeDefaultState();
    state.bpm = 120U;
    state.operatingMode = OperatingMode::UnifiedClock;
    state.unifiedClock.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.unifiedClock.swingPercent = 0U;
    state.unifiedClock.phasePercent = 0U;
    state.unifiedClock.gateLengthMs = 1U;
    state.unifiedClock.humanizeUs = 2000U;
    engine.stop();
    engine.updateConfiguration(state, true);
    engine.play();

    fakefw::writes.clear();
    engine.processSchedulerTick();
    for (const auto pin : pinmap::kGateChannelPins) CHECK(wroteHigh(pin));

    std::array<std::uint32_t, kChannelCount> humanizedTick{};
    humanizedTick.fill(0xFFFFFFFFUL);
    for (std::uint32_t tick = 1U; tick < 10100U; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            if (humanizedTick[channelIndex] == 0xFFFFFFFFUL && wroteHigh(pinmap::kGateChannelPins[channelIndex])) {
                humanizedTick[channelIndex] = tick;
            }
        }
    }
    std::uint32_t minimumTick = 0xFFFFFFFFUL;
    std::uint32_t maximumTick = 0U;
    for (const std::uint32_t tick : humanizedTick) {
        CHECK(tick != 0xFFFFFFFFUL);
        CHECK(tick >= 9959U);
        CHECK(tick <= 10041U);
        minimumTick = std::min(minimumTick, tick);
        maximumTick = std::max(maximumTick, tick);
    }
    CHECK(maximumTick > minimumTick);

    const auto humanizeRow = ui::buildMenuRow(ui::SettingsPage::UnifiedTiming, 5U, 0U, state);
    CHECK(std::strcmp(humanizeRow.label, "HUMANIZE") == 0);
    CHECK(std::strstr(humanizeRow.value, "2000") != nullptr);
    editor.adjust(ui::SettingsPage::UnifiedTiming, 5U, 0U, -1);
    CHECK_EQ(state.unifiedClock.humanizeUs, 1000U);
    state.unifiedClock.humanizeUs = 0U;
    const auto humanizeOffRow = ui::buildMenuRow(ui::SettingsPage::UnifiedTiming, 5U, 0U, state);
    CHECK(std::strcmp(humanizeOffRow.value, "OFF") == 0);

    // A stored Humanize value has no timing effect outside One Clock.
    engine.stop();
    state = makeDefaultState();
    setAllChannelsSimpleClock(state);
    state.operatingMode = OperatingMode::Independent;
    state.unifiedClock.humanizeUs = 2000U;
    engine.updateConfiguration(state, true);
    engine.play();
    fakefw::writes.clear();
    engine.processSchedulerTick();
    std::array<std::uint32_t, kChannelCount> independentTick{};
    independentTick.fill(0xFFFFFFFFUL);
    for (std::uint32_t tick = 1U; tick < 10050U; ++tick) {
        fakefw::writes.clear();
        engine.processSchedulerTick();
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            if (independentTick[channelIndex] == 0xFFFFFFFFUL && wroteHigh(pinmap::kGateChannelPins[channelIndex])) {
                independentTick[channelIndex] = tick;
            }
        }
    }
    for (std::size_t channelIndex = 1U; channelIndex < kChannelCount; ++channelIndex) {
        CHECK_EQ(independentTick[channelIndex], independentTick[0]);
    }
}

void testEngineAuditRegressions() {
    resetFakes();
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    gates.enableOutputStage();

    // External tempo, not the stale internal BPM setting, must limit gate length.
    ClockState state = makeDefaultState();
    for (auto& channel : state.channels) channel.common.mode = ChannelMode::Off;
    state.bpm = 60U;
    state.source = ClockSource::External;
    state.externalSync.lossMode = SyncLossMode::Freewheel;
    state.channels[0].common.mode = ChannelMode::Clock;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 4U, 1U, 1U};
    state.channels[0].common.gateLengthMs = 100U;
    state.channels[0].common.probabilityPercent = 100U;
    engine::ClockEngine externalGateEngine(gates);
    externalGateEngine.begin(state);
    externalGateEngine.setExternalLock(true, 300000U);
    externalGateEngine.play();
    fakefw::writes.clear();
    runEngineTicks(externalGateEngine, 30000U);
    const auto gatePin = pinmap::kGateChannelPins[0];
    const auto countPinValue = [gatePin](const std::uint8_t value) {
        return static_cast<std::size_t>(std::count_if(
            fakefw::writes.begin(), fakefw::writes.end(),
            [gatePin, value](const fakefw::PinWrite& write) {
                return write.pin == gatePin && write.value == value;
            }));
    };
    CHECK(countPinValue(HIGH) > 10U);
    CHECK(countPinValue(LOW) > 10U);

    // External phase references may correct schedules, but scheduler time is monotonic.
    state.bpm = 120U;
    state.source = ClockSource::External;
    state.channels[0].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[0].common.gateLengthMs = 1U;
    externalGateEngine.stop();
    externalGateEngine.updateConfiguration(state, true);
    externalGateEngine.setExternalLock(true, 60000U);
    externalGateEngine.play();
    runEngineTicks(externalGateEngine, 1234U);
    externalGateEngine.acceptExternalPulse(60000U, 1U);
    CHECK_EQ(externalGateEngine.snapshot().masterBeatPhaseQ32, 0ULL);
    runEngineTicks(externalGateEngine, 22000U);
    const auto latePulsePosition = externalGateEngine.snapshot().masterPositionQ32;
    externalGateEngine.acceptExternalPulse(60000U, 1U);
    CHECK_EQ(externalGateEngine.snapshot().masterPositionQ32, latePulsePosition);
    CHECK_EQ(externalGateEngine.snapshot().masterBeatPhaseQ32, 0ULL);
    runEngineTicks(externalGateEngine, 18000U);
    const auto earlyPulsePosition = externalGateEngine.snapshot().masterPositionQ32;
    externalGateEngine.acceptExternalPulse(60000U, 1U);
    CHECK_EQ(externalGateEngine.snapshot().masterPositionQ32, earlyPulsePosition);
    CHECK_EQ(externalGateEngine.snapshot().masterBeatPhaseQ32, 0ULL);

    // Input-capture ISR integration must not re-enable interrupts from inside the ISR.
    fakefw::noInterruptCalls = 0U;
    fakefw::interruptCalls = 0U;
    externalGateEngine.acceptExternalPulseFromIsr(60000U, 1U);
    CHECK_EQ(fakefw::noInterruptCalls, 0U);
    CHECK_EQ(fakefw::interruptCalls, 0U);

    // Full-state updates must force an already-active channel LOW when it becomes muted.
    ClockState muteState = makeDefaultState();
    for (auto& channel : muteState.channels) channel.common.mode = ChannelMode::Off;
    muteState.bpm = 300U;
    muteState.channels[0].common.mode = ChannelMode::Clock;
    muteState.channels[0].common.gateLengthMs = 100U;
    engine::ClockEngine muteEngine(gates);
    muteEngine.begin(muteState);
    muteEngine.play();
    for (std::uint32_t tick = 0U; tick < 5000U && fakefw::pinValues[gatePin] != HIGH; ++tick) {
        muteEngine.processSchedulerTick();
    }
    CHECK_EQ(fakefw::pinValues[gatePin], HIGH);
    muteState.channels[0].common.muted = true;
    muteEngine.updateConfiguration(muteState, false);
    CHECK_EQ(fakefw::pinValues[gatePin], LOW);

    // Legal maximum-rate/swing settings must not silently lose the short swing events.
    ClockState fastState = makeDefaultState();
    for (auto& channel : fastState.channels) channel.common.mode = ChannelMode::Off;
    fastState.bpm = 300U;
    fastState.masterMeter.unit = 16U;
    ChannelConfig& fast = fastState.channels[0];
    fast.common.mode = ChannelMode::Clock;
    fast.common.rate = {ClockRatioMode::Multiply, 32U, 16U, 1U};
    fast.common.swingPercent = 50U;
    fast.common.gateLengthMs = 1U;
    fast.clock.meter.unit = 16U;
    engine::ClockEngine fastEngine(gates);
    fastEngine.begin(fastState);
    fastEngine.play();
    fakefw::writes.clear();
    runEngineTicks(fastEngine, 2000U);
    const std::size_t fastRisingEdges = static_cast<std::size_t>(std::count_if(
        fakefw::writes.begin(), fakefw::writes.end(),
        [gatePin](const fakefw::PinWrite& write) {
            return write.pin == gatePin && write.value == HIGH;
        }));
    CHECK(fastRisingEdges >= 1020U);
    CHECK(fastRisingEdges <= 1025U);
}

void testEngineBoundaryBranches() {
    resetFakes();
    hal::GateOutputDriver gates;
    gates.beginDisabled();
    ClockState state=makeDefaultState();
    setAllChannelsSimpleClock(state);
    state.bpm=300U;

    engine::ClockEngine engine(gates);
    engine.begin(state);
    engine.play();

    // EXTERNAL + STOP must continue when lock is present.
    state.source=ClockSource::External;
    state.externalSync.lossMode=SyncLossMode::Stop;
    engine.updateConfiguration(state,true);
    engine.setExternalLock(true,120000U);
    const auto lockedBefore=engine.snapshot().masterPositionQ32;
    runEngineTicks(engine,10U);
    CHECK(engine.snapshot().masterPositionQ32>lockedBefore);

    // External pulses preserve sub-BPM tempo and establish a phase reference.
    engine.acceptExternalPulse(60000U, 1U);
    const auto firstPulse = engine.snapshot();
    CHECK(firstPulse.externalLocked);
    CHECK_EQ(firstPulse.externalBpmMilli, 60000U);
    engine.acceptExternalPulse(60000U, 1U);
    CHECK(engine.snapshot().masterPositionQ32 >= firstPulse.masterPositionQ32);
    const auto externalBefore = engine.snapshot().masterPositionQ32;
    runEngineTicks(engine, 20000U);
    const auto externalDelta = engine.snapshot().masterPositionQ32 - externalBefore;
    CHECK(externalDelta > core::kQ32One * 9ULL / 10ULL);
    CHECK(externalDelta < core::kQ32One * 11ULL / 10ULL);
    engine.setExternalLock(false, 60000U);

    // Zero beats-per-bar is defensively interpreted as a one-beat bar.
    state.source=ClockSource::Internal;
    state.masterMeter={0U,4U};
    engine.updateConfiguration(state,true);
    runEngineTicks(engine,5000U);
    CHECK(engine.snapshot().masterBar>=2U);

    // A deliberately extreme legal rate creates more than eight due events per
    // scheduler quantum and exercises the ISR event-guard saturation path.
    state.masterMeter={4U,4U};
    state.channels[0].common.rate={ClockRatioMode::Multiply,255U,255U,1U};
    state.channels[0].common.probabilityPercent=100U;
    engine.updateConfiguration(state,true);
    runEngineTicks(engine,1U);

    // Keep the backlog but set zero BPM/unit. Due channel events still need safe
    // pulse-width arithmetic even though the master timeline no longer advances.
    state.bpm=0U;
    state.masterMeter.unit=0U;
    engine.updateConfiguration(state,false);
    runEngineTicks(engine,1U);

    // Zero-sized local cycles are defensively folded to one step. This must hold
    // across all three timing modes so GLOBAL step reconstruction never divides by zero.
    state.bpm=300U;
    state.masterMeter.unit=4U;
    state.channels[0].common.resetMode=ResetMode::Global;
    state.channels[0].common.mode=ChannelMode::Euclid;
    state.channels[0].euclid.steps=0U;
    engine.updateChannel(0U,state.channels[0],false);
    CHECK_EQ(engine.snapshot().channelStep[0],0U);
    state.channels[0].common.mode=ChannelMode::Sequencer;
    state.channels[0].sequencer.length=0U;
    engine.updateChannel(0U,state.channels[0],false);
    CHECK_EQ(engine.snapshot().channelStep[0],0U);
    state.channels[0].common.mode=ChannelMode::Clock;
    state.channels[0].clock.meter.beats=0U;
    state.channels[0].common.swingPercent=99U; // defensive clamp to the supported 50% maximum
    engine.updateChannel(0U,state.channels[0],true);
    runEngineTicks(engine,2U);
    CHECK(engine.snapshot().channelStep[0] < 1U);

    // Corrupt enum input must not emit a gate or crash the scheduler.
    state.channels[0].common.mode=static_cast<ChannelMode>(99U);
    state.channels[0].common.rate={ClockRatioMode::Multiply,255U,255U,1U};
    engine.updateConfiguration(state,true);
    runEngineTicks(engine,1U);
}

void testControlPanel() {
    resetFakes();
    fakefw::setPin(pinmap::kEncoderPhaseAPin,LOW);
    fakefw::setPin(pinmap::kEncoderPhaseBPin,LOW);
    { hal::ControlPanel lowAtBegin; lowAtBegin.begin(); }
    resetFakes();
    hal::ControlPanel controls; controls.begin();
    fakefw::noInterruptCalls = 0U;
    fakefw::interruptCalls = 0U;
    auto sample=controls.sample(0U); CHECK_EQ(sample.encoderDelta,0);
    CHECK_EQ(fakefw::noInterruptCalls, 0U);
    CHECK_EQ(fakefw::interruptCalls, 0U);
    // Debounced press and release for all four buttons.
    const std::uint32_t pins[]={pinmap::kEncoderPushButtonPin,pinmap::kPlayPauseButtonPin,pinmap::kTapTempoButtonPin,pinmap::kResetBackButtonPin};
    for(auto pin:pins) fakefw::setPin(pin,LOW);
    fakefw::nowUs = 1'000U;
    sample=controls.sample(1U); CHECK_EQ(sample.transportButton.edge,hal::ButtonEdge::None);
    fakefw::nowUs = 30'000U;
    sample=controls.sample(30U); CHECK_EQ(sample.transportButton.edge,hal::ButtonEdge::Pressed); CHECK(sample.transportButton.pressed);
    CHECK_EQ(sample.tapButton.edgeTimestampUs, 1'000U);
    for(auto pin:pins) fakefw::setPin(pin,HIGH);
    fakefw::nowUs = 31'000U;
    (void)controls.sample(31U);
    fakefw::nowUs = 60'000U;
    sample=controls.sample(60U); CHECK_EQ(sample.resetButton.edge,hal::ButtonEdge::Released);
    CHECK_EQ(sample.tapButton.edgeTimestampUs, 31'000U);
    fakefw::nowUs = 0U;

    // One full Gray-code detent in each direction. IRQ capture must not depend on
    // foreground sampling between individual A/B transitions.
    auto setEncoderLevel=[&](int a,int b){
        fakefw::setPin(pinmap::kEncoderPhaseAPin,a?HIGH:LOW);
        fakefw::setPin(pinmap::kEncoderPhaseBPin,b?HIGH:LOW);
    };
    setEncoderLevel(1,0); setEncoderLevel(0,0); setEncoderLevel(0,1); setEncoderLevel(1,1);
    const int firstDirection=controls.sample(73U).encoderDelta;
    CHECK(firstDirection==1 || firstDirection==-1);
    setEncoderLevel(0,1); setEncoderLevel(0,0); setEncoderLevel(1,0); setEncoderLevel(1,1);
    CHECK_EQ(controls.sample(83U).encoderDelta,-firstDirection);

    // Regression for the real UI failure: six complete detents may occur while a
    // display transfer blocks every foreground poll. All six must still be delivered.
    for(int detent=0;detent<6;++detent){
        setEncoderLevel(1,0); setEncoderLevel(0,0); setEncoderLevel(0,1); setEncoderLevel(1,1);
    }
    CHECK_EQ(controls.sample(90U).encoderDelta,firstDirection*6);
    CHECK_EQ(controls.sample(91U).encoderDelta,0);

    // A contact bounce that leaves the encoder in the same Gray-code state must
    // cancel rather than manufacture a detent.
    fakefw::setPin(pinmap::kEncoderPhaseAPin,LOW);
    fakefw::setPin(pinmap::kEncoderPhaseAPin,HIGH);
    CHECK_EQ(controls.sample(92U).encoderDelta,0);
}
