/**
 * @file pixel_raid_game.h
 * @brief Hidden original fixed-shooter Easter egg for the 128x64 OLED.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstdint>

#include "game/arcade_leaderboard_store.h"
#include "game/arcade_shell.h"
#include "hal/control_panel.h"
#include "hal/gate_output_driver.h"
#include "hal/oled_display.h"

namespace clockfw::game {

/** @brief Runs the boot-only Pixel Raid Easter egg while all Eurorack gate outputs are disabled. */
class PixelRaidGame final {
public:
    /** @brief Constructs the game around existing display, controls, LED/gate HAL, and score storage. */
    PixelRaidGame(
        hal::OledDisplay& display,
        hal::ControlPanel& controls,
        hal::GateOutputDriver& gateOutputs,
        ArcadeLeaderboardStore& leaderboard);

    /** @brief Runs the game synchronously until the user confirms exit. */
    void run();

#ifdef CLOCK_SIMULATOR
    /** @brief Starts a non-blocking simulator session while keeping the output stage disabled. */
    void beginForSimulator();

    /**
     * @brief Services one non-blocking simulator game iteration.
     * @param nowMs Current virtual MCU time in milliseconds.
     * @return True while the game remains active.
     */
    bool serviceForSimulator(std::uint32_t nowMs);
#endif

private:
    friend struct PixelRaidGameTestAccess;

    static constexpr std::uint8_t kAlienColumns = 8U;
    static constexpr std::uint8_t kAlienRows = 3U;
    static constexpr std::uint8_t kAlienCount = kAlienColumns * kAlienRows;

    /** @brief Starts a fresh three-life session without touching the durable high score. */
    void resetSession();

    /** @brief Advances game physics from one debounced control sample. */
    void update(const hal::ControlSample& controls, std::uint32_t nowMs);

    /** @brief Renders the complete current game state to the OLED framebuffer. */
    void render();

    /** @brief Handles a player shot and alien collision. */
    void updatePlayerShot();

    /** @brief Advances/spawns the enemy projectile and detects player hits. */
    void updateEnemyShot(std::uint32_t nowMs);

    /** @brief Advances the alien formation and starts a new faster wave when cleared. */
    void updateAliens(std::uint32_t nowMs);

    /** @brief Flashes all activity LEDs with the external HCT244 disabled. */
    void flashLifeLostLeds();

    /** @brief Opens the in-game exit confirmation dialog and returns the selected decision. */
    bool confirmExit();

    /** @brief Returns true when one alive alien contains the supplied point. */
    bool hitAlien(std::int16_t x, std::int16_t y);

    /** @brief Returns the x coordinate of a deterministic alive alien column for enemy fire. */
    std::int16_t chooseEnemyShotX();

    /** @brief Draws one original compact alien glyph; no third-party game artwork is used. */
    void drawAlien(std::int16_t x, std::int16_t y) const;

    hal::OledDisplay& display_;
    hal::ControlPanel& controls_;
    hal::GateOutputDriver& gateOutputs_;
    ArcadeShell shell_;
    LeaderboardEntry highScore_{};

    std::array<bool, kAlienCount> aliens_{};
    std::int16_t playerX_ = 61;
    std::int16_t alienOffsetX_ = 7;
    std::int16_t alienOffsetY_ = 13;
    std::int8_t alienDirection_ = 1;
    std::uint8_t lives_ = 3U;
    std::uint8_t wave_ = 1U;
    std::uint32_t score_ = 0U;
    std::uint32_t randomState_ = 0x13579BDFUL;
    std::uint32_t lastAlienMoveAtMs_ = 0U;
    std::uint32_t lastEnemyShotAtMs_ = 0U;
    std::uint32_t lastProjectileUpdateAtMs_ = 0U;
#ifdef CLOCK_SIMULATOR
    std::uint32_t lastSimulatorFrameAtMs_ = 0U;
#endif
    std::uint32_t encoderPressedAtMs_ = 0U;
    bool playerShotActive_ = false;
    std::int16_t playerShotX_ = 0;
    std::int16_t playerShotY_ = 0;
    bool enemyShotActive_ = false;
    std::int16_t enemyShotX_ = 0;
    std::int16_t enemyShotY_ = 0;
    bool exitRequested_ = false;
    bool gameOver_ = false;
};

}  // namespace clockfw::game
