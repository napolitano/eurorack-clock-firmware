/**
 * @file simulator_runtime.h
 * @brief Native host runtime that advances the real firmware against simulated hardware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <deque>
#include <filesystem>
#include <memory>

#include "app/clock_application.h"
#include "domain/clock_types.h"
#include "simulator_persistence.h"
#include "virtual_input_signal.h"

namespace clockfw::sim {

/** @brief Physical front-panel buttons available to simulator input bindings. */
enum class SimButton : std::uint8_t { Encoder, Play, Tap, Stop };

/** @brief One captured gate transition used by the developer timing view. */
struct GateTransition {
    std::uint64_t timestampUs = 0ULL;
    bool high = false;
};

/** @brief Rolling timing telemetry for one physical output. */
struct ChannelTelemetry {
    bool logicHigh = false;
    std::uint64_t risingEdges = 0ULL;
    std::uint64_t lastTransitionUs = 0ULL;
    std::uint64_t lastRisingUs = 0ULL;
    std::uint64_t lastPulseWidthUs = 0ULL;
    std::deque<GateTransition> transitions{};
};

/** @brief Transport timing observed at the simulator/runtime boundary. */
struct TransportTelemetry {
    TransportState state = TransportState::Stopped;
    std::uint32_t startSequence = 0U;
    std::uint64_t lastStartUs = 0ULL;
    std::uint64_t lastStopUs = 0ULL;
};

/** @brief Observable state of the ideal source/comparator path patched to SYNC IN. */
struct SyncInputTelemetry {
    bool cableConnected = false;
    bool generatorRunning = false;
    bool signalHigh = false;
    bool locked = false;
    SignalWaveform waveform = SignalWaveform::Square;
    std::uint32_t bpmMilli = 120000U;
    std::uint8_t ppqn = 1U;
    std::uint8_t firmwarePpqn = 1U;
    std::uint32_t engineBpmMilli = 0U;
    std::uint64_t pulseCount = 0ULL;
};

/** @brief Observable state of the ideal source/comparator path patched to RST IN. */
struct ResetInputTelemetry {
    bool cableConnected = false;
    bool generatorRunning = false;
    bool signalHigh = false;
    SignalWaveform waveform = SignalWaveform::Square;
    std::uint32_t periodMs = 2000U;
    std::uint64_t resetCount = 0ULL;
};

/**
 * @brief Executes the actual application/engine/UI code on a deterministic virtual clock.
 *
 * The runtime owns no duplicate clock logic. It advances the same TIM3 callback,
 * polls the same ControlPanel HAL, renders through the same OledDisplay framebuffer,
 * and mirrors the same persistence bytes used by host tests.
 */
class SimulatorRuntime final {
public:
    /** @brief Constructs the simulator using one durable host-state file. */
    explicit SimulatorRuntime(std::filesystem::path persistencePath);

    /** @brief Loads persisted state and powers the simulated module on. */
    void begin();

    /** @brief Advances virtual MCU time while servicing the 20 kHz scheduler and foreground loop. */
    void advanceMicroseconds(std::uint64_t durationUs);

    /** @brief Queues one or more complete quadrature detents. Positive values rotate clockwise. */
    void rotateEncoder(int detents);

    /** @brief Applies a physical press/release state to one simulated front-panel control. */
    void setButton(SimButton button, bool pressed);

    /** @brief Flushes simulated non-volatile storage when its contents changed. */
    void flushPersistence();

    /** @brief Returns actual 128x64 firmware framebuffer, or a blank frame while powered off. */
    const std::array<std::uint8_t, hal::OledDisplay::kFramebufferSize>& framebuffer() const;

    /** @brief Returns current real firmware application state, or the last safe state while off. */
    const ClockState& state() const;

    /** @brief Returns current rolling gate telemetry for all eight physical outputs. */
    const std::array<ChannelTelemetry, kChannelCount>& telemetry() const;

    /** @brief Returns current virtual MCU time in microseconds. */
    std::uint64_t nowMicroseconds() const;

    /** @brief Returns transport transition timing used by the developer scope timebase. */
    TransportTelemetry transportTelemetry() const;

    /** @brief Freezes or resumes rolling waveform-history pruning for STOP inspection. */
    void setTelemetryHistoryFrozen(bool frozen);

    /** @brief Returns accumulated host-side encoder detents for visible knob feedback. */
    std::int64_t encoderVisualPosition() const;

    /** @brief Returns true when the external 74HCT244 output stage is enabled. */
    bool outputStageEnabled() const;

    /** @brief Returns true while the simulated module has MCU power. */
    bool poweredOn() const;

    /** @brief Powers the module on or off, reconstructing volatile firmware state on power-up. */
    void setPower(bool powered);

    /** @brief Toggles the simulated module power state. */
    void togglePower();

    /** @brief Returns true while Pixel Raid owns the simulator firmware UI. */
    bool pixelRaidActive() const;

    /** @brief Connects or disconnects the virtual source from SYNC IN. */
    void setSyncCableConnected(bool connected);

    /** @brief Starts or stops the SYNC source while leaving cable state unchanged. */
    void setSyncGeneratorRunning(bool running);

