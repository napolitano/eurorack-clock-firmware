/**
 * @file test_main.cpp
 * @brief Comprehensive host tests for firmware logic, UI, and HAL behavior using deterministic fakes.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>
#include <exception>
#include <iostream>
#include <string>

#include <Arduino.h>
#include <HardwareTimer.h>
#include <SPI.h>
#include <Wire.h>
#include <unity.h>

#include "app/clock_application.h"
#include "clock_core.h"
#include "config.h"
#include "defaults.h"
#include "domain/clock_labels.h"
#include "domain/clock_options.h"
#include "domain/default_configuration.h"
#include "engine/clock_engine.h"
#include "engine/output_mode_resolver.h"
#include "game/arcade_leaderboard_store.h"
#include "game/arcade_shell.h"
#include "game/breakout_game.h"
#include "game/easter_egg_score_store.h"
#include "game/formula1_game.h"
#include "game/egg_journey_game.h"
#include "game/pixel_raid_game.h"
#include "game/beatknecht.h"
#include "hal/control_panel.h"
#include "hal/display_font.h"
#include "hal/external_input_capture.h"
#include "hal/gate_output_driver.h"
#include "hal/interrupt_lock.h"
#include "hal/oled_display.h"
#include "hal/periodic_timer.h"
#include "hal/persistent_layout.h"
#include "hal/persistent_storage.h"
#include "hal/system_clock.h"
#include "pin_map.h"
#include "services/persistent_state_service.h"
#include "services/custom_groove_store.h"
#include "services/persistent_state_validation.h"
#include "services/tap_tempo.h"
#include "services/template_service.h"
#include "ui/menu_model.h"
#include "ui/menu_model_sync.h"
#include "ui/mode_functions.h"
#include "ui/pattern_strip_renderer.h"
#include "ui/performance_renderer.h"
#include "ui/preset_name_alphabet.h"
#include "ui/settings_editor.h"
#include "ui/settings_renderer.h"
#include "ui/screensaver_renderer.h"
#include "ui/text_formatter.h"
#include "ui/ui_controller.h"
#include "ui/ui_renderer.h"
#include "ui_text.h"

extern "C" void setup();
extern "C" void loop();

namespace clockfw::game {
struct PixelRaidGameTestAccess {
    static void reset(PixelRaidGame& game) { game.resetSession(); }
    static void update(PixelRaidGame& game, const hal::ControlSample& controls, std::uint32_t nowMs) { game.update(controls, nowMs); }
    static void render(PixelRaidGame& game) { game.render(); }
    static bool hitAlien(PixelRaidGame& game, std::int16_t x, std::int16_t y) { return game.hitAlien(x, y); }
    static std::int16_t chooseEnemyShotX(PixelRaidGame& game) { return game.chooseEnemyShotX(); }
    static void flashLifeLost(PixelRaidGame& game) { game.flashLifeLostLeds(); }
    static bool confirmExit(PixelRaidGame& game) { return game.confirmExit(); }
    static void forceEnemyHit(PixelRaidGame& game, std::uint8_t lives, std::uint32_t score, std::uint32_t nowMs) {
        game.lives_ = lives;
        game.score_ = score;
        game.enemyShotActive_ = true;
        game.enemyShotX_ = static_cast<std::int16_t>(game.playerX_ + 3);
        game.enemyShotY_ = 57;
        game.updateEnemyShot(nowMs);
    }
    static void clearAliensAndAdvance(PixelRaidGame& game, std::uint32_t nowMs) {
        game.aliens_.fill(false);
        game.lastAlienMoveAtMs_ = 0U;
        game.updateAliens(nowMs);
    }
    static void forcePlayerShot(PixelRaidGame& game, std::int16_t x, std::int16_t y) {
        game.playerShotActive_ = true;
        game.playerShotX_ = x;
        game.playerShotY_ = y;
        game.updatePlayerShot();
    }
    static bool gameOver(const PixelRaidGame& game) { return game.gameOver_; }
    static std::uint8_t lives(const PixelRaidGame& game) { return game.lives_; }
    static void setGameOver(PixelRaidGame& game, bool value) { game.gameOver_ = value; }
    static void setPlayerShot(PixelRaidGame& game, bool active, std::int16_t x, std::int16_t y) { game.playerShotActive_ = active; game.playerShotX_ = x; game.playerShotY_ = y; }
    static void setEnemyShot(PixelRaidGame& game, bool active, std::int16_t x, std::int16_t y, std::uint32_t lastAt = 0U) { game.enemyShotActive_ = active; game.enemyShotX_ = x; game.enemyShotY_ = y; game.lastEnemyShotAtMs_ = lastAt; }
    static void setAlienMotion(PixelRaidGame& game, std::int16_t x, std::int8_t direction, std::uint32_t lastAt = 0U) { game.alienOffsetX_ = x; game.alienDirection_ = direction; game.lastAlienMoveAtMs_ = lastAt; }
    static void clearAliens(PixelRaidGame& game) { game.aliens_.fill(false); }
    static void setAlienAlive(PixelRaidGame& game, std::size_t index, bool alive) { if (index < game.aliens_.size()) game.aliens_[index] = alive; }
    static void updatePlayerShot(PixelRaidGame& game) { game.updatePlayerShot(); }
    static void updateEnemyShot(PixelRaidGame& game, std::uint32_t nowMs) { game.updateEnemyShot(nowMs); }
    static void updateAliens(PixelRaidGame& game, std::uint32_t nowMs) { game.updateAliens(nowMs); }
};

struct Formula1GameTestAccess {
    static void reset(Formula1Game& game) { game.resetSession(); }
    static void update(Formula1Game& game, const hal::ControlSample& controls, std::uint32_t nowMs) { game.update(controls, nowMs); }
    static void render(Formula1Game& game) { game.render(); }
    static void setDistance(Formula1Game& game, std::uint32_t distance) { game.distance_ = distance; }
    static std::uint16_t speed(const Formula1Game& game) { return game.speed_; }
    static std::uint32_t distance(const Formula1Game& game) { return game.distance_; }
    static void forceCrash(Formula1Game& game, std::uint32_t nowMs) { game.startCrash(nowMs); }
    static bool crashed(const Formula1Game& game) { return game.crashed_; }
    static std::uint8_t crashesRemaining(const Formula1Game& game) { return game.crashesRemaining_; }
    static void finishScore(Formula1Game& game, std::uint32_t score) { game.score_ = score; game.finishScore(); }
};

struct EggJourneyGameTestAccess {
    static void reset(EggJourneyGame& game) { game.resetSession(); }
    static void update(EggJourneyGame& game, const hal::ControlSample& controls, std::uint32_t nowMs) { game.update(controls, nowMs); }
    static void render(EggJourneyGame& game) { game.render(); }
    static std::int32_t worldX(const EggJourneyGame& game) { return game.playerWorldX_; }
    static std::int32_t cameraX(const EggJourneyGame& game) { return game.cameraWorldX_; }
    static std::int16_t screenX(const EggJourneyGame& game) { return game.playerScreenX_; }
    static std::int16_t jumpHeight(const EggJourneyGame& game) { return game.jumpHeightFp_; }
    static bool gameOver(const EggJourneyGame& game) { return game.gameOver_; }
    static bool awaitingRetry(const EggJourneyGame& game) { return game.awaitingRetry_; }
    static std::uint8_t lives(const EggJourneyGame& game) { return game.lives_; }
    static void setWorldX(EggJourneyGame& game, std::int32_t worldX) {
        game.cameraWorldX_ = worldX - game.playerScreenX_ - 1;
        game.playerWorldX_ = worldX;
    }
    static std::int16_t craterDepth(const EggJourneyGame& game, std::int32_t worldX) { return game.craterDepthAt(worldX); }
    static void forceFailure(EggJourneyGame& game, bool flattened) { game.fail(flattened ? EggJourneyGame::FailureMode::Flattened : EggJourneyGame::FailureMode::Broken); }
    static void setScore(EggJourneyGame& game, std::uint32_t score) { game.score_ = score; game.finishScore(); }
    static void spawnAsteroid(EggJourneyGame& game, std::uint32_t nowMs) { game.spawnAsteroid(nowMs); }
    static bool asteroidActive(const EggJourneyGame& game) { return game.asteroid_.active; }
    static void targetPlayer(EggJourneyGame& game) { game.asteroid_.targetWorldX = game.playerWorldX_; game.asteroid_.y = 33; game.asteroid_.active = true; }
};

struct BeatknechtTestAccess {
    static void reset(Beatknecht& drummer, std::uint32_t nowMs) { drummer.resetSession(nowMs); }
    static void update(Beatknecht& drummer, const hal::ControlSample& controls, std::uint32_t nowMs) { drummer.update(controls, nowMs); }
    static void render(Beatknecht& drummer) { drummer.render(); }
    static std::uint16_t bpm(const Beatknecht& drummer) { return drummer.bpm_; }
    static std::uint8_t style(const Beatknecht& drummer) { return drummer.styleIndex_; }
    static std::uint8_t step(const Beatknecht& drummer) { return drummer.displayStep_; }
    static TransportState transport(const Beatknecht& drummer) { return drummer.transport_; }
};

struct BreakoutGameTestAccess {
    static void reset(BreakoutGame& game) { game.resetSession(); }
    static void update(BreakoutGame& game, const hal::ControlSample& controls, std::uint32_t nowMs) { game.update(controls, nowMs); }
    static void render(BreakoutGame& game) { game.render(); }
    static void substep(BreakoutGame& game, std::uint32_t nowMs, bool moveX, bool moveY) { game.updateBallSubstep(nowMs, moveX, moveY); }
    static void setBall(BreakoutGame& game, std::int16_t x, std::int16_t y, std::int8_t vx, std::int8_t vy, bool launched = true) { game.ballX_ = x; game.ballY_ = y; game.velocityX_ = vx; game.velocityY_ = vy; game.ballLaunched_ = launched; }
    static void setPaddleX(BreakoutGame& game, std::int16_t x) { game.paddleX_ = x; }
    static std::int16_t ballX(const BreakoutGame& game) { return game.ballX_; }
    static std::int16_t ballY(const BreakoutGame& game) { return game.ballY_; }
    static std::int8_t velocityX(const BreakoutGame& game) { return game.velocityX_; }
    static std::int8_t velocityY(const BreakoutGame& game) { return game.velocityY_; }
    static std::int16_t paddleWidth(const BreakoutGame& game) { return game.paddleWidth_; }
    static std::uint8_t lives(const BreakoutGame& game) { return game.lives_; }
    static std::uint8_t level(const BreakoutGame& game) { return game.level_; }
    static std::uint32_t score(const BreakoutGame& game) { return game.score_; }
    static bool launched(const BreakoutGame& game) { return game.ballLaunched_; }
    static bool gameOver(const BreakoutGame& game) { return game.gameOver_; }
    static void setGameOver(BreakoutGame& game, bool value) { game.gameOver_ = value; }
    static void setLives(BreakoutGame& game, std::uint8_t value) { game.lives_ = value; }
    static void setLevel(BreakoutGame& game, std::uint8_t value) { game.level_ = value; }
    static void clearBricks(BreakoutGame& game) { game.bricks_.fill(false); }
    static void setBrick(BreakoutGame& game, std::size_t index, bool value) { if (index < game.bricks_.size()) game.bricks_[index] = value; }
    static bool brick(const BreakoutGame& game, std::size_t index) { return index < game.bricks_.size() && game.bricks_[index]; }
    static void setRandomState(BreakoutGame& game, std::uint32_t value) { game.randomState_ = value; }
    static void maybeSpawn(BreakoutGame& game, std::int16_t x, std::int16_t y) { game.maybeSpawnExtra(x, y); }
    static void setFalling(BreakoutGame& game, std::uint8_t type, std::int16_t x, std::int16_t y, bool active) { game.fallingExtra_.type = static_cast<BreakoutGame::ExtraType>(type); game.fallingExtra_.x = x; game.fallingExtra_.y = y; game.fallingExtra_.active = active; }
    static bool fallingActive(const BreakoutGame& game) { return game.fallingExtra_.active; }
    static std::uint8_t fallingType(const BreakoutGame& game) { return static_cast<std::uint8_t>(game.fallingExtra_.type); }
    static void updateExtra(BreakoutGame& game, std::uint32_t nowMs) { game.updateExtra(nowMs); }
    static void setLastPhysicsAt(BreakoutGame& game, std::uint32_t nowMs) { game.lastPhysicsAtMs_ = nowMs; }
    static void applyLarge(BreakoutGame& game, std::uint32_t nowMs) { game.applyExtra(BreakoutGame::ExtraType::LargeBat, nowMs); }
    static void applySmall(BreakoutGame& game, std::uint32_t nowMs) { game.applyExtra(BreakoutGame::ExtraType::SmallBat, nowMs); }
    static void applyFast(BreakoutGame& game, std::uint32_t nowMs) { game.applyExtra(BreakoutGame::ExtraType::FastBat, nowMs); }
};
}

namespace clockfw::ui {
struct UiControllerTestAccess {
    static NavigationState& navigation(UiController& controller) { return controller.navigation_; }
    static const SequencerPatternV2* activePattern(const UiController& controller) {
        return controller.activeSequencerPattern();
    }
    static SequencerStepMetadata activeMetadata(const UiController& controller) {
        return controller.activeSequencerStepMetadata();
    }
    static void synchronizePattern(UiController& controller, const bool reschedule) {
        controller.synchronizeActiveSequencerPattern(reschedule);
    }
    static void synchronizeMetadata(UiController& controller) {
        controller.synchronizeActiveSequencerStepMetadata();
    }
    static void openStepEditor(UiController& controller) { controller.openSequencerStepEditor(); }
    static void adjustPattern(UiController& controller, const std::int8_t delta, const std::uint32_t nowMs) {
        controller.adjustSequencerV2(delta, nowMs);
    }
    static void adjustMetadata(UiController& controller, const std::int8_t delta, const std::uint32_t nowMs) {
        controller.adjustSequencerStepMetadata(delta, nowMs);
    }
    static void toggleStep(UiController& controller, const std::uint32_t nowMs) {
        controller.toggleSequencerStepV2(nowMs);
    }
    static bool command(UiController& controller, const std::uint8_t row, const std::uint32_t nowMs) {
        return controller.executeSequencerPatternCommandV2(row, nowMs);
    }
    static services::SequencerPatternStore*& patternStore(UiController& controller) {
        return controller.sequencerPatternStore_;
    }
    static services::SequencerStepStore*& stepStore(UiController& controller) {
        return controller.sequencerStepStore_;
    }
    static void clearSequencerClipboard(UiController& controller) {
        controller.sequencerPatternClipboardValid_ = false;
        controller.sequencerStepClipboardValid_ = false;
    }
};
}

void setUp() {}
void tearDown() {}

namespace {
int gChecks = 0;

#define CHECK(expr) do { ++gChecks; TEST_ASSERT_TRUE(expr); } while (0)
#define CHECK_EQ(a,b) CHECK((a) == (b))
#define CHECK_NE(a,b) CHECK((a) != (b))

using namespace clockfw;

void resetFakes() {
    fakefw::resetArduino();
    hal::PersistentStorage::resetForTest();
    Wire.reset();
    SPI.reset();
    HardwareTimer::lastInstance = nullptr;
    fakefw::setPin(pinmap::kEncoderPhaseAPin, HIGH);
    fakefw::setPin(pinmap::kEncoderPhaseBPin, HIGH);
    fakefw::setPin(pinmap::kEncoderPushButtonPin, HIGH);
    fakefw::setPin(pinmap::kPlayPauseButtonPin, HIGH);
    fakefw::setPin(pinmap::kTapTempoButtonPin, HIGH);
    fakefw::setPin(pinmap::kResetBackButtonPin, HIGH);
}

ClockState makeFactoryState() {
    ClockState state{};
    initializeFactoryDefaults(state);
    return state;
}

ClockState makeDefaultState() {
    ClockState state = makeFactoryState();
    // Most host tests exercise the per-channel scheduler independently of the
    // shipping UI default. Keep that fixture explicit while factory-default
    // tests use makeFactoryState().
    state.operatingMode = OperatingMode::Independent;
    state.source = ClockSource::Internal;
    return state;
}


std::uint32_t testCrc32(const std::uint8_t* const data, const std::size_t size) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t index = 0U; index < size; ++index) {
        crc ^= data[index];
        for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
            const bool set = (crc & 1U) != 0U;
            crc >>= 1U;
            if (set) crc ^= 0xEDB88320UL;
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

void writeTestUint16Le(std::uint8_t* const destination, const std::uint16_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

void writeTestUint32Le(std::uint8_t* const destination, const std::uint32_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    destination[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    destination[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}




template <std::size_t CurrentSize, std::size_t LegacySize, std::size_t PayloadOffset>
std::array<std::uint8_t, LegacySize> makeLegacyV11Record(
    const std::array<std::uint8_t, CurrentSize>& current) {
    constexpr std::size_t kV12PayloadSize = 294U;
    constexpr std::size_t kV11PayloadSize = 285U;
    static_assert(kV12PayloadSize == kV11PayloadSize + 9U);
    std::array<std::uint8_t, LegacySize> legacy{};
    if constexpr (PayloadOffset == 8U) {
        writeTestUint32Le(legacy.data(), 0x31315543UL);  // "CU11"
    } else {
        std::copy_n(current.begin() + 8, 16U, legacy.begin() + 8);
        writeTestUint32Le(legacy.data(), 0x31315250UL);  // "PR11"
    }
    legacy[4] = 11U;
    legacy[5] = current[5];
    writeTestUint16Le(legacy.data() + 6U, static_cast<std::uint16_t>(kV11PayloadSize));
    std::copy_n(
        current.begin() + static_cast<std::ptrdiff_t>(PayloadOffset),
        kV11PayloadSize,
        legacy.begin() + static_cast<std::ptrdiff_t>(PayloadOffset));
    writeTestUint32Le(
        legacy.data() + LegacySize - 4U,
        testCrc32(legacy.data(), LegacySize - 4U));
    return legacy;
}

template <std::size_t CurrentSize, std::size_t LegacySize, std::size_t PayloadOffset>
std::array<std::uint8_t, LegacySize> makeLegacyV10Record(
    const std::array<std::uint8_t, CurrentSize>& current) {
    constexpr std::size_t kV11PayloadSize = 285U;
    constexpr std::size_t kV10PayloadSize = 283U;
    static_assert(kV11PayloadSize == kV10PayloadSize + 2U);
    std::array<std::uint8_t, LegacySize> legacy{};
    if constexpr (PayloadOffset == 8U) {
        writeTestUint32Le(legacy.data(), 0x30315543UL);  // "CU10"
    } else {
        std::copy_n(current.begin() + 8, 16U, legacy.begin() + 8);
        writeTestUint32Le(legacy.data(), 0x30315250UL);  // "PR10"
    }
    legacy[4] = 10U;
    legacy[5] = current[5];
    writeTestUint16Le(legacy.data() + 6U, static_cast<std::uint16_t>(kV10PayloadSize));
    std::copy_n(current.begin() + static_cast<std::ptrdiff_t>(PayloadOffset),
                kV10PayloadSize,
                legacy.begin() + static_cast<std::ptrdiff_t>(PayloadOffset));
    writeTestUint32Le(legacy.data() + LegacySize - 4U, testCrc32(legacy.data(), LegacySize - 4U));
    return legacy;
}

template <std::size_t CurrentSize, std::size_t LegacySize, std::size_t PayloadOffset>
std::array<std::uint8_t, LegacySize> makeLegacyV9Record(
    const std::array<std::uint8_t, CurrentSize>& current) {
    constexpr std::size_t kV10PayloadSize = 283U;
    constexpr std::size_t kV9PayloadSize = 256U;
    static_assert(kV10PayloadSize == kV9PayloadSize + 27U);
    std::array<std::uint8_t, LegacySize> legacy{};
    if constexpr (PayloadOffset == 8U) {
        writeTestUint32Le(legacy.data(), 0x39525543UL);  // "CUR9"
    } else {
        std::copy_n(current.begin() + 8, 16U, legacy.begin() + 8);
        writeTestUint32Le(legacy.data(), 0x39455250UL);  // "PRE9"
    }
    legacy[4] = 9U;
    legacy[5] = current[5];
    writeTestUint16Le(legacy.data() + 6U, static_cast<std::uint16_t>(kV9PayloadSize));
    std::copy_n(current.begin() + static_cast<std::ptrdiff_t>(PayloadOffset),
                kV9PayloadSize,
                legacy.begin() + static_cast<std::ptrdiff_t>(PayloadOffset));
    writeTestUint32Le(legacy.data() + LegacySize - 4U, testCrc32(legacy.data(), LegacySize - 4U));
    return legacy;
}

template <std::size_t CurrentSize, std::size_t LegacySize, std::size_t PayloadOffset>
std::array<std::uint8_t, LegacySize> makeLegacyV8Record(
    const std::array<std::uint8_t, CurrentSize>& current) {
    constexpr std::size_t kV9PayloadSize = 256U;
    constexpr std::size_t kV8PayloadSize = 255U;
    static_assert(kV9PayloadSize == kV8PayloadSize + 1U);
    std::array<std::uint8_t, LegacySize> legacy{};

    if constexpr (PayloadOffset == 8U) {
        writeTestUint32Le(legacy.data(), 0x38525543UL);  // "CUR8"
    } else {
        std::copy_n(current.begin() + 8, 16U, legacy.begin() + 8);
        writeTestUint32Le(legacy.data(), 0x38455250UL);  // "PRE8"
    }
    legacy[4] = 8U;
    legacy[5] = current[5];
    writeTestUint16Le(legacy.data() + 6U, static_cast<std::uint16_t>(kV8PayloadSize));
    std::copy_n(current.begin() + static_cast<std::ptrdiff_t>(PayloadOffset),
                kV8PayloadSize,
                legacy.begin() + static_cast<std::ptrdiff_t>(PayloadOffset));
    writeTestUint32Le(legacy.data() + LegacySize - 4U, testCrc32(legacy.data(), LegacySize - 4U));
    return legacy;
}

template <std::size_t CurrentSize, std::size_t LegacySize, std::size_t PayloadOffset>
std::array<std::uint8_t, LegacySize> makeLegacyV7Record(
    const std::array<std::uint8_t, CurrentSize>& current) {
    constexpr std::size_t kV8PayloadSize = 255U;
    constexpr std::size_t kV7PayloadSize = 254U;
    static_assert(kV8PayloadSize == kV7PayloadSize + 1U);
    std::array<std::uint8_t, LegacySize> legacy{};

    if constexpr (PayloadOffset == 8U) {
        writeTestUint32Le(legacy.data(), 0x37525543UL);  // "CUR7"
    } else {
        std::copy_n(current.begin() + 8, 16U, legacy.begin() + 8);
        writeTestUint32Le(legacy.data(), 0x37455250UL);  // "PRE7"
    }
    legacy[4] = 7U;
    legacy[5] = current[5];
    writeTestUint16Le(legacy.data() + 6U, static_cast<std::uint16_t>(kV7PayloadSize));
    std::copy_n(current.begin() + static_cast<std::ptrdiff_t>(PayloadOffset),
                kV7PayloadSize,
                legacy.begin() + static_cast<std::ptrdiff_t>(PayloadOffset));
    writeTestUint32Le(legacy.data() + LegacySize - 4U, testCrc32(legacy.data(), LegacySize - 4U));
    return legacy;
}

template <std::size_t CurrentSize, std::size_t LegacySize, std::size_t PayloadOffset>
std::array<std::uint8_t, LegacySize> makeLegacyV6Record(
    const std::array<std::uint8_t, CurrentSize>& current) {
    constexpr std::size_t kV7PayloadSize = 254U;
    constexpr std::size_t kV6PayloadSize = 252U;
    static_assert(kV7PayloadSize == kV6PayloadSize + 2U);
    std::array<std::uint8_t, LegacySize> legacy{};

    if constexpr (PayloadOffset == 8U) {
        writeTestUint32Le(legacy.data(), 0x36525543UL);  // "CUR6"
    } else {
        std::copy_n(current.begin() + 8, 16U, legacy.begin() + 8);
        writeTestUint32Le(legacy.data(), 0x36455250UL);  // "PRE6"
    }
    legacy[4] = 6U;
    legacy[5] = current[5];
    writeTestUint16Le(legacy.data() + 6U, static_cast<std::uint16_t>(kV6PayloadSize));
    std::copy_n(current.begin() + static_cast<std::ptrdiff_t>(PayloadOffset),
                kV6PayloadSize,
                legacy.begin() + static_cast<std::ptrdiff_t>(PayloadOffset));
    writeTestUint32Le(legacy.data() + LegacySize - 4U, testCrc32(legacy.data(), LegacySize - 4U));
    return legacy;
}

template <std::size_t CurrentSize, std::size_t LegacySize, std::size_t PayloadOffset>
std::array<std::uint8_t, LegacySize> makeLegacyV5Record(
    const std::array<std::uint8_t, CurrentSize>& current) {
    constexpr std::size_t kV6PayloadSize = 252U;
    constexpr std::size_t kV5PayloadSize = 251U;
    constexpr std::size_t kResetModeOffsetV6 = 14U;
    std::array<std::uint8_t, LegacySize> legacy{};
    const std::uint8_t* const sourcePayload = current.data() + PayloadOffset;
    std::uint8_t* const destinationPayload = legacy.data() + PayloadOffset;

    std::copy_n(sourcePayload, kResetModeOffsetV6, destinationPayload);
    std::copy(
        sourcePayload + static_cast<std::ptrdiff_t>(kResetModeOffsetV6 + 1U),
        sourcePayload + static_cast<std::ptrdiff_t>(kV6PayloadSize),
        destinationPayload + static_cast<std::ptrdiff_t>(kResetModeOffsetV6));

    if constexpr (PayloadOffset == 8U) {
        writeTestUint32Le(legacy.data(), 0x35525543UL);  // "CUR5"
    } else {
        std::copy_n(current.begin() + 8, 16U, legacy.begin() + 8);
        writeTestUint32Le(legacy.data(), 0x35455250UL);  // "PRE5"
    }
    legacy[4] = 5U;
    legacy[5] = current[5];
    writeTestUint16Le(legacy.data() + 6U, static_cast<std::uint16_t>(kV5PayloadSize));
    writeTestUint32Le(legacy.data() + LegacySize - 4U, testCrc32(legacy.data(), LegacySize - 4U));
    return legacy;
}

template <std::size_t CurrentSize, std::size_t LegacySize, std::size_t PayloadOffset>
std::array<std::uint8_t, LegacySize> makeLegacyV4Record(
    const std::array<std::uint8_t, CurrentSize>& current) {
    constexpr std::size_t kV5PayloadSize = 251U;
    constexpr std::size_t kV4PayloadSize = 245U;
    constexpr std::size_t kTempoRangeOffsetV5 = 2U;
    constexpr std::size_t kAfterTempoRangeOffsetV5 = 6U;
    constexpr std::size_t kHumanizeOffsetV5 = 26U;
    constexpr std::size_t kAfterHumanizeOffsetV5 = 28U;
    std::array<std::uint8_t, LegacySize> legacy{};
    const std::uint8_t* const sourcePayload = current.data() + PayloadOffset;
    std::uint8_t* const destinationPayload = legacy.data() + PayloadOffset;

    std::copy_n(sourcePayload, kTempoRangeOffsetV5, destinationPayload);
    std::copy(
        sourcePayload + kAfterTempoRangeOffsetV5,
        sourcePayload + kHumanizeOffsetV5,
        destinationPayload + kTempoRangeOffsetV5);
    std::copy(
        sourcePayload + kAfterHumanizeOffsetV5,
        sourcePayload + kV5PayloadSize,
        destinationPayload + 22U);

    if constexpr (PayloadOffset == 8U) {
        writeTestUint32Le(legacy.data(), 0x34525543UL);  // "CUR4"
    } else {
        std::copy_n(current.begin() + 8, 16U, legacy.begin() + 8);
        writeTestUint32Le(legacy.data(), 0x34455250UL);  // "PRE4"
    }
    legacy[4] = 4U;
    legacy[5] = current[5];
    writeTestUint16Le(legacy.data() + 6U, static_cast<std::uint16_t>(kV4PayloadSize));
    writeTestUint32Le(legacy.data() + LegacySize - 4U, testCrc32(legacy.data(), LegacySize - 4U));
    return legacy;
}

template <std::size_t CurrentSize, std::size_t LegacySize, std::size_t PayloadOffset>
std::array<std::uint8_t, LegacySize> makeLegacyV3Record(
    const std::array<std::uint8_t, CurrentSize>& current) {
    constexpr std::size_t kDisplayOffsetWithinPayload = 25U;
    constexpr std::size_t kDisplayFieldCount = 4U;
    constexpr std::size_t kLegacyPayloadSize = 241U;
    std::array<std::uint8_t, LegacySize> legacy{};
    std::copy_n(current.begin(), PayloadOffset + kDisplayOffsetWithinPayload, legacy.begin());
    std::copy(
        current.begin() + static_cast<std::ptrdiff_t>(PayloadOffset + kDisplayOffsetWithinPayload + kDisplayFieldCount),
        current.end() - 4,
        legacy.begin() + static_cast<std::ptrdiff_t>(PayloadOffset + kDisplayOffsetWithinPayload));
    if constexpr (PayloadOffset == 8U) {
        writeTestUint32Le(legacy.data(), 0x33525543UL);  // "CUR3"
    } else {
        writeTestUint32Le(legacy.data(), 0x33455250UL);  // "PRE3"
    }
    legacy[4] = 3U;
    writeTestUint16Le(legacy.data() + 6U, static_cast<std::uint16_t>(kLegacyPayloadSize));
    writeTestUint32Le(legacy.data() + LegacySize - 4U, testCrc32(legacy.data(), LegacySize - 4U));
    return legacy;
}

void prepareDisplaySuccess() {
#if CLOCK_DISPLAY_USE_SPI
    (void)0;
#else
#if CLOCK_DISPLAY_I2C_ADDRESS
    Wire.ack[static_cast<std::uint8_t>(CLOCK_DISPLAY_I2C_ADDRESS)] = 0U;
#else
    Wire.ack[0x3CU] = 0U;
#endif
#endif
}

hal::ButtonSample pressedEdge() { return {hal::ButtonEdge::Pressed, true}; }
hal::ButtonSample releasedEdge() { return {hal::ButtonEdge::Released, false}; }
hal::ButtonSample heldButton() { return {hal::ButtonEdge::None, true}; }

#include "cases/fundamentals_persistence.inc"
#include "cases/engine_hal.inc"
#include "cases/rendering_ui.inc"
#include "cases/ui_games.inc"
#include "cases/groove_application.inc"

} // namespace

int main() {
    UNITY_BEGIN();
    RUN_TEST(testLocalizationAndFont);
    RUN_TEST(testDefaultsTemplatesAndServices);
    RUN_TEST(testPersistentLayoutV1ForwardCompatibilityContract);
    RUN_TEST(testCustomGrooveStorePreservesRegionsAndRejectsCorruption);
    RUN_TEST(testCustomGrooveMutationsDeferPhysicalFlashCommit);
    RUN_TEST(testPersistentStorageTransactionalUpdate);
    RUN_TEST(testPersistentStorageAndStateService);
    RUN_TEST(testMultipleDeferredPresetMutationsSurviveUntilCommit);
    RUN_TEST(testPersistentV3Migration);
    RUN_TEST(testPersistentStateValidationBoundaries);
    RUN_TEST(testMenuModelAndFormatters);
    RUN_TEST(testHalBasicsAndDisplay);
    RUN_TEST(testControlPanel);
    RUN_TEST(testEngineAndSettingsEditor);
    RUN_TEST(testOperatingModesPersistenceAndGlobalEditors);
    RUN_TEST(testChannelRescheduleKeepsSharedMusicalEpoch);
    RUN_TEST(testCrossModeSynchronizationRegressions);
    RUN_TEST(testOneClockHumanizeAndTempoLimits);
    RUN_TEST(testEngineAuditRegressions);
    RUN_TEST(testEngineBoundaryBranches);
    RUN_TEST(testRenderEveryScreenAndState);
    RUN_TEST(testHierarchicalSettingsRenderDistinctRootAndGroupPages);
    RUN_TEST(testSettingsSelectionUsesFullRowInversionWithoutLeftCursor);
    RUN_TEST(testReadOnlyInformationOverflowUsesPopoverWithoutEditingSemantics);
    RUN_TEST(testPerformanceRendererShowsActiveGrooveAtModeSpecificPosition);
    RUN_TEST(testPerformanceRendererShowsStaticPreCountPopoverAndMeterProgress);
    RUN_TEST(testPreCountPopoverRedrawsAtBeatBoundariesAndClearsOnCompletion);
    RUN_TEST(testPerformanceRendererShowsMeasuredExternalBpmWhileLockedAndFreewheeling);
    RUN_TEST(testTapTempoPreservesClockSourceAndPersistence);
    RUN_TEST(testTapIndicatorStartsOnSecondTapAndRestartsEveryFollowingTap);
    RUN_TEST(testTapIndicatorRendersFourShrinkingEightPixelFramesThenClears);
    RUN_TEST(testTapIndicatorSequenceResetsAfterConfiguredMinimumBpmInterval);
    RUN_TEST(testTapIndicatorIsExclusiveToPerformanceTapTempo);
    RUN_TEST(testScreensaverRenderingAndPolicy);
    RUN_TEST(testNestedGroupNavigationCoverage);
    RUN_TEST(testCustomGrooveEditorControllerWorkflow);
    RUN_TEST(testCustomGrooveRecorderControllerWorkflow);
    RUN_TEST(testUiControllerFlows);
    RUN_TEST(testHighScoreResetAppearsAfterStartAndClearsSafely);
    RUN_TEST(testEasterEggGameAndHighScore);
    RUN_TEST(testExternalTapTimestampTranslationSurvivesMicrosecondWrap);
    RUN_TEST(testApplicationAndEntryPoints);
    std::cout << "Host firmware assertions: " << gChecks << "\n";
    return UNITY_END();
}
