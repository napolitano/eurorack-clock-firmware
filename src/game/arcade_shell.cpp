/**
 * @file arcade_shell.cpp
 * @brief Shared retro arcade presentation and Top-100 state machine.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "game/arcade_shell.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "ui_text.h"

namespace clockfw::game {
namespace {
constexpr std::int16_t kMarqueeY = 55;
constexpr std::uint32_t kMarqueePixelMs = 55U;

text::TextId titleId(const ArcadeTitle title) {
    switch (title) {
        case ArcadeTitle::Formula1: return text::TextId::ArcadeFormula1Title;
        case ArcadeTitle::Breakout: return text::TextId::ArcadeBreakoutTitle;
        case ArcadeTitle::EggJourney: return text::TextId::ArcadeEggJourneyTitle;
        case ArcadeTitle::Beatknecht: return text::TextId::ArcadeBeatknechtTitle;
        case ArcadeTitle::PixelRaid:
        default: return text::TextId::ArcadePixelRaidTitle;
    }
}

text::TextId marqueeId(const ArcadeTitle title) {
    switch (title) {
        case ArcadeTitle::Formula1: return text::TextId::ArcadeFormula1Marquee;
        case ArcadeTitle::Breakout: return text::TextId::ArcadeBreakoutMarquee;
        case ArcadeTitle::EggJourney: return text::TextId::ArcadeEggJourneyMarquee;
        case ArcadeTitle::Beatknecht: return text::TextId::ArcadeBeatknechtMarquee;
        case ArcadeTitle::PixelRaid:
        default: return text::TextId::ArcadePixelRaidMarquee;
    }
}
}  // namespace

ArcadeShell::ArcadeShell(hal::OledDisplay& display, const ArcadeTitle title, ArcadeLeaderboardStore* const leaderboard)
    : display_(display), title_(title), leaderboard_(leaderboard) {}

void ArcadeShell::begin(const std::uint32_t nowMs) {
    table_ = leaderboard_ != nullptr ? leaderboard_->load() : LeaderboardTable{};
    screen_ = Screen::Intro;
    introStartedAtMs_ = nowMs;
    pendingScore_ = 0U;
    initials_ = {{'A', 'A', 'A', '\0'}};
    initialPosition_ = 0U;
    alphabetIndex_ = 0U;
    leaderboardScroll_ = 0U;
    highlightedRank_ = -1;
}

ArcadeShell::Action ArcadeShell::update(const hal::ControlSample& controls, const std::uint32_t /*nowMs*/) {
    if (screen_ == Screen::Intro) {
        const bool startRequested = controls.tapButton.edge == hal::ButtonEdge::Pressed ||
            controls.encoderButton.edge == hal::ButtonEdge::Pressed;
        if (startRequested) {
            screen_ = Screen::Playing;
            return Action::StartRun;
        }
        return Action::None;
    }

    if (screen_ == Screen::NameEntry) {
        if (controls.encoderDelta != 0) {
            const int next = static_cast<int>(alphabetIndex_) + static_cast<int>(controls.encoderDelta);
            const int size = static_cast<int>(sizeof(kInitialAlphabet) - 1U);
            alphabetIndex_ = static_cast<std::uint8_t>((next % size + size) % size);
            initials_[initialPosition_] = kInitialAlphabet[alphabetIndex_];
        }
        if (controls.encoderButton.edge == hal::ButtonEdge::Pressed) {
            ++initialPosition_;
            alphabetIndex_ = 0U;
            if (initialPosition_ >= 3U) {
                highlightedRank_ = leaderboard_ != nullptr ? leaderboard_->insertAndSave(pendingScore_, initials_) : -1;
                table_ = leaderboard_ != nullptr ? leaderboard_->load() : LeaderboardTable{};
                screen_ = Screen::Leaderboard;
                setLeaderboardScrollAround(highlightedRank_);
            }
        }
        return Action::None;
    }

    if (screen_ == Screen::Leaderboard) {
        if (controls.encoderDelta != 0 && table_.count > kVisibleLeaderboardRows) {
            const int maximum = static_cast<int>(table_.count - kVisibleLeaderboardRows);
            leaderboardScroll_ = static_cast<std::uint8_t>(std::clamp<int>(
                static_cast<int>(leaderboardScroll_) + static_cast<int>(controls.encoderDelta), 0, maximum));
        }
        const bool restart = controls.resetButton.edge == hal::ButtonEdge::Pressed;
        if (restart) {
            screen_ = Screen::Playing;
            highlightedRank_ = -1;
            return Action::RestartRun;
        }
    }
    return Action::None;
}

