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
#include "game/moon_buggy_game.h"
#include "game/pixel_raid_game.h"
#include "game/beatknecht.h"
#include "hal/control_panel.h"
#include "hal/display_font.h"
#include "hal/gate_output_driver.h"
#include "hal/interrupt_lock.h"
#include "hal/oled_display.h"
#include "hal/periodic_timer.h"
#include "hal/persistent_storage.h"
#include "hal/system_clock.h"
#include "pin_map.h"
#include "services/persistent_state_service.h"
#include "services/persistent_state_validation.h"
#include "services/tap_tempo.h"
#include "services/template_service.h"
#include "ui/menu_model.h"
#include "ui/menu_model_sync.h"
#include "ui/mode_functions.h"
#include "ui/pattern_strip_renderer.h"
#include "ui/preset_name_alphabet.h"
#include "ui/settings_editor.h"
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

struct MoonBuggyGameTestAccess {
    static void reset(MoonBuggyGame& game) { game.resetSession(); }
    static void update(MoonBuggyGame& game, const hal::ControlSample& controls, std::uint32_t nowMs) { game.update(controls, nowMs); }
    static void render(MoonBuggyGame& game) { game.render(); }
    static std::int32_t worldX(const MoonBuggyGame& game) { return game.playerWorldX_; }
    static std::int32_t cameraX(const MoonBuggyGame& game) { return game.cameraWorldX_; }
    static std::int16_t screenX(const MoonBuggyGame& game) { return game.playerScreenX_; }
    static std::int16_t jumpHeight(const MoonBuggyGame& game) { return game.jumpHeightFp_; }
    static bool gameOver(const MoonBuggyGame& game) { return game.gameOver_; }
    static bool awaitingRetry(const MoonBuggyGame& game) { return game.awaitingRetry_; }
    static std::uint8_t lives(const MoonBuggyGame& game) { return game.lives_; }
    static void setWorldX(MoonBuggyGame& game, std::int32_t worldX) {
        game.cameraWorldX_ = worldX - game.playerScreenX_ - 1;
        game.playerWorldX_ = worldX;
    }
    static std::int16_t craterDepth(const MoonBuggyGame& game, std::int32_t worldX) { return game.craterDepthAt(worldX); }
    static void forceFailure(MoonBuggyGame& game, bool flattened) { game.fail(flattened ? MoonBuggyGame::FailureMode::Flattened : MoonBuggyGame::FailureMode::Broken); }
    static void setScore(MoonBuggyGame& game, std::uint32_t score) { game.score_ = score; game.finishScore(); }
    static void spawnAsteroid(MoonBuggyGame& game, std::uint32_t nowMs) { game.spawnAsteroid(nowMs); }
    static bool asteroidActive(const MoonBuggyGame& game) { return game.asteroid_.active; }
    static void targetPlayer(MoonBuggyGame& game) { game.asteroid_.targetWorldX = game.playerWorldX_; game.asteroid_.y = 33; game.asteroid_.active = true; }
};

struct BeatknechtTestAccess {
    static void reset(Beatknecht& drummer, std::uint32_t nowMs) { drummer.resetSession(nowMs); }
    static void update(Beatknecht& drummer, const hal::ControlSample& controls, std::uint32_t nowMs) { drummer.update(controls, nowMs); }
    static void render(Beatknecht& drummer) { drummer.render(); }
    static std::uint16_t bpm(const Beatknecht& drummer) { return drummer.bpm_; }
    static std::uint8_t style(const Beatknecht& drummer) { return drummer.styleIndex_; }
    static std::uint8_t step(const Beatknecht& drummer) { return drummer.displayStep_; }
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

namespace {
int gFailures = 0;
int gChecks = 0;

#define CHECK(expr) do { ++gChecks; if (!(expr)) { ++gFailures; std::cerr << __FILE__ << ':' << __LINE__ << " CHECK failed: " #expr "\n"; } } while (0)
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

void testLocalizationAndFont() {
    for (std::size_t index = 0U; index < text::kTextCount; ++index) {
        const auto id = static_cast<text::TextId>(index);
        CHECK(text::get(id, config::UiLanguage::EnglishUs) != nullptr);
        CHECK(text::get(id, config::UiLanguage::GermanDe) != nullptr);
    }
    CHECK(&text::catalogFor(static_cast<config::UiLanguage>(99)) == &text::kEnglishUs);
    CHECK(std::strcmp(channelModeShortLabel(ChannelMode::Clock), "C") == 0);
    CHECK(std::strcmp(channelModeShortLabel(ChannelMode::Euclid), "E") == 0);
    CHECK(std::strcmp(channelModeShortLabel(ChannelMode::Sequencer), "S") == 0);
    CHECK(std::strcmp(channelModeShortLabel(ChannelMode::Off), "O") == 0);
    CHECK(std::strcmp(channelModeShortLabel(static_cast<ChannelMode>(99)), "C") == 0);
    CHECK(std::strcmp(channelModeLongLabel(ChannelMode::Clock), "CLOCK") == 0);
    CHECK(std::strcmp(channelModeLongLabel(ChannelMode::Euclid), "EUCLID") == 0);
    CHECK(std::strcmp(channelModeLongLabel(ChannelMode::Sequencer), "SEQUENCER") == 0);
    CHECK(std::strcmp(channelModeLongLabel(ChannelMode::Off), "OFF") == 0);
    CHECK(std::strcmp(clockSourceLabel(ClockSource::Internal), "INT") == 0);
    CHECK(std::strcmp(clockSourceLabel(ClockSource::External), "EXT") == 0);
    CHECK(std::strcmp(clockSourceLabel(ClockSource::Auto), "AUTO") == 0);
    CHECK(std::strcmp(clockSourceLabel(static_cast<ClockSource>(99)), "INT") == 0);

    for (char c = 'A'; c <= 'Z'; ++c) CHECK(hal::font::glyphFor(c) != hal::font::kBlank);
    for (char c = '0'; c <= '9'; ++c) CHECK(hal::font::glyphFor(c) != hal::font::kBlank);
    CHECK(hal::font::glyphFor('a') == hal::font::glyphFor('A'));
    for (char c : std::string("/:%-+><.=_|*!?")) CHECK(hal::font::glyphFor(c) != hal::font::kBlank);
    CHECK(hal::font::glyphFor(' ') == hal::font::kBlank);
    CHECK(hal::font::glyphFor('@') == hal::font::kBlank);

    // The tempo font is a dedicated native-resolution numeral set, not scaled 5x7 text.
    for (char digit = '0'; digit <= '9'; ++digit) {
        const hal::font::TempoGlyph glyph = hal::font::tempoGlyphFor(digit);
        CHECK(glyph.width > 0U);
        CHECK(glyph.width <= hal::font::kTempoGlyphMaximumWidth);
        CHECK(std::any_of(glyph.rows.begin(), glyph.rows.end(), [](std::uint16_t row) { return row != 0U; }));
    }
    CHECK_EQ(hal::font::tempoGlyphFor('X').width, 0U);
}

void testDefaultsTemplatesAndServices() {
    ClockState state{};
    initializeFactoryDefaults(state);
    CHECK_EQ(state.bpm, defaults::kMasterBpm);
    CHECK_EQ(state.operatingMode, OperatingMode::UnifiedClock);
    CHECK_EQ(state.tempoRange.minimumBpm, 20U);
    CHECK_EQ(state.tempoRange.maximumBpm, 999U);
    CHECK_EQ(state.unifiedClock.humanizeUs, 0U);
    CHECK_EQ(state.masterMeter.beats, defaults::kMasterMeter.beats);
    CHECK_EQ(state.channels[0].common.mode, ChannelMode::Clock);
    CHECK_EQ(state.channels[0].common.gateLengthMs, 10U);
    CHECK_EQ(state.unifiedClock.gateLengthMs, 10U);
    CHECK_EQ(state.dividerBank.gateLengthMs, 10U);
    CHECK_EQ(kGateLengthOptionsMs, (std::array<std::uint16_t, 7U>{1U, 2U, 5U, 10U, 20U, 50U, 100U}));
    CHECK_EQ(state.channels[0].sequencer.pattern, defaults::kSequencerPatternA);
    CHECK_EQ(state.channels[1].sequencer.pattern, defaults::kSequencerPatternB);
    CHECK_EQ(state.channels[7].euclid.hits, static_cast<std::uint8_t>(defaults::kEuclidBaseHits + 3U));
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Clock);
    CHECK_EQ(state.display.screensaverAfterMinutes, 2U);
    CHECK_EQ(state.display.dimAfterMinutes, 5U);
    CHECK_EQ(state.display.offAfterMinutes, 10U);

    for (std::size_t index = 0U; index < services::TemplateService::kTemplateCount; ++index) {
        ClockState templated{};
        CHECK(services::TemplateService::apply(index, templated));
        CHECK(std::strlen(services::TemplateService::name(index)) > 0U);
        CHECK_EQ(
            templated.operatingMode,
            index == 0U ? OperatingMode::UnifiedClock : OperatingMode::Independent);
        if (index == 1U) CHECK_EQ(templated.channels[1].common.rate.factor, 2U);
        if (index == 2U) CHECK_EQ(templated.channels[7].common.rate.factor, 16U);
        if (index == 3U) CHECK_EQ(templated.channels[6].common.rate.numerator, 5U);
        if (index == 4U) CHECK_EQ(templated.channels[0].common.mode, ChannelMode::Euclid);
        if (index == 5U) CHECK_EQ(templated.channels[7].common.mode, ChannelMode::Sequencer);
    }
    ClockState playingTemplate = makeDefaultState();
    playingTemplate.transport = TransportState::Playing;
    CHECK(services::TemplateService::apply(1U, playingTemplate));
    CHECK_EQ(playingTemplate.transport, TransportState::Playing);

    CHECK(!services::TemplateService::apply(99U, state));
    CHECK(std::strcmp(services::TemplateService::name(99U), "OFF") == 0);

