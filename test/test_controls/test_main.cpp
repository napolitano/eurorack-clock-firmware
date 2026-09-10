/**
 * @file test_main.cpp
 * @brief Atomic native tests for rotary encoder decoding and front-panel button debounce.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <array>
#include <cstdint>
#include <unity.h>
#include <Arduino.h>

#include "hal/control_panel.h"
#include "pin_map.h"

using namespace clockfw;

void setUp(){ fakefw::resetArduino(); }
void tearDown(){}

namespace {

void setEncoder(int a,int b){ fakefw::setPin(pinmap::kEncoderPhaseAPin,a?HIGH:LOW); fakefw::setPin(pinmap::kEncoderPhaseBPin,b?HIGH:LOW); }
void forwardDetent(){ setEncoder(1,0); setEncoder(0,0); setEncoder(0,1); setEncoder(1,1); }
void reverseDetent(){ setEncoder(0,1); setEncoder(0,0); setEncoder(1,0); setEncoder(1,1); }

hal::ButtonSample buttonFrom(const hal::ControlSample& s, std::size_t index){
    switch(index){case 0:return s.encoderButton;case 1:return s.transportButton;case 2:return s.tapButton;default:return s.resetButton;}
}
std::uint32_t pinFor(std::size_t index){
    constexpr std::array<std::uint32_t,4> pins{pinmap::kEncoderPushButtonPin,pinmap::kPlayPauseButtonPin,pinmap::kTapTempoButtonPin,pinmap::kResetBackButtonPin}; return pins[index];
}
void assertButtonDebounce(std::size_t index){
    hal::ControlPanel c; c.begin(); const auto pin=pinFor(index); fakefw::setPin(pin,LOW);
    auto s=c.sample(1U); TEST_ASSERT_EQUAL(hal::ButtonEdge::None,buttonFrom(s,index).edge); TEST_ASSERT_FALSE(buttonFrom(s,index).pressed);
    s=c.sample(25U); TEST_ASSERT_EQUAL(hal::ButtonEdge::None,buttonFrom(s,index).edge);
    s=c.sample(26U); TEST_ASSERT_EQUAL(hal::ButtonEdge::Pressed,buttonFrom(s,index).edge); TEST_ASSERT_TRUE(buttonFrom(s,index).pressed);
}
void assertButtonRelease(std::size_t index){
    hal::ControlPanel c; c.begin(); const auto pin=pinFor(index); fakefw::setPin(pin,LOW); (void)c.sample(1U); (void)c.sample(26U);
    fakefw::setPin(pin,HIGH); auto s=c.sample(27U); TEST_ASSERT_EQUAL(hal::ButtonEdge::None,buttonFrom(s,index).edge);
    s=c.sample(52U); TEST_ASSERT_EQUAL(hal::ButtonEdge::Released,buttonFrom(s,index).edge); TEST_ASSERT_FALSE(buttonFrom(s,index).pressed);
}
void assertHoldNoRepeat(std::size_t index){
    hal::ControlPanel c; c.begin(); const auto pin=pinFor(index); fakefw::setPin(pin,LOW); (void)c.sample(1U); auto s=c.sample(26U); TEST_ASSERT_EQUAL(hal::ButtonEdge::Pressed,buttonFrom(s,index).edge);
    s=c.sample(1000U); TEST_ASSERT_EQUAL(hal::ButtonEdge::None,buttonFrom(s,index).edge); TEST_ASSERT_TRUE(buttonFrom(s,index).pressed);
}

void testIdleSampleDoesNotMaskInterrupts(){ hal::ControlPanel c; c.begin(); fakefw::noInterruptCalls=0;fakefw::interruptCalls=0; TEST_ASSERT_EQUAL(0,c.sample(0U).encoderDelta); TEST_ASSERT_EQUAL_UINT32(0U,fakefw::noInterruptCalls); TEST_ASSERT_EQUAL_UINT32(0U,fakefw::interruptCalls); }
void testGrayCycleAProducesOneDetent(){ hal::ControlPanel c;c.begin();forwardDetent();const int d=c.sample(1U).encoderDelta;TEST_ASSERT_TRUE(d==1 || d==-1); }
void testGrayCycleBProducesOppositeDetent(){ hal::ControlPanel c;c.begin();forwardDetent();const int a=c.sample(1U).encoderDelta;reverseDetent();const int b=c.sample(2U).encoderDelta;TEST_ASSERT_EQUAL(-a,b); }
void testPartialGrayCycleProducesNoDetent(){ hal::ControlPanel c;c.begin();setEncoder(1,0);setEncoder(0,0);TEST_ASSERT_EQUAL(0,c.sample(1U).encoderDelta); }
void testEncoderBounceReturnsToSameStateWithoutDetent(){ hal::ControlPanel c;c.begin();setEncoder(1,0);setEncoder(1,1);TEST_ASSERT_EQUAL(0,c.sample(1U).encoderDelta); }
void testSixFastDetentsAccumulateWithoutForegroundPoll(){ hal::ControlPanel c;c.begin();forwardDetent();const int d=c.sample(1U).encoderDelta;for(int i=0;i<6;++i)forwardDetent();TEST_ASSERT_EQUAL(d*6,c.sample(2U).encoderDelta); }
void testEncoderDrainIsEmptyAfterSample(){ hal::ControlPanel c;c.begin();forwardDetent();const int d=c.sample(1U).encoderDelta;TEST_ASSERT_TRUE(d==1 || d==-1);TEST_ASSERT_EQUAL(0,c.sample(2U).encoderDelta); }
void testEncoderDrainCapsAt127AndPreservesRemainderForCycleA(){ hal::ControlPanel c;c.begin();forwardDetent();const int sign=c.sample(1U).encoderDelta;for(int i=0;i<130;++i)forwardDetent();TEST_ASSERT_EQUAL(sign*127,c.sample(2U).encoderDelta);TEST_ASSERT_EQUAL(sign*3,c.sample(3U).encoderDelta); }
void testEncoderDrainCapsAt127AndPreservesRemainderForCycleB(){ hal::ControlPanel c;c.begin();reverseDetent();const int sign=c.sample(1U).encoderDelta;for(int i=0;i<130;++i)reverseDetent();TEST_ASSERT_EQUAL(sign*127,c.sample(2U).encoderDelta);TEST_ASSERT_EQUAL(sign*3,c.sample(3U).encoderDelta); }
void testDirectionReversalCancelsPendingMovement(){ hal::ControlPanel c;c.begin();forwardDetent();reverseDetent();TEST_ASSERT_EQUAL(0,c.sample(1U).encoderDelta); }
void testEncoderLowAtStartupDoesNotManufactureDetent(){ setEncoder(0,0);hal::ControlPanel c;c.begin();TEST_ASSERT_EQUAL(0,c.sample(0U).encoderDelta); }
void testAllEncoderPinsUsePullups(){ hal::ControlPanel c;c.begin();TEST_ASSERT_EQUAL_UINT32(INPUT_PULLUP,fakefw::pinModes[pinmap::kEncoderPhaseAPin]);TEST_ASSERT_EQUAL_UINT32(INPUT_PULLUP,fakefw::pinModes[pinmap::kEncoderPhaseBPin]); }
void testEncoderPushDebouncesPress(){ assertButtonDebounce(0U); } void testPlayPauseDebouncesPress(){ assertButtonDebounce(1U); } void testTapDebouncesPress(){ assertButtonDebounce(2U); } void testResetBackDebouncesPress(){ assertButtonDebounce(3U); }
void testEncoderPushDebouncesRelease(){ assertButtonRelease(0U); } void testPlayPauseDebouncesRelease(){ assertButtonRelease(1U); } void testTapDebouncesRelease(){ assertButtonRelease(2U); } void testResetBackDebouncesRelease(){ assertButtonRelease(3U); }
void testEncoderPushHoldDoesNotRepeat(){ assertHoldNoRepeat(0U); } void testPlayPauseHoldDoesNotRepeat(){ assertHoldNoRepeat(1U); } void testTapHoldDoesNotRepeat(){ assertHoldNoRepeat(2U); } void testResetBackHoldDoesNotRepeat(){ assertHoldNoRepeat(3U); }
void testButtonBounceBeforeDebounceDoesNotPress(){ hal::ControlPanel c;c.begin();const auto p=pinmap::kTapTempoButtonPin;fakefw::setPin(p,LOW);(void)c.sample(1U);fakefw::setPin(p,HIGH);(void)c.sample(10U);fakefw::setPin(p,LOW);auto s=c.sample(30U);TEST_ASSERT_EQUAL(hal::ButtonEdge::None,s.tapButton.edge);s=c.sample(55U);TEST_ASSERT_EQUAL(hal::ButtonEdge::Pressed,s.tapButton.edge); }
void testSimultaneousButtonsReportIndependentEdges(){ hal::ControlPanel c;c.begin();fakefw::setPin(pinmap::kPlayPauseButtonPin,LOW);fakefw::setPin(pinmap::kTapTempoButtonPin,LOW);(void)c.sample(1U);auto s=c.sample(26U);TEST_ASSERT_EQUAL(hal::ButtonEdge::Pressed,s.transportButton.edge);TEST_ASSERT_EQUAL(hal::ButtonEdge::Pressed,s.tapButton.edge); }
void testButtonActivityDoesNotLoseEncoderDetent(){ hal::ControlPanel c;c.begin();fakefw::setPin(pinmap::kTapTempoButtonPin,LOW);forwardDetent();auto s=c.sample(1U);TEST_ASSERT_TRUE(s.encoderDelta==1 || s.encoderDelta==-1);s=c.sample(26U);TEST_ASSERT_EQUAL(hal::ButtonEdge::Pressed,s.tapButton.edge); }
void testInitiallyPressedButtonIsStableWithoutSyntheticEdge(){ fakefw::setPin(pinmap::kResetBackButtonPin,LOW);hal::ControlPanel c;c.begin();auto s=c.sample(0U);TEST_ASSERT_TRUE(s.resetButton.pressed);TEST_ASSERT_EQUAL(hal::ButtonEdge::None,s.resetButton.edge); }
}

int main(){UNITY_BEGIN();
RUN_TEST(testIdleSampleDoesNotMaskInterrupts);RUN_TEST(testGrayCycleAProducesOneDetent);RUN_TEST(testGrayCycleBProducesOppositeDetent);RUN_TEST(testPartialGrayCycleProducesNoDetent);RUN_TEST(testEncoderBounceReturnsToSameStateWithoutDetent);RUN_TEST(testSixFastDetentsAccumulateWithoutForegroundPoll);RUN_TEST(testEncoderDrainIsEmptyAfterSample);RUN_TEST(testEncoderDrainCapsAt127AndPreservesRemainderForCycleA);RUN_TEST(testEncoderDrainCapsAt127AndPreservesRemainderForCycleB);RUN_TEST(testDirectionReversalCancelsPendingMovement);RUN_TEST(testEncoderLowAtStartupDoesNotManufactureDetent);RUN_TEST(testAllEncoderPinsUsePullups);
RUN_TEST(testEncoderPushDebouncesPress);RUN_TEST(testPlayPauseDebouncesPress);RUN_TEST(testTapDebouncesPress);RUN_TEST(testResetBackDebouncesPress);RUN_TEST(testEncoderPushDebouncesRelease);RUN_TEST(testPlayPauseDebouncesRelease);RUN_TEST(testTapDebouncesRelease);RUN_TEST(testResetBackDebouncesRelease);RUN_TEST(testEncoderPushHoldDoesNotRepeat);RUN_TEST(testPlayPauseHoldDoesNotRepeat);RUN_TEST(testTapHoldDoesNotRepeat);RUN_TEST(testResetBackHoldDoesNotRepeat);RUN_TEST(testButtonBounceBeforeDebounceDoesNotPress);RUN_TEST(testSimultaneousButtonsReportIndependentEdges);RUN_TEST(testButtonActivityDoesNotLoseEncoderDetent);RUN_TEST(testInitiallyPressedButtonIsStableWithoutSyntheticEdge);
return UNITY_END();}
