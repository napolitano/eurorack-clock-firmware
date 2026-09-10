/**
 * @file test_main.cpp
 * @brief Atomic native tests for every STOP-mode screensaver and animation reset behavior.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <unity.h>

#include "hal/oled_display.h"
#include "ui/screensaver_renderer.h"

using namespace clockfw;

void setUp() {}
void tearDown() {}

namespace {

bool hasPixels(const hal::OledDisplay& display) {
    const auto& fb = display.framebufferForTest();
    return std::any_of(fb.begin(), fb.end(), [](std::uint8_t value) { return value != 0U; });
}

void assertModeDraws(ScreensaverMode mode, std::uint32_t frame = 7U) {
    hal::OledDisplay display;
    ui::ScreensaverRenderer renderer(display);
    renderer.render(mode, frame);
    TEST_ASSERT_TRUE(hasPixels(display));
}

void testNoneDoesNotDrawAnything(){ hal::OledDisplay d; ui::ScreensaverRenderer r(d); r.render(ScreensaverMode::None,0U); TEST_ASSERT_FALSE(hasPixels(d)); }
void testFractalDraws(){ assertModeDraws(ScreensaverMode::Fractal); }
void testOrbitDraws(){ assertModeDraws(ScreensaverMode::Orbit); }
void testPlugDraws(){ assertModeDraws(ScreensaverMode::Plug); }
void testClockDraws(){ assertModeDraws(ScreensaverMode::Clock); }
void testHeartbeatDraws(){ assertModeDraws(ScreensaverMode::Heartbeat); }
void testAcidDraws(){ assertModeDraws(ScreensaverMode::Acid); }
void testSpectrumDraws(){ assertModeDraws(ScreensaverMode::Spectrum); }
void testFieldDraws(){ assertModeDraws(ScreensaverMode::Field); }
void testBloxDraws(){ hal::OledDisplay d; ui::ScreensaverRenderer r(d); for(std::uint32_t frame=0U;frame<=8U;++frame) r.render(ScreensaverMode::Blox,frame); TEST_ASSERT_TRUE(hasPixels(d)); }
void testMatrixDraws(){ assertModeDraws(ScreensaverMode::Matrix); }
void testCubeCoverDraws(){ assertModeDraws(ScreensaverMode::CubeCover); }
void testSameClockFrameIsDeterministic(){ hal::OledDisplay a,b; ui::ScreensaverRenderer ra(a),rb(b); ra.render(ScreensaverMode::Clock,123U); rb.render(ScreensaverMode::Clock,123U); TEST_ASSERT_TRUE(a.framebufferForTest()==b.framebufferForTest()); }
void testAcidFrameRewindRestartsDeterministically(){ hal::OledDisplay a,b; ui::ScreensaverRenderer ra(a),rb(b); ra.render(ScreensaverMode::Acid,25U); ra.render(ScreensaverMode::Acid,5U); rb.render(ScreensaverMode::Acid,5U); TEST_ASSERT_TRUE(a.framebufferForTest()==b.framebufferForTest()); }
void testAllModesSurviveLongAnimationSweep(){ hal::OledDisplay d; ui::ScreensaverRenderer r(d); constexpr std::array<ScreensaverMode,11U> modes{{ScreensaverMode::Fractal,ScreensaverMode::Orbit,ScreensaverMode::Plug,ScreensaverMode::Clock,ScreensaverMode::Heartbeat,ScreensaverMode::Acid,ScreensaverMode::Spectrum,ScreensaverMode::Field,ScreensaverMode::Blox,ScreensaverMode::Matrix,ScreensaverMode::CubeCover}}; for(auto mode:modes){ for(std::uint32_t frame=0U;frame<160U;frame+=7U) r.render(mode,frame); TEST_ASSERT_TRUE(hasPixels(d)); } }

}  // namespace

int main(){ UNITY_BEGIN();
RUN_TEST(testNoneDoesNotDrawAnything);RUN_TEST(testFractalDraws);RUN_TEST(testOrbitDraws);RUN_TEST(testPlugDraws);RUN_TEST(testClockDraws);RUN_TEST(testHeartbeatDraws);RUN_TEST(testAcidDraws);RUN_TEST(testSpectrumDraws);RUN_TEST(testFieldDraws);RUN_TEST(testBloxDraws);RUN_TEST(testMatrixDraws);RUN_TEST(testCubeCoverDraws);RUN_TEST(testSameClockFrameIsDeterministic);RUN_TEST(testAcidFrameRewindRestartsDeterministically);RUN_TEST(testAllModesSurviveLongAnimationSweep);
return UNITY_END(); }