    services::TapTempo tap;
    tap.reset();
    CHECK_EQ(tap.registerTap(1000U, 20U, 300U), 0U);
    CHECK_EQ(tap.registerTap(1500U, 20U, 300U), 120U);
    CHECK_EQ(tap.registerTap(2000U, 20U, 300U), 120U);
    CHECK_EQ(tap.registerTap(2500U, 20U, 300U), 120U);
    CHECK_EQ(tap.registerTap(3000U, 20U, 300U), 120U);
    CHECK_EQ(tap.registerTap(3500U, 20U, 300U), 120U); // rolls the four-sample window
    CHECK_EQ(tap.registerTap(3550U, 20U, 300U), 0U);   // 50 ms: below supported ceiling
    tap.reset();
    CHECK_EQ(tap.registerTap(1000U, 1U, 300U), 0U);
    CHECK_EQ(tap.registerTap(61000U, 1U, 300U), 1U);   // 60 s interval = 1 BPM
    CHECK_EQ(tap.registerTap(130000U, 1U, 300U), 0U); // sequence timeout/reset
    CHECK_EQ(tap.registerTap(130500U, 130U, 140U), 130U); // clamp low
    CHECK_EQ(tap.registerTap(131000U, 20U, 100U), 100U);  // clamp high
    tap.reset();
    CHECK_EQ(tap.registerTap(1000U, 20U, 999U), 0U);
    CHECK_EQ(tap.registerTap(1060U, 20U, 999U), 999U);    // 60 ms ~= 1000 BPM, user max clamps to 999
}


void testPersistentStorageAndStateService() {
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    std::uint8_t byte = 0U;
    const std::uint8_t value = 0x42U;

    CHECK(!storage.readBytes(0U, nullptr, 1U));
    CHECK(!storage.readBytes(hal::PersistentStorage::kCapacityBytes, &byte, 1U));
    CHECK(!storage.readBytes(hal::PersistentStorage::kCapacityBytes + 1U, &byte, 0U));
    CHECK(!storage.writeBytes(0U, nullptr, 1U));
    CHECK(!storage.writeBytes(hal::PersistentStorage::kCapacityBytes, &value, 1U));
    CHECK(!storage.writeBytes(hal::PersistentStorage::kCapacityBytes + 1U, &value, 0U));
    CHECK(storage.writeBytes(0U, &value, 1U));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);
    CHECK(storage.readBytes(0U, &byte, 1U));
    CHECK_EQ(byte, value);
    CHECK(storage.writeBytes(0U, &value, 1U));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U); // unchanged data: no wear
    CHECK_EQ(hal::PersistentStorage::kPhysicalSlotBytes, 16U * 1024U);
    CHECK_EQ(hal::PersistentStorage::kMaximumImageBytes, 12U * 1024U);

    // A/B commit safety: an interrupted write must leave the previous generation readable.
    const std::uint8_t newerValue = 0x43U;
    hal::PersistentStorage::powerLossBeforeCommitForTest();
    CHECK(!storage.writeBytes(0U, &newerValue, 1U));
    byte = 0U;
    CHECK(storage.readBytes(0U, &byte, 1U));
    CHECK_EQ(byte, value);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    // After a successful newer generation, corruption of that slot must fall back
    // to the previous independently committed sector.
    CHECK(storage.writeBytes(0U, &newerValue, 1U));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 2U);
    hal::PersistentStorage::corruptNewestPayloadByteForTest(0U);
    byte = 0U;
    CHECK(storage.readBytes(0U, &byte, 1U));
    CHECK_EQ(byte, value);

    // Exercise the mirror fallback as well: after two more alternating commits,
    // slot A is newest. Corrupting it must recover from valid slot B.
    const std::uint8_t thirdValue = 0x44U;
    const std::uint8_t fourthValue = 0x45U;
    CHECK(storage.writeBytes(0U, &thirdValue, 1U));
    CHECK(storage.writeBytes(0U, &fourthValue, 1U));
    hal::PersistentStorage::corruptNewestPayloadByteForTest(0U);
    byte = 0U;
    CHECK(storage.readBytes(0U, &byte, 1U));
    CHECK_EQ(byte, thirdValue);
    hal::PersistentStorage::corruptNewestPayloadByteForTest(hal::PersistentStorage::kCapacityBytes);
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage::corruptNewestPayloadByteForTest(0U); // no valid slot: no-op

    // Validator short-circuit branches must independently reject malformed slot metadata.
    // A single committed slot is used each time so rejection is observable as erased storage.
    const auto expectInvalidHeaderRejected = [&](const std::size_t wordIndex, const std::uint32_t replacement) {
        hal::PersistentStorage::resetForTest();
        CHECK(storage.writeBytes(0U, &value, 1U));
        hal::PersistentStorage::corruptNewestHeaderWordForTest(wordIndex, replacement);
        std::uint8_t rejected = 0U;
        CHECK(storage.readBytes(0U, &rejected, 1U));
        CHECK_EQ(rejected, 0xFFU);
    };
    expectInvalidHeaderRejected(1U, 2U); // format version
    expectInvalidHeaderRejected(3U, 0U); // payload size
    expectInvalidHeaderRejected(7U, 0U); // commit marker
    expectInvalidHeaderRejected(5U, 0U); // header CRC
    hal::PersistentStorage::corruptNewestHeaderWordForTest(8U, 0U); // bounds guard
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage::corruptNewestHeaderWordForTest(0U, 0U); // no active slot

    // Backward migration: a committed alpha.54-era 4-KiB image must remain readable,
    // bytes in the new upper half read erased, and the first new write must promote
    // the whole image to 8 KiB without losing the legacy payload.
    std::array<std::uint8_t, 4096U> legacyImage{};
    legacyImage.fill(0xFFU);
    legacyImage[123U] = 0x5AU;
    CHECK(hal::PersistentStorage::seedLegacyImageForTest(legacyImage.data(), legacyImage.size()));
    std::uint8_t legacyByte = 0U;
    CHECK(storage.readBytes(123U, &legacyByte, 1U));
    CHECK_EQ(legacyByte, 0x5AU);
    std::uint8_t newHalfByte = 0U;
    CHECK(storage.readBytes(5000U, &newHalfByte, 1U));
    CHECK_EQ(newHalfByte, 0xFFU);
    const std::uint8_t promotedValue = 0x6BU;
    CHECK(storage.writeBytes(5000U, &promotedValue, 1U));
    legacyByte = 0U;
    newHalfByte = 0U;
    CHECK(storage.readBytes(123U, &legacyByte, 1U));
    CHECK(storage.readBytes(5000U, &newHalfByte, 1U));
    CHECK_EQ(legacyByte, 0x5AU);
    CHECK_EQ(newHalfByte, promotedValue);

    services::PersistentStateService service(storage);
    hal::PersistentStorage::resetForTest();
    service.begin();
    CHECK(!service.hasStoredTransportState());
    CHECK_EQ(service.storedTransportState(), TransportState::Stopped);

    hal::PersistentStorage::failNextReadForTest();
    service.begin();
    CHECK(!service.hasStoredTransportState());

    service.requestTransportState(TransportState::Playing, 100U);
    // Repeating the same still-pending value must keep the original coalescing window.
    service.requestTransportState(TransportState::Playing, 101U);
    service.service(100U + config::kPersistenceCommitDelayMs - 1U);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);
    hal::PersistentStorage::failNextWriteForTest();
    service.service(100U + config::kPersistenceCommitDelayMs);
    CHECK(!service.hasStoredTransportState());
    service.service(101U + config::kPersistenceCommitDelayMs);
    CHECK(service.hasStoredTransportState());
    CHECK_EQ(service.storedTransportState(), TransportState::Playing);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    // Requesting the already durable value must not schedule another Flash erase.
    service.requestTransportState(TransportState::Playing, 5000U);
    service.service(5000U + config::kPersistenceCommitDelayMs + 1U);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    // A pending change can be cancelled by returning to the already stored state.
    service.requestTransportState(TransportState::Paused, 6000U);
    service.requestTransportState(TransportState::Playing, 6001U);
    service.service(6001U + config::kPersistenceCommitDelayMs + 1U);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    // Changing the pending value restarts the stabilization window.
    service.requestTransportState(TransportState::Paused, 7000U);
    service.requestTransportState(TransportState::Stopped, 7100U);
    service.service(7000U + config::kPersistenceCommitDelayMs);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);
    service.service(7100U + config::kPersistenceCommitDelayMs);
    CHECK_EQ(service.storedTransportState(), TransportState::Stopped);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 2U);

    // Load the serialized record through a fresh service instance.
    services::PersistentStateService reloaded(storage);
    reloaded.begin();
    CHECK(reloaded.hasStoredTransportState());
    CHECK_EQ(reloaded.storedTransportState(), TransportState::Stopped);

    constexpr std::size_t kTransportRecordSize = 12U;
    std::array<std::uint8_t, kTransportRecordSize> validRecord{};
    CHECK(storage.readBytes(0U, validRecord.data(), validRecord.size()));

    auto expectCorruptRecordRejected = [&](std::size_t byteIndex, std::uint8_t replacement) {
        auto corruptRecord = validRecord;
        corruptRecord[byteIndex] = replacement;
        CHECK(storage.writeBytes(0U, corruptRecord.data(), corruptRecord.size()));
        services::PersistentStateService corrupted(storage);
        corrupted.begin();
        CHECK(!corrupted.hasStoredTransportState());
        CHECK_EQ(corrupted.storedTransportState(), TransportState::Stopped);
        CHECK(storage.writeBytes(0U, validRecord.data(), validRecord.size()));
    };

    expectCorruptRecordRejected(0U, static_cast<std::uint8_t>(validRecord[0] ^ 0x01U)); // magic
    expectCorruptRecordRejected(4U, 0x7FU); // schema
    expectCorruptRecordRejected(5U, 0xFFU); // transport enum
    expectCorruptRecordRejected(8U, static_cast<std::uint8_t>(validRecord[8] ^ 0x80U)); // CRC

    // The alpha.6 schema persists the complete musical configuration, not only transport.
    ClockState complete = makeDefaultState();
    complete.bpm = 137U;
    complete.masterMeter = {7U, 8U};
    complete.transport = TransportState::Playing;
    complete.source = ClockSource::Auto;
    complete.externalSync = {24U, SyncEdge::Falling, SyncLossMode::Internal, ExternalResetMode::Gate, 750U, 2200U};
    complete.display = {ScreensaverMode::Orbit, 3U, 7U, 12U};
    for (std::uint8_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        ChannelConfig& channel = complete.channels[channelIndex];
        channel.common.mode = static_cast<ChannelMode>(channelIndex % 4U);
        channel.common.rate = {ClockRatioMode::Multiply, static_cast<std::uint8_t>(channelIndex + 1U), 3U, 2U};
        channel.common.swingPercent = static_cast<std::uint8_t>(channelIndex * 3U);
        channel.common.probabilityPercent = static_cast<std::uint8_t>(100U - channelIndex);
        channel.common.gateLengthMs = static_cast<std::uint16_t>(5U + channelIndex);
        channel.common.phasePercent = static_cast<std::uint8_t>(channelIndex * 7U);
        channel.common.resetMode = (channelIndex & 1U) != 0U ? ResetMode::Free : ResetMode::Global;
        channel.common.muted = channelIndex == 7U;
        channel.euclid = {static_cast<std::uint8_t>(13U + channelIndex), static_cast<std::uint8_t>(5U + channelIndex), 2U};
        channel.sequencer.length = static_cast<std::uint8_t>(17U + channelIndex);
        channel.sequencer.rotation = channelIndex;
        channel.sequencer.pattern = 0xA55A000000000001ULL ^ channelIndex;
    }
    service.requestTransportState(TransportState::Playing, 10000U);
    service.requestCurrentState(complete, 10000U);
    service.service(10000U + config::kPersistenceCommitDelayMs);

    services::PersistentStateService fullReload(storage);
    fullReload.begin();
    ClockState restored{};
    CHECK(fullReload.restoreCurrentState(restored));
    CHECK_EQ(restored.bpm, 137U);
    CHECK_EQ(restored.masterMeter.beats, 7U);
    CHECK_EQ(restored.source, ClockSource::Auto);
    CHECK_EQ(restored.externalSync.pulsesPerQuarterNote, 24U);
    CHECK_EQ(restored.display.screensaverMode, ScreensaverMode::Orbit);
    CHECK_EQ(restored.display.screensaverAfterMinutes, 3U);
    CHECK_EQ(restored.display.dimAfterMinutes, 7U);
    CHECK_EQ(restored.display.offAfterMinutes, 12U);
    CHECK_EQ(restored.channels[6].common.mode, ChannelMode::Sequencer);
    CHECK_EQ(restored.channels[5].euclid.steps, 18U);
    CHECK_EQ(restored.channels[7].sequencer.length, 24U);
    CHECK_EQ(restored.channels[7].sequencer.pattern, 0xA55A000000000006ULL);

    ClockState invalidDisplay = complete;
    invalidDisplay.display.screensaverMode = static_cast<ScreensaverMode>(0U);
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));
    invalidDisplay = complete;
    invalidDisplay.display.screensaverAfterMinutes = 0U;
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));
    invalidDisplay = complete;
    invalidDisplay.display.screensaverAfterMinutes = 8U;
    invalidDisplay.display.dimAfterMinutes = 7U;
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));
    invalidDisplay = complete;
    invalidDisplay.display.dimAfterMinutes = 13U;
    invalidDisplay.display.offAfterMinutes = 12U;
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));
    invalidDisplay = complete;
    invalidDisplay.display.offAfterMinutes = static_cast<std::uint8_t>(config::kMaximumScreensaverMinutes + 1U);
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));

    // Public preset API boundaries and null-output guards.
    CHECK(!fullReload.savePreset(services::PersistentStateService::kUserPresetSlotCount, "OOB", complete));
    CHECK(!fullReload.savePreset(1U, nullptr, complete));
    ClockState boundaryLoad = makeDefaultState();
    CHECK(!fullReload.loadPreset(services::PersistentStateService::kUserPresetSlotCount, boundaryLoad));
    CHECK(!fullReload.renamePreset(services::PersistentStateService::kUserPresetSlotCount, "OOB"));
    CHECK(!fullReload.renamePreset(1U, "EMPTY"));
    CHECK(!fullReload.clearPreset(services::PersistentStateService::kUserPresetSlotCount));
    CHECK(!fullReload.presetExists(services::PersistentStateService::kUserPresetSlotCount));
    fullReload.presetName(0U, nullptr, 17U);
    char zeroSizeName[1]{'X'};
    fullReload.presetName(0U, zeroSizeName, 0U);
    CHECK_EQ(zeroSizeName[0], 'X');

    CHECK(fullReload.savePreset(0U, "groove_1", complete));
    CHECK(fullReload.presetExists(0U));
    char presetName[17]{}; fullReload.presetName(0U, presetName, sizeof(presetName));
    CHECK(std::strcmp(presetName, "GROOVE_1") == 0);
    ClockState loadedPreset = makeDefaultState(); loadedPreset.transport = TransportState::Paused;
    CHECK(fullReload.loadPreset(0U, loadedPreset));
    CHECK_EQ(loadedPreset.bpm, 137U);
    CHECK_EQ(loadedPreset.transport, TransportState::Paused);
    CHECK_EQ(loadedPreset.channels[7].sequencer.pattern, complete.channels[7].sequencer.pattern);
    CHECK(!fullReload.renamePreset(0U, nullptr));
    CHECK(fullReload.renamePreset(0U, "ALT-01"));
    fullReload.presetName(0U, presetName, sizeof(presetName));
    CHECK(std::strcmp(presetName, "ALT-01") == 0);
    CHECK(fullReload.clearPreset(0U));
    CHECK(!fullReload.presetExists(0U));
    CHECK(!fullReload.loadPreset(0U, loadedPreset));

    // Pending and durable preset-name paths both trim right-padding. The durable
    // record also gives us a target for read-failure and corrupt-record tests.
    CHECK(fullReload.savePreset(2U, "PAD   ", complete));
    fullReload.presetName(2U, presetName, sizeof(presetName));
    CHECK(std::strcmp(presetName, "PAD") == 0);
    fullReload.service(30000U, true);
    fullReload.presetName(2U, presetName, sizeof(presetName));
    CHECK(std::strcmp(presetName, "PAD") == 0);
    hal::PersistentStorage::failNextReadForTest();
    CHECK(!fullReload.loadPreset(2U, loadedPreset));
    hal::PersistentStorage::failNextReadForTest();
    CHECK(!fullReload.renamePreset(2U, "READFAIL"));

    constexpr std::size_t kV6CurrentRecordSizeForTest = 264U;
    constexpr std::size_t kV6PresetRecordSizeForTest = 280U;
    constexpr std::size_t kPreset2OffsetForTest =
        kV6CurrentRecordSizeForTest + 2U * kV6PresetRecordSizeForTest;
    std::uint8_t presetMagicByte = 0U;
    CHECK(storage.readBytes(kPreset2OffsetForTest, &presetMagicByte, 1U));
    const std::uint8_t corruptPresetMagic = static_cast<std::uint8_t>(presetMagicByte ^ 0x01U);
    CHECK(storage.writeBytes(kPreset2OffsetForTest, &corruptPresetMagic, 1U));
    CHECK(!fullReload.loadPreset(2U, loadedPreset));
    CHECK(!fullReload.renamePreset(2U, "CORRUPT"));
    CHECK(storage.writeBytes(kPreset2OffsetForTest, &presetMagicByte, 1U));

    // STM32F4 Flash erase/program stalls instruction fetches. Pending CURRENT and
    // preset writes therefore remain RAM-only while PLAY is active and flush only
    // after the application explicitly permits a timing-safe commit.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage deferredStorage;
    services::PersistentStateService deferred(deferredStorage);
    deferred.begin();
    ClockState deferredState = makeDefaultState();
    deferredState.bpm = 149U;
    deferred.requestCurrentState(deferredState, 20000U);
    CHECK(deferred.savePreset(1U, "LIVE", deferredState));
    CHECK(deferred.presetExists(1U));
    ClockState stagedLoad = makeDefaultState();
    CHECK(deferred.loadPreset(1U, stagedLoad));
    CHECK_EQ(stagedLoad.bpm, 149U);
    hal::PersistentStorage::failNextReadForTest();
    deferred.service(20000U + config::kPersistenceCommitDelayMs + 1U, true);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);
    deferred.service(20000U + config::kPersistenceCommitDelayMs + 1U, false);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);
    deferred.service(20000U + config::kPersistenceCommitDelayMs + 1U, true);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);
    services::PersistentStateService deferredReload(deferredStorage);
    deferredReload.begin();
    CHECK(deferredReload.presetExists(1U));
    ClockState durableLoad = makeDefaultState();
    CHECK(deferredReload.loadPreset(1U, durableLoad));
    CHECK_EQ(durableLoad.bpm, 149U);
}


