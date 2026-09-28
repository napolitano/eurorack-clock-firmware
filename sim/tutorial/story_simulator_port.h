/**
 * @file story_simulator_port.h
 * @brief Strict Storybook adapter over the existing native simulator hardware/environment boundary.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "domain/clock_types.h"
#include "simulator_runtime.h"
#include "tutorial/story_contract.h"

namespace clockfw::sim::tutorial {

/** @brief Semantic failures rejected before a Storybook action reaches SimulatorRuntime. */
enum class StoryPortError : std::uint8_t {
    None,
    ModulePoweredOff,
    CableRequired,
    InvalidParameter,
    UnsupportedAction,
};

/** @brief Result of one Storybook-to-simulator boundary operation. */
struct StoryPortResult {
    StoryPortError error = StoryPortError::None;

    /** @brief Returns true when the boundary operation was accepted. */
    explicit operator bool() const;
};

/**
 * @brief Enforces Storybook semantics while delegating all actual CLOCK behaviour to SimulatorRuntime.
 *
 * This adapter deliberately refuses convenience behaviour that is useful in the interactive
 * simulator but ambiguous in a recorded tutorial, such as auto-connecting a cable when a
 * generator is started. It never writes ClockState directly.
 */
class StorySimulatorPort final {
public:
    /** @brief Wraps one already constructed simulator runtime without taking ownership. */
    explicit StorySimulatorPort(SimulatorRuntime& runtime);

    /** @brief Queues exactly one physical encoder detent; direction must be -1 or +1. */
    StoryPortResult rotateEncoderDetent(int direction);

    /** @brief Applies physical press/release state to encoder push, PLAY, TAP, or STOP/BACK. */
    StoryPortResult setModuleControl(ModuleControl control, bool pressed);

    /** @brief Sets virtual module power through the existing simulator power boundary. */
    StoryPortResult setPower(bool powered);

    /** @brief Clears simulator persistence to the erased/factory precondition image and reboots if needed. */
    StoryPortResult resetPersistenceToFactory();

    /** @brief Connects/disconnects SYNC or RST through existing virtual-cable semantics. */
    StoryPortResult setPatchConnected(PatchAction action, bool connected);

    /** @brief Runs/holds a connected SYNC or RST generator without changing cable state. */
    StoryPortResult setGeneratorRunning(ExternalStimulus stimulus, bool running);

    /** @brief Configures exact virtual SYNC source parameters without changing CLOCK SOURCE. */
    StoryPortResult configureSyncSource(
        std::uint32_t bpmMilli,
        std::uint8_t ppqn,
        SignalWaveform waveform);

    /** @brief Configures exact virtual RST source parameters in supported 100-ms increments. */
    StoryPortResult configureResetSource(std::uint32_t periodMs, SignalWaveform waveform);

    /** @brief Emits one external RST pulse through a connected virtual source. */
    StoryPortResult triggerResetPulse();

    /** @brief Advances only simulator/firmware time; interaction/presentation timing remains runner-owned. */
    void advanceFirmwareMicroseconds(std::uint64_t durationUs);

    /** @brief Returns whether the simulated module is powered. */
    bool poweredOn() const;

    /** @brief Returns the production firmware CLOCK SOURCE setting without mutating it. */
    ClockSource clockSource() const;

    /** @brief Returns the production firmware transport state. */
    TransportState transportState() const;

    /** @brief Returns current SYNC environment/firmware telemetry. */
    SyncInputTelemetry syncTelemetry() const;

    /** @brief Returns current RST environment telemetry. */
    ResetInputTelemetry resetTelemetry() const;

    /** @brief Returns the wrapped runtime for read-only downstream renderer/telemetry integration. */
    const SimulatorRuntime& runtime() const;

    /**
     * @brief Returns the host runtime for presentation tooling that maintains simulator-only telemetry views.
     *
     * Story actions must not use this accessor to mutate CLOCK state; it exists for frame/scope presentation only.
     */
    SimulatorRuntime& presentationRuntime();

private:
    /** @brief Maps a Storybook module control to the existing simulator button enum. */
    static StoryPortResult mapButton(ModuleControl control, SimButton& button);

    /** @brief Cycles SYNC waveform deterministically until it matches the requested value. */
    StoryPortResult setSyncWaveform(SignalWaveform waveform);

    /** @brief Cycles RST waveform deterministically until it matches the requested value. */
    StoryPortResult setResetWaveform(SignalWaveform waveform);

    SimulatorRuntime& runtime_;
};

}  // namespace clockfw::sim::tutorial
