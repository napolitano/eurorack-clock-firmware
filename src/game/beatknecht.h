/**
 * @file beatknecht.h
 * @brief Boot-only eight-channel gate rhythm generator Easter egg.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "game/arcade_shell.h"
#include "hal/control_panel.h"
#include "hal/gate_output_driver.h"
#include "hal/oled_display.h"

namespace clockfw::game {

/** @brief Eight-gate one-bar rhythm player with encoder tempo and TAP style selection. */
class Beatknecht final {
public:
    /** @brief Constructs the rhythm generator around the shared display, controls, and gate HAL. */
    Beatknecht(hal::OledDisplay& display, hal::ControlPanel& controls, hal::GateOutputDriver& gateOutputs);
    /** @brief Runs the selected boot rhythm generator synchronously until exit. */
    void run();
#ifdef CLOCK_SIMULATOR
    /** @brief Starts a non-blocking simulator rhythm-generator session. */
    void beginForSimulator();
    /** @brief Services one simulator iteration and returns true while the mode remains active. */
    bool serviceForSimulator(std::uint32_t nowMs);
#endif

private:
    friend struct BeatknechtTestAccess;
    /** @brief Restores the default style, tempo, step, and output state. */
    void resetSession(std::uint32_t nowMs);
    /** @brief Applies controls and advances gate timing. */
    void update(const hal::ControlSample& controls, std::uint32_t nowMs);
    /** @brief Renders all eight patterns and the current step position. */
    void render();
    /** @brief Emits the active gates for one sixteenth-note pattern step. */
    void triggerStep(std::uint32_t nowMs);
    /** @brief Drives all eight gate sources low immediately. */
    void stopOutputs();
    /** @brief Returns the current sixteenth-note duration in milliseconds. */
    std::uint32_t stepDurationMs() const;

    hal::OledDisplay& display_;
    hal::ControlPanel& controls_;
    hal::GateOutputDriver& gateOutputs_;
    ArcadeShell shell_;
    std::uint16_t bpm_ = 120U;
    std::uint8_t styleIndex_ = 0U;
    std::uint8_t nextStep_ = 0U;
    std::uint8_t displayStep_ = 0U;
    std::uint32_t nextStepAtMs_ = 0U;
    std::uint32_t gatesOffAtMs_ = 0U;
    std::uint32_t encoderPressedAtMs_ = 0U;
#ifdef CLOCK_SIMULATOR
    std::uint32_t lastSimulatorFrameAtMs_ = 0U;
#endif
    bool gatesHigh_ = false;
    bool exitRequested_ = false;
};

}  // namespace clockfw::game
