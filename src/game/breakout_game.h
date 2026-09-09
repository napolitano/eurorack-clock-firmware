/**
 * @file breakout_game.h
 * @brief Boot-only monochrome brick-breaker Easter egg.
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

/** @brief Encoder-controlled brick breaker with variable angles and original power-up mechanics. */
class BreakoutGame final {
public:
    /** @brief Constructs the game around the shared display, controls, and safe gate-output HAL. */
    BreakoutGame(
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
    friend struct BreakoutGameTestAccess;
    static constexpr std::uint8_t kColumns = 12U;
    static constexpr std::uint8_t kRows = 4U;
    static constexpr std::uint8_t kBrickCount = kColumns * kRows;

    enum class ExtraType : std::uint8_t { None, SmallBat, LargeBat, FastBat };
    struct FallingExtra final {
        ExtraType type = ExtraType::None;
        std::int16_t x = 0;
        std::int16_t y = 0;
        bool active = false;
    };

    /** @brief Restores bricks, paddle, effects, and waiting-ball state. */
    void resetSession();
    /** @brief Places the ball back on the paddle awaiting a TAP launch. */
    void resetBall();
    /** @brief Advances paddle input, variable-angle physics, bricks, and falling extras. */
    void update(const hal::ControlSample& controls, std::uint32_t nowMs);
    /** @brief Advances one collision-safe substep of ball motion. */
    void updateBallSubstep(std::uint32_t nowMs, bool moveX, bool moveY);
    /** @brief Applies a paddle-angle response based on the impact point. */
    void bounceFromPaddle();
    /** @brief Occasionally creates one deterministic falling extra at a destroyed brick. */
    void maybeSpawnExtra(std::int16_t x, std::int16_t y);
    /** @brief Advances and applies an active falling extra. */
    void updateExtra(std::uint32_t nowMs);
    /** @brief Applies one temporary paddle modifier. */
    void applyExtra(ExtraType type, std::uint32_t nowMs);
    /** @brief Advances the local deterministic PRNG used only by this game. */
    std::uint32_t nextRandom();
    /** @brief Renders walls, bricks, paddle, ball, extra, and launch hint. */
    void render();

    hal::OledDisplay& display_;
    hal::ControlPanel& controls_;
    hal::GateOutputDriver& gateOutputs_;
    ArcadeShell shell_;
    LeaderboardEntry highScore_{};
    std::array<bool, kBrickCount> bricks_{};
    std::int16_t paddleX_ = 54;
    std::int16_t paddleWidth_ = 20;
    std::int16_t paddleStep_ = 4;
    std::int16_t ballX_ = 64;
    std::int16_t ballY_ = 52;
    std::int8_t velocityX_ = 1;
    std::int8_t velocityY_ = -1;
    std::uint32_t randomState_ = 0xB8EA4C31U;
    std::uint32_t score_ = 0U;
    std::uint8_t lives_ = 3U;
    std::uint8_t level_ = 1U;
    std::uint32_t lastPhysicsAtMs_ = 0U;
    std::uint32_t encoderPressedAtMs_ = 0U;
    std::uint32_t extraExpiresAtMs_ = 0U;
    FallingExtra fallingExtra_{};
    ExtraType activeExtra_ = ExtraType::None;
#ifdef CLOCK_SIMULATOR
    std::uint32_t lastSimulatorFrameAtMs_ = 0U;
#endif
    bool ballLaunched_ = false;
    bool gameOver_ = false;
    bool exitRequested_ = false;
};

}  // namespace clockfw::game