void testPersistentV3Migration() {
    constexpr std::size_t kV6CurrentSize = 264U;
    constexpr std::size_t kV6PresetSize = 280U;
    constexpr std::size_t kV5CurrentSize = 263U;
    constexpr std::size_t kV5PresetSize = 279U;
    constexpr std::size_t kV4CurrentSize = 257U;
    constexpr std::size_t kV4PresetSize = 273U;
    constexpr std::size_t kV3CurrentSize = 253U;
    constexpr std::size_t kV3PresetSize = 269U;
    constexpr std::size_t kV6PresetOffset = kV6CurrentSize;
    constexpr std::size_t kV5PresetOffset = kV5CurrentSize;
    constexpr std::size_t kV4PresetOffset = kV4CurrentSize;
    constexpr std::size_t kV3PresetOffset = kV3CurrentSize;
    constexpr std::size_t kScoreOffset = 3072U;

    // Seed a v6 record using the production serializer, then remove only the
    // reset-mode byte to obtain a byte-accurate v5 fixture. Older fixtures are
    // derived from that v5 layout so every migration step is tested independently.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage seedStorage;
    services::PersistentStateService seed(seedStorage);
    seed.begin();
    ClockState legacyState = makeDefaultState();
    legacyState.bpm = 143U;
    legacyState.masterMeter = {7U, 8U};
    legacyState.transport = TransportState::Playing;
    legacyState.externalSync.resetMode = ExternalResetMode::Gate;
    legacyState.channels[2].common.mode = ChannelMode::Euclid;
    legacyState.channels[2].euclid = {13U, 5U, 3U};
    seed.requestTransportState(TransportState::Playing, 10U);
    seed.requestCurrentState(legacyState, 10U);
    seed.service(10U + config::kPersistenceCommitDelayMs);
    CHECK(seed.savePreset(0U, "MIGRATE", legacyState));
    seed.service(5000U);

    std::array<std::uint8_t, kV6CurrentSize> v6Current{};
    std::array<std::uint8_t, kV6PresetSize> v6Preset{};
    CHECK(seedStorage.readBytes(0U, v6Current.data(), v6Current.size()));
    CHECK(seedStorage.readBytes(kV6PresetOffset, v6Preset.data(), v6Preset.size()));
    const auto v5Current = makeLegacyV5Record<kV6CurrentSize, kV5CurrentSize, 8U>(v6Current);
    const auto v5Preset = makeLegacyV5Record<kV6PresetSize, kV5PresetSize, 24U>(v6Preset);
    const auto v4Current = makeLegacyV4Record<kV5CurrentSize, kV4CurrentSize, 8U>(v5Current);
    const auto v4Preset = makeLegacyV4Record<kV5PresetSize, kV4PresetSize, 24U>(v5Preset);
    const auto v3Current = makeLegacyV3Record<kV4CurrentSize, kV3CurrentSize, 8U>(v4Current);
    const auto v3Preset = makeLegacyV3Record<kV4PresetSize, kV3PresetSize, 24U>(v4Preset);

    const std::array<std::uint8_t, 16U> scoreMarker{
        0x52U, 0x41U, 0x49U, 0x44U, 1U, 2U, 3U, 4U,
        5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};

    // v5 -> v6: user state remains intact and the new RST semantic defaults to TRIGGER.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV5Storage;
    CHECK(legacyV5Storage.writeBytes(0U, v5Current.data(), v5Current.size()));
    CHECK(legacyV5Storage.writeBytes(kV5PresetOffset, v5Preset.data(), v5Preset.size()));
    CHECK(legacyV5Storage.writeBytes(kScoreOffset, scoreMarker.data(), scoreMarker.size()));
    services::PersistentStateService migratedV5(legacyV5Storage);
    migratedV5.begin();
    ClockState restoredV5 = makeDefaultState();
    CHECK(migratedV5.restoreCurrentState(restoredV5));
    CHECK_EQ(restoredV5.bpm, 143U);
    CHECK_EQ(restoredV5.externalSync.resetMode, defaults::kExternalResetMode);
    CHECK(migratedV5.presetExists(0U));
    std::array<std::uint8_t, kV6CurrentSize> migratedV6Current{};
    std::array<std::uint8_t, kV6PresetSize> migratedV6Preset{};
    CHECK(legacyV5Storage.readBytes(0U, migratedV6Current.data(), migratedV6Current.size()));
    CHECK(legacyV5Storage.readBytes(kV6PresetOffset, migratedV6Preset.data(), migratedV6Preset.size()));
    CHECK_EQ(migratedV6Current[4], 6U);
    CHECK_EQ(migratedV6Preset[4], 6U);

    // v4 -> v6: user BPM remains intact and newer preferences receive factory defaults.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV4Storage;
    CHECK(legacyV4Storage.writeBytes(0U, v4Current.data(), v4Current.size()));
    CHECK(legacyV4Storage.writeBytes(kV4PresetOffset, v4Preset.data(), v4Preset.size()));
    CHECK(legacyV4Storage.writeBytes(kScoreOffset, scoreMarker.data(), scoreMarker.size()));
    services::PersistentStateService migratedV4(legacyV4Storage);
    migratedV4.begin();
    ClockState restoredV4 = makeDefaultState();
    CHECK(migratedV4.restoreCurrentState(restoredV4));
    CHECK_EQ(restoredV4.bpm, 143U);
    CHECK_EQ(restoredV4.tempoRange.minimumBpm, defaults::kMinimumBpm);
    CHECK_EQ(restoredV4.tempoRange.maximumBpm, defaults::kMaximumBpm);
    CHECK_EQ(restoredV4.unifiedClock.humanizeUs, defaults::kUnifiedClockHumanizeUs);
    CHECK_EQ(restoredV4.externalSync.resetMode, defaults::kExternalResetMode);
    CHECK(migratedV4.presetExists(0U));
    CHECK(legacyV4Storage.readBytes(0U, migratedV6Current.data(), migratedV6Current.size()));
    CHECK(legacyV4Storage.readBytes(kV6PresetOffset, migratedV6Preset.data(), migratedV6Preset.size()));
    CHECK_EQ(migratedV6Current[4], 6U);
    CHECK_EQ(migratedV6Preset[4], 6U);

    // v3 -> v6 through the v4/v5 layouts, preserving the independent high-score area.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyStorage;
    CHECK(legacyStorage.writeBytes(0U, v3Current.data(), v3Current.size()));
    CHECK(legacyStorage.writeBytes(kV3PresetOffset, v3Preset.data(), v3Preset.size()));
    CHECK(legacyStorage.writeBytes(kScoreOffset, scoreMarker.data(), scoreMarker.size()));

    services::PersistentStateService migrated(legacyStorage);
    migrated.begin();
    ClockState restored = makeDefaultState();
    CHECK(migrated.restoreCurrentState(restored));
    CHECK_EQ(restored.bpm, 143U);
    CHECK_EQ(restored.masterMeter.unit, 8U);
    CHECK_EQ(restored.transport, TransportState::Playing);
    CHECK_EQ(restored.channels[2].euclid.steps, 13U);
    CHECK_EQ(restored.display.screensaverMode, defaults::kScreensaverMode);
    CHECK_EQ(restored.display.screensaverAfterMinutes, defaults::kScreensaverAfterMinutes);
    CHECK_EQ(restored.display.dimAfterMinutes, defaults::kScreensaverDimAfterMinutes);
    CHECK_EQ(restored.display.offAfterMinutes, defaults::kScreensaverOffAfterMinutes);
    CHECK_EQ(restored.tempoRange.minimumBpm, defaults::kMinimumBpm);
    CHECK_EQ(restored.tempoRange.maximumBpm, defaults::kMaximumBpm);
    CHECK_EQ(restored.unifiedClock.humanizeUs, 0U);
    CHECK_EQ(restored.externalSync.resetMode, defaults::kExternalResetMode);
    CHECK(migrated.presetExists(0U));
    char presetName[services::PersistentStateService::kPresetNameLength + 1U]{};
    migrated.presetName(0U, presetName, sizeof(presetName));
    CHECK(std::strncmp(presetName, "MIGRATE", 7U) == 0);
    ClockState loadedPreset = makeDefaultState();
    loadedPreset.transport = TransportState::Paused;
    CHECK(migrated.loadPreset(0U, loadedPreset));
    CHECK_EQ(loadedPreset.bpm, 143U);
    CHECK_EQ(loadedPreset.transport, TransportState::Paused);
    CHECK_EQ(loadedPreset.display.screensaverMode, defaults::kScreensaverMode);
    CHECK_EQ(loadedPreset.externalSync.resetMode, defaults::kExternalResetMode);

    std::array<std::uint8_t, 16U> scoreAfter{};
    CHECK(legacyStorage.readBytes(kScoreOffset, scoreAfter.data(), scoreAfter.size()));
    CHECK(scoreAfter == scoreMarker);
    CHECK(legacyStorage.readBytes(0U, migratedV6Current.data(), migratedV6Current.size()));
    CHECK(legacyStorage.readBytes(kV6PresetOffset, migratedV6Preset.data(), migratedV6Preset.size()));
    CHECK_EQ(migratedV6Current[4], 6U);
    CHECK_EQ(migratedV6Preset[4], 6U);

    // A damaged v3 CURRENT must not prevent a valid v3 preset from being recovered.
    auto corruptCurrent = v3Current;
    corruptCurrent[40U] ^= 0x80U;
    hal::PersistentStorage::resetForTest();
    CHECK(legacyStorage.writeBytes(0U, corruptCurrent.data(), corruptCurrent.size()));
    CHECK(legacyStorage.writeBytes(kV3PresetOffset, v3Preset.data(), v3Preset.size()));
    services::PersistentStateService presetOnlyMigration(legacyStorage);
    presetOnlyMigration.begin();
    CHECK(!presetOnlyMigration.hasStoredCurrentState());
    CHECK(presetOnlyMigration.presetExists(0U));

    // A CRC-valid legacy preset with unsupported name characters is rejected.
    auto badNamePreset = v3Preset;
    badNamePreset[8U] = static_cast<std::uint8_t>('?');
    writeTestUint32Le(
        badNamePreset.data() + badNamePreset.size() - 4U,
        testCrc32(badNamePreset.data(), badNamePreset.size() - 4U));
    hal::PersistentStorage::resetForTest();
    CHECK(legacyStorage.writeBytes(kV3PresetOffset, badNamePreset.data(), badNamePreset.size()));
    services::PersistentStateService invalidNameMigration(legacyStorage);
    invalidNameMigration.begin();
    CHECK(!invalidNameMigration.presetExists(0U));

    // Legacy tempo below the new factory minimum remains valid after migration.
    // The migration lowers that user's personal minimum rather than rewriting BPM.
    auto lowTempoV4 = v4Current;
    lowTempoV4[8U] = 12U;
    lowTempoV4[9U] = 0U;
    writeTestUint32Le(
        lowTempoV4.data() + lowTempoV4.size() - 4U,
        testCrc32(lowTempoV4.data(), lowTempoV4.size() - 4U));
    hal::PersistentStorage::resetForTest();
    CHECK(legacyStorage.writeBytes(0U, lowTempoV4.data(), lowTempoV4.size()));
    services::PersistentStateService lowTempoMigration(legacyStorage);
    lowTempoMigration.begin();
    ClockState lowTempoRestored = makeDefaultState();
    CHECK(lowTempoMigration.restoreCurrentState(lowTempoRestored));
    CHECK_EQ(lowTempoRestored.bpm, 12U);
    CHECK_EQ(lowTempoRestored.tempoRange.minimumBpm, 12U);
    CHECK_EQ(lowTempoRestored.tempoRange.maximumBpm, 999U);
    CHECK_EQ(lowTempoRestored.externalSync.resetMode, defaults::kExternalResetMode);
}