void ArcadeShell::finishRun(const std::uint32_t score) {
    if (leaderboard_ == nullptr) return;
    pendingScore_ = score;
    table_ = leaderboard_->load();
    const std::int16_t rank = leaderboard_->qualifyingRank(score, table_);
    if (rank >= 0) {
        screen_ = Screen::NameEntry;
        initials_ = {{'A', 'A', 'A', '\0'}};
        initialPosition_ = 0U;
        alphabetIndex_ = 0U;
        highlightedRank_ = rank;
    } else {
        screen_ = Screen::Leaderboard;
        highlightedRank_ = -1;
        leaderboardScroll_ = 0U;
    }
}

bool ArcadeShell::playing() const { return screen_ == Screen::Playing; }
std::uint32_t ArcadeShell::highScore() const { return table_.count == 0U ? 0U : table_.entries[0].score; }
ArcadeShell::Screen ArcadeShell::screen() const { return screen_; }

void ArcadeShell::render(const std::uint32_t nowMs) {
    switch (screen_) {
        case Screen::Intro: renderIntro(nowMs); break;
        case Screen::NameEntry: renderNameEntry(); break;
        case Screen::Leaderboard: renderLeaderboard(); break;
        case Screen::Playing: break;
    }
}

const char* ArcadeShell::titleText() const { return text::get(titleId(title_)); }
const char* ArcadeShell::marqueeText() const { return text::get(marqueeId(title_)); }

void ArcadeShell::drawCentered(const std::int16_t y, const char* const value) {
    const hal::TextBounds bounds = display_.measureText(value, 0, 0);
    display_.drawText(static_cast<std::int16_t>((hal::OledDisplay::kWidth - static_cast<std::int16_t>(bounds.width)) / 2), y, value);
}

void ArcadeShell::renderIntro(const std::uint32_t nowMs) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    const std::uint32_t elapsed = nowMs - introStartedAtMs_;

    if (title_ == ArcadeTitle::PixelRaid) {
        for (std::int16_t x = 4; x < 124; x = static_cast<std::int16_t>(x + 17)) display_.setPixel(x, static_cast<std::int16_t>(4 + (x * 7) % 22));
        display_.drawRectangle(13, 27, 8, 4); display_.drawRectangle(107, 30, 8, 4);
    } else if (title_ == ArcadeTitle::Formula1) {
        display_.drawLine(54, 27, 31, 48); display_.drawLine(73, 27, 96, 48);
        display_.drawVerticalLine(63, 30, 15); display_.drawHorizontalLine(57, 43, 13);
    } else if (title_ == ArcadeTitle::Breakout) {
        for (std::int16_t x = 22; x <= 94; x = static_cast<std::int16_t>(x + 12)) display_.drawRectangle(x, 29, 10, 5);
        display_.fillRectangle(static_cast<std::int16_t>(35 + (elapsed / 80U) % 50U), 42, 3, 3);
    } else if (title_ == ArcadeTitle::EggJourney) {
        display_.drawRectangle(58, 28, 12, 16); display_.setPixel(61, 34); display_.setPixel(66, 34);
        display_.drawLine(15, 46, 32, 40); display_.drawLine(32, 40, 47, 46); display_.drawLine(82, 46, 99, 39); display_.drawLine(99, 39, 116, 46);
    } else {
        for (std::int16_t index = 0; index < 8; ++index) {
            const std::int16_t height = static_cast<std::int16_t>(3 + ((elapsed / 90U + static_cast<std::uint32_t>(index * 3)) % 13U));
            display_.fillRectangle(static_cast<std::int16_t>(29 + index * 9), static_cast<std::int16_t>(46 - height), 5, height);
        }
    }

    drawCentered(8, titleText());
    if (title_ == ArcadeTitle::Beatknecht) drawCentered(19, text::get(text::TextId::ArcadeBeatknechtHint));
    else drawCentered(19, text::get(text::TextId::ArcadeStartHint));

    const char* const marquee = marqueeText();
    const hal::TextBounds marqueeBounds = display_.measureText(marquee, 0, 0);
    const std::int16_t travel = static_cast<std::int16_t>(hal::OledDisplay::kWidth + marqueeBounds.width);
    const std::int16_t offset = static_cast<std::int16_t>((elapsed / kMarqueePixelMs) % static_cast<std::uint32_t>(std::max<std::int16_t>(1, travel)));
    display_.drawHorizontalLine(0, 53, hal::OledDisplay::kWidth);
    display_.drawText(static_cast<std::int16_t>(hal::OledDisplay::kWidth - offset), kMarqueeY, marquee);
    display_.present();
}

