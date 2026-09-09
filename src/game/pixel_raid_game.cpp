/**
 * @file pixel_raid_game.cpp
 * @brief Hidden original fixed-shooter Easter egg for the 128x64 OLED.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "game/pixel_raid_game.h"

#include <algorithm>
#include <cstdio>
#include "hal/system_clock.h"
#include "ui_text.h"
namespace clockfw::game {
namespace {
constexpr std::uint32_t kFrameIntervalMs = 40U;
constexpr std::uint32_t kExitLongPressMs = 900U;
constexpr std::uint32_t kEnemyShotIntervalMs = 950U;
constexpr std::int16_t kPlayerY = 59;
constexpr std::int16_t kPlayerWidth = 7;
constexpr std::int16_t kAlienWidth = 5;
constexpr std::int16_t kAlienHeight = 3;
constexpr std::int16_t kAlienCellWidth = 14;
constexpr std::int16_t kAlienCellHeight = 7;
}  // namespace
PixelRaidGame::PixelRaidGame(
    hal::OledDisplay& display,
    hal::ControlPanel& controls,
    hal::GateOutputDriver& gateOutputs,
    ArcadeLeaderboardStore& leaderboard)
    : display_(display),
      controls_(controls),
      gateOutputs_(gateOutputs),
      shell_(display, ArcadeTitle::PixelRaid, &leaderboard) {}
void PixelRaidGame::run() {
    gateOutputs_.disableOutputStage();
    gateOutputs_.setAllChannelsLow();
    const std::uint32_t startedAtMs = hal::SystemClock::milliseconds();
    shell_.begin(startedAtMs);
#ifdef CLOCK_HOST_TEST
    shell_.startImmediatelyForTest();
    highScore_.score = shell_.highScore();
    resetSession();
    render();
    return;
#else
    std::uint32_t lastFrameAtMs = startedAtMs;
    while (!exitRequested_) {
        const std::uint32_t nowMs = hal::SystemClock::milliseconds();
        const hal::ControlSample controls = controls_.sample(nowMs);
        if (controls.encoderButton.edge == hal::ButtonEdge::Pressed) encoderPressedAtMs_ = nowMs;
        if (!controls.encoderButton.pressed) encoderPressedAtMs_ = 0U;
        else if (encoderPressedAtMs_ != 0U && nowMs - encoderPressedAtMs_ >= kExitLongPressMs) {
            encoderPressedAtMs_ = 0U;
            if (confirmExit()) { exitRequested_ = true; break; }
        }
        const ArcadeShell::Action action = shell_.update(controls, nowMs);
        if (action == ArcadeShell::Action::StartRun || action == ArcadeShell::Action::RestartRun) {
            highScore_.score = shell_.highScore();
            resetSession();
        }
        if (shell_.playing()) update(controls, nowMs);
        if (nowMs - lastFrameAtMs >= kFrameIntervalMs) {
            if (shell_.playing()) render(); else shell_.render(nowMs);
            lastFrameAtMs = nowMs;
        }
        (void)display_.service();
        hal::SystemClock::delayMilliseconds(1U);
    }
    gateOutputs_.setAllChannelsLow();
    gateOutputs_.disableOutputStage();
#endif
}
#ifdef CLOCK_SIMULATOR
void PixelRaidGame::beginForSimulator() {
    gateOutputs_.disableOutputStage(); gateOutputs_.setAllChannelsLow();
    const std::uint32_t nowMs = hal::SystemClock::milliseconds();
    shell_.begin(nowMs); lastSimulatorFrameAtMs_ = nowMs; shell_.render(nowMs);
}
bool PixelRaidGame::serviceForSimulator(const std::uint32_t nowMs) {
    const hal::ControlSample controls = controls_.sample(nowMs);
    if (controls.encoderButton.edge == hal::ButtonEdge::Pressed) encoderPressedAtMs_ = nowMs;
    if (!controls.encoderButton.pressed) encoderPressedAtMs_ = 0U;
    else if (encoderPressedAtMs_ != 0U && nowMs - encoderPressedAtMs_ >= kExitLongPressMs) exitRequested_ = true;
    const ArcadeShell::Action action = shell_.update(controls, nowMs);
    if (action == ArcadeShell::Action::StartRun || action == ArcadeShell::Action::RestartRun) {
        highScore_.score = shell_.highScore(); resetSession();
    }
    if (shell_.playing()) update(controls, nowMs);
    if (nowMs - lastSimulatorFrameAtMs_ >= kFrameIntervalMs) {
        if (shell_.playing()) render(); else shell_.render(nowMs);
        lastSimulatorFrameAtMs_ = nowMs;
    }
    if (!exitRequested_) return true;
    gateOutputs_.setAllChannelsLow(); gateOutputs_.disableOutputStage(); return false;
}
#endif

void PixelRaidGame::resetSession() {
    aliens_.fill(true);
    playerX_ = 61;
    alienOffsetX_ = 7;
    alienOffsetY_ = 13;
    alienDirection_ = 1;
    lives_ = 3U;
    wave_ = 1U;
    score_ = 0U;
    lastAlienMoveAtMs_ = 0U;
    lastEnemyShotAtMs_ = 0U;
    lastProjectileUpdateAtMs_ = 0U;
    playerShotActive_ = false;
    enemyShotActive_ = false;
    exitRequested_ = false;
    gameOver_ = false;
}

void PixelRaidGame::update(const hal::ControlSample& controls, const std::uint32_t nowMs) {
    if (gameOver_) return;

    if (controls.encoderDelta != 0) {
        playerX_ = static_cast<std::int16_t>(std::clamp<int>(
            static_cast<int>(playerX_) + static_cast<int>(controls.encoderDelta) * 4,
            1,
            hal::OledDisplay::kWidth - kPlayerWidth - 1));
    }

    if (controls.tapButton.edge == hal::ButtonEdge::Pressed && !playerShotActive_) {
        playerShotActive_ = true;
        playerShotX_ = static_cast<std::int16_t>(playerX_ + kPlayerWidth / 2);
        playerShotY_ = static_cast<std::int16_t>(kPlayerY - 2);
    }

    if (nowMs - lastProjectileUpdateAtMs_ >= kFrameIntervalMs) {
        lastProjectileUpdateAtMs_ = nowMs;
        updatePlayerShot();
        updateEnemyShot(nowMs);
    }
    updateAliens(nowMs);
}

void PixelRaidGame::updatePlayerShot() {
    if (!playerShotActive_) {
        return;
    }

    playerShotY_ = static_cast<std::int16_t>(playerShotY_ - 2);
    if (playerShotY_ < 9) {
        playerShotActive_ = false;
        return;
    }

    if (hitAlien(playerShotX_, playerShotY_)) {
        playerShotActive_ = false;
        score_ += 10U * wave_;
    }
}

void PixelRaidGame::updateEnemyShot(const std::uint32_t nowMs) {
    if (!enemyShotActive_) {
        if (nowMs - lastEnemyShotAtMs_ < kEnemyShotIntervalMs) {
            return;
        }
        enemyShotActive_ = true;
        enemyShotX_ = chooseEnemyShotX();
        enemyShotY_ = static_cast<std::int16_t>(alienOffsetY_ + kAlienRows * kAlienCellHeight);
        lastEnemyShotAtMs_ = nowMs;
        return;
    }

    enemyShotY_ = static_cast<std::int16_t>(enemyShotY_ + 2);
    if (enemyShotY_ >= hal::OledDisplay::kHeight) {
        enemyShotActive_ = false;
        return;
    }

    if (enemyShotY_ >= kPlayerY - 1 &&
        enemyShotX_ >= playerX_ && enemyShotX_ < playerX_ + kPlayerWidth) {
        enemyShotActive_ = false;
        if (lives_ > 0U) {
            --lives_;
        }
        flashLifeLostLeds();
        if (lives_ == 0U) {
            gameOver_ = true;
            shell_.finishRun(score_);
            highScore_.score = std::max<std::uint32_t>(highScore_.score, score_);
        }
    }
}

void PixelRaidGame::updateAliens(const std::uint32_t nowMs) {
    const std::uint32_t moveIntervalMs = std::max<std::uint32_t>(120U, 420U - wave_ * 20U);
    if (nowMs - lastAlienMoveAtMs_ < moveIntervalMs) {
        return;
    }
    lastAlienMoveAtMs_ = nowMs;

    bool anyAlive = false;
    for (const bool alive : aliens_) {
        anyAlive = anyAlive || alive;
    }
    if (!anyAlive) {
        ++wave_;
        aliens_.fill(true);
        alienOffsetX_ = 7;
        alienOffsetY_ = 13;
        alienDirection_ = 1;
        return;
    }

    const std::int16_t nextOffset = static_cast<std::int16_t>(alienOffsetX_ + alienDirection_ * 2);
    const std::int16_t formationWidth = static_cast<std::int16_t>(
        (kAlienColumns - 1U) * kAlienCellWidth + kAlienWidth);
    if (nextOffset < 1 || nextOffset + formationWidth >= hal::OledDisplay::kWidth - 1) {
        alienDirection_ = static_cast<std::int8_t>(-alienDirection_);
        alienOffsetY_ = static_cast<std::int16_t>(alienOffsetY_ + 2);
    } else {
        alienOffsetX_ = nextOffset;
    }
}

void PixelRaidGame::render() {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    char header[32]{};
    std::snprintf(header, sizeof(header), text::get(text::TextId::GameHeaderFormat),
        static_cast<unsigned long>(score_),
        static_cast<unsigned long>(highScore_.score),
        lives_);
    display_.drawText(1, 1, header);

    if (gameOver_) {
        display_.drawText(34, 24, text::get(text::TextId::GameOver));
        display_.drawText(31, 39, text::get(text::TextId::TapRetry));
        display_.present();
        return;
    }

    for (std::uint8_t row = 0U; row < kAlienRows; ++row) {
        for (std::uint8_t column = 0U; column < kAlienColumns; ++column) {
            const std::size_t index = static_cast<std::size_t>(row) * kAlienColumns + column;
            if (!aliens_[index]) {
                continue;
            }
            drawAlien(
                static_cast<std::int16_t>(alienOffsetX_ + column * kAlienCellWidth),
                static_cast<std::int16_t>(alienOffsetY_ + row * kAlienCellHeight));
        }
    }

    display_.drawHorizontalLine(playerX_, kPlayerY, kPlayerWidth);
    display_.drawHorizontalLine(static_cast<std::int16_t>(playerX_ + 2), kPlayerY - 1, 3);
    display_.drawVerticalLine(static_cast<std::int16_t>(playerX_ + 3), kPlayerY - 3, 3);

    if (playerShotActive_) {
        display_.drawVerticalLine(playerShotX_, playerShotY_, 3);
    }
    if (enemyShotActive_) {
        display_.drawVerticalLine(enemyShotX_, enemyShotY_, 2);
    }

    display_.present();
}

void PixelRaidGame::flashLifeLostLeds() {
    // LEDs are connected on the MCU side of the 74HCT244. Keeping /OE disabled
    // lets us animate those LEDs without driving a positive gate level onto the jacks.
    gateOutputs_.disableOutputStage();
    for (std::uint8_t cycle = 0U; cycle < 2U; ++cycle) {
        for (std::size_t channel = 0U; channel < 8U; ++channel) {
            gateOutputs_.setChannelState(channel, true);
        }
        hal::SystemClock::delayMilliseconds(70U);
        gateOutputs_.setAllChannelsLow();
        hal::SystemClock::delayMilliseconds(70U);
    }
}

bool PixelRaidGame::confirmExit() {
#ifdef CLOCK_HOST_TEST
    return true;
#else
    bool yesSelected = false;
    bool waitForRelease = true;
    while (true) {
        const std::uint32_t nowMs = hal::SystemClock::milliseconds();
        const hal::ControlSample controls = controls_.sample(nowMs);
        if (waitForRelease) {
            waitForRelease = controls.encoderButton.pressed;
        } else {
            if (controls.encoderDelta != 0) {
                yesSelected = !yesSelected;
            }
            if (controls.encoderButton.edge == hal::ButtonEdge::Pressed) {
                return yesSelected;
            }
        }

        display_.clear();
        display_.setFont(hal::DisplayFont::Small);
        display_.setTextColor(hal::PixelColor::White);
        display_.drawRectangle(22, 19, 84, 28);
        display_.drawText(39, 23, text::get(text::TextId::ExitGame));
        if (!yesSelected) {
            display_.fillRectangle(37, 35, 20, 9);
            display_.setTextColor(hal::PixelColor::Black);
        }
        display_.drawText(41, 36, text::get(text::TextId::No));
        display_.setTextColor(hal::PixelColor::White);
        if (yesSelected) {
            display_.fillRectangle(68, 35, 24, 9);
            display_.setTextColor(hal::PixelColor::Black);
        }
        display_.drawText(71, 36, text::get(text::TextId::Yes));
        display_.setTextColor(hal::PixelColor::White);
        display_.present();
        (void)display_.service();
        hal::SystemClock::delayMilliseconds(2U);
    }
#endif
}



bool PixelRaidGame::hitAlien(const std::int16_t x, const std::int16_t y) {
    for (std::uint8_t row = 0U; row < kAlienRows; ++row) {
        for (std::uint8_t column = 0U; column < kAlienColumns; ++column) {
            const std::size_t index = static_cast<std::size_t>(row) * kAlienColumns + column;
            if (!aliens_[index]) {
                continue;
            }
            const std::int16_t alienX = static_cast<std::int16_t>(alienOffsetX_ + column * kAlienCellWidth);
            const std::int16_t alienY = static_cast<std::int16_t>(alienOffsetY_ + row * kAlienCellHeight);
            if (x >= alienX && x < alienX + kAlienWidth &&
                y >= alienY && y < alienY + kAlienHeight) {
                aliens_[index] = false;
                return true;
            }
        }
    }
    return false;
}

std::int16_t PixelRaidGame::chooseEnemyShotX() {
    randomState_ ^= randomState_ << 13U;
    randomState_ ^= randomState_ >> 17U;
    randomState_ ^= randomState_ << 5U;

    const std::uint8_t startColumn = static_cast<std::uint8_t>(randomState_ % kAlienColumns);
    for (std::uint8_t offset = 0U; offset < kAlienColumns; ++offset) {
        const std::uint8_t column = static_cast<std::uint8_t>((startColumn + offset) % kAlienColumns);
        for (std::int8_t row = static_cast<std::int8_t>(kAlienRows) - 1; row >= 0; --row) {
            const std::size_t index = static_cast<std::size_t>(row) * kAlienColumns + column;
            if (aliens_[index]) {
                return static_cast<std::int16_t>(
                    alienOffsetX_ + column * kAlienCellWidth + kAlienWidth / 2);
            }
        }
    }
    return 64;
}

void PixelRaidGame::drawAlien(const std::int16_t x, const std::int16_t y) const {
    // Original five-by-three glyph: broad shoulders, two eyes, and a center foot.
    display_.drawHorizontalLine(x, y + 1, 5);
    display_.drawHorizontalLine(x + 1, y, 1);
    display_.drawHorizontalLine(x + 3, y, 1);
    display_.drawHorizontalLine(x + 2, y + 2, 1);
}

}  // namespace clockfw::game
