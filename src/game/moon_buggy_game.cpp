/**
 * @file moon_buggy_game.cpp
 * @brief Boot-only lunar traversal Easter egg implementation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "game/moon_buggy_game.h"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>

#include "hal/system_clock.h"
#include "ui_text.h"

namespace clockfw::game {
namespace {
constexpr std::uint32_t kFrameIntervalMs = 45U;
constexpr std::uint32_t kExitLongPressMs = 900U;
constexpr std::int16_t kGroundY = 46;
constexpr std::int16_t kInitialPlayerScreenX = 42;
constexpr std::int16_t kMinPlayerScreenX = 20;
constexpr std::int16_t kMaxPlayerScreenX = 104;
constexpr std::int16_t kEggHeight = 11;
constexpr std::int16_t kPhysicsScale = 8;
constexpr std::int16_t kJumpImpulse = 27;
constexpr std::int16_t kGravity = 3;
constexpr std::int16_t kMaxHorizontalVelocity = 7;

std::int32_t floorDiv(const std::int32_t value, const std::int32_t divisor) {
    const std::int32_t quotient = value / divisor;
    const std::int32_t remainder = value % divisor;
    return remainder < 0 ? quotient - 1 : quotient;
}


std::uint8_t journeyStage(const std::int32_t worldX) {
    if (worldX <= 0) return 1U;
    const std::int32_t stage = 1 + worldX / 900;
    return static_cast<std::uint8_t>(std::min<std::int32_t>(8, stage));
}

std::int32_t autoScrollForStage(const std::uint8_t stage) {
    return stage >= 7U ? 3 : (stage >= 4U ? 2 : 1);
}

std::uint32_t hashCell(const std::int32_t cell) {
    std::uint32_t value = static_cast<std::uint32_t>(cell) ^ 0x9E3779B9U;
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    value *= 0x846CA68BU;
    value ^= value >> 16U;
    return value;
}

bool staticCraterForCell(const std::int32_t cell, std::int32_t& center, std::int16_t& radius) {
    if (cell == 0) return false;
    const std::uint32_t hash = hashCell(cell);
    const std::uint8_t stage = journeyStage(std::abs(cell) * 58);
    const std::uint32_t densityPercent = static_cast<std::uint32_t>(45U + (stage - 1U) * 3U);
    if (std::abs(cell) != 1 && (hash % 100U) >= densityPercent) return false;
    center = cell * 58 + 18 + static_cast<std::int32_t>((hash >> 8U) % 23U);
    radius = static_cast<std::int16_t>(5 + (hash >> 16U) % 5U + stage / 3U);
    return true;
}

std::int16_t craterDepth(const std::int32_t worldX, const std::int32_t center, const std::int16_t radius) {
    const std::int32_t delta = std::abs(worldX - center);
    if (delta >= radius) return 0;
    return static_cast<std::int16_t>((static_cast<std::int32_t>(radius) - delta) * 6 / radius);
}

}  // namespace

MoonBuggyGame::MoonBuggyGame(
    hal::OledDisplay& display,
    hal::ControlPanel& controls,
    hal::GateOutputDriver& gateOutputs,
    ArcadeLeaderboardStore& leaderboard)
    : display_(display), controls_(controls), gateOutputs_(gateOutputs), shell_(display, ArcadeTitle::EggJourney, &leaderboard) {}

void MoonBuggyGame::run() {
    gateOutputs_.disableOutputStage(); gateOutputs_.setAllChannelsLow();
    const std::uint32_t startedAtMs = hal::SystemClock::milliseconds(); shell_.begin(startedAtMs);
#ifdef CLOCK_HOST_TEST
    shell_.startImmediatelyForTest(); highScore_.score = shell_.highScore(); resetSession(); render(); return;
#else
    std::uint32_t lastFrameAtMs = startedAtMs;
    while (!exitRequested_) {
        const std::uint32_t nowMs = hal::SystemClock::milliseconds();
        const hal::ControlSample controls = controls_.sample(nowMs);
        if (controls.encoderButton.edge == hal::ButtonEdge::Pressed) encoderPressedAtMs_ = nowMs;
        if (!controls.encoderButton.pressed) encoderPressedAtMs_ = 0U;
        else if (encoderPressedAtMs_ != 0U && nowMs - encoderPressedAtMs_ >= kExitLongPressMs) exitRequested_ = true;
        const ArcadeShell::Action action = shell_.update(controls, nowMs);
        if (action == ArcadeShell::Action::StartRun || action == ArcadeShell::Action::RestartRun) { highScore_.score = shell_.highScore(); resetSession(); }
        if (shell_.playing()) update(controls, nowMs);
        if (nowMs - lastFrameAtMs >= kFrameIntervalMs) { if (shell_.playing()) render(); else shell_.render(nowMs); lastFrameAtMs = nowMs; }
        (void)display_.service();
        hal::SystemClock::delayMilliseconds(1U);
    }
    gateOutputs_.setAllChannelsLow(); gateOutputs_.disableOutputStage();
#endif
}

#ifdef CLOCK_SIMULATOR
void MoonBuggyGame::beginForSimulator() {
    gateOutputs_.disableOutputStage(); gateOutputs_.setAllChannelsLow();
    const std::uint32_t nowMs = hal::SystemClock::milliseconds(); shell_.begin(nowMs);
    lastSimulatorFrameAtMs_ = nowMs; shell_.render(nowMs);
}

bool MoonBuggyGame::serviceForSimulator(const std::uint32_t nowMs) {
    const hal::ControlSample controls = controls_.sample(nowMs);
    if (controls.encoderButton.edge == hal::ButtonEdge::Pressed) encoderPressedAtMs_ = nowMs;
    if (!controls.encoderButton.pressed) encoderPressedAtMs_ = 0U;
    else if (encoderPressedAtMs_ != 0U && nowMs - encoderPressedAtMs_ >= kExitLongPressMs) exitRequested_ = true;
    const ArcadeShell::Action action = shell_.update(controls, nowMs);
    if (action == ArcadeShell::Action::StartRun || action == ArcadeShell::Action::RestartRun) { highScore_.score = shell_.highScore(); resetSession(); }
    if (shell_.playing()) update(controls, nowMs);
    if (nowMs - lastSimulatorFrameAtMs_ >= kFrameIntervalMs) { if (shell_.playing()) render(); else shell_.render(nowMs); lastSimulatorFrameAtMs_ = nowMs; }
    if (exitRequested_) { gateOutputs_.setAllChannelsLow(); gateOutputs_.disableOutputStage(); }
    return !exitRequested_;
}
#endif

void MoonBuggyGame::resetSession() {
    for (ImpactCrater& crater : impactCraters_) crater = {};
    asteroid_ = {};
    nextImpactCraterSlot_ = 0U;
    cameraWorldX_ = 0;
    playerScreenX_ = kInitialPlayerScreenX;
    playerWorldX_ = cameraWorldX_ + playerScreenX_;
    furthestDistance_ = 0;
    horizontalVelocity_ = 0;
    jumpHeightFp_ = 0;
    jumpVelocityFp_ = 0;
    score_ = 0U;
    randomState_ = 0x4D4F4F4EU;
    lastPhysicsAtMs_ = 0U;
    lastAsteroidAtMs_ = 0U;
    nextAsteroidDelayMs_ = 5200U;
    impactFlashUntilMs_ = 0U;
    encoderPressedAtMs_ = 0U;
    lives_ = 3U;
    failureMode_ = FailureMode::None;
    gameOver_ = false;
    awaitingRetry_ = false;
    retryArmed_ = false;
    scoreFinalized_ = false;
    exitRequested_ = false;
}

std::uint32_t MoonBuggyGame::nextRandom() {
    randomState_ ^= randomState_ << 13U;
    randomState_ ^= randomState_ >> 17U;
    randomState_ ^= randomState_ << 5U;
    return randomState_;
}

void MoonBuggyGame::spawnAsteroid(const std::uint32_t nowMs) {
    if (asteroid_.active || gameOver_) return;
    const std::int32_t offset = 24 + static_cast<std::int32_t>(nextRandom() % 34U);
    const std::int32_t direction = (nextRandom() & 1U) != 0U ? 1 : -1;
    asteroid_.targetWorldX = playerWorldX_ + direction * offset;
    asteroid_.y = 3;
    asteroid_.active = true;
    lastAsteroidAtMs_ = nowMs;
    const std::uint8_t stage = journeyStage(cameraWorldX_);
    const std::uint32_t baseDelay = static_cast<std::uint32_t>(4800U - (stage - 1U) * 350U);
    nextAsteroidDelayMs_ = baseDelay + nextRandom() % 2200U;
}

void MoonBuggyGame::addImpactCrater(const std::int32_t worldX, const std::int16_t radius) {
    ImpactCrater& crater = impactCraters_[nextImpactCraterSlot_];
    crater.worldX = worldX;
    crater.radius = radius;
    crater.active = true;
    nextImpactCraterSlot_ = (nextImpactCraterSlot_ + 1U) % impactCraters_.size();
}

void MoonBuggyGame::impactAsteroid(const std::uint32_t nowMs) {
    if (!asteroid_.active) return;
    const std::int32_t target = asteroid_.targetWorldX;
    asteroid_.active = false;
    addImpactCrater(target, 9);
    impactFlashUntilMs_ = nowMs + 320U;
    const std::int32_t playerDelta = std::abs(playerWorldX_ - target);
    if (playerDelta <= 7) {
        fail(FailureMode::Flattened);
        return;
    }
    score_ = std::min<std::uint32_t>(999999U, score_ + 100U + static_cast<std::uint32_t>(journeyStage(cameraWorldX_)) * 25U);
}

std::int16_t MoonBuggyGame::craterDepthAt(const std::int32_t worldX) const {
    std::int16_t depth = 0;
    const std::int32_t cell = floorDiv(worldX, 58);
    for (std::int32_t offset = -1; offset <= 1; ++offset) {
        std::int32_t center = 0;
        std::int16_t radius = 0;
        if (staticCraterForCell(cell + offset, center, radius)) {
            depth = std::max(depth, craterDepth(worldX, center, radius));
        }
    }
    for (const ImpactCrater& crater : impactCraters_) {
        if (crater.active) depth = std::max(depth, craterDepth(worldX, crater.worldX, crater.radius));
    }
    return depth;
}

void MoonBuggyGame::fail(const FailureMode mode) {
    if (gameOver_ || awaitingRetry_) return;
    failureMode_ = mode;
    horizontalVelocity_ = 0;
    jumpVelocityFp_ = 0;
    asteroid_.active = false;
    if (lives_ > 0U) --lives_;
    retryArmed_ = false;
    if (lives_ == 0U) {
        gameOver_ = true;
        finishScore();
    } else {
        awaitingRetry_ = true;
    }
}

void MoonBuggyGame::respawnAfterFailure() {
    // Advance the camera enough that the crater which consumed the life is
    // behind the respawn point. This prevents an immediate second collision.
    cameraWorldX_ += 24;
    playerScreenX_ = kInitialPlayerScreenX;
    playerWorldX_ = cameraWorldX_ + playerScreenX_;
    horizontalVelocity_ = 0;
    jumpHeightFp_ = 0;
    jumpVelocityFp_ = 0;
    asteroid_ = {};
    failureMode_ = FailureMode::None;
    awaitingRetry_ = false;
    retryArmed_ = false;
    lastAsteroidAtMs_ = lastPhysicsAtMs_;
}

void MoonBuggyGame::update(const hal::ControlSample& controls, const std::uint32_t nowMs) {
    if (gameOver_) return;
    if (awaitingRetry_) {
        // A fresh physical press is required. Merely holding TAP across the
        // collision cannot accidentally consume the next life.
        if (!controls.tapButton.pressed) retryArmed_ = true;
        if (retryArmed_ && controls.tapButton.edge == hal::ButtonEdge::Pressed) respawnAfterFailure();
        return;
    }

    if (controls.encoderDelta != 0) {
        const int velocity = static_cast<int>(horizontalVelocity_) + static_cast<int>(controls.encoderDelta) * 2;
        horizontalVelocity_ = static_cast<std::int16_t>(std::clamp<int>(velocity, -kMaxHorizontalVelocity, kMaxHorizontalVelocity));
    }
    if (controls.tapButton.edge == hal::ButtonEdge::Pressed && jumpHeightFp_ == 0) {
        jumpVelocityFp_ = kJumpImpulse;
    }

    if (nowMs - lastPhysicsAtMs_ < kFrameIntervalMs) return;
    lastPhysicsAtMs_ = nowMs;

    const std::uint8_t stage = journeyStage(cameraWorldX_);
    cameraWorldX_ += autoScrollForStage(stage);
    playerScreenX_ = static_cast<std::int16_t>(std::clamp<int>(
        static_cast<int>(playerScreenX_) + static_cast<int>(horizontalVelocity_),
        kMinPlayerScreenX,
        kMaxPlayerScreenX));
    playerWorldX_ = cameraWorldX_ + playerScreenX_;
    if (horizontalVelocity_ > 0) --horizontalVelocity_;
    else if (horizontalVelocity_ < 0) ++horizontalVelocity_;

    if (jumpHeightFp_ > 0 || jumpVelocityFp_ > 0) {
        jumpHeightFp_ = static_cast<std::int16_t>(jumpHeightFp_ + jumpVelocityFp_);
        jumpVelocityFp_ = static_cast<std::int16_t>(jumpVelocityFp_ - kGravity);
        if (jumpHeightFp_ <= 0) {
            jumpHeightFp_ = 0;
            jumpVelocityFp_ = 0;
        }
    }

    if (cameraWorldX_ > furthestDistance_) {
        const std::uint32_t delta = static_cast<std::uint32_t>(cameraWorldX_ - furthestDistance_);
        furthestDistance_ = cameraWorldX_;
        score_ = std::min<std::uint32_t>(999999U, score_ + delta);
    }

    if (jumpHeightFp_ == 0 && craterDepthAt(playerWorldX_) >= 3) {
        fail(FailureMode::Broken);
        return;
    }

    if (!asteroid_.active && nowMs - lastAsteroidAtMs_ >= nextAsteroidDelayMs_) {
        spawnAsteroid(nowMs);
    }
    if (asteroid_.active) {
        const std::uint8_t asteroidStage = journeyStage(cameraWorldX_);
        const std::int16_t fallSpeed = static_cast<std::int16_t>(2 + (asteroidStage >= 4U ? 1 : 0) + (asteroidStage >= 7U ? 1 : 0));
        asteroid_.y = static_cast<std::int16_t>(asteroid_.y + fallSpeed);
        const std::int16_t playerBottom = static_cast<std::int16_t>(kGroundY - jumpHeightFp_ / kPhysicsScale);
        const std::int16_t playerTop = static_cast<std::int16_t>(playerBottom - kEggHeight + 1);
        const std::int32_t horizontalDistance = std::abs(playerWorldX_ - asteroid_.targetWorldX);
        if (horizontalDistance <= 5 && asteroid_.y + 4 >= playerTop && asteroid_.y <= playerBottom) {
            impactAsteroid(nowMs);
            return;
        }
        const std::int16_t targetSurface = static_cast<std::int16_t>(kGroundY + craterDepthAt(asteroid_.targetWorldX));
        if (asteroid_.y + 4 >= targetSurface) impactAsteroid(nowMs);
    }
}

void MoonBuggyGame::finishScore() {
    if (scoreFinalized_) return;
    shell_.finishRun(score_);
    highScore_.score = std::max<std::uint32_t>(highScore_.score, score_);
    scoreFinalized_ = true;
}





}  // namespace clockfw::game
