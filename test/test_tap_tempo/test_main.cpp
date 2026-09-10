/**
 * @file test_main.cpp
 * @brief Atomic native behavioral tests for the tap-tempo estimator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <cstdint>
#include <limits>
#include <unity.h>

#include "config.h"
#include "services/tap_tempo.h"

using clockfw::services::TapTempo;

void setUp() {}
void tearDown() {}

namespace {
void testFirstTapAtZeroIsAcceptedAsAnchor() { TapTempo t; TEST_ASSERT_EQUAL_UINT32(0U, t.registerTap(0U, 20U, 999U)); TEST_ASSERT_EQUAL_UINT32(120U, t.registerTap(500U, 20U, 999U)); }
void testFirstNonzeroTapReturnsNoTempo() { TapTempo t; TEST_ASSERT_EQUAL_UINT32(0U, t.registerTap(1000U, 20U, 999U)); }
void testFiveHundredMillisecondsIs120Bpm() { TapTempo t; t.registerTap(100U,20U,999U); TEST_ASSERT_EQUAL_UINT32(120U,t.registerTap(600U,20U,999U)); }
void testOneSecondIs60Bpm() { TapTempo t; t.registerTap(100U,20U,999U); TEST_ASSERT_EQUAL_UINT32(60U,t.registerTap(1100U,20U,999U)); }
void testTwoSecondsIs30Bpm() { TapTempo t; t.registerTap(100U,20U,999U); TEST_ASSERT_EQUAL_UINT32(30U,t.registerTap(2100U,20U,999U)); }
void testQuarterSecondIs240Bpm() { TapTempo t; t.registerTap(100U,20U,999U); TEST_ASSERT_EQUAL_UINT32(240U,t.registerTap(350U,20U,999U)); }
void testSixtyMillisecondsClampsTo999Bpm() { TapTempo t; t.registerTap(100U,20U,999U); TEST_ASSERT_EQUAL_UINT32(999U,t.registerTap(160U,20U,999U)); }
void testTempoClampsToUserMinimum() { TapTempo t; t.registerTap(100U,90U,110U); TEST_ASSERT_EQUAL_UINT32(90U,t.registerTap(1100U,90U,110U)); }
void testTempoClampsToUserMaximum() { TapTempo t; t.registerTap(100U,90U,110U); TEST_ASSERT_EQUAL_UINT32(110U,t.registerTap(600U,90U,110U)); }
void testZeroMinimumIsRejected() { TapTempo t; TEST_ASSERT_EQUAL_UINT32(0U,t.registerTap(100U,0U,120U)); }
void testZeroMaximumIsRejected() { TapTempo t; TEST_ASSERT_EQUAL_UINT32(0U,t.registerTap(100U,20U,0U)); }
void testInvertedRangeIsRejected() { TapTempo t; TEST_ASSERT_EQUAL_UINT32(0U,t.registerTap(100U,121U,120U)); }
void testDuplicateTapIsRejectedAndResetsHistory() { TapTempo t; t.registerTap(1000U,20U,999U); TEST_ASSERT_EQUAL_UINT32(0U,t.registerTap(1000U,20U,999U)); TEST_ASSERT_EQUAL_UINT32(120U,t.registerTap(1500U,20U,999U)); }
void testTooFastTapResetsHistory() { TapTempo t; t.registerTap(1000U,20U,999U); TEST_ASSERT_EQUAL_UINT32(0U,t.registerTap(1000U + clockfw::config::kTapMinimumIntervalMs - 1U,20U,999U)); }
void testTooSlowTapStartsNewSequence() { TapTempo t; t.registerTap(1000U,20U,999U); TEST_ASSERT_EQUAL_UINT32(0U,t.registerTap(1000U + clockfw::config::kTapSequenceResetMs + 1U,20U,999U)); }
void testExplicitResetForgetsPreviousTap() { TapTempo t; t.registerTap(1000U,20U,999U); t.reset(); TEST_ASSERT_EQUAL_UINT32(0U,t.registerTap(1500U,20U,999U)); }
void testTwoIntervalsAreAveraged() { TapTempo t; t.registerTap(0U,20U,999U); t.registerTap(500U,20U,999U); TEST_ASSERT_EQUAL_UINT32(109U,t.registerTap(1100U,20U,999U)); }
void testThreeIntervalsAreAveraged() { TapTempo t; t.registerTap(0U,20U,999U); t.registerTap(500U,20U,999U); t.registerTap(1000U,20U,999U); TEST_ASSERT_EQUAL_UINT32(120U,t.registerTap(1500U,20U,999U)); }
void testRollingWindowDropsOldestAfterFourIntervals() { TapTempo t; t.registerTap(0U,20U,999U); t.registerTap(1000U,20U,999U); t.registerTap(1500U,20U,999U); t.registerTap(2000U,20U,999U); t.registerTap(2500U,20U,999U); TEST_ASSERT_EQUAL_UINT32(120U,t.registerTap(3000U,20U,999U)); }
void testSmallJitterAveragesNear120Bpm() { TapTempo t; t.registerTap(0U,20U,999U); t.registerTap(490U,20U,999U); t.registerTap(1000U,20U,999U); t.registerTap(1495U,20U,999U); const auto bpm=t.registerTap(2005U,20U,999U); TEST_ASSERT_TRUE(bpm>=119U && bpm<=121U); }
void testAlternatingFastSlowTapsAverageStably() { TapTempo t; t.registerTap(0U,20U,999U); t.registerTap(400U,20U,999U); t.registerTap(1000U,20U,999U); t.registerTap(1400U,20U,999U); TEST_ASSERT_EQUAL_UINT32(120U,t.registerTap(2000U,20U,999U)); }
void testTimestampWrapProducesCorrectInterval() { TapTempo t; const std::uint32_t a=std::numeric_limits<std::uint32_t>::max()-249U; TEST_ASSERT_EQUAL_UINT32(0U,t.registerTap(a,20U,999U)); TEST_ASSERT_EQUAL_UINT32(120U,t.registerTap(250U,20U,999U)); }
}

int main(){
    UNITY_BEGIN();
    RUN_TEST(testFirstTapAtZeroIsAcceptedAsAnchor); RUN_TEST(testFirstNonzeroTapReturnsNoTempo);
    RUN_TEST(testFiveHundredMillisecondsIs120Bpm); RUN_TEST(testOneSecondIs60Bpm); RUN_TEST(testTwoSecondsIs30Bpm); RUN_TEST(testQuarterSecondIs240Bpm);
    RUN_TEST(testSixtyMillisecondsClampsTo999Bpm); RUN_TEST(testTempoClampsToUserMinimum); RUN_TEST(testTempoClampsToUserMaximum);
    RUN_TEST(testZeroMinimumIsRejected); RUN_TEST(testZeroMaximumIsRejected); RUN_TEST(testInvertedRangeIsRejected);
    RUN_TEST(testDuplicateTapIsRejectedAndResetsHistory); RUN_TEST(testTooFastTapResetsHistory); RUN_TEST(testTooSlowTapStartsNewSequence); RUN_TEST(testExplicitResetForgetsPreviousTap);
    RUN_TEST(testTwoIntervalsAreAveraged); RUN_TEST(testThreeIntervalsAreAveraged); RUN_TEST(testRollingWindowDropsOldestAfterFourIntervals);
    RUN_TEST(testSmallJitterAveragesNear120Bpm); RUN_TEST(testAlternatingFastSlowTapsAverageStably); RUN_TEST(testTimestampWrapProducesCorrectInterval);
    return UNITY_END();
}