void ArcadeShell::renderNameEntry() {
    display_.clear(); display_.setFont(hal::DisplayFont::Small); display_.setTextColor(hal::PixelColor::White);
    drawCentered(5, text::get(text::TextId::ArcadeNewTop100));
    char scoreText[20]{};
    std::snprintf(scoreText, sizeof(scoreText), text::get(text::TextId::ArcadeScoreFormat), static_cast<unsigned long>(pendingScore_));
    drawCentered(17, scoreText);
    char band[8]{};
    for (int offset = -3; offset <= 3; ++offset) {
        const int size = static_cast<int>(sizeof(kInitialAlphabet) - 1U);
        const int index = (static_cast<int>(alphabetIndex_) + offset + size) % size;
        band[offset + 3] = kInitialAlphabet[index];
    }
    band[7] = '\0'; display_.drawText(43, 31, band); display_.drawRectangle(60, 29, 8, 10);
    char name[4]{initials_[0], initials_[1], initials_[2], '\0'};
    display_.drawText(55, 45, name);
    if (initialPosition_ < 3U) display_.drawHorizontalLine(static_cast<std::int16_t>(55 + initialPosition_ * 6U), 53, 5);
    display_.present();
}

void ArcadeShell::setLeaderboardScrollAround(const std::int16_t rank) {
    if (rank < 0 || table_.count <= kVisibleLeaderboardRows) { leaderboardScroll_ = 0U; return; }
    const int maximum = static_cast<int>(table_.count - kVisibleLeaderboardRows);
    leaderboardScroll_ = static_cast<std::uint8_t>(std::clamp<int>(static_cast<int>(rank) - 2, 0, maximum));
}

void ArcadeShell::renderLeaderboard() {
    display_.clear(); display_.setFont(hal::DisplayFont::Small); display_.setTextColor(hal::PixelColor::White);
    display_.drawText(1, 0, text::get(text::TextId::ArcadeTop100));
    if (table_.count > 0U) {
        char range[16]{};
        const std::uint8_t first = static_cast<std::uint8_t>(leaderboardScroll_ + 1U);
        const std::uint8_t last = static_cast<std::uint8_t>(std::min<int>(table_.count, leaderboardScroll_ + kVisibleLeaderboardRows));
        std::snprintf(range, sizeof(range), text::get(text::TextId::ArcadeRangeFormat), static_cast<unsigned>(first), static_cast<unsigned>(last), static_cast<unsigned>(table_.count));
        const hal::TextBounds bounds = display_.measureText(range, 0, 0);
        display_.drawText(static_cast<std::int16_t>(127 - bounds.width), 0, range);
    }
    if (table_.count == 0U) {
        drawCentered(25, text::get(text::TextId::ArcadeNoScores));
    } else {
        for (std::uint8_t row = 0U; row < kVisibleLeaderboardRows; ++row) {
            const std::size_t index = static_cast<std::size_t>(leaderboardScroll_) + row;
            if (index >= table_.count) break;
            char line[24]{};
            std::snprintf(line, sizeof(line), text::get(text::TextId::ArcadeRankFormat),
                static_cast<unsigned>(index + 1U), table_.entries[index].initials.data(), static_cast<unsigned long>(table_.entries[index].score));
            const std::int16_t y = static_cast<std::int16_t>(9 + row * 9U);
            if (static_cast<std::int16_t>(index) == highlightedRank_) {
                display_.fillRectangle(0, static_cast<std::int16_t>(y - 1), 96, 9);
                display_.setTextColor(hal::PixelColor::Black); display_.drawText(1, y, line); display_.setTextColor(hal::PixelColor::White);
            } else display_.drawText(1, y, line);
        }
    }
    display_.drawHorizontalLine(0, 54, hal::OledDisplay::kWidth);
    drawCentered(56, text::get(text::TextId::ArcadeRestartHint));
    display_.present();
}

#ifdef CLOCK_HOST_TEST
void ArcadeShell::startImmediatelyForTest() { screen_ = Screen::Playing; }
void ArcadeShell::forceLeaderboardForTest(const std::uint32_t score, const std::int16_t highlightedRank) {
    pendingScore_ = score; table_ = leaderboard_ != nullptr ? leaderboard_->load() : LeaderboardTable{};
    highlightedRank_ = highlightedRank; screen_ = Screen::Leaderboard; setLeaderboardScrollAround(highlightedRank);
}
#endif

}  // namespace clockfw::game