void testPersistentStateValidationBoundaries() {
    ClockState state = makeDefaultState();
    CHECK(services::isPersistentStateValid(state));

    const auto invalid = [](const auto& mutate) {
        ClockState candidate = makeDefaultState();
        mutate(candidate);
        CHECK(!services::isPersistentStateValid(candidate));
    };

    invalid([](ClockState& s) { s.tempoRange.minimumBpm = 0U; });
    invalid([](ClockState& s) { s.tempoRange.maximumBpm = 1000U; });
    invalid([](ClockState& s) { s.tempoRange.minimumBpm = 200U; s.tempoRange.maximumBpm = 100U; });
    invalid([](ClockState& s) { s.bpm = 19U; });
    invalid([](ClockState& s) { s.bpm = 1000U; });
    invalid([](ClockState& s) { s.masterMeter.beats = 0U; });
    invalid([](ClockState& s) { s.masterMeter.beats = 17U; });
    invalid([](ClockState& s) { s.masterMeter.unit = 3U; });
    // Exercise every valid meter denominator so the validator's short-circuit
    // chain is covered on each accepted exit path, not only the factory 4/4.
    for (const std::uint8_t unit : std::array<std::uint8_t, 5U>{1U, 2U, 4U, 8U, 16U}) {
        ClockState candidate = makeDefaultState();
        candidate.masterMeter.unit = unit;
        CHECK(services::isPersistentStateValid(candidate));
    }
    invalid([](ClockState& s) { s.transport = static_cast<TransportState>(99U); });
    invalid([](ClockState& s) { s.source = static_cast<ClockSource>(99U); });
    invalid([](ClockState& s) { s.operatingMode = static_cast<OperatingMode>(99U); });
    invalid([](ClockState& s) { s.externalSync.pulsesPerQuarterNote = 0U; });
    invalid([](ClockState& s) { s.externalSync.edge = static_cast<SyncEdge>(99U); });
    invalid([](ClockState& s) { s.externalSync.lossMode = static_cast<SyncLossMode>(99U); });
    invalid([](ClockState& s) { s.externalSync.resetMode = static_cast<ExternalResetMode>(99U); });

    invalid([](ClockState& s) { s.unifiedClock.rate.mode = static_cast<ClockRatioMode>(99U); });
    invalid([](ClockState& s) { s.unifiedClock.rate.factor = 0U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.factor = 33U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.numerator = 0U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.numerator = 17U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.denominator = 0U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.denominator = 17U; });
    invalid([](ClockState& s) { s.unifiedClock.swingPercent = 51U; });
    invalid([](ClockState& s) { s.unifiedClock.phasePercent = 100U; });
    invalid([](ClockState& s) { s.unifiedClock.humanizeUs = static_cast<std::uint16_t>(config::kMaximumHumanizeUs + 1U); });
    invalid([](ClockState& s) { s.dividerBank.bank = static_cast<DividerBank>(99U); });
    invalid([](ClockState& s) { s.display.screensaverMode = static_cast<ScreensaverMode>(0U); });
    invalid([](ClockState& s) { s.display.screensaverMode = static_cast<ScreensaverMode>(13U); });
    invalid([](ClockState& s) { s.display.screensaverAfterMinutes = 0U; });
    invalid([](ClockState& s) { s.display.screensaverAfterMinutes = 6U; s.display.dimAfterMinutes = 5U; });
    invalid([](ClockState& s) { s.display.dimAfterMinutes = 11U; s.display.offAfterMinutes = 10U; });
    invalid([](ClockState& s) { s.display.offAfterMinutes = static_cast<std::uint8_t>(config::kMaximumScreensaverMinutes + 1U); });

    invalid([](ClockState& s) { s.channels[0].common.mode = static_cast<ChannelMode>(99U); });
    invalid([](ClockState& s) { s.channels[0].common.rate.mode = static_cast<ClockRatioMode>(99U); });
    invalid([](ClockState& s) { s.channels[0].common.rate.factor = 0U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.factor = 33U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.numerator = 0U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.numerator = 17U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.denominator = 0U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.denominator = 17U; });
    invalid([](ClockState& s) { s.channels[0].common.swingPercent = 51U; });
    invalid([](ClockState& s) { s.channels[0].common.probabilityPercent = 101U; });
    invalid([](ClockState& s) { s.channels[0].common.phasePercent = 100U; });
    invalid([](ClockState& s) { s.channels[0].common.resetMode = static_cast<ResetMode>(99U); });
    invalid([](ClockState& s) { s.channels[0].clock.meter.beats = 0U; });
    invalid([](ClockState& s) { s.channels[0].clock.meter.beats = 17U; });
    invalid([](ClockState& s) { s.channels[0].clock.meter.unit = 3U; });
    for (const std::uint8_t unit : std::array<std::uint8_t, 5U>{1U, 2U, 4U, 8U, 16U}) {
        ClockState candidate = makeDefaultState();
        candidate.channels[0].clock.meter.unit = unit;
        CHECK(services::isPersistentStateValid(candidate));
    }
    invalid([](ClockState& s) { s.channels[0].euclid.steps = 0U; });
    invalid([](ClockState& s) { s.channels[0].euclid.steps = 65U; });
    invalid([](ClockState& s) { s.channels[0].euclid.steps = 8U; s.channels[0].euclid.hits = 9U; });
    invalid([](ClockState& s) { s.channels[0].euclid.steps = 8U; s.channels[0].euclid.rotation = 8U; });
    invalid([](ClockState& s) { s.channels[0].sequencer.length = 0U; });
    invalid([](ClockState& s) { s.channels[0].sequencer.length = 65U; });
    invalid([](ClockState& s) { s.channels[0].sequencer.length = 8U; s.channels[0].sequencer.rotation = 8U; });
}

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

    const ui::SettingsPage pages[] = {ui::SettingsPage::Root,ui::SettingsPage::General,ui::SettingsPage::Master,ui::SettingsPage::Sync,ui::SettingsPage::Preferences,ui::SettingsPage::Screensaver,ui::SettingsPage::Info,ui::SettingsPage::Licenses,ui::SettingsPage::Updates,ui::SettingsPage::Channel,ui::SettingsPage::Rate,ui::SettingsPage::Clock,ui::SettingsPage::Euclid,ui::SettingsPage::Sequencer,ui::SettingsPage::UnifiedClock,ui::SettingsPage::DividerBank};
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
    CHECK(std::strcmp(generalRoot.label, "GENERAL SETTINGS") == 0);
    CHECK(std::strcmp(channelRoot.label, "CHANNEL SETTINGS") == 0);
    CHECK(std::strcmp(presetsRoot.label, "PRESETS") == 0);
    CHECK(std::strcmp(infoRoot.label, "INFO") == 0);
    const auto generalSync = ui::buildMenuRow(ui::SettingsPage::General, 1U, 0U, state);
    const auto generalSaver = ui::buildMenuRow(ui::SettingsPage::General, 2U, 0U, state);
    CHECK(std::strcmp(generalSync.label, "SYNC") == 0);
    CHECK(std::strcmp(generalSaver.label, "SCREENSAVER") == 0);
    const auto infoProduct = ui::buildMenuRow(ui::SettingsPage::Info, 0U, 0U, state);
    CHECK(std::strcmp(infoProduct.value, "SSL CLOCK") == 0);
    const auto infoLicenses = ui::buildMenuRow(ui::SettingsPage::Info, 3U, 0U, state);
    const auto infoUpdates = ui::buildMenuRow(ui::SettingsPage::Info, 4U, 0U, state);
    CHECK(std::strcmp(infoLicenses.label, "LICENSES") == 0);
    CHECK(std::strcmp(infoUpdates.label, "UPDATES") == 0);
    state.channels[0].common.mode = ChannelMode::Sequencer;
    const auto channelModeRow = ui::buildMenuRow(ui::SettingsPage::Channel, 0U, 0U, state);
    CHECK(std::strcmp(channelModeRow.value, "SEQUENCER") == 0);
    const auto templatePresetRow = ui::buildMenuRow(ui::SettingsPage::Preferences, 3U, 0U, state);
    CHECK(std::strcmp(templatePresetRow.label, "TEMPLATES") == 0);

    CHECK_EQ(ui::settingsPageItemCount(static_cast<ui::SettingsPage>(99)),0U);
    CHECK(std::strlen(ui::settingsPageTitle(static_cast<ui::SettingsPage>(99)))>0U);
    CHECK(!ui::isChannelSettingsPage(ui::SettingsPage::Root));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::Channel));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::Rate));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::Clock));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::Euclid));
    CHECK(ui::isChannelSettingsPage(ui::SettingsPage::Sequencer));

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
    const auto offModeConfigRow = ui::buildMenuRow(ui::SettingsPage::Channel,2U,0U,state);
    CHECK_EQ(std::strlen(offModeConfigRow.value), 0U);
        state.channels[0].common.muted=false; (void)ui::buildMenuRow(ui::SettingsPage::Channel,7U,0U,state);
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
    {
        hal::InterruptLock lock;
        CHECK_EQ(fakefw::noInterruptCalls,1U);
    }
    CHECK_EQ(fakefw::interruptCalls,1U);
    CHECK_EQ(hal::SystemClock::milliseconds(),0U);
    hal::SystemClock::delayMilliseconds(7U); CHECK_EQ(hal::SystemClock::milliseconds(),7U);

    hal::GateOutputDriver gates;
    gates.beginDisabled();
    CHECK_EQ(fakefw::pinValues[pinmap::kGateBufferOutputEnablePin],pinmap::kGateBufferDisabledLevel);
    gates.enableOutputStage(); CHECK_EQ(fakefw::pinValues[pinmap::kGateBufferOutputEnablePin],pinmap::kGateBufferEnabledLevel);
    gates.disableOutputStage(); gates.setChannelState(0U,true); CHECK_EQ(fakefw::pinValues[pinmap::kChannel1GateLedPin],HIGH);
    gates.setChannelState(0U,false); gates.setChannelState(99U,true); gates.setAllChannelsLow();
    for (auto pin: pinmap::kGateChannelPins) CHECK_EQ(fakefw::pinValues[pin],LOW);

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
    state.tempoRange = {20U, 300U}; editor.changeMasterTempo(127); CHECK_EQ(state.bpm,300U);
    editor.changeMasterTempo(-128); CHECK_EQ(state.bpm,172U);
    // Exercise every editable row and both directions.
    const ui::SettingsPage pages[]={ui::SettingsPage::Master,ui::SettingsPage::Sync,ui::SettingsPage::Channel,ui::SettingsPage::Rate,ui::SettingsPage::Clock,ui::SettingsPage::Euclid,ui::SettingsPage::Sequencer};
    for(auto page:pages){ const auto count=ui::settingsPageItemCount(page); for(std::uint8_t row=0;row<count;++row){ editor.adjust(page,row,0U,1); editor.adjust(page,row,0U,-1); } }
    editor.adjust(ui::SettingsPage::Info,0U,0U,1); editor.adjust(ui::SettingsPage::Root,0U,0U,1); editor.adjust(ui::SettingsPage::Master,0U,99U,1);
    editor.adjust(ui::SettingsPage::Master,99U,0U,1);
    state.externalSync.pulsesPerQuarterNote=3U; editor.adjust(ui::SettingsPage::Sync,1U,0U,1);
    editor.adjust(ui::SettingsPage::Sync,99U,0U,1);
    editor.adjust(ui::SettingsPage::Rate,99U,0U,1);
    editor.adjust(ui::SettingsPage::Clock,99U,0U,1);
    editor.adjust(ui::SettingsPage::Euclid,99U,0U,1);
    editor.adjust(ui::SettingsPage::Sequencer,99U,0U,1);
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

    // The graphical six-function palette maps both directions without hidden state.
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
    for (std::uint8_t row = 0U; row < ui::settingsPageItemCount(ui::SettingsPage::UnifiedClock); ++row) {
        editor.adjust(ui::SettingsPage::UnifiedClock, row, 0U, 1);
        editor.adjust(ui::SettingsPage::UnifiedClock, row, 0U, -1);
    }
    // Also exercise a non-listed ratio so the rate-option fallback branch is covered.
    state.unifiedClock.rate = {ClockRatioMode::Multiply, 99U, 1U, 1U};
    editor.adjust(ui::SettingsPage::UnifiedClock, 1U, 0U, 1);
    editor.adjust(ui::SettingsPage::UnifiedClock, 99U, 0U, 1);
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

    const auto humanizeRow = ui::buildMenuRow(ui::SettingsPage::UnifiedClock, 7U, 0U, state);
    CHECK(std::strcmp(humanizeRow.label, "HUMANIZE") == 0);
    CHECK(std::strstr(humanizeRow.value, "2000") != nullptr);
    editor.adjust(ui::SettingsPage::UnifiedClock, 7U, 0U, -1);
    CHECK_EQ(state.unifiedClock.humanizeUs, 1000U);
    state.unifiedClock.humanizeUs = 0U;
    const auto humanizeOffRow = ui::buildMenuRow(ui::SettingsPage::UnifiedClock, 7U, 0U, state);
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
    auto sample=controls.sample(0U); CHECK_EQ(sample.encoderDelta,0);
    // Debounced press and release for all four buttons.
    const std::uint32_t pins[]={pinmap::kEncoderPushButtonPin,pinmap::kPlayPauseButtonPin,pinmap::kTapTempoButtonPin,pinmap::kResetBackButtonPin};
    for(auto pin:pins) fakefw::setPin(pin,LOW);
    sample=controls.sample(1U); CHECK_EQ(sample.transportButton.edge,hal::ButtonEdge::None);
    sample=controls.sample(30U); CHECK_EQ(sample.transportButton.edge,hal::ButtonEdge::Pressed); CHECK(sample.transportButton.pressed);
    for(auto pin:pins) fakefw::setPin(pin,HIGH);
    (void)controls.sample(31U); sample=controls.sample(60U); CHECK_EQ(sample.resetButton.edge,hal::ButtonEdge::Released);

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

void renderEveryScreenAndState() {
    resetFakes(); prepareDisplaySuccess(); hal::OledDisplay display; CHECK(display.begin());
    ClockState state=makeDefaultState();
    hal::GateOutputDriver gates; gates.beginDisabled(); engine::ClockEngine engine(gates); engine.begin(state); engine.play();
    hal::PersistentStorage storage; hal::PersistentStorage::resetForTest(); services::PersistentStateService persistentState(storage); persistentState.begin();
    ui::UiRenderer renderer(display, persistentState); ui::NavigationState nav{};

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
    nav.screen=ui::Screen::ModeSelect; for(std::uint8_t c=0;c<6U;++c){nav.cursor=c;renderer.render(state,nav,engine.snapshot());}
    nav.screen=ui::Screen::ModeChangeConfirm;
    for (std::uint8_t c=0U; c<6U; ++c) {
        nav.pendingModeFunction = static_cast<ui::ModeFunction>(c);
        for (std::uint8_t choice=0U; choice<2U; ++choice) {
            nav.cursor=choice;
            renderer.render(state,nav,engine.snapshot());
        }
    }
    nav.pendingModeFunction = static_cast<ui::ModeFunction>(99U);
    renderer.render(state,nav,engine.snapshot());
    nav.screen=ui::Screen::SequencerEditor; state.channels[0].sequencer={20U,0U,0xAAAAULL}; for(std::uint8_t page=0;page<4U;++page){nav.sequencerPage=page;nav.sequencerCursor=7U;renderer.render(state,nav,engine.snapshot());}
    nav.screen=ui::Screen::Settings;
    const ui::SettingsPage pages[]={ui::SettingsPage::Root,ui::SettingsPage::General,ui::SettingsPage::Master,ui::SettingsPage::Sync,ui::SettingsPage::Preferences,ui::SettingsPage::Screensaver,ui::SettingsPage::Info,ui::SettingsPage::Licenses,ui::SettingsPage::Updates,ui::SettingsPage::Rate,ui::SettingsPage::Clock,ui::SettingsPage::Euclid,ui::SettingsPage::Sequencer,ui::SettingsPage::UnifiedClock,ui::SettingsPage::DividerBank};
    for(auto page:pages){nav.settingsPage=page;const auto count=ui::settingsPageItemCount(page);for(std::uint8_t row=0;row<count;++row){nav.cursor=row;nav.scrollOffset=row>4U?static_cast<std::uint8_t>(row-4U):0U;nav.editing=(row&1U)!=0U;renderer.render(state,nav,engine.snapshot());}}
    nav.settingsPage=ui::SettingsPage::Channel;
    for(const auto mode:{ChannelMode::Off,ChannelMode::Clock,ChannelMode::Euclid,ChannelMode::Sequencer}){state.channels[0].common.mode=mode;const auto count=ui::settingsPageItemCount(ui::SettingsPage::Channel,mode);for(std::uint8_t row=0;row<count;++row){nav.cursor=row;nav.scrollOffset=row>4U?static_cast<std::uint8_t>(row-4U):0U;nav.editing=(row&1U)!=0U;renderer.render(state,nav,engine.snapshot());}}
    nav.screen=ui::Screen::Templates; for(std::uint8_t i=0;i<services::TemplateService::kTemplateCount;++i){nav.cursor=i;nav.scrollOffset=i>3U?static_cast<std::uint8_t>(i-3U):0U;renderer.render(state,nav,engine.snapshot());}
    nav.screen=ui::Screen::PresetSlots; for(auto action:{ui::PresetSlotAction::Load,ui::PresetSlotAction::Save}){nav.presetSlotAction=action;for(std::uint8_t i=0;i<8U;++i){nav.cursor=i;nav.scrollOffset=i>4U?static_cast<std::uint8_t>(i-4U):0U;renderer.render(state,nav,engine.snapshot());}}
    nav.screen=ui::Screen::OverwriteConfirm; nav.selectedPresetSlot=0U; for(std::uint8_t choice=0U;choice<2U;++choice){nav.cursor=choice;renderer.render(state,nav,engine.snapshot());}
    nav.screen=ui::Screen::NameEntry; nav.presetNameBuffer.fill(' '); nav.presetNameBuffer.back()='\0'; nav.presetNameBuffer[0]='A'; for(std::uint8_t i=0;i<16U;++i){nav.nameCharacterIndex=i;renderer.render(state,nav,engine.snapshot());}
    nav.cursor=5U; nav.scrollOffset=5U; renderer.render(state,nav,engine.snapshot());
    renderer.renderBootScreen(0U); renderer.renderBootScreen(config::kBootDurationMs/2U); renderer.renderBootScreen(config::kBootDurationMs+100U);
    nav.screen=static_cast<ui::Screen>(99U); renderer.render(state,nav,engine.snapshot());
}

