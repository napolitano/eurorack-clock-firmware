/**
 * @file formula1_game.cpp
 * @brief Boot-only monochrome pseudo-3D racer Easter egg implementation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "game/formula1_game.h"

#include <algorithm>
#include <array>
#include <cstdio>

#include "hal/system_clock.h"
#include "ui_text.h"

namespace clockfw::game {
namespace {
constexpr std::uint32_t kFrameIntervalMs = 45U;
constexpr std::uint32_t kExitLongPressMs = 900U;
constexpr std::uint32_t kCrashDurationMs = 820U;
constexpr std::int16_t kHorizonY = 12;
constexpr std::int16_t kRoadBottomY = 46;
constexpr std::int16_t kRoadLeftBottom = 18;
constexpr std::int16_t kRoadRightBottom = 109;
constexpr std::int16_t kRoadLeftTop = 54;
constexpr std::int16_t kRoadRightTop = 73;
constexpr std::int16_t kPlayerY = 38;
constexpr std::int16_t kPlayerBodyWidth = 12;
constexpr std::int16_t kPlayerBodyHeight = 8;
constexpr std::uint32_t kTrackSegmentLength = 180U;
constexpr std::array<std::int8_t, 16U> kCurveProfile{{0, 0, 4, 9, 14, 10, 5, 0, -5, -11, -15, -10, -4, 0, 5, 0}};
struct MiniTrackPoint final {
    std::int8_t x;
    std::int8_t y;
};

constexpr std::array<MiniTrackPoint, 18U> kMiniTrack{{
    {-10, 4}, {-12, 1}, {-11, -3}, {-7, -5}, {-3, -5}, {0, -2}, {3, -5}, {8, -5}, {11, -2},
    {11, 2}, {8, 5}, {4, 5}, {1, 3}, {-2, 5}, {-6, 5}, {-10, 4}, {-12, 1}, {-10, 4}
}};

std::int16_t clampToRange(const std::int16_t value, const std::int16_t low, const std::int16_t high) {
    return static_cast<std::int16_t>(std::clamp<int>(value, low, high));
}
std::int16_t baseInterpolate(const std::int16_t y, const std::int16_t a, const std::int16_t b) {
    const std::int16_t clampedY = clampToRange(y, kHorizonY, kRoadBottomY);
    const std::int32_t numerator = static_cast<std::int32_t>(clampedY - kHorizonY) * static_cast<std::int32_t>(b - a);
    return static_cast<std::int16_t>(a + numerator / (kRoadBottomY - kHorizonY));
}
std::int16_t curveStrengthAt(const std::uint32_t distance) {
    const std::size_t index = static_cast<std::size_t>((distance / kTrackSegmentLength) % kCurveProfile.size());
    const std::size_t next = (index + 1U) % kCurveProfile.size();
    const std::uint32_t local = distance % kTrackSegmentLength;
    return static_cast<std::int16_t>(
        static_cast<std::int32_t>(kCurveProfile[index]) +
        (static_cast<std::int32_t>(kCurveProfile[next]) - static_cast<std::int32_t>(kCurveProfile[index])) *
            static_cast<std::int32_t>(local) / static_cast<std::int32_t>(kTrackSegmentLength));
}
std::int16_t curveOffsetAt(const std::uint32_t distance, const std::int16_t y) {
    const std::int16_t clampedY = clampToRange(y, kHorizonY, kRoadBottomY);
    const std::int32_t remaining = static_cast<std::int32_t>(kRoadBottomY - clampedY);
    const std::int32_t span = static_cast<std::int32_t>(kRoadBottomY - kHorizonY);
    return static_cast<std::int16_t>(static_cast<std::int32_t>(curveStrengthAt(distance)) * remaining * remaining / (span * span));
}
std::int16_t roadLeftAt(const std::uint32_t distance, const std::int16_t y) {
    return static_cast<std::int16_t>(baseInterpolate(y, kRoadLeftTop, kRoadLeftBottom) + curveOffsetAt(distance, y));
}
std::int16_t roadRightAt(const std::uint32_t distance, const std::int16_t y) {
    return static_cast<std::int16_t>(baseInterpolate(y, kRoadRightTop, kRoadRightBottom) + curveOffsetAt(distance, y));
}
std::int16_t roadCenterAt(const std::uint32_t distance, const std::int16_t y) {
    return static_cast<std::int16_t>((roadLeftAt(distance, y) + roadRightAt(distance, y)) / 2);
}
std::int16_t roadWidthAt(const std::uint32_t distance, const std::int16_t y) {
    return static_cast<std::int16_t>(roadRightAt(distance, y) - roadLeftAt(distance, y));
}
std::int16_t laneCenterAt(const std::uint32_t distance, const std::int8_t lane, const std::int16_t y) {
    const std::int16_t spacing = static_cast<std::int16_t>(roadWidthAt(distance, y) / 4);
    return static_cast<std::int16_t>(roadCenterAt(distance, y) + static_cast<std::int16_t>(lane) * spacing);
}
std::int16_t trafficCarWidthAt(const std::int16_t y) {
    return static_cast<std::int16_t>(4 + std::clamp<int>((y - kHorizonY) / 7, 0, 7));
}
std::int16_t trafficCarHeightAt(const std::int16_t y) {
    return static_cast<std::int16_t>(3 + std::clamp<int>((y - kHorizonY) / 10, 0, 4));
}
void drawCloud(hal::OledDisplay& display, const std::int16_t x, const std::int16_t y) {
    display.drawHorizontalLine(static_cast<std::int16_t>(x + 1), y, 4);
    display.drawHorizontalLine(x, static_cast<std::int16_t>(y + 1), 7);
    display.drawHorizontalLine(static_cast<std::int16_t>(x + 2), static_cast<std::int16_t>(y + 2), 5);
}
void drawMountains(hal::OledDisplay& display) {
    display.drawLine(2, 11, 10, 7); display.drawLine(10, 7, 18, 11); display.drawLine(18, 11, 24, 8); display.drawLine(24, 8, 30, 11);
    display.drawLine(98, 11, 106, 8); display.drawLine(106, 8, 114, 11); display.drawLine(114, 11, 122, 7); display.drawLine(122, 7, 127, 11);
}
void drawPlayerCar(hal::OledDisplay& display, const std::int16_t centerX) {
    const std::int16_t x = static_cast<std::int16_t>(centerX - kPlayerBodyWidth / 2);
    const std::int16_t y = kPlayerY;
    display.drawLine(centerX, y, static_cast<std::int16_t>(x + 2), static_cast<std::int16_t>(y + 3));
    display.drawLine(centerX, y, static_cast<std::int16_t>(x + 9), static_cast<std::int16_t>(y + 3));
    display.fillRectangle(static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 3), 6, 2);
    display.drawRectangle(static_cast<std::int16_t>(x + 2), static_cast<std::int16_t>(y + 5), 8, 2);
    display.fillRectangle(x, static_cast<std::int16_t>(y + 3), 2, 3);
    display.fillRectangle(static_cast<std::int16_t>(x + 10), static_cast<std::int16_t>(y + 3), 2, 3);
    display.fillRectangle(static_cast<std::int16_t>(x + 1), static_cast<std::int16_t>(y + 7), 2, 2);
    display.fillRectangle(static_cast<std::int16_t>(x + 9), static_cast<std::int16_t>(y + 7), 2, 2);
}
void drawTrafficCar(hal::OledDisplay& display, const std::int16_t centerX, const std::int16_t y) {
    const std::int16_t width = trafficCarWidthAt(y);
    const std::int16_t height = trafficCarHeightAt(y);
    const std::int16_t x = static_cast<std::int16_t>(centerX - width / 2);
    display.drawLine(centerX, y, static_cast<std::int16_t>(x + 1), static_cast<std::int16_t>(y + 1));
    display.drawLine(centerX, y, static_cast<std::int16_t>(x + width - 2), static_cast<std::int16_t>(y + 1));
    display.drawRectangle(static_cast<std::int16_t>(x + 1), static_cast<std::int16_t>(y + 2), std::max<std::int16_t>(2, static_cast<std::int16_t>(width - 2)), std::max<std::int16_t>(1, static_cast<std::int16_t>(height - 1)));
    display.setPixel(x, static_cast<std::int16_t>(y + 2));
    display.setPixel(static_cast<std::int16_t>(x + width - 1), static_cast<std::int16_t>(y + 2));
}
void drawRoad(hal::OledDisplay& display, const std::uint32_t distance) {
    constexpr std::int16_t kStep = 3;
    std::int16_t lastY = kHorizonY;
    std::int16_t lastLeft = roadLeftAt(distance, lastY);
    std::int16_t lastRight = roadRightAt(distance, lastY);
    std::int16_t lastOuterLeft = static_cast<std::int16_t>(lastLeft - 3);
    std::int16_t lastOuterRight = static_cast<std::int16_t>(lastRight + 3);
    for (std::int16_t y = static_cast<std::int16_t>(kHorizonY + kStep); y <= kRoadBottomY; y = static_cast<std::int16_t>(y + kStep)) {
        const std::int16_t left = roadLeftAt(distance, y);
        const std::int16_t right = roadRightAt(distance, y);
        const std::int16_t shoulder = static_cast<std::int16_t>(3 + (y - kHorizonY) / 7);
        const std::int16_t outerLeft = static_cast<std::int16_t>(left - shoulder);
        const std::int16_t outerRight = static_cast<std::int16_t>(right + shoulder);
        display.drawLine(lastLeft, lastY, left, y); display.drawLine(lastRight, lastY, right, y);
        display.drawLine(lastOuterLeft, lastY, outerLeft, y); display.drawLine(lastOuterRight, lastY, outerRight, y);
        lastY = y; lastLeft = left; lastRight = right; lastOuterLeft = outerLeft; lastOuterRight = outerRight;
    }
}
void drawMiniTrack(hal::OledDisplay& display, const std::uint32_t distance) {
    constexpr std::int16_t originX = 63;
    constexpr std::int16_t originY = 57;
    for (std::size_t i = 1U; i < kMiniTrack.size(); ++i) {
        display.drawLine(
            static_cast<std::int16_t>(originX + kMiniTrack[i - 1U].x), static_cast<std::int16_t>(originY + kMiniTrack[i - 1U].y),
            static_cast<std::int16_t>(originX + kMiniTrack[i].x), static_cast<std::int16_t>(originY + kMiniTrack[i].y));
    }
    const std::size_t segment = static_cast<std::size_t>((distance / 90U) % (kMiniTrack.size() - 1U));
    const auto marker = kMiniTrack[segment];
    display.fillRectangle(static_cast<std::int16_t>(originX + marker.x - 1), static_cast<std::int16_t>(originY + marker.y - 1), 3, 3);
}
void drawCrashBurst(hal::OledDisplay& display, const std::int16_t centerX, const std::uint32_t phase) {
    const std::int16_t cy = 40;
    const std::int16_t radius = static_cast<std::int16_t>(4 + (phase % 4U) * 2U);
    display.drawLine(static_cast<std::int16_t>(centerX - radius), cy, static_cast<std::int16_t>(centerX + radius), cy);
    display.drawLine(centerX, static_cast<std::int16_t>(cy - radius), centerX, static_cast<std::int16_t>(cy + radius));
    display.drawLine(static_cast<std::int16_t>(centerX - radius + 2), static_cast<std::int16_t>(cy - radius + 2), static_cast<std::int16_t>(centerX + radius - 2), static_cast<std::int16_t>(cy + radius - 2));
    display.drawLine(static_cast<std::int16_t>(centerX + radius - 2), static_cast<std::int16_t>(cy - radius + 2), static_cast<std::int16_t>(centerX - radius + 2), static_cast<std::int16_t>(cy + radius - 2));
}
}  // namespace

Formula1Game::Formula1Game(
    hal::OledDisplay& display,
    hal::ControlPanel& controls,
    hal::GateOutputDriver& gateOutputs,
    ArcadeLeaderboardStore& leaderboard)
    : display_(display), controls_(controls), gateOutputs_(gateOutputs), shell_(display, ArcadeTitle::Formula1, &leaderboard) {}

void Formula1Game::run() {
    gateOutputs_.disableOutputStage(); gateOutputs_.setAllChannelsLow();
    const std::uint32_t startedAtMs = hal::SystemClock::milliseconds();
    shell_.begin(startedAtMs);
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
        if (action == ArcadeShell::Action::StartRun || action == ArcadeShell::Action::RestartRun) {
            highScore_.score = shell_.highScore(); resetSession();
        }
        if (shell_.playing()) update(controls, nowMs);
        if (nowMs - lastFrameAtMs >= kFrameIntervalMs) {
            if (shell_.playing()) render(); else shell_.render(nowMs);
            lastFrameAtMs = nowMs;
        }
        (void)display_.service();
        hal::SystemClock::delayMilliseconds(1U);
    }
    gateOutputs_.setAllChannelsLow(); gateOutputs_.disableOutputStage();
#endif
}

#ifdef CLOCK_SIMULATOR
void Formula1Game::beginForSimulator() {
    gateOutputs_.disableOutputStage(); gateOutputs_.setAllChannelsLow();
    const std::uint32_t nowMs = hal::SystemClock::milliseconds(); shell_.begin(nowMs);
    lastSimulatorFrameAtMs_ = nowMs; shell_.render(nowMs);
}
bool Formula1Game::serviceForSimulator(const std::uint32_t nowMs) {
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

void Formula1Game::resetSession() {
    for (Car& car : traffic_) car = {};
    playerX_ = 63; speed_ = 92U; distance_ = 0U; elapsedMs_ = 0U; score_ = 0U; lap_ = 1U; crashesRemaining_ = 3U;
    lastPhysicsAtMs_ = 0U; lastSpawnAtMs_ = 0U; encoderPressedAtMs_ = 0U; crashStartedAtMs_ = 0U;
    crashed_ = false; gameOver_ = false; scoreFinalized_ = false; exitRequested_ = false;
    spawnTraffic(); traffic_[0].y = 18; spawnTraffic(); traffic_[1].y = 27;
}

std::uint32_t Formula1Game::nextRandom() {
    randomState_ ^= randomState_ << 13U; randomState_ ^= randomState_ >> 17U; randomState_ ^= randomState_ << 5U; return randomState_;
}
void Formula1Game::spawnTraffic() {
    constexpr std::array<std::int8_t, 3U> lanes{{-1, 0, 1}};
    for (Car& car : traffic_) if (!car.active) { car.lane = lanes[nextRandom() % lanes.size()]; car.y = kHorizonY + 1; car.active = true; return; }
}
void Formula1Game::startCrash(const std::uint32_t nowMs) {
    crashed_ = true; crashStartedAtMs_ = nowMs; speed_ = 0U;
    if (crashesRemaining_ > 0U) --crashesRemaining_;
    score_ = score_ > 100U ? score_ - 100U : 0U;
}

void Formula1Game::update(const hal::ControlSample& controls, const std::uint32_t nowMs) {
    if (gameOver_) return;
    if (crashed_) {
        if (nowMs - crashStartedAtMs_ >= kCrashDurationMs) {
            crashed_ = false;
            if (crashesRemaining_ == 0U) { gameOver_ = true; finishScore(); return; }
            for (Car& car : traffic_) car = {};
            speed_ = 96U; spawnTraffic(); traffic_[0].y = 18;
        }
        return;
    }
    if (controls.encoderDelta != 0) {
        const std::int16_t leftLimit = static_cast<std::int16_t>(roadLeftAt(distance_, kPlayerY + 6) + 6);
        const std::int16_t rightLimit = static_cast<std::int16_t>(roadRightAt(distance_, kPlayerY + 6) - 6);
        playerX_ = clampToRange(static_cast<std::int16_t>(playerX_ + static_cast<std::int16_t>(controls.encoderDelta) * 3), leftLimit, rightLimit);
    }
    if (nowMs - lastPhysicsAtMs_ < kFrameIntervalMs) return;
    elapsedMs_ += lastPhysicsAtMs_ == 0U ? kFrameIntervalMs : nowMs - lastPhysicsAtMs_;
    lastPhysicsAtMs_ = nowMs;

    const std::uint32_t speedPhase = (distance_ / 300U) % 6U;
    const std::uint16_t target = (speedPhase == 1U || speedPhase == 4U) ? 166U : ((speedPhase == 2U || speedPhase == 5U) ? 142U : 184U);
    if (speed_ < target) speed_ = static_cast<std::uint16_t>(std::min<std::uint16_t>(target, static_cast<std::uint16_t>(speed_ + 3U)));
    else if (speed_ > target) speed_ = static_cast<std::uint16_t>(speed_ - 2U);
    distance_ += static_cast<std::uint32_t>(1U + speed_ / 55U);
    score_ = std::min<std::uint32_t>(999999U, score_ + 1U + speed_ / 72U);
    lap_ = static_cast<std::uint8_t>(1U + (distance_ / 2600U) % 9U);

    if (nowMs - lastSpawnAtMs_ > 1050U + (nextRandom() % 700U)) { spawnTraffic(); lastSpawnAtMs_ = nowMs; }
    const std::int16_t advance = static_cast<std::int16_t>(1 + speed_ / 90U);
    const std::int16_t playerLeft = static_cast<std::int16_t>(playerX_ - kPlayerBodyWidth / 2);
    const std::int16_t playerRight = static_cast<std::int16_t>(playerLeft + kPlayerBodyWidth - 1);
    for (Car& car : traffic_) {
        if (!car.active) continue;
        car.y = static_cast<std::int16_t>(car.y + advance);
        if (car.y > kRoadBottomY + 8) { car.active = false; score_ = std::min<std::uint32_t>(999999U, score_ + 35U); continue; }
        const std::int16_t centerX = laneCenterAt(distance_, car.lane, static_cast<std::int16_t>(car.y + 2));
        const std::int16_t width = trafficCarWidthAt(car.y);
        const std::int16_t height = trafficCarHeightAt(car.y);
        const std::int16_t left = static_cast<std::int16_t>(centerX - width / 2);
        const std::int16_t right = static_cast<std::int16_t>(left + width - 1);
        const std::int16_t bottom = static_cast<std::int16_t>(car.y + height + 1);
        if (right >= playerLeft && left <= playerRight && bottom >= kPlayerY && car.y <= kPlayerY + kPlayerBodyHeight) {
            car.active = false; startCrash(nowMs); break;
        }
    }
}

void Formula1Game::finishScore() {
    if (scoreFinalized_) return;
    shell_.finishRun(score_);
    highScore_.score = std::max<std::uint32_t>(highScore_.score, score_);
    scoreFinalized_ = true;
}



void Formula1Game::render() {
    display_.clear(); display_.setFont(hal::DisplayFont::Small); display_.setTextColor(hal::PixelColor::White);
    drawCloud(display_, 18, 2); drawCloud(display_, 66, 1); drawMountains(display_); display_.drawHorizontalLine(0, kHorizonY, 128); drawRoad(display_, distance_);
    const std::int16_t scroll = static_cast<std::int16_t>((distance_ / 2U) % 13U);
    std::int16_t firstY = static_cast<std::int16_t>(kHorizonY + 3 + (12 - scroll));
    while (firstY >= kHorizonY + 15) firstY = static_cast<std::int16_t>(firstY - 12);
    while (firstY < kHorizonY + 3) firstY = static_cast<std::int16_t>(firstY + 12);
    for (std::int16_t y = firstY; y < kRoadBottomY; y = static_cast<std::int16_t>(y + 12)) {
        const std::int16_t width = roadWidthAt(distance_, y);
        const std::int16_t dashHalf = std::max<std::int16_t>(1, static_cast<std::int16_t>(width / 18));
        const std::int16_t dashHeight = std::max<std::int16_t>(2, static_cast<std::int16_t>(width / 22));
        const std::int16_t dashY = std::max<std::int16_t>(static_cast<std::int16_t>(kHorizonY + 2), y);
        const std::int16_t clippedHeight = std::min<std::int16_t>(dashHeight, static_cast<std::int16_t>(kRoadBottomY - dashY));
        if (clippedHeight > 0) display_.fillRectangle(static_cast<std::int16_t>(roadCenterAt(distance_, dashY) - dashHalf), dashY, static_cast<std::int16_t>(dashHalf * 2 + 1), clippedHeight);
    }
    for (const Car& car : traffic_) if (car.active) drawTrafficCar(display_, laneCenterAt(distance_, car.lane, static_cast<std::int16_t>(car.y + 2)), car.y);
    if (crashed_) drawCrashBurst(display_, playerX_, (elapsedMs_ + crashStartedAtMs_) / 90U); else drawPlayerCar(display_, playerX_);

    display_.drawHorizontalLine(0, 48, 128);
    if (gameOver_) {
        display_.drawText(34, 20, text::get(text::TextId::GameOver));
        display_.drawText(31, 32, text::get(text::TextId::TapRetry));
    }
    char left1[12]{}; char left2[12]{}; char right1[12]{}; char right2[12]{};
    std::snprintf(left1, sizeof(left1), "T%03lu", static_cast<unsigned long>((elapsedMs_ / 1000U) % 1000U));
    std::snprintf(left2, sizeof(left2), "L%u V%03u", static_cast<unsigned>(lap_), static_cast<unsigned>(speed_));
    std::snprintf(right1, sizeof(right1), "S%05lu", static_cast<unsigned long>(score_ % 100000U));
    std::snprintf(right2, sizeof(right2), "H%05lu", static_cast<unsigned long>(highScore_.score % 100000U));
    display_.drawText(1, 50, left1); display_.drawText(1, 57, left2); display_.drawText(86, 50, right1); display_.drawText(86, 57, right2);
    drawMiniTrack(display_, distance_);
    for (std::uint8_t i = 0U; i < crashesRemaining_; ++i) display_.setPixel(static_cast<std::int16_t>(45 + i * 3), 62);
    display_.present();
}

}  // namespace clockfw::game
