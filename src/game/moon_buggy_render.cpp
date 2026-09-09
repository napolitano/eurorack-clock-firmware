/**
 * @file moon_buggy_render.cpp
 * @brief Monochrome EGG JOURNEY scenery and sprite rendering.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "game/moon_buggy_game.h"

#include <array>
#include <cstdio>

#include "ui_text.h"

namespace clockfw::game {
namespace {
constexpr std::int16_t kGroundY = 46;
constexpr std::int16_t kEggHeight = 11;
constexpr std::int16_t kPhysicsScale = 8;

std::uint8_t journeyStage(const std::int32_t worldX) {
    if (worldX <= 0) return 1U;
    const std::int32_t stage = 1 + worldX / 900;
    return static_cast<std::uint8_t>(stage > 8 ? 8 : stage);
}

std::int16_t triangleWave(const std::int32_t value, const std::int16_t period) {
    const std::int32_t safePeriod = static_cast<std::int32_t>(period);
    std::int32_t phase = value % safePeriod;
    if (phase < 0) phase += safePeriod;
    const std::int32_t half = safePeriod / 2;
    const std::int32_t distance = phase <= half ? phase : safePeriod - phase;
    return static_cast<std::int16_t>(distance);
}

void drawBackground(hal::OledDisplay& display, const std::int32_t cameraWorldX) {
    constexpr std::array<std::int16_t, 7U> kStarBaseX{{7, 25, 49, 70, 91, 111, 124}};
    constexpr std::array<std::int16_t, 7U> kStarY{{10, 5, 12, 7, 3, 9, 14}};
    const std::int16_t starOffset = static_cast<std::int16_t>((cameraWorldX / 16) % hal::OledDisplay::kWidth);
    for (std::size_t index = 0U; index < kStarBaseX.size(); ++index) {
        std::int16_t x = static_cast<std::int16_t>(kStarBaseX[index] - starOffset);
        while (x < 0) x = static_cast<std::int16_t>(x + hal::OledDisplay::kWidth);
        while (x >= hal::OledDisplay::kWidth) x = static_cast<std::int16_t>(x - hal::OledDisplay::kWidth);
        display.setPixel(x, kStarY[index]);
    }

    auto drawRidge = [&display](const std::int32_t parallaxWorldX, const std::int16_t baseY, const std::int16_t amplitude, const std::int16_t period) {
        std::int16_t previousY = static_cast<std::int16_t>(baseY + triangleWave(parallaxWorldX / 5, period) * amplitude / period);
        for (std::int16_t x = 4; x < hal::OledDisplay::kWidth; x = static_cast<std::int16_t>(x + 4)) {
            const std::int32_t world = parallaxWorldX + static_cast<std::int32_t>(x) * 2;
            const std::int16_t y = static_cast<std::int16_t>(baseY + triangleWave(world / 7, period) * amplitude / period);
            display.drawLine(static_cast<std::int16_t>(x - 4), previousY, x, y);
            previousY = y;
        }
    };
    drawRidge(cameraWorldX / 6, 23, 7, 26);
    drawRidge(cameraWorldX / 3, 29, 9, 20);
}

void drawEgg(hal::OledDisplay& display, const std::int16_t centerX, const std::int16_t bottomY) {
    const std::int16_t topY = static_cast<std::int16_t>(bottomY - kEggHeight + 1);
    display.setPixel(centerX, topY);
    display.drawLine(centerX, topY, static_cast<std::int16_t>(centerX - 3), static_cast<std::int16_t>(topY + 4));
    display.drawLine(centerX, topY, static_cast<std::int16_t>(centerX + 3), static_cast<std::int16_t>(topY + 4));
    display.drawLine(static_cast<std::int16_t>(centerX - 3), static_cast<std::int16_t>(topY + 4), static_cast<std::int16_t>(centerX - 4), static_cast<std::int16_t>(topY + 8));
    display.drawLine(static_cast<std::int16_t>(centerX + 3), static_cast<std::int16_t>(topY + 4), static_cast<std::int16_t>(centerX + 4), static_cast<std::int16_t>(topY + 8));
    display.drawLine(static_cast<std::int16_t>(centerX - 4), static_cast<std::int16_t>(topY + 8), static_cast<std::int16_t>(centerX - 2), bottomY);
    display.drawLine(static_cast<std::int16_t>(centerX + 4), static_cast<std::int16_t>(topY + 8), static_cast<std::int16_t>(centerX + 2), bottomY);
    display.drawHorizontalLine(static_cast<std::int16_t>(centerX - 2), bottomY, 5);
    display.setPixel(static_cast<std::int16_t>(centerX - 2), static_cast<std::int16_t>(topY + 5));
    display.setPixel(static_cast<std::int16_t>(centerX + 2), static_cast<std::int16_t>(topY + 5));
    display.setPixel(static_cast<std::int16_t>(centerX - 1), static_cast<std::int16_t>(topY + 8));
    display.setPixel(centerX, static_cast<std::int16_t>(topY + 9));
    display.setPixel(static_cast<std::int16_t>(centerX + 1), static_cast<std::int16_t>(topY + 8));
}

void drawBrokenEgg(hal::OledDisplay& display, const std::int16_t centerX, const std::int16_t groundY) {
    display.drawLine(static_cast<std::int16_t>(centerX - 8), groundY, static_cast<std::int16_t>(centerX - 4), static_cast<std::int16_t>(groundY - 4));
    display.drawLine(static_cast<std::int16_t>(centerX - 4), static_cast<std::int16_t>(groundY - 4), centerX, groundY);
    display.drawLine(centerX, groundY, static_cast<std::int16_t>(centerX + 4), static_cast<std::int16_t>(groundY - 5));
    display.drawLine(static_cast<std::int16_t>(centerX + 4), static_cast<std::int16_t>(groundY - 5), static_cast<std::int16_t>(centerX + 8), groundY);
    display.setPixel(static_cast<std::int16_t>(centerX - 6), static_cast<std::int16_t>(groundY - 7));
    display.setPixel(static_cast<std::int16_t>(centerX + 6), static_cast<std::int16_t>(groundY - 8));
    display.setPixel(static_cast<std::int16_t>(centerX - 1), static_cast<std::int16_t>(groundY - 9));
}

void drawFlatEgg(hal::OledDisplay& display, const std::int16_t centerX, const std::int16_t groundY) {
    display.drawHorizontalLine(static_cast<std::int16_t>(centerX - 8), static_cast<std::int16_t>(groundY - 2), 17);
    display.drawHorizontalLine(static_cast<std::int16_t>(centerX - 5), static_cast<std::int16_t>(groundY - 4), 11);
    display.setPixel(static_cast<std::int16_t>(centerX - 2), static_cast<std::int16_t>(groundY - 3));
    display.setPixel(static_cast<std::int16_t>(centerX + 2), static_cast<std::int16_t>(groundY - 3));
}

void drawAsteroid(hal::OledDisplay& display, const std::int16_t x, const std::int16_t y) {
    display.drawRectangle(static_cast<std::int16_t>(x - 2), y, 5, 4);
    display.setPixel(static_cast<std::int16_t>(x - 1), static_cast<std::int16_t>(y - 2));
    display.setPixel(static_cast<std::int16_t>(x + 1), static_cast<std::int16_t>(y - 4));
    display.setPixel(static_cast<std::int16_t>(x + 2), static_cast<std::int16_t>(y - 6));
}

void drawImpactBurst(hal::OledDisplay& display, const std::int16_t x, const std::int16_t y) {
    display.drawLine(static_cast<std::int16_t>(x - 5), y, static_cast<std::int16_t>(x + 5), y);
    display.drawLine(x, static_cast<std::int16_t>(y - 5), x, static_cast<std::int16_t>(y + 2));
    display.drawLine(static_cast<std::int16_t>(x - 4), static_cast<std::int16_t>(y - 4), static_cast<std::int16_t>(x + 4), static_cast<std::int16_t>(y + 2));
    display.drawLine(static_cast<std::int16_t>(x + 4), static_cast<std::int16_t>(y - 4), static_cast<std::int16_t>(x - 4), static_cast<std::int16_t>(y + 2));
}
}  // namespace

void MoonBuggyGame::render() {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    char scoreText[12]{};
    char highText[12]{};
    std::snprintf(scoreText, sizeof(scoreText), "S%05lu", static_cast<unsigned long>(score_ % 100000U));
    std::snprintf(highText, sizeof(highText), "H%05lu", static_cast<unsigned long>(highScore_.score % 100000U));
    display_.drawText(1, 0, scoreText);
    char livesText[5]{};
    std::snprintf(livesText, sizeof(livesText), "L%u", static_cast<unsigned>(lives_));
    display_.drawText(58, 0, livesText);
    char stageText[7]{};
    std::snprintf(stageText, sizeof(stageText), "ST%u", static_cast<unsigned>(journeyStage(cameraWorldX_)));
    display_.drawText(72, 0, stageText);
    display_.drawText(94, 0, highText);

    const std::int32_t cameraWorldX = cameraWorldX_;
    drawBackground(display_, cameraWorldX);

    for (std::int16_t x = 0; x < hal::OledDisplay::kWidth; ++x) {
        const std::int32_t worldX = cameraWorldX + x;
        const std::int16_t surfaceY = static_cast<std::int16_t>(kGroundY + craterDepthAt(worldX));
        display_.setPixel(x, surfaceY);
        if ((x % 9) == 0 && surfaceY < 49) display_.setPixel(x, static_cast<std::int16_t>(surfaceY + 1));
    }

    if (asteroid_.active) {
        const std::int16_t asteroidX = static_cast<std::int16_t>(asteroid_.targetWorldX - cameraWorldX);
        if (asteroidX >= -4 && asteroidX <= hal::OledDisplay::kWidth + 4) {
            drawAsteroid(display_, asteroidX, asteroid_.y);
            const std::int16_t markerY = static_cast<std::int16_t>(kGroundY + craterDepthAt(asteroid_.targetWorldX) - 2);
            display_.setPixel(static_cast<std::int16_t>(asteroidX - 2), markerY);
            display_.setPixel(asteroidX, markerY);
            display_.setPixel(static_cast<std::int16_t>(asteroidX + 2), markerY);
        }
    }

    if (impactFlashUntilMs_ > lastPhysicsAtMs_) {
        for (const ImpactCrater& crater : impactCraters_) {
            if (!crater.active) continue;
            const std::int16_t x = static_cast<std::int16_t>(crater.worldX - cameraWorldX);
            if (x >= 0 && x < hal::OledDisplay::kWidth) drawImpactBurst(display_, x, kGroundY);
        }
    }

    const std::int16_t playerBottom = static_cast<std::int16_t>(kGroundY - jumpHeightFp_ / kPhysicsScale);
    if (failureMode_ == FailureMode::Broken) drawBrokenEgg(display_, playerScreenX_, kGroundY);
    else if (failureMode_ == FailureMode::Flattened) drawFlatEgg(display_, playerScreenX_, kGroundY);
    else drawEgg(display_, playerScreenX_, playerBottom);

    if (gameOver_) {
        display_.drawText(38, 14, text::get(text::TextId::GameOver));
        display_.drawText(35, 22, text::get(text::TextId::TapRetry));
    } else if (awaitingRetry_) {
        display_.drawText(35, 17, text::get(text::TextId::TapRetry));
    }
    display_.present();
}

}  // namespace clockfw::game