void controllerShortPress(ui::UiController& controller,std::uint32_t& now){hal::ControlSample s{};s.encoderButton=pressedEdge();controller.processControls(s,now++);s={};s.encoderButton=releasedEdge();controller.processControls(s,now++);}
void controllerLongPress(ui::UiController& controller,std::uint32_t& now){hal::ControlSample s{};s.encoderButton=pressedEdge();controller.processControls(s,now++);s={};s.encoderButton=heldButton();now += config::kEncoderLongPressMs;controller.processControls(s,now++);s={};s.encoderButton=releasedEdge();controller.processControls(s,now++);}
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
    ui::UiController controller(state, engine, renderer, persistence);

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

    // MODE 3 deliberately keeps the ordinary STOP screen until DIM/OFF.
    state.display = {ScreensaverMode::None, 1U, 2U, 3U};
    controller.processControls(wake, 200'000U);
    controller.serviceRendering(260'001U);

    // Screensaver logic is disabled while transport is not STOPPED.
    state.transport = TransportState::Playing;
    controller.serviceRendering(400'000U);
}

void testUiControllerFlows() {
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
    std::uint32_t now = 100U;

    controller.invalidate();
    controller.serviceRendering(now);
    controller.invalidate();
    controller.serviceRendering(now + 1U); // refresh throttle
    controller.serviceRendering(now + config::kDisplayRefreshMinimumMs + 1U);

    const std::uint16_t originalBpm = state.bpm;
    controllerTurn(controller, 1, now);
    CHECK_EQ(state.bpm, originalBpm + 1U);
    controllerTurn(controller, -1, now);

    // A short encoder push opens the compact eight-channel overview. Turning
    // only moves the cursor; a second short push commits the channel and returns.
    const std::uint8_t overviewStart = controller.navigation().selectedChannel;
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    controllerTurn(controller, 2, now);
    CHECK_EQ(
        controller.navigation().cursor,
        static_cast<std::uint8_t>((overviewStart + 2U) % kChannelCount));
    CHECK_EQ(controller.navigation().selectedChannel, overviewStart);
    const std::uint8_t committedOverviewChannel = controller.navigation().cursor;
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    CHECK_EQ(controller.navigation().selectedChannel, committedOverviewChannel);

    // Channel settings are deliberately long-push only from Performance or overview.
    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);
    const std::uint8_t longPushChannel = controller.navigation().cursor;
    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(controller.navigation().selectedChannel, longPushChannel);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // TAP + Turn inside the overview opens the six-function mode palette.
    // Releasing TAP now requests a mode change; changing mode always requires
    // an explicit YES before the new configuration is applied.
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    hal::ControlSample sample{};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    sample.encoderDelta = 1; // CLOCK -> EUCLID
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeSelect);
    CHECK_EQ(controller.navigation().cursor, 3U);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    CHECK_EQ(
        state.channels[controller.navigation().selectedChannel].common.mode,
        ChannelMode::Clock);
    controllerConfirmModeYes(controller, now);
    CHECK_EQ(state.operatingMode, OperatingMode::Independent);
    CHECK_EQ(
        state.channels[controller.navigation().selectedChannel].common.mode,
        ChannelMode::Euclid);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Euclid);

    // Selecting the already active mode needs no destructive-change confirmation.
    controllerReset(controller, now);
    controllerReset(controller, now);
    controllerShortPress(controller, now);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    sample.encoderDelta = 1; // EUCLID -> SEQ candidate
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    controllerShortPress(controller, now); // Default NO
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeSelect);
    CHECK_EQ(state.channels[controller.navigation().selectedChannel].common.mode, ChannelMode::Euclid);
    controllerReset(controller, now); // ModeSelect -> overview
    controllerReset(controller, now); // overview -> performance

    // Return to Performance, then select the global one-clock-to-eight mode.
    controllerReset(controller, now); // Euclid -> Channel
    controllerReset(controller, now); // Channel -> Performance
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    sample.encoderDelta = 3; // EUCLID -> ONE CLOCK
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    controllerConfirmModeYes(controller, now);
    CHECK_EQ(state.operatingMode, OperatingMode::UnifiedClock);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedClock);
    CHECK_EQ(controller.navigation().cursor, 1U);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // In a global mode the overview is informational rather than a fake channel
    // selector. Short push returns; long push enters the relevant global menu.
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    const std::uint8_t channelBeforeGlobalTurn = controller.navigation().selectedChannel;
    controllerTurn(controller, 1, now);
    CHECK_EQ(controller.navigation().selectedChannel, channelBeforeGlobalTurn);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    controllerShortPress(controller, now);
    controllerLongPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::UnifiedClock);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ChannelQuickSelect);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // Switching back to ordinary CLOCK confirms first and then returns directly
    // to the performance screen as requested by the mode workflow.
    controllerShortPress(controller, now);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    sample.encoderDelta = 2; // ONE CLOCK -> CLOCK
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    controllerConfirmModeYes(controller, now);
    CHECK_EQ(state.operatingMode, OperatingMode::Independent);
    CHECK_EQ(
        state.channels[controller.navigation().selectedChannel].common.mode,
        ChannelMode::Clock);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // TAP + encoder push is the only gesture for the combined settings tree.
    // It must not leak a tap-tempo event or a second encoder action on release.
    const std::uint16_t bpmBeforeChord = state.bpm;
    controllerOpenSettingsChord(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Root);
    CHECK_EQ(state.bpm, bpmBeforeChord);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // Exercise all five root actions.
    for (std::uint8_t rootRow = 0U; rootRow < 5U; ++rootRow) {
        controllerOpenSettingsChord(controller, now);
        controllerTurn(controller, static_cast<std::int8_t>(rootRow), now);
        controllerShortPress(controller, now);
        if (rootRow == 4U) {
            CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Root);
        } else {
            CHECK_EQ(controller.navigation().screen, ui::Screen::Settings);
            if (rootRow == 0U) {
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::General);
            } else if (rootRow == 1U) {
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
            } else if (rootRow == 2U) {
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Preferences);
            } else {
                CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Info);
                controllerShortPress(controller, now);
                CHECK(!controller.navigation().editing);
            }
            controllerReset(controller, now); // child -> Root
        }
        controllerReset(controller, now); // Root -> Performance
        CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    }

    // PRESETS owns factory templates as well as load/save operations.
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 2, now);     // PRESETS
    controllerShortPress(controller, now);
    controllerTurn(controller, 3, now);     // Templates
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Templates);
    controllerTurn(controller, 5, now);
    controllerTurn(controller, -5, now);
    state.channels[0].common.mode = ChannelMode::Off;
    controllerShortPress(controller, now); // apply ALL MASTER template
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    CHECK_EQ(state.channels[0].common.mode, ChannelMode::Clock);

    // PRESETS: save a complete named preset, then verify occupied slots ask
    // before overwrite and that NO is the safe default.
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 2, now);     // PRESETS
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Preferences);
    controllerTurn(controller, 2, now);     // Save preset
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::PresetSlots);
    controllerShortPress(controller, now);  // empty slot 1 -> name entry
    CHECK_EQ(controller.navigation().screen, ui::Screen::NameEntry);
    controllerTurn(controller, 1, now);     // rotate character band
    for (std::uint8_t character = 0U;
         character < services::PersistentStateService::kPresetNameLength;
         ++character) {
        controllerShortPress(controller, now);
    }
    CHECK_EQ(controller.navigation().screen, ui::Screen::PresetSlots);
    persistentState.service(now + config::kPersistenceCommitDelayMs, true);
    CHECK(persistentState.presetExists(0U));

    // Selecting the same occupied slot for SAVE must not jump straight to name entry.
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::OverwriteConfirm);
    CHECK_EQ(controller.navigation().cursor, 0U); // NO
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::PresetSlots);
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now); // YES
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::NameEntry);
    controllerReset(controller, now); // cancel name entry
    controllerReset(controller, now); // slots -> Preferences
    controllerReset(controller, now); // Preferences -> Root
    controllerReset(controller, now); // Root -> Performance

    // Load the saved slot.
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 2, now);     // PRESETS
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now);     // Load preset
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::PresetSlots);
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);

    // The saved ALL MASTER preset correctly restores ONE CLOCK. Switch to the
    // independent channel architecture for the remaining per-channel UI tests.
    state.operatingMode = OperatingMode::Independent;
    engine.updateConfiguration(state, true);

    // The selected channel page exposes only mode-relevant branches. OFF has only
    // MODE, while active modes expose RATE, their own mode page, and common timing.
    std::uint8_t selectedChannel = controller.navigation().selectedChannel;
    state.channels[selectedChannel].common.mode = ChannelMode::Off;
    engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now); // Channel
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Channel);
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Channel, ChannelMode::Off), 1U);
    controllerShortPress(controller, now); // MODE
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeSelect);
    CHECK_EQ(controller.navigation().cursor, 5U);
    controllerTurn(controller, -3, now); // OFF -> CLOCK
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::ModeChangeConfirm);
    controllerConfirmModeYes(controller, now);
    CHECK_EQ(state.channels[selectedChannel].common.mode, ChannelMode::Clock);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    CHECK_EQ(ui::settingsPageItemCount(ui::SettingsPage::Channel, ChannelMode::Clock), 9U);

    // RATE and CLOCK settings remain ordinary push-to-edit pages.
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now); // Channel
    controllerShortPress(controller, now);
    controllerTurn(controller, 1, now); // RATE
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Rate);
    controllerShortPress(controller, now);
    CHECK(controller.navigation().editing);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    CHECK(!controller.navigation().editing);
    controllerReset(controller, now); // Rate -> Channel
    controllerTurn(controller, 2, now); // mode-specific CLOCK page
    controllerShortPress(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Clock);
    controllerReset(controller, now);
    controllerReset(controller, now); // Channel -> Root
    controllerReset(controller, now); // Root -> Performance

    // EUCLID and SEQ expose only their own mode-specific page from CHANNEL.
    for (const ChannelMode mode : {ChannelMode::Euclid, ChannelMode::Sequencer}) {
        state.channels[selectedChannel].common.mode = mode;
        engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
        controllerOpenSettingsChord(controller, now);
        controllerTurn(controller, 1, now); // Channel
        controllerShortPress(controller, now);
        controllerTurn(controller, 2, now); // mode-specific page
        controllerShortPress(controller, now);
        CHECK_EQ(
            controller.navigation().settingsPage,
            mode == ChannelMode::Euclid ? ui::SettingsPage::Euclid : ui::SettingsPage::Sequencer);
        controllerReset(controller, now); // mode page -> Channel
        controllerReset(controller, now); // Channel -> Root
        controllerReset(controller, now); // Root -> Performance
    }

    // TAP no longer has a long-press shortcut. Holding it without turning must
    // never leave Performance; mode-specific parameters are reached through menus.
    state.channels[selectedChannel].common.mode = ChannelMode::Euclid;
    engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, now++);
    sample = {};
    sample.tapButton = heldButton();
    controller.processControls(sample, now + 1500U);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, now + 1501U);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    now += 1502U;

    // Sequencer editor, page navigation, and command execution remain accessible
    // through CHANNEL -> SEQUENCER -> EDITOR without a long-press gesture.
    state.channels[selectedChannel].common.mode = ChannelMode::Sequencer;
    state.channels[selectedChannel].sequencer.length = 64U;
    engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
    controllerOpenSettingsChord(controller, now);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now); // Channel
    controllerTurn(controller, 2, now);
    controllerShortPress(controller, now); // Sequencer page
    controllerShortPress(controller, now); // Editor
    CHECK_EQ(controller.navigation().screen, ui::Screen::SequencerEditor);
    controllerTurn(controller, 5, now);
    controllerShortPress(controller, now);
    for (int page = 0; page < 4; ++page) {
        sample = {};
        sample.transportButton = pressedEdge();
        controller.processControls(sample, now++);
    }
    CHECK_EQ(controller.navigation().sequencerPage, 3U);
    for (int page = 0; page < 4; ++page) {
        sample = {};
        sample.tapButton = pressedEdge();
        controller.processControls(sample, now++);
        sample = {};
        sample.tapButton = releasedEdge();
        controller.processControls(sample, now++);
    }
    CHECK_EQ(controller.navigation().sequencerPage, 0U);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().settingsPage, ui::SettingsPage::Sequencer);
    controllerTurn(controller, 3, now); // INVERT
    controllerShortPress(controller, now);
    controllerReset(controller, now); // Sequencer -> Channel
    controllerReset(controller, now); // Channel -> Root
    controllerReset(controller, now); // Root -> Performance

    // Transport and tap tempo remain dedicated only on Performance.
    sample = {};
    sample.transportButton = pressedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(state.transport, TransportState::Playing);
    sample.transportButton = pressedEdge();
    controller.processControls(sample, now++);
    CHECK_EQ(state.transport, TransportState::Paused);
    controllerReset(controller, now);
    CHECK_EQ(state.transport, TransportState::Stopped);

    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, 1000U);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, 1001U);
    sample = {};
    sample.tapButton = pressedEdge();
    controller.processControls(sample, 1500U);
    sample = {};
    sample.tapButton = releasedEdge();
    controller.processControls(sample, 1501U);
    CHECK_EQ(state.bpm, 120U);

    // Rendering invalidation reacts to external-lock changes and visible pattern steps.
    state.source = ClockSource::Internal;
    selectedChannel = controller.navigation().selectedChannel;
    state.channels[selectedChannel].common.mode = ChannelMode::Euclid;
    engine.updateChannel(selectedChannel, state.channels[selectedChannel], true);
    controller.invalidate();
    controller.serviceRendering(now + config::kDisplayRefreshMinimumMs + 1U);
    state.channels[selectedChannel].common.mode = ChannelMode::Sequencer;
    state.channels[selectedChannel].common.rate = {ClockRatioMode::Multiply, 1U, 1U, 1U};
    state.channels[selectedChannel].sequencer.length = 16U;
    engine.updateConfiguration(state, true);
    engine.play();
    controller.invalidate();
    controller.serviceRendering(now + config::kDisplayRefreshMinimumMs + 2U);
    engine.setExternalLock(true, 120000U);
    controller.serviceRendering(now + (2U * config::kDisplayRefreshMinimumMs) + 2U);
    runEngineTicks(engine, 20001U);
    CHECK(engine.snapshot().channelStep[selectedChannel] != 0U);
    controller.serviceRendering(now + (3U * config::kDisplayRefreshMinimumMs) + 3U);

    // Defensive invalid-screen and no-op paths.
    auto& mutableNavigation = const_cast<ui::NavigationState&>(controller.navigation());
    mutableNavigation.screen = static_cast<ui::Screen>(99U);
    controllerTurn(controller, 1, now);
    controllerShortPress(controller, now);
    controllerReset(controller, now);
    CHECK_EQ(controller.navigation().screen, ui::Screen::Performance);
    sample = {};
    controller.processControls(sample, now++);
    controller.serviceRendering(now);
}

