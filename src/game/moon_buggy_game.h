/**
 * @file moon_buggy_game.h
 * @brief Boot-only monochrome lunar traversal Easter egg starring a fragile egg.
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

/** @brief Lunar obstacle game with craters, falling asteroids, jumping, and a durable high score. */
class MoonBuggyGame final {
public:
    /** @brief Constructs the game around the shared display, controls, safe gate HAL, and score slot. */
    MoonBuggyGame(
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
    friend struct MoonBuggyGameTestAccess;

    enum class FailureMode : std::uint8_t { None, Broken, Flattened };

    struct ImpactCrater final {
        std::int32_t worldX = 0;
        std::int16_t radius = 0;
        bool active = false;
    };

    struct Asteroid final {
        std::int32_t targetWorldX = 0;
        std::int16_t y = 0;
        bool active = false;
    };

    /** @brief Restores player, terrain-event, and score state for a fresh run. */
    void resetSession();
    /** @brief Advances horizontal movement, jump physics, crater collisions, and asteroid state. */
    void update(const hal::ControlSample& controls, std::uint32_t nowMs);
    /** @brief Renders the current lunar landscape and player state. */
    void render();
    /** @brief Advances the deterministic local PRNG used for terrain events. */
    std::uint32_t nextRandom();
    /** @brief Starts one telegraphed asteroid fall at an avoidable offset from the player. */
    void spawnAsteroid(std::uint32_t nowMs);
    /** @brief Resolves asteroid impact and leaves a persistent crater in the current session. */
    void impactAsteroid(std::uint32_t nowMs);
    /** @brief Adds one dynamic impact crater, recycling the oldest slot when necessary. */
    void addImpactCrater(std::int32_t worldX, std::int16_t radius);
    /** @brief Returns crater depression in pixels at the supplied world coordinate. */
    std::int16_t craterDepthAt(std::int32_t worldX) const;
    /** @brief Consumes one life and enters the requested visible failure state. */
    void fail(FailureMode mode);
    /** @brief Respawns the egg safely after a non-terminal failure. */
    void respawnAfterFailure();
    /** @brief Routes the completed score into the shared arcade ranking flow. */
    void finishScore();

    hal::OledDisplay& display_;
    hal::ControlPanel& controls_;
    hal::GateOutputDriver& gateOutputs_;
    ArcadeShell shell_;
    LeaderboardEntry highScore_{};
    std::array<ImpactCrater, 6U> impactCraters_{};
    Asteroid asteroid_{};
    std::size_t nextImpactCraterSlot_ = 0U;
    std::int32_t playerWorldX_ = 0;
    std::int32_t cameraWorldX_ = 0;
    std::int16_t playerScreenX_ = 42;
    std::int32_t furthestDistance_ = 0;
    std::int16_t horizontalVelocity_ = 0;
    std::int16_t jumpHeightFp_ = 0;
    std::int16_t jumpVelocityFp_ = 0;
    std::uint32_t score_ = 0U;
    std::uint32_t randomState_ = 0x4D4F4F4EU;
    std::uint32_t lastPhysicsAtMs_ = 0U;
    std::uint32_t lastAsteroidAtMs_ = 0U;
    std::uint32_t nextAsteroidDelayMs_ = 5200U;
    std::uint32_t impactFlashUntilMs_ = 0U;
    std::uint32_t encoderPressedAtMs_ = 0U;
    std::uint8_t lives_ = 3U;
#ifdef CLOCK_SIMULATOR
    std::uint32_t lastSimulatorFrameAtMs_ = 0U;
#endif
    FailureMode failureMode_ = FailureMode::None;
    bool gameOver_ = false;
    bool awaitingRetry_ = false;
    bool retryArmed_ = false;
    bool scoreFinalized_ = false;
    bool exitRequested_ = false;
};

}  // namespace clockfw::game