    /** @brief Sets virtual external tempo in milli-BPM, clamped to 1..999 BPM. */
    void setSyncBpmMilli(std::uint32_t bpmMilli);

    /** @brief Adjusts virtual sync tempo by a signed whole-BPM delta. */
    void adjustSyncBpm(int deltaBpm);

    /** @brief Cycles the virtual generator through firmware-supported PPQN values. */
    void cycleSyncPpqn();

    /** @brief Cycles SQUARE/SINE/TRIANGLE ahead of the ideal SYNC comparator. */
    void cycleSyncWaveform();

    /** @brief Returns current virtual sync-input state for rendering/developer tools. */
    SyncInputTelemetry syncInputTelemetry() const;

    /** @brief Connects or disconnects the virtual source from RST IN. */
    void setResetCableConnected(bool connected);

    /** @brief Starts or stops the continuous RST source while leaving cable state unchanged. */
    void setResetGeneratorRunning(bool running);

    /** @brief Adjusts continuous RST period by a signed number of 100-ms steps. */
    void adjustResetPeriod(int deltaSteps);

    /** @brief Cycles SQUARE/SINE/TRIANGLE ahead of the ideal RST comparator. */
    void cycleResetWaveform();

    /** @brief Injects one conditioned RST edge independently of the continuous generator. */
    void triggerResetPulse();

    /** @brief Returns current virtual reset-input state for rendering/developer tools. */
    ResetInputTelemetry resetInputTelemetry() const;

private:
    /** @brief Applies one queued quadrature phase before the next foreground input poll. */
    void serviceEncoderSequence();

    /** @brief Releases short simulated clicks only after the firmware debounce interval can observe them. */
    void serviceDeferredButtonReleases();

    /** @brief Captures newly emitted GPIO transitions into bounded waveform telemetry. */
    void collectGateTransitions();

    /** @brief Drops waveform history older than the largest supported developer-scope window. */
    void pruneTelemetry();

    /** @brief Captures firmware transport transitions at their exact simulated MCU timestamp. */
    void serviceTransportTelemetry();

    /** @brief Samples the ideal SYNC source/comparator and emits conditioned rising edges. */
    void serviceExternalSync();

    /** @brief Samples the ideal RST source/comparator and emits conditioned rising edges. */
    void serviceExternalReset();

    /** @brief Resets SYNC acquisition after cable, tempo, PPQN, waveform, or power changes. */
    void resetExternalSyncAcquisition();

    /** @brief Resets RST comparator edge tracking after source or power changes. */
    void resetExternalResetAcquisition();

    /** @brief Restores stored physical button levels after a virtual MCU power-on reset. */
    void applyStoredButtonLevels();

    /** @brief Converts one simulated button to its active-low GPIO. */
    static std::uint32_t buttonPin(SimButton button);

    /** @brief Writes one two-bit quadrature state to the encoder phase pins. */
    static void setEncoderPhase(std::uint8_t phaseState);

    std::unique_ptr<app::ClockApplication> application_{};
    SimulatorPersistence persistence_;
    ClockState poweredOffState_{};
    std::array<std::uint8_t, hal::OledDisplay::kFramebufferSize> poweredOffFramebuffer_{};
    std::array<ChannelTelemetry, kChannelCount> telemetry_{};
    std::size_t processedWriteCount_ = 0U;
    std::uint64_t foregroundAccumulatorUs_ = 0ULL;
    std::uint64_t persistenceAccumulatorUs_ = 0ULL;
    int pendingEncoderDetents_ = 0;
    std::int64_t encoderVisualPosition_ = 0;
    std::uint8_t encoderSequenceIndex_ = 0U;
    int activeEncoderDirection_ = 0;
    std::array<std::uint64_t, 4U> minimumButtonReleaseUs_{};
    std::array<bool, 4U> deferredButtonRelease_{};
    std::array<bool, 4U> buttonPressed_{};
    bool poweredOn_ = false;

    bool syncCableConnected_ = false;
    bool syncGeneratorRunning_ = false;
    bool syncComparatorHigh_ = false;
    bool syncLocked_ = false;
    SignalWaveform syncWaveform_ = SignalWaveform::Square;
    std::uint32_t syncBpmMilli_ = 120000U;
    std::uint8_t syncPpqn_ = 1U;
    std::uint64_t syncEpochUs_ = 0ULL;
    std::uint64_t syncLastPulseUs_ = 0ULL;
    std::uint64_t syncPulseCount_ = 0ULL;

    bool resetCableConnected_ = false;
    bool resetGeneratorRunning_ = false;
    bool resetComparatorHigh_ = false;
    SignalWaveform resetWaveform_ = SignalWaveform::Square;
    std::uint32_t resetPeriodMs_ = 2000U;
    std::uint64_t resetEpochUs_ = 0ULL;
    std::uint64_t resetCount_ = 0ULL;

    TransportState observedTransport_ = TransportState::Stopped;
    std::uint32_t transportStartSequence_ = 0U;
    std::uint64_t lastTransportStartUs_ = 0ULL;
    std::uint64_t lastTransportStopUs_ = 0ULL;
    bool telemetryHistoryFrozen_ = false;
};

}  // namespace clockfw::sim