void testEasterEggGameAndHighScore() {
    resetFakes();
    prepareDisplaySuccess();

    hal::PersistentStorage storage;
    game::EasterEggScoreStore scoreStore(storage);
    const game::HighScoreEntry emptyScore = scoreStore.load();
    CHECK_EQ(emptyScore.score, 0U);
    CHECK_EQ(emptyScore.initials[0], 'A');

    game::HighScoreEntry score{};
    score.score = 1234U;
    score.initials = {{'A', 'X', 'L', '\0'}};
    CHECK(scoreStore.save(score));
    const game::HighScoreEntry loadedScore = scoreStore.load();
    CHECK_EQ(loadedScore.score, 1234U);
    CHECK_EQ(loadedScore.initials[0], 'A');
    CHECK_EQ(loadedScore.initials[1], 'X');
    CHECK_EQ(loadedScore.initials[2], 'L');

    // Game-specific score slots must be independent while preserving the historical Pixel Raid offset.
    game::EasterEggScoreStore formulaScoreSlot(storage, game::EasterEggScoreId::Formula1);
    CHECK_EQ(formulaScoreSlot.load().score, 0U);
    game::HighScoreEntry formulaScore{};
    formulaScore.score = 2468U;
    formulaScore.initials = {{'F', 'O', 'M', '\0'}};
    CHECK(formulaScoreSlot.save(formulaScore));
    CHECK_EQ(formulaScoreSlot.load().score, 2468U);
    CHECK_EQ(scoreStore.load().score, 1234U);
    game::EasterEggScoreStore moonBuggyScoreSlot(storage, game::EasterEggScoreId::MoonBuggy);
    CHECK_EQ(moonBuggyScoreSlot.load().score, 0U);
    game::HighScoreEntry moonScore{};
    moonScore.score = 1357U;
    moonScore.initials = {{'E', 'G', 'G', '\0'}};
    CHECK(moonBuggyScoreSlot.save(moonScore));
    CHECK_EQ(moonBuggyScoreSlot.load().score, 1357U);
    CHECK_EQ(formulaScoreSlot.load().score, 2468U);
    CHECK_EQ(scoreStore.load().score, 1234U);

    game::HighScoreEntry invalidScore = score;
    invalidScore.initials[0] = '?';
    CHECK(!scoreStore.save(invalidScore));
    invalidScore = score; invalidScore.initials[1] = '?'; CHECK(!scoreStore.save(invalidScore));
    invalidScore = score; invalidScore.initials[2] = '?'; CHECK(!scoreStore.save(invalidScore));
    hal::PersistentStorage::failNextReadForTest();
    CHECK_EQ(scoreStore.load().score, 0U);

    // Corrupt record metadata and initials to exercise every validation stage.
    std::array<std::uint8_t, 16U> rawScore{};
    CHECK(storage.readBytes(3072U, rawScore.data(), rawScore.size()));
    for (const std::size_t corruptIndex : std::array<std::size_t, 5U>{{0U, 1U, 2U, 3U, 4U}}) {
        auto corrupt = rawScore;
        corrupt[corruptIndex] ^= 0x01U;
        CHECK(storage.writeBytes(3072U, corrupt.data(), corrupt.size()));
        CHECK_EQ(scoreStore.load().score, 0U);
        CHECK(storage.writeBytes(3072U, rawScore.data(), rawScore.size()));
    }
    auto badCrc = rawScore; badCrc[15] ^= 0x01U; CHECK(storage.writeBytes(3072U, badCrc.data(), badCrc.size())); CHECK_EQ(scoreStore.load().score, 0U);
    CHECK(storage.writeBytes(3072U, rawScore.data(), rawScore.size()));
    for (const std::size_t initialIndex : std::array<std::size_t, 3U>{{9U, 10U, 11U}}) {
        auto corrupt = rawScore; corrupt[initialIndex] = static_cast<std::uint8_t>('?');
        // Recompute is intentionally skipped: this still validates CRC rejection before initials.
        CHECK(storage.writeBytes(3072U, corrupt.data(), corrupt.size()));
        CHECK_EQ(scoreStore.load().score, 0U);
        CHECK(storage.writeBytes(3072U, rawScore.data(), rawScore.size()));
    }

    // New Top-100 stores migrate the historical single-score slots without erasing them.
    game::ArcadeLeaderboardStore pixelLeaderboard(storage, game::ArcadeGameId::PixelRaid);
    game::ArcadeLeaderboardStore formulaLeaderboard(storage, game::ArcadeGameId::Formula1);
    game::ArcadeLeaderboardStore breakoutLeaderboard(storage, game::ArcadeGameId::Breakout);
    game::ArcadeLeaderboardStore eggLeaderboard(storage, game::ArcadeGameId::EggJourney);
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
    CHECK_EQ(formulaLeaderboard.load().entries[0].score, 2468U);
    CHECK_EQ(eggLeaderboard.load().entries[0].score, 1357U);
    const std::array<char, 4U> topName{{'T','O','P','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(5000U, topName), 0);
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 5000U);
    CHECK_EQ(pixelLeaderboard.load().entries[1].score, 1234U);
    CHECK_EQ(formulaLeaderboard.load().entries[0].score, 2468U);
    CHECK_EQ(breakoutLeaderboard.load().count, 0U);

    // Leaderboard durable-record rejection is tested field-by-field so header,
    // CRC, initials, ordering, read failure, and write failure cannot silently regress.
    constexpr std::size_t kLeaderboardRecordBytes = 812U;
    constexpr std::size_t kPixelLeaderboardOffset = 4096U;
    std::array<std::uint8_t, kLeaderboardRecordBytes> pixelRecord{};
    CHECK(storage.readBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));
    for (const std::size_t headerIndex : std::array<std::size_t, 6U>{{0U, 1U, 2U, 3U, 4U, 5U}}) {
        auto corrupt = pixelRecord;
        corrupt[headerIndex] = headerIndex == 5U ? 101U : static_cast<std::uint8_t>(corrupt[headerIndex] ^ 0x5AU);
        CHECK(storage.writeBytes(kPixelLeaderboardOffset, corrupt.data(), corrupt.size()));
        CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
        CHECK(storage.writeBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));
    }
    auto corruptLeaderboardCrc = pixelRecord;
    corruptLeaderboardCrc.back() ^= 0x01U;
    CHECK(storage.writeBytes(kPixelLeaderboardOffset, corruptLeaderboardCrc.data(), corruptLeaderboardCrc.size()));
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
    CHECK(storage.writeBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));

    for (const std::size_t initialByte : std::array<std::size_t, 3U>{{12U, 13U, 14U}}) {
        auto corrupt = pixelRecord;
        corrupt[initialByte] = static_cast<std::uint8_t>('?');
        writeTestUint32Le(corrupt.data() + kLeaderboardRecordBytes - 4U,
            testCrc32(corrupt.data(), kLeaderboardRecordBytes - 4U));
        CHECK(storage.writeBytes(kPixelLeaderboardOffset, corrupt.data(), corrupt.size()));
        CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
        CHECK(storage.writeBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));
    }
    auto unsorted = pixelRecord;
    writeTestUint32Le(unsorted.data() + 16U, 6000U);
    writeTestUint32Le(unsorted.data() + kLeaderboardRecordBytes - 4U,
        testCrc32(unsorted.data(), kLeaderboardRecordBytes - 4U));
    CHECK(storage.writeBytes(kPixelLeaderboardOffset, unsorted.data(), unsorted.size()));
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
    CHECK(storage.writeBytes(kPixelLeaderboardOffset, pixelRecord.data(), pixelRecord.size()));

    hal::PersistentStorage::failNextReadForTest();
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 1234U);
    std::array<char, 4U> invalidArcadeInitials{{'?','A','A','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, invalidArcadeInitials), -1);
    invalidArcadeInitials = {{'A','?','A','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, invalidArcadeInitials), -1);
    invalidArcadeInitials = {{'A','A','?','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, invalidArcadeInitials), -1);
    invalidArcadeInitials = {{'[','A','A','\0'}};
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, invalidArcadeInitials), -1);
    CHECK_EQ(pixelLeaderboard.insertAndSave(0U, topName), -1);
    hal::PersistentStorage::failNextWriteForTest();
    CHECK_EQ(pixelLeaderboard.insertAndSave(7000U, topName), -1);

    game::LeaderboardTable fullTable{};
    fullTable.count = 100U;
    for (std::size_t index = 0U; index < fullTable.entries.size(); ++index) {
        fullTable.entries[index].score = static_cast<std::uint32_t>(10000U - index * 50U);
    }
    CHECK_EQ(pixelLeaderboard.qualifyingRank(10001U, fullTable), 0);
    CHECK_EQ(pixelLeaderboard.qualifyingRank(9999U, fullTable), 1);
    CHECK_EQ(pixelLeaderboard.qualifyingRank(5051U, fullTable), 99);
    CHECK_EQ(pixelLeaderboard.qualifyingRank(100U, fullTable), -1);
    CHECK_EQ(pixelLeaderboard.qualifyingRank(0U, fullTable), -1);

    hal::OledDisplay display;
    CHECK(display.begin());
    hal::ControlPanel controls;
    controls.begin();
    hal::GateOutputDriver gates;
    gates.beginDisabled();

    // Every Easter-egg intro is user-controlled: it must never time out into gameplay.
    const std::array<game::ArcadeTitle, 5U> arcadeTitles{{
        game::ArcadeTitle::PixelRaid, game::ArcadeTitle::Formula1, game::ArcadeTitle::Breakout,
        game::ArcadeTitle::EggJourney, game::ArcadeTitle::Beatknecht}};
    for (const game::ArcadeTitle title : arcadeTitles) {
        game::ArcadeShell introShell(display, title, title == game::ArcadeTitle::Beatknecht ? nullptr : &pixelLeaderboard);
        introShell.begin(100U);
        introShell.render(100U);
        introShell.render(10'100U);
        hal::ControlSample idleIntroControls{};
        CHECK(introShell.update(idleIntroControls, 60'100U) == game::ArcadeShell::Action::None);
        CHECK(introShell.screen() == game::ArcadeShell::Screen::Intro);
        idleIntroControls.transportButton = pressedEdge();
        CHECK(introShell.update(idleIntroControls, 60'101U) == game::ArcadeShell::Action::None);
        CHECK(introShell.screen() == game::ArcadeShell::Screen::Intro);
    }

    // Shared arcade shell: individual intro -> play -> qualifying initials -> Top 100 -> restart.
    game::ArcadeShell shell(display, game::ArcadeTitle::PixelRaid, &pixelLeaderboard);
    shell.begin(100U);
    shell.render(900U);
    CHECK(shell.screen() == game::ArcadeShell::Screen::Intro);
    hal::ControlSample shellControls{};
    shellControls.tapButton = pressedEdge();
    CHECK(shell.update(shellControls, 901U) == game::ArcadeShell::Action::StartRun);
    CHECK(shell.playing());
    shell.finishRun(9000U);
    CHECK(shell.screen() == game::ArcadeShell::Screen::NameEntry);
    shellControls = {};
    shellControls.encoderDelta = -1;
    CHECK(shell.update(shellControls, 902U) == game::ArcadeShell::Action::None);
    shell.render(903U);
    shellControls.encoderDelta = 2;
    CHECK(shell.update(shellControls, 904U) == game::ArcadeShell::Action::None);
    shell.render(905U);
    for (int letter = 0; letter < 3; ++letter) {
        shellControls = {};
        shellControls.encoderButton = pressedEdge();
        (void)shell.update(shellControls, 910U + static_cast<std::uint32_t>(letter));
    }
    CHECK(shell.screen() == game::ArcadeShell::Screen::Leaderboard);
    CHECK_EQ(pixelLeaderboard.load().entries[0].score, 9000U);
    shell.render(920U);
    shellControls = {};
    shellControls.resetButton = pressedEdge();
    CHECK(shell.update(shellControls, 930U) == game::ArcadeShell::Action::RestartRun);
    CHECK(shell.playing());

    const std::array<char, 4U> aaa{{'A','A','A','\0'}};
    CHECK(pixelLeaderboard.insertAndSave(4500U, aaa) >= 0);
    CHECK(pixelLeaderboard.insertAndSave(4000U, aaa) >= 0);
    CHECK(pixelLeaderboard.insertAndSave(3500U, aaa) >= 0);
    CHECK(pixelLeaderboard.insertAndSave(3000U, aaa) >= 0);
    game::ArcadeShell scrollShell(display, game::ArcadeTitle::PixelRaid, &pixelLeaderboard);
    scrollShell.begin(940U);
    scrollShell.forceLeaderboardForTest(0U, -1);
    scrollShell.render(941U);
    const auto firstLeaderboardFrame = display.framebufferForTest();
    shellControls = {};
    shellControls.encoderDelta = 1;
    CHECK(scrollShell.update(shellControls, 942U) == game::ArcadeShell::Action::None);
    scrollShell.render(943U);
    CHECK(firstLeaderboardFrame != display.framebufferForTest());
    shellControls.encoderDelta = -100;
    CHECK(scrollShell.update(shellControls, 944U) == game::ArcadeShell::Action::None);
    shellControls.encoderDelta = 100;
    CHECK(scrollShell.update(shellControls, 945U) == game::ArcadeShell::Action::None);

    // Non-qualifying/empty and no-leaderboard paths remain stable and renderable.
    game::ArcadeShell emptyShell(display, game::ArcadeTitle::Breakout, &breakoutLeaderboard);
    emptyShell.begin(950U);
    hal::ControlSample emptyControls{};
    emptyControls.encoderButton = pressedEdge();
    CHECK(emptyShell.update(emptyControls, 951U) == game::ArcadeShell::Action::StartRun);
    emptyShell.finishRun(0U);
    CHECK(emptyShell.screen() == game::ArcadeShell::Screen::Leaderboard);
    emptyShell.render(952U);
    emptyControls = {};
    emptyControls.encoderDelta = 1;
    CHECK(emptyShell.update(emptyControls, 953U) == game::ArcadeShell::Action::None);

    game::ArcadeShell noStoreShell(display, static_cast<game::ArcadeTitle>(99U), nullptr);
    noStoreShell.begin(960U);
    noStoreShell.render(961U);
    noStoreShell.finishRun(123U);
    CHECK(noStoreShell.screen() == game::ArcadeShell::Screen::Intro);
    emptyControls = {};
    emptyControls.encoderButton = pressedEdge();
    CHECK(noStoreShell.update(emptyControls, 962U) == game::ArcadeShell::Action::StartRun);
    CHECK_EQ(noStoreShell.highScore(), 0U);

    game::PixelRaidGame pixelRaid(display, controls, gates, pixelLeaderboard);

    // Host run executes the real reset/render path without entering a blocking game loop.
    pixelRaid.run();
    game::PixelRaidGameTestAccess::reset(pixelRaid);
    game::PixelRaidGameTestAccess::render(pixelRaid);
    CHECK(game::PixelRaidGameTestAccess::chooseEnemyShotX(pixelRaid) >= 0);
    CHECK(game::PixelRaidGameTestAccess::hitAlien(pixelRaid, 7, 13));
    CHECK(!game::PixelRaidGameTestAccess::hitAlien(pixelRaid, 127, 63));

    hal::ControlSample sample{};
    sample.encoderDelta = 1;
    sample.tapButton = pressedEdge();
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 1000U);
    // A second TAP while the player projectile is active must not spawn another shot.
    sample = {}; sample.tapButton = pressedEdge();
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 1010U);
    sample = {};
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 2000U);
    game::PixelRaidGameTestAccess::forcePlayerShot(pixelRaid, 7, 15);
    game::PixelRaidGameTestAccess::clearAliensAndAdvance(pixelRaid, 3000U);

    // Exercise inactive/expired/missed shot and alien-motion branches.
    game::PixelRaidGameTestAccess::setPlayerShot(pixelRaid, false, 0, 0);
    game::PixelRaidGameTestAccess::updatePlayerShot(pixelRaid);
    game::PixelRaidGameTestAccess::setPlayerShot(pixelRaid, true, 120, 10);
    game::PixelRaidGameTestAccess::updatePlayerShot(pixelRaid);
    game::PixelRaidGameTestAccess::setPlayerShot(pixelRaid, true, 120, 30);
    game::PixelRaidGameTestAccess::updatePlayerShot(pixelRaid);
    game::PixelRaidGameTestAccess::setEnemyShot(pixelRaid, false, 0, 0, 4000U);
    game::PixelRaidGameTestAccess::updateEnemyShot(pixelRaid, 4500U);
    game::PixelRaidGameTestAccess::updateEnemyShot(pixelRaid, 6000U);
    game::PixelRaidGameTestAccess::setEnemyShot(pixelRaid, true, 0, 63);
    game::PixelRaidGameTestAccess::updateEnemyShot(pixelRaid, 6100U);
    game::PixelRaidGameTestAccess::setEnemyShot(pixelRaid, true, 0, 20);
    game::PixelRaidGameTestAccess::updateEnemyShot(pixelRaid, 6200U);
    game::PixelRaidGameTestAccess::setAlienMotion(pixelRaid, 7, 1, 7000U);
    game::PixelRaidGameTestAccess::updateAliens(pixelRaid, 7100U);
    game::PixelRaidGameTestAccess::setAlienMotion(pixelRaid, 120, 1, 0U);
    game::PixelRaidGameTestAccess::updateAliens(pixelRaid, 8000U);
    game::PixelRaidGameTestAccess::setAlienMotion(pixelRaid, 20, 1, 0U);
    game::PixelRaidGameTestAccess::updateAliens(pixelRaid, 9000U);
    game::PixelRaidGameTestAccess::setAlienAlive(pixelRaid, 0U, false);
    game::PixelRaidGameTestAccess::setPlayerShot(pixelRaid, true, 60, 30);
    game::PixelRaidGameTestAccess::setEnemyShot(pixelRaid, true, 10, 30);
    game::PixelRaidGameTestAccess::render(pixelRaid);
    game::PixelRaidGameTestAccess::clearAliens(pixelRaid);
    CHECK_EQ(game::PixelRaidGameTestAccess::chooseEnemyShotX(pixelRaid), 64);

    game::PixelRaidGameTestAccess::flashLifeLost(pixelRaid);
    CHECK_EQ(fakefw::pinValues[pinmap::kGateBufferOutputEnablePin], pinmap::kGateBufferDisabledLevel);
    for (const std::uint32_t pin : pinmap::kGateChannelPins) CHECK_EQ(fakefw::pinValues[pin], LOW);

    CHECK(game::PixelRaidGameTestAccess::confirmExit(pixelRaid));

    game::PixelRaidGameTestAccess::reset(pixelRaid);
    game::PixelRaidGameTestAccess::forceEnemyHit(pixelRaid, 2U, 10U, 4000U);
    CHECK_EQ(game::PixelRaidGameTestAccess::lives(pixelRaid), 1U);
    game::PixelRaidGameTestAccess::forceEnemyHit(pixelRaid, 1U, 10U, 5000U);
    CHECK(game::PixelRaidGameTestAccess::gameOver(pixelRaid));
    game::PixelRaidGameTestAccess::render(pixelRaid);
    sample = {};
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 5500U);
    CHECK(game::PixelRaidGameTestAccess::gameOver(pixelRaid));

    // Final death is handed to the shared arcade shell; gameplay no longer self-restarts.
    sample = {};
    sample.tapButton = pressedEdge();
    game::PixelRaidGameTestAccess::update(pixelRaid, sample, 6000U);
    CHECK(game::PixelRaidGameTestAccess::gameOver(pixelRaid));

    // Alternative compile-time Easter eggs share the same safe boot-only HAL boundary.
    game::Formula1Game formula1(display, controls, gates, formulaLeaderboard);
    formula1.run();
    CHECK(std::any_of(display.framebufferForTest().begin(), display.framebufferForTest().end(), [](const std::uint8_t value) { return value != 0U; }));
    CHECK_EQ(fakefw::pinValues[pinmap::kGateBufferOutputEnablePin], pinmap::kGateBufferDisabledLevel);

    // Formula 1 regression: world motion must stay encoder-playable, curves must alter the frame,
    // and lane dashes must never escape above the horizon into the central sky region.
    game::Formula1GameTestAccess::reset(formula1);
    hal::ControlSample formulaControls{};
    for (std::uint32_t nowMs = 55U; nowMs <= 5500U; nowMs += 55U) {
        game::Formula1GameTestAccess::update(formula1, formulaControls, nowMs);
    }
    CHECK(game::Formula1GameTestAccess::speed(formula1) <= 184U);
    CHECK(game::Formula1GameTestAccess::distance(formula1) > 250U);
    CHECK(game::Formula1GameTestAccess::distance(formula1) < 650U);
    game::Formula1GameTestAccess::setDistance(formula1, 0U);
    game::Formula1GameTestAccess::render(formula1);
    const auto straightFormulaFrame = display.framebufferForTest();
    game::Formula1GameTestAccess::setDistance(formula1, 700U);
    game::Formula1GameTestAccess::render(formula1);
    CHECK(display.framebufferForTest() != straightFormulaFrame);
    const auto& curveFrame = display.framebufferForTest();
    bool centralSkyIsClear = true;
    for (std::int16_t y = 4; y < 12; ++y) {
        for (std::int16_t x = 40; x < 96; ++x) {
            const std::size_t index = static_cast<std::size_t>(x) + static_cast<std::size_t>(y / 8) * static_cast<std::size_t>(hal::OledDisplay::kWidth);
            const std::uint8_t mask = static_cast<std::uint8_t>(1U << static_cast<std::uint8_t>(y & 7));
            if ((curveFrame[index] & mask) != 0U) centralSkyIsClear = false;
        }
    }
    CHECK(centralSkyIsClear);

    game::Formula1GameTestAccess::reset(formula1);
    game::Formula1GameTestAccess::forceCrash(formula1, 1000U);
    CHECK(game::Formula1GameTestAccess::crashed(formula1));
    CHECK_EQ(game::Formula1GameTestAccess::crashesRemaining(formula1), 2U);
    game::Formula1GameTestAccess::render(formula1);
    const auto crashFrame = display.framebufferForTest();
    game::Formula1GameTestAccess::update(formula1, formulaControls, 1900U);
    CHECK(!game::Formula1GameTestAccess::crashed(formula1));
    game::Formula1GameTestAccess::finishScore(formula1, 4321U);
    CHECK_EQ(formulaLeaderboard.qualifyingRank(4321U, formulaLeaderboard.load()), 0);
    CHECK(std::any_of(crashFrame.begin(), crashFrame.end(), [](const std::uint8_t value) { return value != 0U; }));

    game::BreakoutGame breakout(display, controls, gates, breakoutLeaderboard);
    breakout.run();
    CHECK(std::any_of(display.framebufferForTest().begin(), display.framebufferForTest().end(), [](const std::uint8_t value) { return value != 0U; }));
    CHECK_EQ(fakefw::pinValues[pinmap::kGateBufferOutputEnablePin], pinmap::kGateBufferDisabledLevel);

    // Breakout paddle zones must produce different rebound angles, and modifiers change the bat geometry.
    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setPaddleX(breakout, 50);
    game::BreakoutGameTestAccess::setBall(breakout, 52, 56, 1, 1);
    game::BreakoutGameTestAccess::update(breakout, formulaControls, 35U);
    const std::int8_t leftBounceX = game::BreakoutGameTestAccess::velocityX(breakout);
    const std::int8_t leftBounceY = game::BreakoutGameTestAccess::velocityY(breakout);
    game::BreakoutGameTestAccess::setBall(breakout, 68, 56, -1, 1);
    game::BreakoutGameTestAccess::update(breakout, formulaControls, 70U);
    CHECK_NE(game::BreakoutGameTestAccess::velocityX(breakout), leftBounceX);
    CHECK(leftBounceY < 0);
    CHECK(game::BreakoutGameTestAccess::velocityY(breakout) < 0);
    game::BreakoutGameTestAccess::applySmall(breakout, 100U);
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 12);
    game::BreakoutGameTestAccess::applyLarge(breakout, 200U);
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 30);
    game::BreakoutGameTestAccess::applyFast(breakout, 300U);
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 20);

    // Breakout deterministic edge matrix: launch/debounce, all walls, brick hit,
    // falling extras, lost balls, level clear, game-over guard, and render variants.
    game::BreakoutGameTestAccess::reset(breakout);
    hal::ControlSample breakoutControls{};
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 10U);
    CHECK(!game::BreakoutGameTestAccess::launched(breakout));
    breakoutControls.encoderDelta = -50;
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 11U);
    CHECK(game::BreakoutGameTestAccess::ballX(breakout) >= 2);
    breakoutControls = {};
    breakoutControls.tapButton = pressedEdge();
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 12U);
    CHECK(game::BreakoutGameTestAccess::launched(breakout));

    game::BreakoutGameTestAccess::setBall(breakout, 2, 30, -2, -1);
    game::BreakoutGameTestAccess::substep(breakout, 40U, true, false);
    CHECK(game::BreakoutGameTestAccess::velocityX(breakout) > 0);
    game::BreakoutGameTestAccess::setBall(breakout, 125, 30, 2, -1);
    game::BreakoutGameTestAccess::substep(breakout, 41U, true, false);
    CHECK(game::BreakoutGameTestAccess::velocityX(breakout) < 0);
    game::BreakoutGameTestAccess::setBall(breakout, 30, 2, 1, -2);
    game::BreakoutGameTestAccess::substep(breakout, 42U, false, true);
    CHECK(game::BreakoutGameTestAccess::velocityY(breakout) > 0);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::clearBricks(breakout);
    game::BreakoutGameTestAccess::setBrick(breakout, 0U, true);
    game::BreakoutGameTestAccess::setBall(breakout, 5, 5, 1, 1);
    game::BreakoutGameTestAccess::substep(breakout, 43U, false, false);
    CHECK(!game::BreakoutGameTestAccess::brick(breakout, 0U));
    CHECK_EQ(game::BreakoutGameTestAccess::score(breakout), 10U);

    // Exercise deterministic modifier spawn suppression and all three selector values.
    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setFalling(breakout, 1U, 10, 10, true);
    game::BreakoutGameTestAccess::maybeSpawn(breakout, 20, 20);
    CHECK(game::BreakoutGameTestAccess::fallingActive(breakout));
    std::array<bool, 4U> seenExtra{};
    for (std::uint32_t seed = 1U; seed < 20000U && !(seenExtra[1] && seenExtra[2] && seenExtra[3]); ++seed) {
        game::BreakoutGameTestAccess::reset(breakout);
        game::BreakoutGameTestAccess::setRandomState(breakout, seed);
        game::BreakoutGameTestAccess::maybeSpawn(breakout, 64, 20);
        if (game::BreakoutGameTestAccess::fallingActive(breakout)) {
            const std::uint8_t type = game::BreakoutGameTestAccess::fallingType(breakout);
            if (type < seenExtra.size()) seenExtra[type] = true;
        }
    }
    CHECK(seenExtra[1] && seenExtra[2] && seenExtra[3]);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setPaddleX(breakout, 50);
    game::BreakoutGameTestAccess::setFalling(breakout, 2U, 55, 56, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 1000U);
    CHECK(!game::BreakoutGameTestAccess::fallingActive(breakout));
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 30);
    game::BreakoutGameTestAccess::setFalling(breakout, 3U, 5, 63, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 1001U);
    CHECK(!game::BreakoutGameTestAccess::fallingActive(breakout));
    game::BreakoutGameTestAccess::updateExtra(breakout, 9001U);
    CHECK_EQ(game::BreakoutGameTestAccess::paddleWidth(breakout), 20);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setLives(breakout, 2U);
    game::BreakoutGameTestAccess::setBall(breakout, 20, 64, 1, 2);
    game::BreakoutGameTestAccess::substep(breakout, 1100U, false, false);
    CHECK_EQ(game::BreakoutGameTestAccess::lives(breakout), 1U);
    CHECK(!game::BreakoutGameTestAccess::gameOver(breakout));
    game::BreakoutGameTestAccess::setLives(breakout, 1U);
    game::BreakoutGameTestAccess::setBall(breakout, 20, 64, 1, 2);
    game::BreakoutGameTestAccess::substep(breakout, 1101U, false, false);
    CHECK_EQ(game::BreakoutGameTestAccess::lives(breakout), 0U);
    CHECK(game::BreakoutGameTestAccess::gameOver(breakout));
    breakoutControls = {};
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 1200U);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::clearBricks(breakout);
    game::BreakoutGameTestAccess::setLevel(breakout, 98U);
    game::BreakoutGameTestAccess::setBall(breakout, 64, 40, 1, -1, true);
    game::BreakoutGameTestAccess::setLastPhysicsAt(breakout, 0U);
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 2000U);
    CHECK_EQ(game::BreakoutGameTestAccess::level(breakout), 99U);
    CHECK(!game::BreakoutGameTestAccess::launched(breakout));
    game::BreakoutGameTestAccess::clearBricks(breakout);
    game::BreakoutGameTestAccess::setBall(breakout, 64, 40, 1, -1, true);
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 2040U);
    CHECK_EQ(game::BreakoutGameTestAccess::level(breakout), 99U);

    for (std::uint8_t type = 1U; type <= 3U; ++type) {
        game::BreakoutGameTestAccess::reset(breakout);
        game::BreakoutGameTestAccess::setFalling(breakout, type, 64, 30, true);
        game::BreakoutGameTestAccess::render(breakout);
        game::BreakoutGameTestAccess::setBall(breakout, 64, 40, 1, -1, true);
        game::BreakoutGameTestAccess::render(breakout);
    }

    // Remaining collision/control branches: paddle near-misses, already-launched
    // encoder motion, zero-life underflow guard, substep ratios, and empty render.
    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setPaddleX(breakout, 50);
    game::BreakoutGameTestAccess::setFalling(breakout, 1U, 49, 55, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 3000U);
    CHECK(game::BreakoutGameTestAccess::fallingActive(breakout));
    game::BreakoutGameTestAccess::setFalling(breakout, 1U, 71, 55, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 3001U);
    CHECK(game::BreakoutGameTestAccess::fallingActive(breakout));
    game::BreakoutGameTestAccess::setFalling(breakout, 1U, 5, 63, true);
    game::BreakoutGameTestAccess::updateExtra(breakout, 3002U);
    CHECK(!game::BreakoutGameTestAccess::fallingActive(breakout));

    game::BreakoutGameTestAccess::setBall(breakout, 55, 56, 1, 1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3008U, false, false);
    game::BreakoutGameTestAccess::setBall(breakout, 55, 61, 1, 1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3009U, false, false);
    game::BreakoutGameTestAccess::setBall(breakout, 55, 55, 1, -1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3010U, false, false);
    game::BreakoutGameTestAccess::setBall(breakout, 49, 55, 1, 1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3011U, false, false);
    game::BreakoutGameTestAccess::setBall(breakout, 71, 55, 1, 1, true);
    game::BreakoutGameTestAccess::substep(breakout, 3012U, false, false);

    breakoutControls = {};
    breakoutControls.encoderDelta = 1;
    const std::int16_t launchedBallBefore = game::BreakoutGameTestAccess::ballX(breakout);
    game::BreakoutGameTestAccess::setLastPhysicsAt(breakout, 3013U);
    game::BreakoutGameTestAccess::update(breakout, breakoutControls, 3013U);
    CHECK_EQ(game::BreakoutGameTestAccess::ballX(breakout), launchedBallBefore);
    game::BreakoutGameTestAccess::setBall(breakout, 20, 64, 1, 2, true);
    game::BreakoutGameTestAccess::setLives(breakout, 0U);
    game::BreakoutGameTestAccess::substep(breakout, 3014U, false, false);
    CHECK_EQ(game::BreakoutGameTestAccess::lives(breakout), 0U);

    game::BreakoutGameTestAccess::reset(breakout);
    game::BreakoutGameTestAccess::setLastPhysicsAt(breakout, 0U);
    game::BreakoutGameTestAccess::update(breakout, {}, 40U);
    CHECK(!game::BreakoutGameTestAccess::launched(breakout));
    game::BreakoutGameTestAccess::setBall(breakout, 80, 40, 2, 1, true);
    game::BreakoutGameTestAccess::setLastPhysicsAt(breakout, 0U);
    game::BreakoutGameTestAccess::update(breakout, {}, 40U);
    game::BreakoutGameTestAccess::setBall(breakout, 80, 40, 1, 2, true);
    game::BreakoutGameTestAccess::update(breakout, {}, 80U);
    game::BreakoutGameTestAccess::clearBricks(breakout);
    game::BreakoutGameTestAccess::render(breakout);

    // BEATKNECHT: TAP cycles styles, encoder adjusts tempo, all eight
    // gate rows are rendered, and the step engine drives real gate source pins.
    game::Beatknecht drummer(display, controls, gates);
    drummer.run();
    CHECK(std::any_of(display.framebufferForTest().begin(), display.framebufferForTest().end(), [](const std::uint8_t value) { return value != 0U; }));
    game::BeatknechtTestAccess::reset(drummer, 0U);
    CHECK_EQ(game::BeatknechtTestAccess::bpm(drummer), 128U);
    hal::ControlSample drumControls{};
    drumControls.tapButton = pressedEdge();
    game::BeatknechtTestAccess::update(drummer, drumControls, 1U);
    CHECK_EQ(game::BeatknechtTestAccess::style(drummer), 1U);
    CHECK_EQ(game::BeatknechtTestAccess::bpm(drummer), 128U);
    drumControls = {};
    drumControls.encoderDelta = 6;
    game::BeatknechtTestAccess::update(drummer, drumControls, 2U);
    CHECK_EQ(game::BeatknechtTestAccess::bpm(drummer), 134U);
    game::BeatknechtTestAccess::reset(drummer, 0U);
    drumControls = {};
    game::BeatknechtTestAccess::update(drummer, drumControls, 0U);
    CHECK_EQ(game::BeatknechtTestAccess::step(drummer), 0U);
    CHECK_EQ(fakefw::pinValues[pinmap::kChannel1GateLedPin], HIGH);

    // Egg Journey: the terrain auto-scrolls, encoder changes the egg's screen position,
    // TAP jumps, failures consume three lives, TAP RETRY requires a fresh press, and the
    // high score owns its own slot.
    game::MoonBuggyGame moonBuggy(display, controls, gates, eggLeaderboard);
    moonBuggy.run();
    CHECK(std::any_of(display.framebufferForTest().begin(), display.framebufferForTest().end(), [](const std::uint8_t value) { return value != 0U; }));
    CHECK_EQ(fakefw::pinValues[pinmap::kGateBufferOutputEnablePin], pinmap::kGateBufferDisabledLevel);
    game::MoonBuggyGameTestAccess::reset(moonBuggy);
    const std::int32_t cameraBefore = game::MoonBuggyGameTestAccess::cameraX(moonBuggy);
    const std::int16_t screenBefore = game::MoonBuggyGameTestAccess::screenX(moonBuggy);
    hal::ControlSample moonControls{};
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 45U);
    CHECK(game::MoonBuggyGameTestAccess::cameraX(moonBuggy) > cameraBefore);
    CHECK_EQ(game::MoonBuggyGameTestAccess::screenX(moonBuggy), screenBefore);
    game::MoonBuggyGameTestAccess::reset(moonBuggy);
    game::MoonBuggyGameTestAccess::setWorldX(moonBuggy, 3700);
    const std::int32_t advancedCameraBefore = game::MoonBuggyGameTestAccess::cameraX(moonBuggy);
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 45U);
    CHECK(game::MoonBuggyGameTestAccess::cameraX(moonBuggy) - advancedCameraBefore >= 2);
    game::MoonBuggyGameTestAccess::reset(moonBuggy);
    moonControls = {};
    moonControls.encoderDelta = 2;
    moonControls.tapButton = pressedEdge();
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 90U);
    CHECK(game::MoonBuggyGameTestAccess::screenX(moonBuggy) > screenBefore);
    CHECK(game::MoonBuggyGameTestAccess::jumpHeight(moonBuggy) > 0);
    game::MoonBuggyGameTestAccess::spawnAsteroid(moonBuggy, 1000U);
    CHECK(game::MoonBuggyGameTestAccess::asteroidActive(moonBuggy));
    game::MoonBuggyGameTestAccess::render(moonBuggy);
    const auto moonActionFrame = display.framebufferForTest();
    CHECK(std::any_of(moonActionFrame.begin(), moonActionFrame.end(), [](const std::uint8_t value) { return value != 0U; }));

    game::MoonBuggyGameTestAccess::reset(moonBuggy);
    CHECK_EQ(game::MoonBuggyGameTestAccess::lives(moonBuggy), 3U);
    CHECK(game::MoonBuggyGameTestAccess::craterDepth(moonBuggy, -18) >= 3);
    game::MoonBuggyGameTestAccess::setWorldX(moonBuggy, -18);
    moonControls = {};
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 45U);
    CHECK(!game::MoonBuggyGameTestAccess::gameOver(moonBuggy));
    CHECK(game::MoonBuggyGameTestAccess::awaitingRetry(moonBuggy));
    CHECK_EQ(game::MoonBuggyGameTestAccess::lives(moonBuggy), 2U);
    game::MoonBuggyGameTestAccess::render(moonBuggy);
    const auto brokenEggFrame = display.framebufferForTest();

    // TAP RETRY is explicitly release-armed, then accepts a new press.
    moonControls = {};
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 46U);
    moonControls.tapButton = pressedEdge();
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 47U);
    CHECK(!game::MoonBuggyGameTestAccess::awaitingRetry(moonBuggy));
    CHECK(!game::MoonBuggyGameTestAccess::gameOver(moonBuggy));
    CHECK_EQ(game::MoonBuggyGameTestAccess::lives(moonBuggy), 2U);

    game::MoonBuggyGameTestAccess::forceFailure(moonBuggy, true);
    CHECK_EQ(game::MoonBuggyGameTestAccess::lives(moonBuggy), 1U);
    CHECK(game::MoonBuggyGameTestAccess::awaitingRetry(moonBuggy));
    moonControls = {};
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 48U);
    moonControls.tapButton = pressedEdge();
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 49U);
    CHECK(!game::MoonBuggyGameTestAccess::awaitingRetry(moonBuggy));

    game::MoonBuggyGameTestAccess::forceFailure(moonBuggy, true);
    CHECK_EQ(game::MoonBuggyGameTestAccess::lives(moonBuggy), 0U);
    CHECK(game::MoonBuggyGameTestAccess::gameOver(moonBuggy));
    game::MoonBuggyGameTestAccess::render(moonBuggy);
    CHECK(display.framebufferForTest() != brokenEggFrame);

    // Final GAME OVER is handed to the shared arcade results flow, not the retry state.
    moonControls = {};
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 50U);
    moonControls.tapButton = pressedEdge();
    game::MoonBuggyGameTestAccess::update(moonBuggy, moonControls, 51U);
    CHECK(game::MoonBuggyGameTestAccess::gameOver(moonBuggy));
    CHECK_EQ(game::MoonBuggyGameTestAccess::lives(moonBuggy), 0U);

    game::MoonBuggyGameTestAccess::reset(moonBuggy);
    game::MoonBuggyGameTestAccess::setScore(moonBuggy, 7777U);
    CHECK_EQ(eggLeaderboard.qualifyingRank(7777U, eggLeaderboard.load()), 0);
    CHECK_EQ(formulaLeaderboard.load().entries[0].score, 2468U);
    CHECK_EQ(scoreStore.load().score, 1234U);

    const std::array<char, 4U> ids{{'I','D','S','\0'}};
    CHECK(formulaLeaderboard.insertAndSave(8000U, ids) >= 0);
    CHECK(breakoutLeaderboard.insertAndSave(8000U, ids) >= 0);
    CHECK(eggLeaderboard.insertAndSave(8000U, ids) >= 0);
}

