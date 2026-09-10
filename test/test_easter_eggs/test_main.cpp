/**
 * @file test_main.cpp
 * @brief Atomic native tests for Easter-egg launch, gameplay safety, and shared arcade flow.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <unity.h>
#include <Arduino.h>

#include "game/arcade_leaderboard_store.h"
#include "game/arcade_shell.h"
#include "game/beatknecht.h"
#include "game/breakout_game.h"
#include "game/formula1_game.h"
#include "game/moon_buggy_game.h"
#include "game/pixel_raid_game.h"
#include "hal/control_panel.h"
#include "hal/gate_output_driver.h"
#include "hal/oled_display.h"
#include "hal/persistent_storage.h"
#include "pin_map.h"

using namespace clockfw;

namespace clockfw::game {
struct PixelRaidGameTestAccess {
    static void reset(PixelRaidGame& game){ game.resetSession(); }
    static std::uint8_t lives(const PixelRaidGame& game){ return game.lives_; }
    static bool hitAlien(PixelRaidGame& game,std::int16_t x,std::int16_t y){ return game.hitAlien(x,y); }
};
struct Formula1GameTestAccess {
    static void reset(Formula1Game& game){ game.resetSession(); }
    static void update(Formula1Game& game,const hal::ControlSample& controls,std::uint32_t nowMs){ game.update(controls,nowMs); }
    static std::uint16_t speed(const Formula1Game& game){ return game.speed_; }
};
struct BreakoutGameTestAccess {
    static void reset(BreakoutGame& game){ game.resetSession(); }
    static bool launched(const BreakoutGame& game){ return game.ballLaunched_; }
    static std::uint8_t lives(const BreakoutGame& game){ return game.lives_; }
};
struct MoonBuggyGameTestAccess {
    static void reset(MoonBuggyGame& game){ game.resetSession(); }
    static std::uint8_t lives(const MoonBuggyGame& game){ return game.lives_; }
    static bool gameOver(const MoonBuggyGame& game){ return game.gameOver_; }
};
struct BeatknechtTestAccess {
    static void reset(Beatknecht& game,std::uint32_t nowMs){ game.resetSession(nowMs); }
    static void update(Beatknecht& game,const hal::ControlSample& controls,std::uint32_t nowMs){ game.update(controls,nowMs); }
    static std::uint16_t bpm(const Beatknecht& game){ return game.bpm_; }
    static std::uint8_t style(const Beatknecht& game){ return game.styleIndex_; }
};
}

void setUp(){ fakefw::resetArduino(); hal::PersistentStorage::resetForTest(); }
void tearDown(){}

namespace {

hal::ControlSample tapPress(){ hal::ControlSample c{}; c.tapButton={hal::ButtonEdge::Pressed,true}; return c; }
bool hasPixels(const hal::OledDisplay& d){ const auto& fb=d.framebufferForTest(); return std::any_of(fb.begin(),fb.end(),[](std::uint8_t v){return v!=0U;}); }

struct Fixture {
    hal::PersistentStorage storage{};
    game::ArcadeLeaderboardStore leaderboard;
    hal::OledDisplay display{};
    hal::ControlPanel controls{};
    hal::GateOutputDriver gates{};
    Fixture():leaderboard(storage,game::ArcadeGameId::PixelRaid){ controls.begin(); gates.beginDisabled(); }
};

void assertIntroDoesNotAutoStart(game::ArcadeTitle title){ Fixture f; game::ArcadeShell shell(f.display,title,title==game::ArcadeTitle::Beatknecht?nullptr:&f.leaderboard); shell.begin(0U); hal::ControlSample idle{}; TEST_ASSERT_EQUAL(game::ArcadeShell::Action::None,shell.update(idle,600000U)); TEST_ASSERT_EQUAL(game::ArcadeShell::Screen::Intro,shell.screen()); }
void assertIntroStartsOnTap(game::ArcadeTitle title){ Fixture f; game::ArcadeShell shell(f.display,title,title==game::ArcadeTitle::Beatknecht?nullptr:&f.leaderboard); shell.begin(0U); const auto c=tapPress(); TEST_ASSERT_EQUAL(game::ArcadeShell::Action::StartRun,shell.update(c,1U)); TEST_ASSERT_TRUE(shell.playing()); }

void testPixelRaidIntroNeverAutoStarts(){ assertIntroDoesNotAutoStart(game::ArcadeTitle::PixelRaid); }
void testFormula1IntroNeverAutoStarts(){ assertIntroDoesNotAutoStart(game::ArcadeTitle::Formula1); }
void testBreakoutIntroNeverAutoStarts(){ assertIntroDoesNotAutoStart(game::ArcadeTitle::Breakout); }
void testMoonBuggyIntroNeverAutoStarts(){ assertIntroDoesNotAutoStart(game::ArcadeTitle::EggJourney); }
void testBeatknechtIntroNeverAutoStarts(){ assertIntroDoesNotAutoStart(game::ArcadeTitle::Beatknecht); }
void testPixelRaidIntroStartsOnTap(){ assertIntroStartsOnTap(game::ArcadeTitle::PixelRaid); }
void testFormula1IntroStartsOnTap(){ assertIntroStartsOnTap(game::ArcadeTitle::Formula1); }
void testBreakoutIntroStartsOnTap(){ assertIntroStartsOnTap(game::ArcadeTitle::Breakout); }
void testMoonBuggyIntroStartsOnTap(){ assertIntroStartsOnTap(game::ArcadeTitle::EggJourney); }
void testBeatknechtIntroStartsOnTap(){ assertIntroStartsOnTap(game::ArcadeTitle::Beatknecht); }

void testPixelRaidHostRunKeepsRackOutputStageDisabled(){ Fixture f; game::PixelRaidGame g(f.display,f.controls,f.gates,f.leaderboard); g.run(); TEST_ASSERT_EQUAL_UINT32(pinmap::kGateBufferDisabledLevel,fakefw::pinValues[pinmap::kGateBufferOutputEnablePin]); TEST_ASSERT_TRUE(hasPixels(f.display)); }
void testFormula1HostRunKeepsRackOutputStageDisabled(){ Fixture f; game::ArcadeLeaderboardStore l(f.storage,game::ArcadeGameId::Formula1); game::Formula1Game g(f.display,f.controls,f.gates,l); g.run(); TEST_ASSERT_EQUAL_UINT32(pinmap::kGateBufferDisabledLevel,fakefw::pinValues[pinmap::kGateBufferOutputEnablePin]); TEST_ASSERT_TRUE(hasPixels(f.display)); }
void testBreakoutHostRunKeepsRackOutputStageDisabled(){ Fixture f; game::ArcadeLeaderboardStore l(f.storage,game::ArcadeGameId::Breakout); game::BreakoutGame g(f.display,f.controls,f.gates,l); g.run(); TEST_ASSERT_EQUAL_UINT32(pinmap::kGateBufferDisabledLevel,fakefw::pinValues[pinmap::kGateBufferOutputEnablePin]); TEST_ASSERT_TRUE(hasPixels(f.display)); }
void testMoonBuggyHostRunKeepsRackOutputStageDisabled(){ Fixture f; game::ArcadeLeaderboardStore l(f.storage,game::ArcadeGameId::EggJourney); game::MoonBuggyGame g(f.display,f.controls,f.gates,l); g.run(); TEST_ASSERT_EQUAL_UINT32(pinmap::kGateBufferDisabledLevel,fakefw::pinValues[pinmap::kGateBufferOutputEnablePin]); TEST_ASSERT_TRUE(hasPixels(f.display)); }
void testBeatknechtHostRunStartsWithRackOutputStageDisabled(){ Fixture f; game::Beatknecht g(f.display,f.controls,f.gates); g.run(); TEST_ASSERT_EQUAL_UINT32(pinmap::kGateBufferDisabledLevel,fakefw::pinValues[pinmap::kGateBufferOutputEnablePin]); TEST_ASSERT_TRUE(hasPixels(f.display)); }

void testPixelRaidResetStartsWithThreeLives(){ Fixture f; game::PixelRaidGame g(f.display,f.controls,f.gates,f.leaderboard); game::PixelRaidGameTestAccess::reset(g); TEST_ASSERT_EQUAL_UINT8(3U,game::PixelRaidGameTestAccess::lives(g)); }
void testPixelRaidInitialAlienFieldHasExpectedCollision(){ Fixture f; game::PixelRaidGame g(f.display,f.controls,f.gates,f.leaderboard); game::PixelRaidGameTestAccess::reset(g); TEST_ASSERT_TRUE(game::PixelRaidGameTestAccess::hitAlien(g,7,13)); }
void testFormula1AcceleratesDuringNormalPlay(){ Fixture f; game::ArcadeLeaderboardStore l(f.storage,game::ArcadeGameId::Formula1); game::Formula1Game g(f.display,f.controls,f.gates,l); game::Formula1GameTestAccess::reset(g); hal::ControlSample idle{}; for(std::uint32_t t=55U;t<=1100U;t+=55U) game::Formula1GameTestAccess::update(g,idle,t); TEST_ASSERT_TRUE(game::Formula1GameTestAccess::speed(g)>0U); }
void testBreakoutResetWaitsForExplicitLaunch(){ Fixture f; game::ArcadeLeaderboardStore l(f.storage,game::ArcadeGameId::Breakout); game::BreakoutGame g(f.display,f.controls,f.gates,l); game::BreakoutGameTestAccess::reset(g); TEST_ASSERT_FALSE(game::BreakoutGameTestAccess::launched(g)); TEST_ASSERT_EQUAL_UINT8(3U,game::BreakoutGameTestAccess::lives(g)); }
void testMoonBuggyResetStartsAliveWithThreeLives(){ Fixture f; game::ArcadeLeaderboardStore l(f.storage,game::ArcadeGameId::EggJourney); game::MoonBuggyGame g(f.display,f.controls,f.gates,l); game::MoonBuggyGameTestAccess::reset(g); TEST_ASSERT_FALSE(game::MoonBuggyGameTestAccess::gameOver(g)); TEST_ASSERT_EQUAL_UINT8(3U,game::MoonBuggyGameTestAccess::lives(g)); }
void testBeatknechtEncoderChangesTempoWithoutChangingStyle(){ Fixture f; game::Beatknecht g(f.display,f.controls,f.gates); game::BeatknechtTestAccess::reset(g,0U); const auto style=game::BeatknechtTestAccess::style(g); hal::ControlSample c{}; c.encoderDelta=5; game::BeatknechtTestAccess::update(g,c,1U); TEST_ASSERT_TRUE(game::BeatknechtTestAccess::bpm(g)>120U); TEST_ASSERT_EQUAL_UINT8(style,game::BeatknechtTestAccess::style(g)); }
void testBeatknechtTapChangesStyleWithoutResettingTempo(){ Fixture f; game::Beatknecht g(f.display,f.controls,f.gates); game::BeatknechtTestAccess::reset(g,0U); hal::ControlSample turn{}; turn.encoderDelta=5; game::BeatknechtTestAccess::update(g,turn,1U); const auto bpm=game::BeatknechtTestAccess::bpm(g); auto tap=tapPress(); game::BeatknechtTestAccess::update(g,tap,2U); TEST_ASSERT_EQUAL_UINT32(bpm,game::BeatknechtTestAccess::bpm(g)); TEST_ASSERT_EQUAL_UINT8(1U,game::BeatknechtTestAccess::style(g)); }

}  // namespace

int main(){ UNITY_BEGIN();
RUN_TEST(testPixelRaidIntroNeverAutoStarts);RUN_TEST(testFormula1IntroNeverAutoStarts);RUN_TEST(testBreakoutIntroNeverAutoStarts);RUN_TEST(testMoonBuggyIntroNeverAutoStarts);RUN_TEST(testBeatknechtIntroNeverAutoStarts);
RUN_TEST(testPixelRaidIntroStartsOnTap);RUN_TEST(testFormula1IntroStartsOnTap);RUN_TEST(testBreakoutIntroStartsOnTap);RUN_TEST(testMoonBuggyIntroStartsOnTap);RUN_TEST(testBeatknechtIntroStartsOnTap);
RUN_TEST(testPixelRaidHostRunKeepsRackOutputStageDisabled);RUN_TEST(testFormula1HostRunKeepsRackOutputStageDisabled);RUN_TEST(testBreakoutHostRunKeepsRackOutputStageDisabled);RUN_TEST(testMoonBuggyHostRunKeepsRackOutputStageDisabled);RUN_TEST(testBeatknechtHostRunStartsWithRackOutputStageDisabled);
RUN_TEST(testPixelRaidResetStartsWithThreeLives);RUN_TEST(testPixelRaidInitialAlienFieldHasExpectedCollision);RUN_TEST(testFormula1AcceleratesDuringNormalPlay);RUN_TEST(testBreakoutResetWaitsForExplicitLaunch);RUN_TEST(testMoonBuggyResetStartsAliveWithThreeLives);RUN_TEST(testBeatknechtEncoderChangesTempoWithoutChangingStyle);RUN_TEST(testBeatknechtTapChangesStyleWithoutResettingTempo);
return UNITY_END(); }
