/**
 * @file arcade_shell.h
 * @brief Shared retro intro, name-entry, and Top-100 flow for boot Easter eggs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstdint>

#include "game/arcade_leaderboard_store.h"
#include "hal/control_panel.h"
#include "hal/oled_display.h"

namespace clockfw::game {

enum class ArcadeTitle : std::uint8_t { PixelRaid, Formula1, Breakout, EggJourney, Beatknecht };

class ArcadeShell final {
public:
    enum class Screen : std::uint8_t { Intro, Playing, NameEntry, Leaderboard };
    enum class Action : std::uint8_t { None, StartRun, RestartRun };

    /** @brief Constructs the shared presentation state for one Easter egg. */
    ArcadeShell(hal::OledDisplay& display, ArcadeTitle title, ArcadeLeaderboardStore* leaderboard);
    /** @brief Starts the individual retro intro and refreshes any durable ranking table. */
    void begin(std::uint32_t nowMs);
    /** @brief Advances intro, initials, leaderboard scrolling, and restart controls. */
    Action update(const hal::ControlSample& controls, std::uint32_t nowMs);
    /** @brief Renders whichever non-gameplay arcade screen currently owns the OLED. */
    void render(std::uint32_t nowMs);
    /** @brief Routes a completed score to initials entry or directly to the Top-100 list. */
    void finishRun(std::uint32_t score);
    /** @brief Returns true while the game implementation owns controls and rendering. */
    bool playing() const;
    /** @brief Returns the current durable rank-one score, or zero when the list is empty. */
    std::uint32_t highScore() const;
    /** @brief Returns the active shared arcade screen for tests and game integration. */
    Screen screen() const;
#ifdef CLOCK_HOST_TEST
    /** @brief Bypasses the interactive intro in deterministic host gameplay tests. */
    void startImmediatelyForTest();
    /** @brief Places the shell on the leaderboard for deterministic screenshot coverage. */
    void forceLeaderboardForTest(std::uint32_t score, std::int16_t highlightedRank);
#endif

private:
    static constexpr std::uint8_t kVisibleLeaderboardRows = 5U;
    static constexpr char kInitialAlphabet[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    /** @brief Renders the game-specific retro splash and horizontally scrolling message. */
    void renderIntro(std::uint32_t nowMs);
    /** @brief Renders the shared three-initial Top-100 entry editor. */
    void renderNameEntry();
    /** @brief Renders one scrollable five-row window into the Top-100 table. */
    void renderLeaderboard();
    /** @brief Resolves the localized title for the selected Easter egg. */
    const char* titleText() const;
    /** @brief Resolves the localized marquee sentence for the selected Easter egg. */
    const char* marqueeText() const;
    /** @brief Draws one small-font string horizontally centered on the OLED. */
    void drawCentered(std::int16_t y, const char* value);
    /** @brief Positions the leaderboard window around a newly inserted rank. */
    void setLeaderboardScrollAround(std::int16_t rank);

    hal::OledDisplay& display_;
    ArcadeTitle title_;
    ArcadeLeaderboardStore* leaderboard_;
    LeaderboardTable table_{};
    Screen screen_ = Screen::Intro;
    std::uint32_t introStartedAtMs_ = 0U;
    std::uint32_t pendingScore_ = 0U;
    std::array<char, 4U> initials_{{'A', 'A', 'A', '\0'}};
    std::uint8_t initialPosition_ = 0U;
    std::uint8_t alphabetIndex_ = 0U;
    std::uint8_t leaderboardScroll_ = 0U;
    std::int16_t highlightedRank_ = -1;
};

}  // namespace clockfw::game
