/**
 * @file test_main.cpp
 * @brief Atomic native tests for editable settings, clamps, and cross-field invariants.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <cstdint>
#include <cstring>
#include <unity.h>
#include <Arduino.h>

#include "domain/default_configuration.h"
#include "domain/clock_labels.h"
#include "engine/clock_engine.h"
#include "hal/gate_output_driver.h"
#include "ui/settings_editor.h"
#include "ui/menu_model.h"
#include "ui/menu_model_channel.h"
#include "ui/menu_model_sync.h"
#include "ui_text.h"

using namespace clockfw;

void setUp() { fakefw::resetArduino(); }
void tearDown() {}

namespace {

struct Harness {
    ClockState state{};
    hal::GateOutputDriver gates{};
    engine::ClockEngine engine;
    ui::SettingsEditor editor;

    Harness() : engine(gates), editor(state, engine) {
        initializeFactoryDefaults(state);
        gates.beginDisabled();
        engine.begin(state);
    }
};

void testHardwareEncoderDirectionCanBeReversed(){ Harness h; TEST_ASSERT_FALSE(h.state.device.encoderDirectionReversed); h.editor.adjust(ui::SettingsPage::Hardware,0U,0U,1); TEST_ASSERT_TRUE(h.state.device.encoderDirectionReversed); }
void testHardwareEncoderDirectionCanReturnToNormal(){ Harness h; h.state.device.encoderDirectionReversed=true; h.editor.adjust(ui::SettingsPage::Hardware,0U,0U,-1); TEST_ASSERT_FALSE(h.state.device.encoderDirectionReversed); }
void testHardwareDisplayOrientationCanRotate180(){ Harness h; TEST_ASSERT_FALSE(h.state.device.displayRotated180); h.editor.adjust(ui::SettingsPage::Hardware,1U,0U,1); TEST_ASSERT_TRUE(h.state.device.displayRotated180); }
void testHardwareDisplayOrientationCanReturnToZero(){ Harness h; h.state.device.displayRotated180=true; h.editor.adjust(ui::SettingsPage::Hardware,1U,0U,-1); TEST_ASSERT_FALSE(h.state.device.displayRotated180); }
void testInputAssignmentsDefaultToSyncAndReset(){ Harness h; TEST_ASSERT_EQUAL(InputFunction::Sync,h.state.inputs.input1); TEST_ASSERT_EQUAL(InputFunction::Reset,h.state.inputs.input2); const auto row1=ui::buildMenuRow(ui::SettingsPage::InputAssignments,0U,0U,h.state); const auto row2=ui::buildMenuRow(ui::SettingsPage::InputAssignments,1U,0U,h.state); TEST_ASSERT_TRUE(std::strcmp("SYNC",row1.value)==0); TEST_ASSERT_TRUE(std::strcmp("RESET",row2.value)==0); }
void testInputAssignmentsSkipFunctionAlreadyUsedByOtherInput(){ Harness h; h.editor.adjust(ui::SettingsPage::InputAssignments,1U,0U,-1); TEST_ASSERT_EQUAL(InputFunction::Off,h.state.inputs.input2); h.editor.adjust(ui::SettingsPage::InputAssignments,0U,0U,1); TEST_ASSERT_EQUAL(InputFunction::Reset,h.state.inputs.input1); }
void testInputAssignmentsAllowOffOnBothInputs(){ Harness h; h.state.inputs={InputFunction::Off,InputFunction::Reset}; h.editor.adjust(ui::SettingsPage::InputAssignments,1U,0U,-2); TEST_ASSERT_EQUAL(InputFunction::Off,h.state.inputs.input1); TEST_ASSERT_EQUAL(InputFunction::Off,h.state.inputs.input2); }
void testFillIsReservedAndNotSelectableYet(){ Harness h; h.state.inputs={InputFunction::Tap,InputFunction::Off}; h.editor.adjust(ui::SettingsPage::InputAssignments,0U,0U,1); TEST_ASSERT_EQUAL(InputFunction::Tap,h.state.inputs.input1); }

void testInputFunctionLabelsCoverAllRoles(){
    struct Case { InputFunction function; const char* label; };
    constexpr Case cases[] = {
        {InputFunction::Off, "OFF"},
        {InputFunction::Sync, "SYNC"},
        {InputFunction::Reset, "RESET"},
        {InputFunction::Run, "RUN"},
        {InputFunction::Start, "START"},
        {InputFunction::Stop, "STOP"},
        {InputFunction::Restart, "RESTART"},
        {InputFunction::Tap, "TAP"},
        {InputFunction::Fill, "FILL"},
    };
    for (const auto& entry : cases) {
        TEST_ASSERT_TRUE(std::strcmp(entry.label, inputFunctionLabel(entry.function)) == 0);
    }
    TEST_ASSERT_TRUE(std::strcmp("OFF", inputFunctionLabel(static_cast<InputFunction>(255U))) == 0);
}

void testInputAssignmentEditorCoversBothRowsAndBoundaries(){
    Harness h;
    h.state.inputs = {InputFunction::Off, InputFunction::Reset};
    h.editor.adjust(ui::SettingsPage::InputAssignments, 0U, 0U, 1);
    TEST_ASSERT_EQUAL(InputFunction::Sync, h.state.inputs.input1);
    h.editor.adjust(ui::SettingsPage::InputAssignments, 1U, 0U, 1);
    TEST_ASSERT_EQUAL(InputFunction::Run, h.state.inputs.input2);
    h.editor.adjust(ui::SettingsPage::InputAssignments, 0U, 0U, -1);
    TEST_ASSERT_EQUAL(InputFunction::Off, h.state.inputs.input1);
    h.state.inputs.input1 = InputFunction::Tap;
    h.editor.adjust(ui::SettingsPage::InputAssignments, 0U, 0U, 1);
    TEST_ASSERT_EQUAL(InputFunction::Tap, h.state.inputs.input1);
    h.state.inputs.input2 = InputFunction::Off;
    h.editor.adjust(ui::SettingsPage::InputAssignments, 1U, 0U, -1);
    TEST_ASSERT_EQUAL(InputFunction::Off, h.state.inputs.input2);
}

void testInputAssignmentEditorIgnoresZeroDeltaAndInvalidRow(){
    Harness h;
    const auto before = h.state.inputs;
    h.editor.adjust(ui::SettingsPage::InputAssignments, 0U, 0U, 0);
    h.editor.adjust(ui::SettingsPage::InputAssignments, 2U, 0U, 1);
    TEST_ASSERT_EQUAL(before.input1, h.state.inputs.input1);
    TEST_ASSERT_EQUAL(before.input2, h.state.inputs.input2);
}

void testHardwareEditorIgnoresUnknownRow(){
    Harness h;
    const auto before = h.state.device;
    h.editor.adjust(ui::SettingsPage::Hardware, 2U, 0U, 1);
    TEST_ASSERT_EQUAL(before.encoderDirectionReversed, h.state.device.encoderDirectionReversed);
    TEST_ASSERT_EQUAL(before.displayRotated180, h.state.device.displayRotated180);
}

void testMasterTempoClampsAtConfiguredMaximum(){ Harness h; h.state.bpm=998U; h.state.tempoRange.maximumBpm=999U; h.editor.changeMasterTempo(10); TEST_ASSERT_EQUAL_UINT32(999U,h.state.bpm); }
void testMasterTempoClampsAtConfiguredMinimum(){ Harness h; h.state.bpm=21U; h.state.tempoRange.minimumBpm=20U; h.editor.changeMasterTempo(-10); TEST_ASSERT_EQUAL_UINT32(20U,h.state.bpm); }
void testMinimumBpmClampsAtTechnicalMinimum(){ Harness h; h.editor.adjust(ui::SettingsPage::Master,1U,0U,-127); TEST_ASSERT_EQUAL_UINT32(1U,h.state.tempoRange.minimumBpm); }
void testMinimumBpmCannotExceedMaximum(){ Harness h; h.state.tempoRange={120U,121U}; h.editor.adjust(ui::SettingsPage::Master,1U,0U,127); TEST_ASSERT_EQUAL_UINT32(121U,h.state.tempoRange.minimumBpm); }
void testRaisingMinimumClampsCurrentTempo(){ Harness h; h.state.bpm=100U; h.state.tempoRange={90U,200U}; h.editor.adjust(ui::SettingsPage::Master,1U,0U,20); TEST_ASSERT_EQUAL_UINT32(110U,h.state.bpm); }
void testMaximumBpmClampsAtTechnicalMaximum(){ Harness h; h.state.tempoRange.maximumBpm=990U; h.editor.adjust(ui::SettingsPage::Master,2U,0U,127); TEST_ASSERT_EQUAL_UINT32(999U,h.state.tempoRange.maximumBpm); }
void testMaximumBpmCannotFallBelowMinimum(){ Harness h; h.state.tempoRange={119U,120U}; h.editor.adjust(ui::SettingsPage::Master,2U,0U,-127); TEST_ASSERT_EQUAL_UINT32(119U,h.state.tempoRange.maximumBpm); }
void testLoweringMaximumClampsCurrentTempo(){ Harness h; h.state.bpm=180U; h.state.tempoRange={20U,200U}; h.editor.adjust(ui::SettingsPage::Master,2U,0U,-50); TEST_ASSERT_EQUAL_UINT32(150U,h.state.bpm); }
void testMasterMeterBeatsClampAtOne(){ Harness h; h.state.masterMeter.beats=2U; h.editor.adjust(ui::SettingsPage::Master,3U,0U,-10); TEST_ASSERT_EQUAL_UINT8(1U,h.state.masterMeter.beats); }
void testMasterMeterBeatsClampAtSixteen(){ Harness h; h.state.masterMeter.beats=15U; h.editor.adjust(ui::SettingsPage::Master,3U,0U,10); TEST_ASSERT_EQUAL_UINT8(16U,h.state.masterMeter.beats); }
void testMasterBeatUnitStepsThroughOptions(){ Harness h; h.state.masterMeter.unit=4U; h.editor.adjust(ui::SettingsPage::Master,4U,0U,1); TEST_ASSERT_EQUAL_UINT8(8U,h.state.masterMeter.unit); }
void testPreCountDefaultsOffAndMenuShowsOff(){ Harness h; TEST_ASSERT_EQUAL_UINT8(0U,h.state.preCountSteps); const auto row=ui::buildMenuRow(ui::SettingsPage::Master,5U,0U,h.state); TEST_ASSERT_TRUE(std::strcmp(text::get(text::TextId::PreCount),row.label)==0); TEST_ASSERT_TRUE(std::strcmp(text::get(text::TextId::Off),row.value)==0); }
void testPreCountSelectableRangeIsOneToSixtyFourPlusOff(){ Harness h; h.editor.adjust(ui::SettingsPage::Master,5U,0U,1); TEST_ASSERT_EQUAL_UINT8(1U,h.state.preCountSteps); h.editor.adjust(ui::SettingsPage::Master,5U,0U,127); TEST_ASSERT_EQUAL_UINT8(64U,h.state.preCountSteps); h.editor.adjust(ui::SettingsPage::Master,5U,0U,-127); TEST_ASSERT_EQUAL_UINT8(0U,h.state.preCountSteps); }
void testPreCountMenuShowsConfiguredCount(){ Harness h; h.state.preCountSteps=64U; const auto row=ui::buildMenuRow(ui::SettingsPage::Master,5U,0U,h.state); TEST_ASSERT_TRUE(std::strcmp("64",row.value)==0); }
void testSyncSourceClampsInternal(){ Harness h; h.state.source=ClockSource::Internal; h.editor.adjust(ui::SettingsPage::Sync,0U,0U,-10); TEST_ASSERT_EQUAL(ClockSource::Internal,h.state.source); }
void testSyncSourceClampsAuto(){ Harness h; h.state.source=ClockSource::External; h.editor.adjust(ui::SettingsPage::Sync,0U,0U,10); TEST_ASSERT_EQUAL(ClockSource::Auto,h.state.source); }
void testSyncPpqnStepsOneTwoFourTwentyFour(){ Harness h; h.state.externalSync.pulsesPerQuarterNote=1U; h.editor.adjust(ui::SettingsPage::Sync,1U,0U,1); TEST_ASSERT_EQUAL_UINT8(2U,h.state.externalSync.pulsesPerQuarterNote); h.editor.adjust(ui::SettingsPage::Sync,1U,0U,1); TEST_ASSERT_EQUAL_UINT8(4U,h.state.externalSync.pulsesPerQuarterNote); h.editor.adjust(ui::SettingsPage::Sync,1U,0U,1); TEST_ASSERT_EQUAL_UINT8(24U,h.state.externalSync.pulsesPerQuarterNote); }
void testSyncPpqnInvalidStoredValueFallsBackToOptionStart(){ Harness h; h.state.externalSync.pulsesPerQuarterNote=3U; h.editor.adjust(ui::SettingsPage::Sync,1U,0U,1); TEST_ASSERT_EQUAL_UINT8(2U,h.state.externalSync.pulsesPerQuarterNote); }
void testSyncEdgePositiveDeltaSelectsFalling(){ Harness h; h.editor.adjust(ui::SettingsPage::Sync,2U,0U,1); TEST_ASSERT_EQUAL(SyncEdge::Falling,h.state.externalSync.edge); }
void testSyncEdgeNegativeDeltaSelectsRising(){ Harness h; h.state.externalSync.edge=SyncEdge::Falling; h.editor.adjust(ui::SettingsPage::Sync,2U,0U,-1); TEST_ASSERT_EQUAL(SyncEdge::Rising,h.state.externalSync.edge); }
void testSyncLossModeClampsAcrossEnum(){ Harness h; h.state.externalSync.lossMode=SyncLossMode::Stop; h.editor.adjust(ui::SettingsPage::Sync,3U,0U,10); TEST_ASSERT_EQUAL(SyncLossMode::Internal,h.state.externalSync.lossMode); }
void testSyncResetModeSelectsGateAndTrigger(){ Harness h; h.editor.adjust(ui::SettingsPage::Sync,4U,0U,1); TEST_ASSERT_EQUAL(ExternalResetMode::Gate,h.state.externalSync.resetMode); h.editor.adjust(ui::SettingsPage::Sync,4U,0U,-1); TEST_ASSERT_EQUAL(ExternalResetMode::Trigger,h.state.externalSync.resetMode); }
void testSyncGlitchFilterClampsZeroToFiveMilliseconds(){ Harness h; h.state.externalSync.glitchFilterUs=250U; h.editor.adjust(ui::SettingsPage::Sync,5U,0U,-10); TEST_ASSERT_EQUAL_UINT32(0U,h.state.externalSync.glitchFilterUs); h.editor.adjust(ui::SettingsPage::Sync,5U,0U,127); TEST_ASSERT_EQUAL_UINT32(5000U,h.state.externalSync.glitchFilterUs); }
void testSyncSmoothingClampsOffToFull(){ Harness h; h.state.externalSync.smoothing=SyncSmoothing::Low; h.editor.adjust(ui::SettingsPage::Sync,6U,0U,-10); TEST_ASSERT_EQUAL(SyncSmoothing::Off,h.state.externalSync.smoothing); h.editor.adjust(ui::SettingsPage::Sync,6U,0U,127); TEST_ASSERT_EQUAL(SyncSmoothing::Full,h.state.externalSync.smoothing); }
void testSyncSmoothingMenuLabelsAllModes(){ Harness h;
    struct Case { SyncSmoothing mode; text::TextId label; };
    constexpr Case cases[] = {
        {SyncSmoothing::Off, text::TextId::Off},
        {SyncSmoothing::Low, text::TextId::SyncLow},
        {SyncSmoothing::Medium, text::TextId::SyncMedium},
        {SyncSmoothing::Full, text::TextId::SyncFull},
    };
    for (const auto& item : cases) {
        h.state.externalSync.smoothing = item.mode;
        const auto row = ui::buildSyncMenuRow(6U, h.state);
        TEST_ASSERT_TRUE(std::strcmp(text::get(text::TextId::Smoothing), row.label) == 0);
        TEST_ASSERT_TRUE(std::strcmp(text::get(item.label), row.value) == 0);
    }
}
void testScreensaverMenuLabelsAllModes(){ Harness h;
    struct Case { ScreensaverMode mode; text::TextId label; };
    constexpr Case cases[] = {
        {ScreensaverMode::None, text::TextId::ScreensaverNone},
        {ScreensaverMode::Fractal, text::TextId::ScreensaverFractal},
        {ScreensaverMode::Orbit, text::TextId::ScreensaverOrbit},
        {ScreensaverMode::Plug, text::TextId::ScreensaverPlug},
        {ScreensaverMode::Clock, text::TextId::ScreensaverClock},
        {ScreensaverMode::Heartbeat, text::TextId::ScreensaverHeartbeat},
        {ScreensaverMode::Acid, text::TextId::ScreensaverAcid},
        {ScreensaverMode::Spectrum, text::TextId::ScreensaverSpectrum},
        {ScreensaverMode::Field, text::TextId::ScreensaverField},
        {ScreensaverMode::Blox, text::TextId::ScreensaverBlox},
        {ScreensaverMode::Matrix, text::TextId::ScreensaverMatrix},
        {ScreensaverMode::CubeCover, text::TextId::ScreensaverCubeCover},
        {ScreensaverMode::MakeMusic, text::TextId::ScreensaverMakeMusic},
        {ScreensaverMode::Labyrinth, text::TextId::ScreensaverLabyrinth},
        {ScreensaverMode::Starfield, text::TextId::ScreensaverStarfield},
        {ScreensaverMode::Fireworks, text::TextId::ScreensaverFireworks},
    };
    for (const auto& item : cases) {
        h.state.display.screensaverMode = item.mode;
        const auto row = ui::buildMenuRow(ui::SettingsPage::Screensaver, 0U, 0U, h.state);
        TEST_ASSERT_TRUE(std::strcmp(text::get(item.label), row.value) == 0);
    }
}
void testSyncTimeoutClampsTwoHundredToFiveThousandMs(){ Harness h; h.state.externalSync.timeoutMs=300U; h.editor.adjust(ui::SettingsPage::Sync,7U,0U,-10); TEST_ASSERT_EQUAL_UINT32(200U,h.state.externalSync.timeoutMs); h.editor.adjust(ui::SettingsPage::Sync,7U,0U,127); TEST_ASSERT_EQUAL_UINT32(5000U,h.state.externalSync.timeoutMs); }
void testScreensaverModeOrderStartsWithOff(){ Harness h; h.state.display.screensaverMode=ScreensaverMode::None; h.editor.adjust(ui::SettingsPage::Screensaver,0U,0U,-10); TEST_ASSERT_EQUAL(ScreensaverMode::None,h.state.display.screensaverMode); }
void testScreensaverModeCanReachFireworks(){ Harness h; h.state.display.screensaverMode=ScreensaverMode::None; h.editor.adjust(ui::SettingsPage::Screensaver,0U,0U,127); TEST_ASSERT_EQUAL(ScreensaverMode::Fireworks,h.state.display.screensaverMode); }
void testScreensaverDelayCannotExceedDimDelay(){ Harness h; h.state.display={ScreensaverMode::Clock,2U,5U,10U}; h.editor.adjust(ui::SettingsPage::Screensaver,1U,0U,127); TEST_ASSERT_EQUAL_UINT8(5U,h.state.display.screensaverAfterMinutes); }
void testDimDelayCannotPrecedeScreensaver(){ Harness h; h.state.display={ScreensaverMode::Clock,4U,5U,10U}; h.editor.adjust(ui::SettingsPage::Screensaver,2U,0U,-127); TEST_ASSERT_EQUAL_UINT8(4U,h.state.display.dimAfterMinutes); }
void testDimDelayCannotExceedOffDelay(){ Harness h; h.state.display={ScreensaverMode::Clock,2U,5U,6U}; h.editor.adjust(ui::SettingsPage::Screensaver,2U,0U,127); TEST_ASSERT_EQUAL_UINT8(6U,h.state.display.dimAfterMinutes); }
void testOffDelayCannotPrecedeDimDelay(){ Harness h; h.state.display={ScreensaverMode::Clock,2U,7U,10U}; h.editor.adjust(ui::SettingsPage::Screensaver,3U,0U,-127); TEST_ASSERT_EQUAL_UINT8(7U,h.state.display.offAfterMinutes); }
void testInvalidChannelIndexIsIgnored(){ Harness h; const auto before=h.state.channels[0]; h.editor.adjust(ui::SettingsPage::ChannelTiming,3U,99U,20); TEST_ASSERT_EQUAL_UINT8(before.common.swingPercent,h.state.channels[0].common.swingPercent); }
void testChannelSwingClampsZeroToFifty(){ Harness h; h.editor.adjust(ui::SettingsPage::ChannelTiming,3U,0U,127); TEST_ASSERT_EQUAL_UINT8(50U,h.state.channels[0].common.swingPercent); h.editor.adjust(ui::SettingsPage::ChannelTiming,3U,0U,-127); TEST_ASSERT_EQUAL_UINT8(0U,h.state.channels[0].common.swingPercent); }
void testChannelProbabilityClampsZeroToHundred(){ Harness h; h.editor.adjust(ui::SettingsPage::ChannelOutput,0U,0U,-127); TEST_ASSERT_EQUAL_UINT8(0U,h.state.channels[0].common.probabilityPercent); h.editor.adjust(ui::SettingsPage::ChannelOutput,0U,0U,127); TEST_ASSERT_EQUAL_UINT8(100U,h.state.channels[0].common.probabilityPercent); }
void testChannelGateLengthUsesCuratedOptions(){ Harness h; h.state.channels[0].common.gateLengthMs=10U; h.editor.adjust(ui::SettingsPage::ChannelOutput,1U,0U,1); TEST_ASSERT_EQUAL_UINT32(20U,h.state.channels[0].common.gateLengthMs); }
void testChannelPhaseClampsZeroToNinetyNine(){ Harness h; h.editor.adjust(ui::SettingsPage::ChannelOutput,2U,0U,127); TEST_ASSERT_EQUAL_UINT8(99U,h.state.channels[0].common.phasePercent); }
void testChannelResetModeFollowsDeltaSign(){ Harness h; h.editor.adjust(ui::SettingsPage::ChannelOutput,3U,0U,1); TEST_ASSERT_EQUAL(ResetMode::Free,h.state.channels[0].common.resetMode); h.editor.adjust(ui::SettingsPage::ChannelOutput,3U,0U,-1); TEST_ASSERT_EQUAL(ResetMode::Global,h.state.channels[0].common.resetMode); }
void testChannelMuteTogglesOncePerAdjustment(){ Harness h; TEST_ASSERT_FALSE(h.state.channels[0].common.muted); h.editor.adjust(ui::SettingsPage::ChannelOutput,4U,0U,1); TEST_ASSERT_TRUE(h.state.channels[0].common.muted); h.editor.adjust(ui::SettingsPage::ChannelOutput,4U,0U,1); TEST_ASSERT_FALSE(h.state.channels[0].common.muted); }
void testRateFactorTraversesCuratedList(){ Harness h; h.state.channels[0].common.rate={ClockRatioMode::Multiply,1U,1U,1U}; h.editor.adjust(ui::SettingsPage::ChannelTiming,0U,0U,1); TEST_ASSERT_EQUAL_UINT8(2U,h.state.channels[0].common.rate.factor); }
void testRateNumeratorClampsOneToSixteen(){ Harness h; h.editor.adjust(ui::SettingsPage::ChannelTiming,1U,0U,127); TEST_ASSERT_EQUAL_UINT8(16U,h.state.channels[0].common.rate.numerator); }
void testRateDenominatorClampsOneToSixteen(){ Harness h; h.editor.adjust(ui::SettingsPage::ChannelTiming,2U,0U,-127); TEST_ASSERT_EQUAL_UINT8(1U,h.state.channels[0].common.rate.denominator); }
void testClockMeterBeatsClampOneToSixteen(){ Harness h; h.state.channels[0].clock.meter.beats=4U; h.editor.adjust(ui::SettingsPage::Clock,0U,0U,127); TEST_ASSERT_EQUAL_UINT8(16U,h.state.channels[0].clock.meter.beats); }
void testEuclidShrinkingStepsClampsHitsAndRotation(){ Harness h; h.state.channels[0].common.mode=ChannelMode::Euclid; h.state.channels[0].euclid={16U,12U,15U}; h.editor.adjust(ui::SettingsPage::Euclid,0U,0U,-12); TEST_ASSERT_EQUAL_UINT8(4U,h.state.channels[0].euclid.steps); TEST_ASSERT_EQUAL_UINT8(4U,h.state.channels[0].euclid.hits); TEST_ASSERT_EQUAL_UINT8(0U,h.state.channels[0].euclid.rotation); }
void testEuclidHitsCannotExceedSteps(){ Harness h; h.state.channels[0].common.mode=ChannelMode::Euclid; h.state.channels[0].euclid={7U,6U,0U}; h.editor.adjust(ui::SettingsPage::Euclid,1U,0U,127); TEST_ASSERT_EQUAL_UINT8(7U,h.state.channels[0].euclid.hits); }
void testEuclidRotationCannotExceedLastStep(){ Harness h; h.state.channels[0].common.mode=ChannelMode::Euclid; h.state.channels[0].euclid={7U,3U,0U}; h.editor.adjust(ui::SettingsPage::Euclid,2U,0U,127); TEST_ASSERT_EQUAL_UINT8(6U,h.state.channels[0].euclid.rotation); }
void testSequencerLengthClampResetsInvalidRotation(){ Harness h; h.state.channels[0].common.mode=ChannelMode::Sequencer; h.state.channels[0].sequencer={16U,15U,0xFFFFU}; h.editor.adjust(ui::SettingsPage::Sequencer,0U,0U,-12); TEST_ASSERT_EQUAL_UINT8(4U,h.state.channels[0].sequencer.length); TEST_ASSERT_EQUAL_UINT8(0U,h.state.channels[0].sequencer.rotation); }
void testSequencerRotationClampsToLengthMinusOne(){ Harness h; h.state.channels[0].common.mode=ChannelMode::Sequencer; h.state.channels[0].sequencer={8U,0U,1U}; h.editor.adjust(ui::SettingsPage::Sequencer,1U,0U,127); TEST_ASSERT_EQUAL_UINT8(7U,h.state.channels[0].sequencer.rotation); }
void testSequencerToggleRejectsStepOutsideLength(){ Harness h; h.state.channels[0].sequencer={8U,0U,0x01U}; h.editor.toggleSequencerStep(0U,8U); TEST_ASSERT_EQUAL_UINT64(0x01U,h.state.channels[0].sequencer.pattern); }
void testSequencerToggleFlipsValidStep(){ Harness h; h.state.channels[0].sequencer={8U,0U,0U}; h.editor.toggleSequencerStep(0U,3U); TEST_ASSERT_EQUAL_UINT64(0x08U,h.state.channels[0].sequencer.pattern); }
void testSequencerPasteBeforeCopyIsRejected(){ Harness h; TEST_ASSERT_FALSE(h.editor.executeSequencerCommand(0U,5U)); }
void testSequencerCopyPasteClampsPatternToTargetLength(){ Harness h; h.state.channels[0].sequencer={16U,0U,0xFFFFU}; TEST_ASSERT_TRUE(h.editor.executeSequencerCommand(0U,4U)); h.state.channels[0].sequencer={4U,0U,0U}; TEST_ASSERT_TRUE(h.editor.executeSequencerCommand(0U,5U)); TEST_ASSERT_EQUAL_UINT64(0x0FU,h.state.channels[0].sequencer.pattern); }
void testGrooveDefaultsOffAtFullAmount(){ Harness h; TEST_ASSERT_EQUAL(GroovePreset::Off,h.state.unifiedClock.groove.preset); TEST_ASSERT_EQUAL_UINT8(100U,h.state.unifiedClock.groove.amountPercent); TEST_ASSERT_EQUAL_UINT8(0U,h.state.unifiedClock.groove.rotation); }
void testUnifiedGrooveEditorOwnsGlobalSettings(){ Harness h; h.state.operatingMode=OperatingMode::UnifiedClock; h.editor.adjust(ui::SettingsPage::Groove,0U,0U,1); TEST_ASSERT_EQUAL(GroovePreset::Swing54,h.state.unifiedClock.groove.preset); TEST_ASSERT_EQUAL(GroovePreset::Off,h.state.channels[0].common.groove.preset); }
void testIndependentGrooveEditorOwnsSelectedChannel(){ Harness h; h.state.operatingMode=OperatingMode::Independent; h.editor.adjust(ui::SettingsPage::Groove,0U,3U,3); TEST_ASSERT_EQUAL(GroovePreset::Swing62,h.state.channels[3].common.groove.preset); TEST_ASSERT_EQUAL(GroovePreset::Off,h.state.channels[2].common.groove.preset); TEST_ASSERT_EQUAL(GroovePreset::Off,h.state.unifiedClock.groove.preset); }
void testGrooveAmountClampsZeroToHundred(){ Harness h; h.state.operatingMode=OperatingMode::UnifiedClock; h.editor.adjust(ui::SettingsPage::Groove,1U,0U,-127); TEST_ASSERT_EQUAL_UINT8(0U,h.state.unifiedClock.groove.amountPercent); h.editor.adjust(ui::SettingsPage::Groove,1U,0U,127); TEST_ASSERT_EQUAL_UINT8(100U,h.state.unifiedClock.groove.amountPercent); }
void testGrooveRotationClampsToPatternLength(){ Harness h; h.state.operatingMode=OperatingMode::UnifiedClock; h.state.unifiedClock.groove.preset=GroovePreset::Swing54; h.editor.adjust(ui::SettingsPage::Groove,2U,0U,127); TEST_ASSERT_EQUAL_UINT8(1U,h.state.unifiedClock.groove.rotation); h.state.unifiedClock.groove.preset=GroovePreset::PocketC; h.editor.adjust(ui::SettingsPage::Groove,2U,0U,127); TEST_ASSERT_EQUAL_UINT8(15U,h.state.unifiedClock.groove.rotation); }
void testGroovePresetChangeNormalizesRotation(){ Harness h; h.state.operatingMode=OperatingMode::UnifiedClock; h.state.unifiedClock.groove={GroovePreset::PocketC,100U,15U}; h.editor.adjust(ui::SettingsPage::Groove,0U,0U,-1); TEST_ASSERT_TRUE(h.state.unifiedClock.groove.rotation < 8U); }
void testGrooveMenuShapeIsStable(){
    Harness h;
    TEST_ASSERT_EQUAL_UINT8(4U,ui::settingsPageItemCount(ui::SettingsPage::Channel,ChannelMode::Clock,false));
    TEST_ASSERT_EQUAL_UINT8(4U,ui::settingsPageItemCount(ui::SettingsPage::Channel,ChannelMode::Euclid,false));
    TEST_ASSERT_EQUAL_UINT8(4U,ui::settingsPageItemCount(ui::SettingsPage::Channel,ChannelMode::Sequencer,false));
    TEST_ASSERT_EQUAL_UINT8(1U,ui::settingsPageItemCount(ui::SettingsPage::Channel,ChannelMode::Off,false));
    TEST_ASSERT_EQUAL_UINT8(5U,ui::settingsPageItemCount(ui::SettingsPage::ChannelTiming,ChannelMode::Clock,false));
    TEST_ASSERT_EQUAL_UINT8(3U,ui::settingsPageItemCount(ui::SettingsPage::UnifiedClock,ChannelMode::Clock,false));
    TEST_ASSERT_EQUAL_UINT8(6U,ui::settingsPageItemCount(ui::SettingsPage::UnifiedTiming,ChannelMode::Clock,false));
    TEST_ASSERT_EQUAL_UINT8(7U,ui::settingsPageItemCount(ui::SettingsPage::Groove,ChannelMode::Clock,false));
    const auto row=ui::buildMenuRow(ui::SettingsPage::Groove,0U,0U,h.state);
    TEST_ASSERT_TRUE(std::strcmp(text::get(text::TextId::GroovePreset),row.label)==0);
    TEST_ASSERT_TRUE(std::strcmp("OFF",row.value)==0);
}

void testChannelRootUsesModeAwareGroupPriority(){
    Harness h;
    h.state.channels[0].common.mode=ChannelMode::Clock;
    TEST_ASSERT_TRUE(std::strcmp("MODE",ui::buildMenuRow(ui::SettingsPage::Channel,0U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("TIMING",ui::buildMenuRow(ui::SettingsPage::Channel,1U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("CLOCK",ui::buildMenuRow(ui::SettingsPage::Channel,2U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("OUTPUT",ui::buildMenuRow(ui::SettingsPage::Channel,3U,0U,h.state).label)==0);
    h.state.channels[0].common.mode=ChannelMode::Euclid;
    TEST_ASSERT_TRUE(std::strcmp("EUCLID",ui::buildMenuRow(ui::SettingsPage::Channel,1U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("TIMING",ui::buildMenuRow(ui::SettingsPage::Channel,2U,0U,h.state).label)==0);
    h.state.channels[0].common.mode=ChannelMode::Sequencer;
    TEST_ASSERT_TRUE(std::strcmp("SEQUENCER",ui::buildMenuRow(ui::SettingsPage::Channel,1U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("TIMING",ui::buildMenuRow(ui::SettingsPage::Channel,2U,0U,h.state).label)==0);
}

void testNestedGroupPagesExposeFormerGroupedParameters(){
    Harness h;
    TEST_ASSERT_TRUE(std::strcmp("DIV/MULT",ui::buildMenuRow(ui::SettingsPage::ChannelTiming,0U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("GROOVE",ui::buildMenuRow(ui::SettingsPage::ChannelTiming,4U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("PROB",ui::buildMenuRow(ui::SettingsPage::ChannelOutput,0U,0U,h.state).label)==0);
    h.state.channels[0].common.mode=ChannelMode::Sequencer;
    TEST_ASSERT_TRUE(std::strcmp("LENGTH",ui::buildMenuRow(ui::SettingsPage::Sequencer,0U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("PATTERN",ui::buildMenuRow(ui::SettingsPage::Sequencer,2U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("EDITOR",ui::buildMenuRow(ui::SettingsPage::SequencerPattern,0U,0U,h.state).label)==0);
    h.state.operatingMode=OperatingMode::UnifiedClock;
    TEST_ASSERT_TRUE(std::strcmp("TIMING",ui::buildMenuRow(ui::SettingsPage::UnifiedClock,1U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("OUTPUT",ui::buildMenuRow(ui::SettingsPage::UnifiedClock,2U,0U,h.state).label)==0);
    TEST_ASSERT_TRUE(std::strcmp("HUMANIZE",ui::buildMenuRow(ui::SettingsPage::UnifiedTiming,5U,0U,h.state).label)==0);
}

void testAllSettingsPageTitlesAndCountsAreReachable(){
    constexpr ui::SettingsPage pages[] = {
        ui::SettingsPage::Root, ui::SettingsPage::General, ui::SettingsPage::InputAssignments, ui::SettingsPage::Hardware, ui::SettingsPage::Diagnostics,
        ui::SettingsPage::DiagnosticsInputs, ui::SettingsPage::DiagnosticsOutputs, ui::SettingsPage::Master,
        ui::SettingsPage::Sync, ui::SettingsPage::Preferences, ui::SettingsPage::Screensaver,
        ui::SettingsPage::Info, ui::SettingsPage::Licenses, ui::SettingsPage::Updates,
        ui::SettingsPage::Channel, ui::SettingsPage::ChannelTiming, ui::SettingsPage::ChannelOutput,
        ui::SettingsPage::Clock, ui::SettingsPage::Euclid, ui::SettingsPage::Sequencer,
        ui::SettingsPage::SequencerPattern, ui::SettingsPage::UnifiedClock,
        ui::SettingsPage::UnifiedTiming, ui::SettingsPage::UnifiedOutput,
        ui::SettingsPage::Groove, ui::SettingsPage::DividerBank};
    for (const auto page : pages) {
        TEST_ASSERT_TRUE(ui::settingsPageTitle(page) != nullptr);
        (void)ui::settingsPageItemCount(page, ChannelMode::Clock, false);
    }
    TEST_ASSERT_EQUAL_UINT8(6U, ui::settingsPageItemCount(ui::SettingsPage::Root, ChannelMode::Clock, true));
    TEST_ASSERT_EQUAL_UINT8(0U, ui::settingsPageItemCount(static_cast<ui::SettingsPage>(255), ChannelMode::Clock, false));
    TEST_ASSERT_TRUE(std::strlen(ui::settingsPageTitle(static_cast<ui::SettingsPage>(255))) > 0U);
}

void testAllChannelRootRowsAreReachable(){
    Harness h;
    constexpr ChannelMode modes[] = {ChannelMode::Clock, ChannelMode::Euclid, ChannelMode::Sequencer, ChannelMode::Off};
    for (const auto mode : modes) {
        h.state.channels[0].common.mode = mode;
        const std::uint8_t count = ui::settingsPageItemCount(ui::SettingsPage::Channel, mode, false);
        for (std::uint8_t row = 0U; row < count; ++row) {
            const auto menuRow = ui::buildMenuRow(ui::SettingsPage::Channel, row, 0U, h.state);
            TEST_ASSERT_TRUE(std::strlen(menuRow.label) > 0U);
            (void)ui::channelMenuAction(mode, row);
        }
    }
    TEST_ASSERT_EQUAL_UINT8(
        0U, ui::channelMenuIndexForAction(ChannelMode::Off, ui::ChannelMenuAction::Timing));
}

void testGrooveMenuRowsAndNoOpBranchesAreReachable(){
    Harness h;
    h.state.operatingMode = OperatingMode::UnifiedClock;
    h.state.unifiedClock.groove = {GroovePreset::PocketC, 73U, 5U};
    for (std::uint8_t row = 0U; row < 7U; ++row) {
        const auto menuRow = ui::buildMenuRow(ui::SettingsPage::Groove, row, 0U, h.state);
        TEST_ASSERT_TRUE(std::strlen(menuRow.label) > 0U);
        TEST_ASSERT_TRUE(std::strlen(menuRow.value) > 0U);
    }
    const auto before = h.state.unifiedClock.groove;
    h.editor.adjust(ui::SettingsPage::Groove, 99U, 0U, 1);
    TEST_ASSERT_EQUAL(before.preset, h.state.unifiedClock.groove.preset);
    TEST_ASSERT_EQUAL_UINT8(before.amountPercent, h.state.unifiedClock.groove.amountPercent);
    TEST_ASSERT_EQUAL_UINT8(before.rotation, h.state.unifiedClock.groove.rotation);
    h.state.operatingMode = OperatingMode::Independent;
    h.state.channels[2].common.groove = {GroovePreset::PocketA, 44U, 2U};
    for (std::uint8_t row = 0U; row < 7U; ++row) {
        const auto menuRow = ui::buildMenuRow(ui::SettingsPage::Groove, row, 2U, h.state);
        TEST_ASSERT_TRUE(std::strlen(menuRow.label) > 0U);
    }
}


void testChannelHierarchyFallbacksAndActionLookupAreReachable(){
    constexpr ChannelMode modes[] = {ChannelMode::Clock, ChannelMode::Euclid, ChannelMode::Sequencer};
    constexpr ui::ChannelMenuAction actions[] = {
        ui::ChannelMenuAction::Mode, ui::ChannelMenuAction::Timing, ui::ChannelMenuAction::Clock,
        ui::ChannelMenuAction::Euclid, ui::ChannelMenuAction::Sequencer, ui::ChannelMenuAction::Output};
    for (const auto mode : modes) {
        TEST_ASSERT_EQUAL(ui::ChannelMenuAction::Mode, ui::channelMenuAction(mode, 99U));
        for (const auto action : actions) {
            const auto index = ui::channelMenuIndexForAction(mode, action);
            TEST_ASSERT_TRUE(index < ui::channelMenuItemCount(mode));
        }
    }
}

void testMenuModelConditionalRowsCoverBothStates(){
    Harness h;
    auto row = ui::buildMenuRow(ui::SettingsPage::Root, 4U, 0U, h.state, true);
    TEST_ASSERT_TRUE(std::strcmp(text::get(text::TextId::HighScores), row.label) == 0);
    row = ui::buildMenuRow(ui::SettingsPage::Root, 4U, 0U, h.state, false);
    TEST_ASSERT_TRUE(std::strlen(row.label) > 0U);

    h.state.device.encoderDirectionReversed = true;
    row = ui::buildMenuRow(ui::SettingsPage::Hardware, 0U, 0U, h.state);
    TEST_ASSERT_TRUE(std::strlen(row.value) > 0U);
    h.state.device.encoderDirectionReversed = false;
    (void)ui::buildMenuRow(ui::SettingsPage::Hardware, 0U, 0U, h.state);
    h.state.device.displayRotated180 = true;
    (void)ui::buildMenuRow(ui::SettingsPage::Hardware, 1U, 0U, h.state);
    h.state.device.displayRotated180 = false;
    (void)ui::buildMenuRow(ui::SettingsPage::Hardware, 1U, 0U, h.state);
    (void)ui::buildMenuRow(ui::SettingsPage::InputAssignments, 0U, 0U, h.state);
    (void)ui::buildMenuRow(ui::SettingsPage::InputAssignments, 1U, 0U, h.state);
    (void)ui::buildMenuRow(ui::SettingsPage::InputAssignments, 2U, 0U, h.state);
}

void testGrooveOffRotationFormattingCoversZeroLength(){
    Harness h;
    h.state.channels[0].common.groove = {GroovePreset::Off, 100U, 0U};
    const auto row = ui::buildMenuRow(ui::SettingsPage::Groove, 2U, 0U, h.state);
    TEST_ASSERT_TRUE(std::strcmp("0/0", row.value) == 0);
}

void testEditorDefensiveAndSearchBranchesAreReachable(){
    Harness h;
    const bool encoderBefore = h.state.device.encoderDirectionReversed;
    h.editor.adjust(ui::SettingsPage::Hardware, 0U, 0U, 0);
    TEST_ASSERT_EQUAL(encoderBefore, h.state.device.encoderDirectionReversed);

    h.state.display.screensaverMode = ScreensaverMode::Matrix;
    h.editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    TEST_ASSERT_EQUAL(ScreensaverMode::CubeCover, h.state.display.screensaverMode);

    h.state.unifiedClock.humanizeUs = 2000U;
    h.editor.adjust(ui::SettingsPage::UnifiedTiming, 5U, 0U, -1);
    TEST_ASSERT_EQUAL_UINT32(1000U, h.state.unifiedClock.humanizeUs);

    const auto grooveBefore = h.state.channels[0].common.groove;
    h.editor.adjust(ui::SettingsPage::ChannelTiming, 4U, 0U, 1);
    TEST_ASSERT_EQUAL(grooveBefore.preset, h.state.channels[0].common.groove.preset);
    h.editor.adjust(ui::SettingsPage::UnifiedTiming, 4U, 0U, 1);
}


void testScreensaverEditorCoversInvalidStoredModeAndOffDelayRow(){
    Harness h;
    h.state.display.screensaverMode = static_cast<ScreensaverMode>(255U);
    h.editor.adjust(ui::SettingsPage::Screensaver, 0U, 0U, 1);
    TEST_ASSERT_EQUAL(ScreensaverMode::Clock, h.state.display.screensaverMode);
    h.state.display.dimAfterMinutes = 5U;
    h.state.display.offAfterMinutes = 6U;
    h.editor.adjust(ui::SettingsPage::Screensaver, 3U, 0U, 1);
    TEST_ASSERT_EQUAL_UINT8(7U, h.state.display.offAfterMinutes);
}

void testUnifiedHumanizeEditorNormalizesUnknownStoredValue(){
    Harness h;
    h.state.unifiedClock.humanizeUs = 1234U;
    h.editor.adjust(ui::SettingsPage::UnifiedTiming, 5U, 0U, 1);
    TEST_ASSERT_EQUAL_UINT32(250U, h.state.unifiedClock.humanizeUs);
}

void testUnifiedClockSwingClampsAtFifty(){ Harness h; h.editor.adjust(ui::SettingsPage::UnifiedTiming,3U,0U,127); TEST_ASSERT_EQUAL_UINT8(50U,h.state.unifiedClock.swingPercent); }
void testUnifiedClockHumanizeUsesCuratedOptions(){ Harness h; h.state.unifiedClock.humanizeUs=0U; h.editor.adjust(ui::SettingsPage::UnifiedTiming,5U,0U,4); TEST_ASSERT_EQUAL_UINT32(2000U,h.state.unifiedClock.humanizeUs); }
void testDividerBankClampsAtPrimes(){ Harness h; h.state.dividerBank.bank=DividerBank::PowersOfTwo; h.editor.adjust(ui::SettingsPage::DividerBank,1U,0U,127); TEST_ASSERT_EQUAL(DividerBank::Primes,h.state.dividerBank.bank); }
void testDividerBankGateLengthUsesCuratedOptions(){ Harness h; h.state.dividerBank.gateLengthMs=10U; h.editor.adjust(ui::SettingsPage::DividerBank,2U,0U,1); TEST_ASSERT_EQUAL_UINT32(20U,h.state.dividerBank.gateLengthMs); }
void testNonEditablePageDoesNotChangeTempo(){ Harness h; const auto before=h.state.bpm; h.editor.adjust(ui::SettingsPage::Info,0U,0U,127); TEST_ASSERT_EQUAL_UINT32(before,h.state.bpm); }

}  // namespace

int main(){
    UNITY_BEGIN();
    RUN_TEST(testHardwareEncoderDirectionCanBeReversed); RUN_TEST(testHardwareEncoderDirectionCanReturnToNormal); RUN_TEST(testHardwareDisplayOrientationCanRotate180); RUN_TEST(testHardwareDisplayOrientationCanReturnToZero); RUN_TEST(testInputAssignmentsDefaultToSyncAndReset); RUN_TEST(testInputAssignmentsSkipFunctionAlreadyUsedByOtherInput); RUN_TEST(testInputAssignmentsAllowOffOnBothInputs); RUN_TEST(testFillIsReservedAndNotSelectableYet);
    RUN_TEST(testInputFunctionLabelsCoverAllRoles); RUN_TEST(testInputAssignmentEditorCoversBothRowsAndBoundaries); RUN_TEST(testInputAssignmentEditorIgnoresZeroDeltaAndInvalidRow); RUN_TEST(testHardwareEditorIgnoresUnknownRow);
    RUN_TEST(testMasterTempoClampsAtConfiguredMaximum); RUN_TEST(testMasterTempoClampsAtConfiguredMinimum);
    RUN_TEST(testMinimumBpmClampsAtTechnicalMinimum); RUN_TEST(testMinimumBpmCannotExceedMaximum); RUN_TEST(testRaisingMinimumClampsCurrentTempo);
    RUN_TEST(testMaximumBpmClampsAtTechnicalMaximum); RUN_TEST(testMaximumBpmCannotFallBelowMinimum); RUN_TEST(testLoweringMaximumClampsCurrentTempo);
    RUN_TEST(testMasterMeterBeatsClampAtOne); RUN_TEST(testMasterMeterBeatsClampAtSixteen); RUN_TEST(testMasterBeatUnitStepsThroughOptions); RUN_TEST(testPreCountDefaultsOffAndMenuShowsOff); RUN_TEST(testPreCountSelectableRangeIsOneToSixtyFourPlusOff); RUN_TEST(testPreCountMenuShowsConfiguredCount);
    RUN_TEST(testSyncSourceClampsInternal); RUN_TEST(testSyncSourceClampsAuto); RUN_TEST(testSyncPpqnStepsOneTwoFourTwentyFour); RUN_TEST(testSyncPpqnInvalidStoredValueFallsBackToOptionStart);
    RUN_TEST(testSyncEdgePositiveDeltaSelectsFalling); RUN_TEST(testSyncEdgeNegativeDeltaSelectsRising); RUN_TEST(testSyncLossModeClampsAcrossEnum); RUN_TEST(testSyncResetModeSelectsGateAndTrigger);
    RUN_TEST(testSyncGlitchFilterClampsZeroToFiveMilliseconds); RUN_TEST(testSyncSmoothingClampsOffToFull); RUN_TEST(testSyncSmoothingMenuLabelsAllModes); RUN_TEST(testSyncTimeoutClampsTwoHundredToFiveThousandMs);
    RUN_TEST(testScreensaverModeOrderStartsWithOff); RUN_TEST(testScreensaverModeCanReachFireworks); RUN_TEST(testScreensaverMenuLabelsAllModes); RUN_TEST(testScreensaverDelayCannotExceedDimDelay); RUN_TEST(testDimDelayCannotPrecedeScreensaver); RUN_TEST(testDimDelayCannotExceedOffDelay); RUN_TEST(testOffDelayCannotPrecedeDimDelay);
    RUN_TEST(testInvalidChannelIndexIsIgnored); RUN_TEST(testChannelSwingClampsZeroToFifty); RUN_TEST(testChannelProbabilityClampsZeroToHundred); RUN_TEST(testChannelGateLengthUsesCuratedOptions); RUN_TEST(testChannelPhaseClampsZeroToNinetyNine); RUN_TEST(testChannelResetModeFollowsDeltaSign); RUN_TEST(testChannelMuteTogglesOncePerAdjustment);
    RUN_TEST(testRateFactorTraversesCuratedList); RUN_TEST(testRateNumeratorClampsOneToSixteen); RUN_TEST(testRateDenominatorClampsOneToSixteen); RUN_TEST(testClockMeterBeatsClampOneToSixteen);
    RUN_TEST(testEuclidShrinkingStepsClampsHitsAndRotation); RUN_TEST(testEuclidHitsCannotExceedSteps); RUN_TEST(testEuclidRotationCannotExceedLastStep);
    RUN_TEST(testSequencerLengthClampResetsInvalidRotation); RUN_TEST(testSequencerRotationClampsToLengthMinusOne); RUN_TEST(testSequencerToggleRejectsStepOutsideLength); RUN_TEST(testSequencerToggleFlipsValidStep); RUN_TEST(testSequencerPasteBeforeCopyIsRejected); RUN_TEST(testSequencerCopyPasteClampsPatternToTargetLength);
    RUN_TEST(testGrooveDefaultsOffAtFullAmount); RUN_TEST(testUnifiedGrooveEditorOwnsGlobalSettings); RUN_TEST(testIndependentGrooveEditorOwnsSelectedChannel); RUN_TEST(testGrooveAmountClampsZeroToHundred); RUN_TEST(testGrooveRotationClampsToPatternLength); RUN_TEST(testGroovePresetChangeNormalizesRotation); RUN_TEST(testGrooveMenuShapeIsStable); RUN_TEST(testChannelRootUsesModeAwareGroupPriority); RUN_TEST(testNestedGroupPagesExposeFormerGroupedParameters);
    RUN_TEST(testAllSettingsPageTitlesAndCountsAreReachable); RUN_TEST(testAllChannelRootRowsAreReachable); RUN_TEST(testGrooveMenuRowsAndNoOpBranchesAreReachable); RUN_TEST(testChannelHierarchyFallbacksAndActionLookupAreReachable); RUN_TEST(testMenuModelConditionalRowsCoverBothStates); RUN_TEST(testGrooveOffRotationFormattingCoversZeroLength); RUN_TEST(testEditorDefensiveAndSearchBranchesAreReachable);
    RUN_TEST(testScreensaverEditorCoversInvalidStoredModeAndOffDelayRow); RUN_TEST(testUnifiedHumanizeEditorNormalizesUnknownStoredValue);
    RUN_TEST(testUnifiedClockSwingClampsAtFifty); RUN_TEST(testUnifiedClockHumanizeUsesCuratedOptions); RUN_TEST(testDividerBankClampsAtPrimes); RUN_TEST(testDividerBankGateLengthUsesCuratedOptions); RUN_TEST(testNonEditablePageDoesNotChangeTempo);
    return UNITY_END();
}
