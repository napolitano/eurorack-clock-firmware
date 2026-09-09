/**
 * @file formula1_game.h
 * @brief Boot-only monochrome pseudo-3D lane racer Easter egg.
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

/** @brief Automatic-speed pseudo-3D racer with traffic, crashes, laps, and a durable high score. */
class Formula1Game final {
public:
    /** @brief Constructs the game around the shared display, controls, safe gate HAL, and score slot. */
    Formula1Game(
        hal::OledDisplay& display,
        hal::ControlPanel& controls,
        hal::GateOutputDriver& gateOutputs,
        ArcadeLeaderboardStore& leaderboard);
    /** @brief Runs the selected boot game synchronously until exit. */
    void run();
#ifdef CLOCK_SIMULATOR
    /** @brief Starts a non-blocking simulator game session. */
    void beginForSimulator();
    /** @brief Services one simulator frame and returns true while the game remains active. */
    bool serviceForSimulator(std::uint32_t nowMs);
#endif

private:
    friend struct Formula1GameTestAccess;
    struct Car final { std::int8_t lane = 0; std::int16_t y = -20; bool active = false; };

    /** @brief Restores player, speed, traffic, crashes, and score state for a new run. */
    void resetSession();
    /** @brief Advances steering, automatic speed, traffic, crash recovery, and score state. */
    void update(const hal::ControlSample& controls, std::uint32_t nowMs);
    /** @brief Renders the complete current racer frame to the 128x64 OLED. */
    void render();
    /** @brief Activates one deterministic pseudo-random traffic vehicle. */
    void spawnTraffic();
    /** @brief Enters the visible collision animation and consumes one crash allowance. */
    void startCrash(std::uint32_t nowMs);
    /** @brief Routes the completed score into the shared arcade ranking flow. */
    void finishScore();
    /** @brief Advances the local deterministic PRNG used only for game traffic. */
    std::uint32_t nextRandom();

    hal::OledDisplay& display_;
    hal::ControlPanel& controls_;
    hal::GateOutputDriver& gateOutputs_;
    ArcadeShell shell_;
    LeaderboardEntry highScore_{};
    std::array<Car, 4U> traffic_{};
    std::int16_t playerX_ = 61;
    std::uint16_t speed_ = 0U;
    std::uint32_t distance_ = 0U;
    std::uint32_t elapsedMs_ = 0U;
    std::uint32_t score_ = 0U;
    std::uint8_t lap_ = 1U;
    std::uint8_t crashesRemaining_ = 3U;
    std::uint32_t randomState_ = 0xC064F1A5U;
    std::uint32_t lastPhysicsAtMs_ = 0U;
    std::uint32_t lastSpawnAtMs_ = 0U;
    std::uint32_t encoderPressedAtMs_ = 0U;
    std::uint32_t crashStartedAtMs_ = 0U;
#ifdef CLOCK_SIMULATOR
    std::uint32_t lastSimulatorFrameAtMs_ = 0U;
#endif
    bool crashed_ = false;
    bool gameOver_ = false;
    bool scoreFinalized_ = false;
    bool exitRequested_ = false;
};

}  // namespace clockfw::game
