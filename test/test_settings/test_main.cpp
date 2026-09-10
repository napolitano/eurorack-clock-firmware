/**
 * @file test_main.cpp
 * @brief Atomic native tests for editable settings, clamps, and cross-field invariants.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <cstdint>
#include <unity.h>
#include <Arduino.h>

#include "domain/default_configuration.h"
#include "engine/clock_engine.h"
#include "hal/gate_output_driver.h"
#include "ui/settings_editor.h"

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
void testSyncSourceClampsInternal(){ Harness h; h.state.source=ClockSource::Internal; h.editor.adjust(ui::SettingsPage::Sync,0U,0U,-10); TEST_ASSERT_EQUAL(ClockSource::Internal,h.state.source); }
void testSyncSourceClampsAuto(){ Harness h; h.state.source=ClockSource::External; h.editor.adjust(ui::SettingsPage::Sync,0U,0U,10); TEST_ASSERT_EQUAL(ClockSource::Auto,h.state.source); }
void testSyncPpqnStepsOneTwoFourTwentyFour(){ Harness h; h.state.externalSync.pulsesPerQuarterNote=1U; h.editor.adjust(ui::SettingsPage::Sync,1U,0U,1); TEST_ASSERT_EQUAL_UINT8(2U,h.state.externalSync.pulsesPerQuarterNote); h.editor.adjust(ui::SettingsPage::Sync,1U,0U,1); TEST_ASSERT_EQUAL_UINT8(4U,h.state.externalSync.pulsesPerQuarterNote); h.editor.adjust(ui::SettingsPage::Sync,1U,0U,1); TEST_ASSERT_EQUAL_UINT8(24U,h.state.externalSync.pulsesPerQuarterNote); }
void testSyncPpqnInvalidStoredValueFallsBackToOptionStart(){ Harness h; h.state.externalSync.pulsesPerQuarterNote=3U; h.editor.adjust(ui::SettingsPage::Sync,1U,0U,1); TEST_ASSERT_EQUAL_UINT8(2U,h.state.externalSync.pulsesPerQuarterNote); }
void testSyncEdgePositiveDeltaSelectsFalling(){ Harness h; h.editor.adjust(ui::SettingsPage::Sync,2U,0U,1); TEST_ASSERT_EQUAL(SyncEdge::Falling,h.state.externalSync.edge); }
void testSyncEdgeNegativeDeltaSelectsRising(){ Harness h; h.state.externalSync.edge=SyncEdge::Falling; h.editor.adjust(ui::SettingsPage::Sync,2U,0U,-1); TEST_ASSERT_EQUAL(SyncEdge::Rising,h.state.externalSync.edge); }
void testSyncLossModeClampsAcrossEnum(){ Harness h; h.state.externalSync.lossMode=SyncLossMode::Stop; h.editor.adjust(ui::SettingsPage::Sync,3U,0U,10); TEST_ASSERT_EQUAL(SyncLossMode::Internal,h.state.externalSync.lossMode); }
void testSyncResetModeSelectsGateAndTrigger(){ Harness h; h.editor.adjust(ui::SettingsPage::Sync,4U,0U,1); TEST_ASSERT_EQUAL(ExternalResetMode::Gate,h.state.externalSync.resetMode); h.editor.adjust(ui::SettingsPage::Sync,4U,0U,-1); TEST_ASSERT_EQUAL(ExternalResetMode::Trigger,h.state.externalSync.resetMode); }
void testSyncGlitchFilterClampsZeroToFiveMilliseconds(){ Harness h; h.state.externalSync.glitchFilterUs=250U; h.editor.adjust(ui::SettingsPage::Sync,5U,0U,-10); TEST_ASSERT_EQUAL_UINT32(0U,h.state.externalSync.glitchFilterUs); h.editor.adjust(ui::SettingsPage::Sync,5U,0U,127); TEST_ASSERT_EQUAL_UINT32(5000U,h.state.externalSync.glitchFilterUs); }
void testSyncTimeoutClampsTwoHundredToFiveThousandMs(){ Harness h; h.state.externalSync.timeoutMs=300U; h.editor.adjust(ui::SettingsPage::Sync,6U,0U,-10); TEST_ASSERT_EQUAL_UINT32(200U,h.state.externalSync.timeoutMs); h.editor.adjust(ui::SettingsPage::Sync,6U,0U,127); TEST_ASSERT_EQUAL_UINT32(5000U,h.state.externalSync.timeoutMs); }
void testScreensaverModeOrderStartsWithOff(){ Harness h; h.state.display.screensaverMode=ScreensaverMode::None; h.editor.adjust(ui::SettingsPage::Screensaver,0U,0U,-10); TEST_ASSERT_EQUAL(ScreensaverMode::None,h.state.display.screensaverMode); }
void testScreensaverModeCanReachOrbit(){ Harness h; h.state.display.screensaverMode=ScreensaverMode::None; h.editor.adjust(ui::SettingsPage::Screensaver,0U,0U,127); TEST_ASSERT_EQUAL(ScreensaverMode::Orbit,h.state.display.screensaverMode); }
void testScreensaverDelayCannotExceedDimDelay(){ Harness h; h.state.display={ScreensaverMode::Clock,2U,5U,10U}; h.editor.adjust(ui::SettingsPage::Screensaver,1U,0U,127); TEST_ASSERT_EQUAL_UINT8(5U,h.state.display.screensaverAfterMinutes); }
void testDimDelayCannotPrecedeScreensaver(){ Harness h; h.state.display={ScreensaverMode::Clock,4U,5U,10U}; h.editor.adjust(ui::SettingsPage::Screensaver,2U,0U,-127); TEST_ASSERT_EQUAL_UINT8(4U,h.state.display.dimAfterMinutes); }
void testDimDelayCannotExceedOffDelay(){ Harness h; h.state.display={ScreensaverMode::Clock,2U,5U,6U}; h.editor.adjust(ui::SettingsPage::Screensaver,2U,0U,127); TEST_ASSERT_EQUAL_UINT8(6U,h.state.display.dimAfterMinutes); }
void testOffDelayCannotPrecedeDimDelay(){ Harness h; h.state.display={ScreensaverMode::Clock,2U,7U,10U}; h.editor.adjust(ui::SettingsPage::Screensaver,3U,0U,-127); TEST_ASSERT_EQUAL_UINT8(7U,h.state.display.offAfterMinutes); }
void testInvalidChannelIndexIsIgnored(){ Harness h; const auto before=h.state.channels[0]; h.editor.adjust(ui::SettingsPage::Channel,3U,99U,20); TEST_ASSERT_EQUAL_UINT8(before.common.swingPercent,h.state.channels[0].common.swingPercent); }
void testChannelSwingClampsZeroToFifty(){ Harness h; h.editor.adjust(ui::SettingsPage::Channel,3U,0U,127); TEST_ASSERT_EQUAL_UINT8(50U,h.state.channels[0].common.swingPercent); h.editor.adjust(ui::SettingsPage::Channel,3U,0U,-127); TEST_ASSERT_EQUAL_UINT8(0U,h.state.channels[0].common.swingPercent); }
void testChannelProbabilityClampsZeroToHundred(){ Harness h; h.editor.adjust(ui::SettingsPage::Channel,4U,0U,-127); TEST_ASSERT_EQUAL_UINT8(0U,h.state.channels[0].common.probabilityPercent); h.editor.adjust(ui::SettingsPage::Channel,4U,0U,127); TEST_ASSERT_EQUAL_UINT8(100U,h.state.channels[0].common.probabilityPercent); }
void testChannelGateLengthUsesCuratedOptions(){ Harness h; h.state.channels[0].common.gateLengthMs=10U; h.editor.adjust(ui::SettingsPage::Channel,5U,0U,1); TEST_ASSERT_EQUAL_UINT32(20U,h.state.channels[0].common.gateLengthMs); }
void testChannelPhaseClampsZeroToNinetyNine(){ Harness h; h.editor.adjust(ui::SettingsPage::Channel,6U,0U,127); TEST_ASSERT_EQUAL_UINT8(99U,h.state.channels[0].common.phasePercent); }
void testChannelResetModeFollowsDeltaSign(){ Harness h; h.editor.adjust(ui::SettingsPage::Channel,7U,0U,1); TEST_ASSERT_EQUAL(ResetMode::Free,h.state.channels[0].common.resetMode); h.editor.adjust(ui::SettingsPage::Channel,7U,0U,-1); TEST_ASSERT_EQUAL(ResetMode::Global,h.state.channels[0].common.resetMode); }
void testChannelMuteTogglesOncePerAdjustment(){ Harness h; TEST_ASSERT_FALSE(h.state.channels[0].common.muted); h.editor.adjust(ui::SettingsPage::Channel,8U,0U,1); TEST_ASSERT_TRUE(h.state.channels[0].common.muted); h.editor.adjust(ui::SettingsPage::Channel,8U,0U,1); TEST_ASSERT_FALSE(h.state.channels[0].common.muted); }
void testRateFactorTraversesCuratedList(){ Harness h; h.state.channels[0].common.rate={ClockRatioMode::Multiply,1U,1U,1U}; h.editor.adjust(ui::SettingsPage::Rate,0U,0U,1); TEST_ASSERT_EQUAL_UINT8(2U,h.state.channels[0].common.rate.factor); }
void testRateNumeratorClampsOneToSixteen(){ Harness h; h.editor.adjust(ui::SettingsPage::Rate,1U,0U,127); TEST_ASSERT_EQUAL_UINT8(16U,h.state.channels[0].common.rate.numerator); }
void testRateDenominatorClampsOneToSixteen(){ Harness h; h.editor.adjust(ui::SettingsPage::Rate,2U,0U,-127); TEST_ASSERT_EQUAL_UINT8(1U,h.state.channels[0].common.rate.denominator); }
void testClockMeterBeatsClampOneToSixteen(){ Harness h; h.state.channels[0].clock.meter.beats=4U; h.editor.adjust(ui::SettingsPage::Clock,0U,0U,127); TEST_ASSERT_EQUAL_UINT8(16U,h.state.channels[0].clock.meter.beats); }
void testEuclidShrinkingStepsClampsHitsAndRotation(){ Harness h; h.state.channels[0].euclid={16U,12U,15U}; h.editor.adjust(ui::SettingsPage::Euclid,0U,0U,-12); TEST_ASSERT_EQUAL_UINT8(4U,h.state.channels[0].euclid.steps); TEST_ASSERT_EQUAL_UINT8(4U,h.state.channels[0].euclid.hits); TEST_ASSERT_EQUAL_UINT8(0U,h.state.channels[0].euclid.rotation); }
void testEuclidHitsCannotExceedSteps(){ Harness h; h.state.channels[0].euclid={7U,6U,0U}; h.editor.adjust(ui::SettingsPage::Euclid,1U,0U,127); TEST_ASSERT_EQUAL_UINT8(7U,h.state.channels[0].euclid.hits); }
void testEuclidRotationCannotExceedLastStep(){ Harness h; h.state.channels[0].euclid={7U,3U,0U}; h.editor.adjust(ui::SettingsPage::Euclid,2U,0U,127); TEST_ASSERT_EQUAL_UINT8(6U,h.state.channels[0].euclid.rotation); }
void testSequencerLengthClampResetsInvalidRotation(){ Harness h; h.state.channels[0].sequencer={16U,15U,0xFFFFU}; h.editor.adjust(ui::SettingsPage::Sequencer,1U,0U,-12); TEST_ASSERT_EQUAL_UINT8(4U,h.state.channels[0].sequencer.length); TEST_ASSERT_EQUAL_UINT8(0U,h.state.channels[0].sequencer.rotation); }
void testSequencerRotationClampsToLengthMinusOne(){ Harness h; h.state.channels[0].sequencer={8U,0U,1U}; h.editor.adjust(ui::SettingsPage::Sequencer,2U,0U,127); TEST_ASSERT_EQUAL_UINT8(7U,h.state.channels[0].sequencer.rotation); }
void testSequencerToggleRejectsStepOutsideLength(){ Harness h; h.state.channels[0].sequencer={8U,0U,0x01U}; h.editor.toggleSequencerStep(0U,8U); TEST_ASSERT_EQUAL_UINT64(0x01U,h.state.channels[0].sequencer.pattern); }
void testSequencerToggleFlipsValidStep(){ Harness h; h.state.channels[0].sequencer={8U,0U,0U}; h.editor.toggleSequencerStep(0U,3U); TEST_ASSERT_EQUAL_UINT64(0x08U,h.state.channels[0].sequencer.pattern); }
void testSequencerPasteBeforeCopyIsRejected(){ Harness h; TEST_ASSERT_FALSE(h.editor.executeSequencerCommand(0U,7U)); }
void testSequencerCopyPasteClampsPatternToTargetLength(){ Harness h; h.state.channels[0].sequencer={16U,0U,0xFFFFU}; TEST_ASSERT_TRUE(h.editor.executeSequencerCommand(0U,6U)); h.state.channels[0].sequencer={4U,0U,0U}; TEST_ASSERT_TRUE(h.editor.executeSequencerCommand(0U,7U)); TEST_ASSERT_EQUAL_UINT64(0x0FU,h.state.channels[0].sequencer.pattern); }
void testUnifiedClockSwingClampsAtFifty(){ Harness h; h.editor.adjust(ui::SettingsPage::UnifiedClock,4U,0U,127); TEST_ASSERT_EQUAL_UINT8(50U,h.state.unifiedClock.swingPercent); }
void testUnifiedClockHumanizeUsesCuratedOptions(){ Harness h; h.state.unifiedClock.humanizeUs=0U; h.editor.adjust(ui::SettingsPage::UnifiedClock,7U,0U,4); TEST_ASSERT_EQUAL_UINT32(2000U,h.state.unifiedClock.humanizeUs); }
void testDividerBankClampsAtPrimes(){ Harness h; h.state.dividerBank.bank=DividerBank::PowersOfTwo; h.editor.adjust(ui::SettingsPage::DividerBank,1U,0U,127); TEST_ASSERT_EQUAL(DividerBank::Primes,h.state.dividerBank.bank); }
void testDividerBankGateLengthUsesCuratedOptions(){ Harness h; h.state.dividerBank.gateLengthMs=10U; h.editor.adjust(ui::SettingsPage::DividerBank,2U,0U,1); TEST_ASSERT_EQUAL_UINT32(20U,h.state.dividerBank.gateLengthMs); }
void testNonEditablePageDoesNotChangeTempo(){ Harness h; const auto before=h.state.bpm; h.editor.adjust(ui::SettingsPage::Info,0U,0U,127); TEST_ASSERT_EQUAL_UINT32(before,h.state.bpm); }

}  // namespace

int main(){
    UNITY_BEGIN();
    RUN_TEST(testMasterTempoClampsAtConfiguredMaximum); RUN_TEST(testMasterTempoClampsAtConfiguredMinimum);
    RUN_TEST(testMinimumBpmClampsAtTechnicalMinimum); RUN_TEST(testMinimumBpmCannotExceedMaximum); RUN_TEST(testRaisingMinimumClampsCurrentTempo);
    RUN_TEST(testMaximumBpmClampsAtTechnicalMaximum); RUN_TEST(testMaximumBpmCannotFallBelowMinimum); RUN_TEST(testLoweringMaximumClampsCurrentTempo);
    RUN_TEST(testMasterMeterBeatsClampAtOne); RUN_TEST(testMasterMeterBeatsClampAtSixteen); RUN_TEST(testMasterBeatUnitStepsThroughOptions);
    RUN_TEST(testSyncSourceClampsInternal); RUN_TEST(testSyncSourceClampsAuto); RUN_TEST(testSyncPpqnStepsOneTwoFourTwentyFour); RUN_TEST(testSyncPpqnInvalidStoredValueFallsBackToOptionStart);
    RUN_TEST(testSyncEdgePositiveDeltaSelectsFalling); RUN_TEST(testSyncEdgeNegativeDeltaSelectsRising); RUN_TEST(testSyncLossModeClampsAcrossEnum); RUN_TEST(testSyncResetModeSelectsGateAndTrigger);
    RUN_TEST(testSyncGlitchFilterClampsZeroToFiveMilliseconds); RUN_TEST(testSyncTimeoutClampsTwoHundredToFiveThousandMs);
    RUN_TEST(testScreensaverModeOrderStartsWithOff); RUN_TEST(testScreensaverModeCanReachOrbit); RUN_TEST(testScreensaverDelayCannotExceedDimDelay); RUN_TEST(testDimDelayCannotPrecedeScreensaver); RUN_TEST(testDimDelayCannotExceedOffDelay); RUN_TEST(testOffDelayCannotPrecedeDimDelay);
    RUN_TEST(testInvalidChannelIndexIsIgnored); RUN_TEST(testChannelSwingClampsZeroToFifty); RUN_TEST(testChannelProbabilityClampsZeroToHundred); RUN_TEST(testChannelGateLengthUsesCuratedOptions); RUN_TEST(testChannelPhaseClampsZeroToNinetyNine); RUN_TEST(testChannelResetModeFollowsDeltaSign); RUN_TEST(testChannelMuteTogglesOncePerAdjustment);
    RUN_TEST(testRateFactorTraversesCuratedList); RUN_TEST(testRateNumeratorClampsOneToSixteen); RUN_TEST(testRateDenominatorClampsOneToSixteen); RUN_TEST(testClockMeterBeatsClampOneToSixteen);
    RUN_TEST(testEuclidShrinkingStepsClampsHitsAndRotation); RUN_TEST(testEuclidHitsCannotExceedSteps); RUN_TEST(testEuclidRotationCannotExceedLastStep);
    RUN_TEST(testSequencerLengthClampResetsInvalidRotation); RUN_TEST(testSequencerRotationClampsToLengthMinusOne); RUN_TEST(testSequencerToggleRejectsStepOutsideLength); RUN_TEST(testSequencerToggleFlipsValidStep); RUN_TEST(testSequencerPasteBeforeCopyIsRejected); RUN_TEST(testSequencerCopyPasteClampsPatternToTargetLength);
    RUN_TEST(testUnifiedClockSwingClampsAtFifty); RUN_TEST(testUnifiedClockHumanizeUsesCuratedOptions); RUN_TEST(testDividerBankClampsAtPrimes); RUN_TEST(testDividerBankGateLengthUsesCuratedOptions); RUN_TEST(testNonEditablePageDoesNotChangeTempo);
    return UNITY_END();
}