void testApplicationAndEntryPoints() {
#if !CLOCK_DISPLAY_USE_SPI
    resetFakes(); // failure path, abort infinite safe halt via fake delay exception
    fakefw::throwOnDelay=true;
    try { app::ClockApplication failed; failed.begin(); CHECK(false); } catch(const std::runtime_error&) { CHECK(true); }
#endif
    resetFakes(); prepareDisplaySuccess(); fakefw::delayAdvanceOverrideMs = 333U;
    app::ClockApplication application; application.begin(); CHECK(fakefw::nowMs>=config::kBootDurationMs); application.runOnce();
    resetFakes(); prepareDisplaySuccess(); fakefw::delayAdvanceOverrideMs = 333U;
    ClockState stoppedState=makeDefaultState(); stoppedState.transport=TransportState::Stopped;
    app::ClockApplication stoppedApplication; stoppedApplication.begin(stoppedState); stoppedApplication.runOnce();
    CHECK(HardwareTimer::lastInstance!=nullptr); HardwareTimer::lastInstance->fire();

    // Regression: a persisted PLAY state is remembered as metadata, but power-up
    // must still enter STOP and must never emit a gate until the user explicitly plays.
    resetFakes();
    {
        hal::PersistentStorage seedStorage;
        services::PersistentStateService seedState(seedStorage);
        seedState.begin();
        seedState.requestTransportState(TransportState::Playing, 10U);
        seedState.service(10U + config::kPersistenceCommitDelayMs);
        CHECK(seedState.hasStoredTransportState());
        CHECK_EQ(seedState.storedTransportState(), TransportState::Playing);
    }
    prepareDisplaySuccess();
    fakefw::delayAdvanceOverrideMs = 333U;
    app::ClockApplication persistedPlayApplication;
    persistedPlayApplication.begin();
    CHECK(HardwareTimer::lastInstance != nullptr);

    // Confirm that begin() did not erase or silently rewrite the historical state.
    {
        hal::PersistentStorage verifyStorage;
        services::PersistentStateService verifyState(verifyStorage);
        verifyState.begin();
        CHECK_EQ(verifyState.storedTransportState(), TransportState::Playing);
    }

    fakefw::writes.clear();
    for (std::uint32_t tick = 0U; tick < 25000U; ++tick) {
        HardwareTimer::lastInstance->fire();
    }
    for (const std::uint32_t gatePin : pinmap::kGateChannelPins) {
        CHECK_EQ(fakefw::pinValues[gatePin], LOW);
        const bool emittedHighGate = std::any_of(
            fakefw::writes.begin(),
            fakefw::writes.end(),
            [gatePin](const fakefw::PinWrite& write) {
                return write.pin == gatePin && write.value == HIGH;
            });
        CHECK(!emittedHighGate);
    }

    // Framework glue functions use the translation-unit static application.
    resetFakes(); prepareDisplaySuccess(); setup(); loop(); CHECK(HardwareTimer::lastInstance!=nullptr);
}


} // namespace

int main() {
    testLocalizationAndFont();
    testDefaultsTemplatesAndServices();
    testPersistentStorageAndStateService();
    testPersistentV3Migration();
    testPersistentStateValidationBoundaries();
    testMenuModelAndFormatters();
    testHalBasicsAndDisplay();
    testControlPanel();
    testEngineAndSettingsEditor();
    testOperatingModesPersistenceAndGlobalEditors();
    testChannelRescheduleKeepsSharedMusicalEpoch();
    testCrossModeSynchronizationRegressions();
    testOneClockHumanizeAndTempoLimits();
    testEngineAuditRegressions();
    testEngineBoundaryBranches();
    renderEveryScreenAndState();
    testScreensaverRenderingAndPolicy();
    testUiControllerFlows();
    testEasterEggGameAndHighScore();
    testApplicationAndEntryPoints();
    std::cout << "Host firmware tests: " << gChecks << " checks, " << gFailures << " failures\n";
    return gFailures == 0 ? 0 : 1;
}
