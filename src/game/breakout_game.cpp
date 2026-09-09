/**
 * @file breakout_game.cpp
 * @brief Boot-only monochrome brick-breaker Easter egg implementation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "game/breakout_game.h"

#include <algorithm>
#include <cstdlib>
#include <cstdio>

#include "hal/system_clock.h"
#include "ui_text.h"

namespace clockfw::game {
namespace {
constexpr std::uint32_t kFrameIntervalMs = 35U;
constexpr std::uint32_t kExitLongPressMs = 900U;
constexpr std::uint32_t kExtraDurationMs = 8000U;
constexpr std::int16_t kPaddleY = 59;
constexpr std::int16_t kDefaultPaddleWidth = 20;
constexpr std::int16_t kSmallPaddleWidth = 12;
constexpr std::int16_t kLargePaddleWidth = 30;
constexpr std::int16_t kDefaultPaddleStep = 4;
constexpr std::int16_t kFastPaddleStep = 7;
constexpr std::int16_t kLeftWall = 0;
constexpr std::int16_t kRightWall = hal::OledDisplay::kWidth - 1;
constexpr std::int16_t kTopWall = 0;
constexpr std::int16_t kBrickWidth = 10;
constexpr std::int16_t kBrickHeight = 6;
}  // namespace

BreakoutGame::BreakoutGame(
    hal::OledDisplay& display,
    hal::ControlPanel& controls,
    hal::GateOutputDriver& gateOutputs,
    ArcadeLeaderboardStore& leaderboard)
    : display_(display), controls_(controls), gateOutputs_(gateOutputs), shell_(display, ArcadeTitle::Breakout, &leaderboard) {}

void BreakoutGame::run() {
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
void BreakoutGame::beginForSimulator() {
    gateOutputs_.disableOutputStage(); gateOutputs_.setAllChannelsLow();
    const std::uint32_t nowMs = hal::SystemClock::milliseconds(); shell_.begin(nowMs);
    lastSimulatorFrameAtMs_ = nowMs; shell_.render(nowMs);
}
bool BreakoutGame::serviceForSimulator(const std::uint32_t nowMs) {
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

void BreakoutGame::resetSession() {
    bricks_.fill(true); paddleWidth_ = kDefaultPaddleWidth; paddleStep_ = kDefaultPaddleStep; paddleX_ = 54;
    activeExtra_ = ExtraType::None; fallingExtra_ = {}; extraExpiresAtMs_ = 0U; lastPhysicsAtMs_ = 0U;
    encoderPressedAtMs_ = 0U; exitRequested_ = false; gameOver_ = false; score_ = 0U; lives_ = 3U; level_ = 1U; resetBall();
}

void BreakoutGame::resetBall() {
    ballLaunched_ = false;
    ballX_ = static_cast<std::int16_t>(paddleX_ + paddleWidth_ / 2);
    ballY_ = static_cast<std::int16_t>(kPaddleY - 3);
    velocityX_ = (nextRandom() & 1U) != 0U ? 1 : -1;
    velocityY_ = -2;
}

std::uint32_t BreakoutGame::nextRandom() {
    randomState_ ^= randomState_ << 13U; randomState_ ^= randomState_ >> 17U; randomState_ ^= randomState_ << 5U; return randomState_;
}

void BreakoutGame::bounceFromPaddle() {
    const std::int16_t relative = static_cast<std::int16_t>(ballX_ - paddleX_);
    const std::int16_t zone = static_cast<std::int16_t>(std::clamp<int>(
        static_cast<int>(relative) * 7 / std::max<std::int16_t>(1, paddleWidth_), 0, 6));
    constexpr std::array<std::int8_t, 7U> kHorizontal{{-2, -2, -1, 1, 1, 2, 2}};
    constexpr std::array<std::int8_t, 7U> kVertical{{-1, -1, -2, -2, -2, -1, -1}};
    velocityX_ = kHorizontal[static_cast<std::size_t>(zone)];
    velocityY_ = kVertical[static_cast<std::size_t>(zone)];
}

void BreakoutGame::maybeSpawnExtra(const std::int16_t x, const std::int16_t y) {
    if (fallingExtra_.active || (nextRandom() % 7U) != 0U) return;
    const std::uint32_t selector = nextRandom() % 3U;
    fallingExtra_.type = selector == 0U ? ExtraType::SmallBat : (selector == 1U ? ExtraType::LargeBat : ExtraType::FastBat);
    fallingExtra_.x = x; fallingExtra_.y = y; fallingExtra_.active = true;
}

void BreakoutGame::applyExtra(const ExtraType type, const std::uint32_t nowMs) {
    activeExtra_ = type; extraExpiresAtMs_ = nowMs + kExtraDurationMs;
    paddleWidth_ = type == ExtraType::SmallBat ? kSmallPaddleWidth : (type == ExtraType::LargeBat ? kLargePaddleWidth : kDefaultPaddleWidth);
    paddleStep_ = type == ExtraType::FastBat ? kFastPaddleStep : kDefaultPaddleStep;
    paddleX_ = static_cast<std::int16_t>(std::clamp<int>(paddleX_, 2, hal::OledDisplay::kWidth - paddleWidth_ - 2));
}

void BreakoutGame::updateExtra(const std::uint32_t nowMs) {
    if (activeExtra_ != ExtraType::None && nowMs >= extraExpiresAtMs_) {
        activeExtra_ = ExtraType::None; paddleWidth_ = kDefaultPaddleWidth; paddleStep_ = kDefaultPaddleStep;
        paddleX_ = static_cast<std::int16_t>(std::clamp<int>(paddleX_, 2, hal::OledDisplay::kWidth - paddleWidth_ - 2));
    }
    if (!fallingExtra_.active) return;
    fallingExtra_.y = static_cast<std::int16_t>(fallingExtra_.y + 1);
    if (fallingExtra_.y >= kPaddleY - 2 && fallingExtra_.x >= paddleX_ && fallingExtra_.x < paddleX_ + paddleWidth_) {
        applyExtra(fallingExtra_.type, nowMs); fallingExtra_ = {}; return;
    }
    if (fallingExtra_.y >= hal::OledDisplay::kHeight) fallingExtra_ = {};
}

void BreakoutGame::updateBallSubstep(const std::uint32_t nowMs, const bool moveX, const bool moveY) {
    if (moveX) ballX_ = static_cast<std::int16_t>(ballX_ + (velocityX_ > 0 ? 1 : -1));
    if (moveY) ballY_ = static_cast<std::int16_t>(ballY_ + (velocityY_ > 0 ? 1 : -1));

    if (ballX_ <= kLeftWall + 2) { ballX_ = kLeftWall + 2; velocityX_ = static_cast<std::int8_t>(std::abs(velocityX_)); }
    if (ballX_ >= kRightWall - 2) { ballX_ = kRightWall - 2; velocityX_ = static_cast<std::int8_t>(-std::abs(velocityX_)); }
    if (ballY_ <= kTopWall + 2) { ballY_ = kTopWall + 2; velocityY_ = static_cast<std::int8_t>(std::abs(velocityY_)); }

    if (velocityY_ > 0 && ballY_ >= kPaddleY - 2 && ballY_ <= kPaddleY + 1 && ballX_ >= paddleX_ && ballX_ < paddleX_ + paddleWidth_) {
        ballY_ = static_cast<std::int16_t>(kPaddleY - 3); bounceFromPaddle();
    }

    for (std::uint8_t row = 0U; row < kRows; ++row) {
        for (std::uint8_t column = 0U; column < kColumns; ++column) {
            const std::size_t index = static_cast<std::size_t>(row) * kColumns + column;
            if (!bricks_[index]) continue;
            const std::int16_t x = static_cast<std::int16_t>(4 + static_cast<int>(column) * kBrickWidth);
            const std::int16_t y = static_cast<std::int16_t>(4 + static_cast<int>(row) * kBrickHeight);
            if (ballX_ >= x && ballX_ < x + kBrickWidth - 1 && ballY_ >= y && ballY_ < y + kBrickHeight - 1) {
                bricks_[index] = false;
                score_ = std::min<std::uint32_t>(999999U, score_ + 10U * static_cast<std::uint32_t>(level_));
                velocityY_ = static_cast<std::int8_t>(-velocityY_);
                maybeSpawnExtra(static_cast<std::int16_t>(x + kBrickWidth / 2), static_cast<std::int16_t>(y + kBrickHeight));
                return;
            }
        }
    }

    if (ballY_ >= hal::OledDisplay::kHeight) {
        if (lives_ > 0U) --lives_;
        if (lives_ == 0U) {
            gameOver_ = true;
            ballLaunched_ = false;
            shell_.finishRun(score_);
            highScore_.score = std::max<std::uint32_t>(highScore_.score, score_);
        } else resetBall();
    }
    (void)nowMs;
}

void BreakoutGame::update(const hal::ControlSample& controls, const std::uint32_t nowMs) {
    if (gameOver_) return;
    if (controls.encoderDelta != 0) {
        paddleX_ = static_cast<std::int16_t>(std::clamp<int>(
            static_cast<int>(paddleX_) + static_cast<int>(controls.encoderDelta) * paddleStep_,
            2, hal::OledDisplay::kWidth - paddleWidth_ - 2));
        if (!ballLaunched_) ballX_ = static_cast<std::int16_t>(paddleX_ + paddleWidth_ / 2);
    }
    if (!ballLaunched_ && controls.tapButton.edge == hal::ButtonEdge::Pressed) ballLaunched_ = true;
    if (nowMs - lastPhysicsAtMs_ < kFrameIntervalMs) return;
    lastPhysicsAtMs_ = nowMs; updateExtra(nowMs);
    if (!ballLaunched_) return;

    const int absX = std::abs(static_cast<int>(velocityX_));
    const int absY = std::abs(static_cast<int>(velocityY_));
    const int steps = std::max(absX, absY);
    int xError = 0; int yError = 0;
    for (int step = 0; step < steps && ballLaunched_; ++step) {
        xError += absX; yError += absY;
        const bool moveX = xError >= steps; const bool moveY = yError >= steps;
        if (moveX) xError -= steps;
        if (moveY) yError -= steps;
        updateBallSubstep(nowMs, moveX, moveY);
    }

    bool anyBrick = false;
    for (const bool brick : bricks_) anyBrick = anyBrick || brick;
    if (!anyBrick) {
        score_ = std::min<std::uint32_t>(999999U, score_ + 500U * static_cast<std::uint32_t>(level_));
        if (level_ < 99U) ++level_;
        bricks_.fill(true); fallingExtra_ = {}; resetBall();
    }
}

void BreakoutGame::render() {
    display_.clear();
    display_.drawHorizontalLine(0, kTopWall, hal::OledDisplay::kWidth);
    display_.drawVerticalLine(kLeftWall, 0, hal::OledDisplay::kHeight);
    display_.drawVerticalLine(kRightWall, 0, hal::OledDisplay::kHeight);

    for (std::uint8_t row = 0U; row < kRows; ++row) {
        for (std::uint8_t column = 0U; column < kColumns; ++column) {
            const std::size_t index = static_cast<std::size_t>(row) * kColumns + column;
            if (!bricks_[index]) continue;
            const std::int16_t x = static_cast<std::int16_t>(4 + static_cast<int>(column) * kBrickWidth);
            const std::int16_t y = static_cast<std::int16_t>(4 + static_cast<int>(row) * kBrickHeight);
            display_.drawRectangle(x, y, kBrickWidth - 1, kBrickHeight - 1);
        }
    }

    if (fallingExtra_.active) {
        const std::int16_t x = fallingExtra_.x; const std::int16_t y = fallingExtra_.y;
        display_.drawRectangle(static_cast<std::int16_t>(x - 3), static_cast<std::int16_t>(y - 2), 7, 5);
        if (fallingExtra_.type == ExtraType::SmallBat) display_.drawHorizontalLine(static_cast<std::int16_t>(x - 1), y, 3);
        else if (fallingExtra_.type == ExtraType::LargeBat) display_.drawHorizontalLine(static_cast<std::int16_t>(x - 2), y, 5);
        else { display_.drawLine(static_cast<std::int16_t>(x - 1), static_cast<std::int16_t>(y - 1), static_cast<std::int16_t>(x + 1), y); display_.drawLine(static_cast<std::int16_t>(x + 1), y, static_cast<std::int16_t>(x - 1), static_cast<std::int16_t>(y + 1)); }
    }

    display_.fillRectangle(paddleX_, kPaddleY, paddleWidth_, 3);
    display_.fillRectangle(static_cast<std::int16_t>(ballX_ - 1), static_cast<std::int16_t>(ballY_ - 1), 3, 3);
    display_.setFont(hal::DisplayFont::Small); display_.setTextColor(hal::PixelColor::White);
    if (!ballLaunched_) display_.drawText(44, 43, text::get(text::TextId::BreakoutTapStart));
    char header[32]{};
    std::snprintf(header, sizeof(header), text::get(text::TextId::GameHeaderFormat),
        static_cast<unsigned long>(score_), static_cast<unsigned long>(highScore_.score), lives_);
    display_.drawText(2, 50, header);
    display_.present();
}

}  // namespace clockfw::game
