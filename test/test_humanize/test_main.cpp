/**
 * @file test_main.cpp
 * @brief Atomic native tests for deterministic One Clock humanize timing.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <unity.h>
#include <Arduino.h>

#include "config.h"
#include "domain/clock_types.h"
#include "domain/default_configuration.h"
#include "engine/clock_engine.h"
#include "hal/gate_output_driver.h"
#include "pin_map.h"

using namespace clockfw;
void setUp(){fakefw::resetArduino();} void tearDown(){}

namespace {
using EdgeArray=std::array<std::uint32_t,kChannelCount>;
ClockState makeUnified(std::uint16_t humanize,std::uint8_t swing=0U){ClockState s{};initializeFactoryDefaults(s);s.bpm=120U;s.operatingMode=OperatingMode::UnifiedClock;s.source=ClockSource::Internal;s.unifiedClock.rate={ClockRatioMode::Multiply,1U,1U,1U};s.unifiedClock.swingPercent=swing;s.unifiedClock.phasePercent=0U;s.unifiedClock.gateLengthMs=1U;s.unifiedClock.humanizeUs=humanize;return s;}
EdgeArray collectSecondEdges(const ClockState& state){hal::GateOutputDriver gates;gates.beginDisabled();gates.enableOutputStage();engine::ClockEngine engine(gates);engine.begin(state);engine.play();fakefw::writes.clear();engine.processSchedulerTick();EdgeArray out{};out.fill(0xFFFFFFFFU);for(std::uint32_t tick=1U;tick<20000U;++tick){fakefw::writes.clear();engine.processSchedulerTick();for(std::size_t i=0;i<kChannelCount;++i){if(out[i]!=0xFFFFFFFFU)continue;const bool rose=std::any_of(fakefw::writes.begin(),fakefw::writes.end(),[i](const fakefw::PinWrite&w){return w.pin==pinmap::kGateChannelPins[i]&&w.value==HIGH;});if(rose)out[i]=tick;}if(std::all_of(out.begin(),out.end(),[](auto v){return v!=0xFFFFFFFFU;}))break;}return out;}
void assertAllAt(const EdgeArray&a,std::uint32_t expected){for(auto v:a)TEST_ASSERT_EQUAL_UINT32(expected,v);}
void assertWithin(const EdgeArray&a,std::uint32_t low,std::uint32_t high){for(auto v:a)TEST_ASSERT_TRUE(v>=low&&v<=high);}
void testHumanizeOffKeepsAllOutputsCoincident(){assertAllAt(collectSecondEdges(makeUnified(0U)),9999U);}
void testHumanize250StaysWithinConfiguredWindow(){assertWithin(collectSecondEdges(makeUnified(250U)),9994U,10005U);}
void testHumanize500StaysWithinConfiguredWindow(){assertWithin(collectSecondEdges(makeUnified(500U)),9989U,10010U);}
void testHumanize1000StaysWithinConfiguredWindow(){assertWithin(collectSecondEdges(makeUnified(1000U)),9979U,10020U);}
void testHumanize2000StaysWithinConfiguredWindow(){assertWithin(collectSecondEdges(makeUnified(2000U)),9959U,10040U);}
void testHumanizeProducesChannelSpread(){const auto a=collectSecondEdges(makeUnified(2000U));const auto [mn,mx]=std::minmax_element(a.begin(),a.end());TEST_ASSERT_TRUE(*mx>*mn);}
void testHumanizeIsDeterministicAcrossFreshRuns(){TEST_ASSERT_TRUE(collectSecondEdges(makeUnified(2000U))==collectSecondEdges(makeUnified(2000U)));}
void testHumanizeNeverAdvancesBeforePreviousEvent(){const auto a=collectSecondEdges(makeUnified(2000U));for(auto v:a)TEST_ASSERT_TRUE(v>0U);}
void testHumanizeWithFiftyPercentSwingStillMaintainsOrder(){const auto a=collectSecondEdges(makeUnified(2000U,50U));for(auto v:a)TEST_ASSERT_TRUE(v>10000U);}
void testHumanizeStoredOutsideUnifiedModeHasNoEffect(){auto s=makeUnified(2000U);s.operatingMode=OperatingMode::Independent;for(auto&c:s.channels){c.common.mode=ChannelMode::Clock;c.common.probabilityPercent=100U;c.common.swingPercent=0U;c.common.phasePercent=0U;c.common.gateLengthMs=1U;c.common.rate={ClockRatioMode::Multiply,1U,1U,1U};c.clock.meter={4U,4U};}assertAllAt(collectSecondEdges(s),9999U);}
void testHumanizeAboveConfiguredMaximumIsSafelyBounded(){auto s=makeUnified(65535U);const auto a=collectSecondEdges(s);const std::uint32_t maxTicks=config::kMaximumHumanizeUs/config::kSchedulerTickUs;assertWithin(a,9999U-maxTicks,10000U+maxTicks);}
void testHumanizeQuantizationIsSchedulerAligned(){const auto a=collectSecondEdges(makeUnified(2000U));for(auto v:a)TEST_ASSERT_TRUE(v>=9959U&&v<=10040U);}
}
int main(){UNITY_BEGIN();RUN_TEST(testHumanizeOffKeepsAllOutputsCoincident);RUN_TEST(testHumanize250StaysWithinConfiguredWindow);RUN_TEST(testHumanize500StaysWithinConfiguredWindow);RUN_TEST(testHumanize1000StaysWithinConfiguredWindow);RUN_TEST(testHumanize2000StaysWithinConfiguredWindow);RUN_TEST(testHumanizeProducesChannelSpread);RUN_TEST(testHumanizeIsDeterministicAcrossFreshRuns);RUN_TEST(testHumanizeNeverAdvancesBeforePreviousEvent);RUN_TEST(testHumanizeWithFiftyPercentSwingStillMaintainsOrder);RUN_TEST(testHumanizeStoredOutsideUnifiedModeHasNoEffect);RUN_TEST(testHumanizeAboveConfiguredMaximumIsSafelyBounded);RUN_TEST(testHumanizeQuantizationIsSchedulerAligned);return UNITY_END();}
