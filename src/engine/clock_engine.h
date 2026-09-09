/**
 * @file clock_engine.h
 * @brief Deterministic shared scheduler for all eight gate channels.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/clock_types.h"
#include "hal/gate_output_driver.h"

namespace clockfw::engine {

/** @brief Atomic read-only snapshot consumed by the user interface. */
struct EngineSnapshot {
    bool playing = false;
    std::uint64_t masterPositionQ32 = 0U;
    std::uint64_t masterBeatPhaseQ32 = 0U;
    std::uint32_t masterBeatSerial = 0U;
    std::uint8_t masterBeat = 1U;
    std::uint16_t masterBar = 1U;
    bool externalLocked = false;
    bool externalResetHeld = false;
    std::uint32_t externalBpmMilli = 0U;
    std::uint8_t channelStep[kChannelCount]{};
};

/**
 * @brief Runs the master timeline, channel schedulers, probability gates, and gate-off timing.
 *
 * The engine is intentionally independent from OLED rendering and control-panel polling.
 * Only the periodic timer ISR calls processSchedulerTick(); configuration mutations are
 * copied into the engine inside interrupt-safe critical sections.
 */
class ClockEngine final {
public:
    /**
     * @brief Constructs the engine around the physical gate output driver.
     * @param gateOutputs HAL driver used for all gate/LED GPIO state changes.
     */
    explicit ClockEngine(hal::GateOutputDriver& gateOutputs);

    /**
     * @brief Initializes runtime state from the complete user configuration.
     * @param state Application state to copy into the real-time engine.
     */
    void begin(const ClockState& state);

    /**
     * @brief Copies all mutable configuration into the real-time engine.
     * @param state Current application state.
     * @param rescheduleChannels True to restart all local channel schedules from now.
     */
    void updateConfiguration(const ClockState& state, bool rescheduleChannels);

    /**
     * @brief Updates one channel without disturbing the other seven channels.
     * @param channelIndex Zero-based channel index.
     * @param channelConfiguration New channel configuration.
     * @param rescheduleChannel True to rebuild the selected channel's next event time.
     */
    void updateChannel(
        std::size_t channelIndex,
        const ChannelConfig& channelConfiguration,
        bool rescheduleChannel);

    /** @brief Starts or resumes scheduler progression. */
    void play();

    /** @brief Pauses scheduler progression and forces all currently active gates LOW. */
    void pause();

    /** @brief Stops scheduler progression and resets the master/global channel phase. */
    void stop();

    /** @brief Applies a global reset while respecting each channel's GLOBAL/FREE reset policy. */
    void resetGlobalPhase();

    /**
     * @brief ISR-safe global reset variant for an already-conditioned external reset edge.
     *
     * The caller must already execute in an interrupt context that cannot race the
     * scheduler ISR. This method deliberately does not manipulate interrupt state.
     */
    void resetGlobalPhaseFromIsr();

    /**
     * @brief Updates the external synchronization lock information.
     * @param locked True when the external clock estimator is locked.
     * @param bpmMilli Smoothed external tempo in milli-BPM.
     */
    void setExternalLock(bool locked, std::uint32_t bpmMilli);

    /** @brief ISR-safe external lock update used by the real-time input controller. */
    void setExternalLockFromIsr(bool locked, std::uint32_t bpmMilli);

    /** @brief Applies or releases a level-sensitive external reset hold from scheduler context. */
    void setExternalResetGateFromIsr(bool high);

    /**
     * @brief Accepts one phase reference from an external sync source.
     * @param bpmMilli Filtered external tempo in milli-BPM.
     * @param pulsesPerQuarterNote Number of input pulses per quarter note.
     *
     * The first pulse establishes a shared musical epoch for GLOBAL channels;
     * subsequent pulses snap the master timeline to the exact rational pulse grid.
     */
    void acceptExternalPulse(std::uint32_t bpmMilli, std::uint8_t pulsesPerQuarterNote);

    /**
     * @brief ISR-safe variant of acceptExternalPulse().
     * @param bpmMilli Filtered external tempo in milli-BPM.
     * @param pulsesPerQuarterNote Number of input pulses per quarter note.
     *
     * The caller must already execute in an interrupt context that cannot race the
     * scheduler ISR. This method deliberately does not manipulate global interrupt state.
     */
    void acceptExternalPulseFromIsr(
        std::uint32_t bpmMilli,
        std::uint8_t pulsesPerQuarterNote);

    /**
     * @brief Executes one deterministic scheduler quantum.
     *
     * This method is ISR-only and must remain bounded, allocation-free, and non-blocking.
     */
    void processSchedulerTick();

    /** @brief Returns interrupt-safe snapshot of the timing state used by the UI. */
    EngineSnapshot snapshot() const;

private:
    /** @brief Real-time copy of configuration values needed inside the ISR. */
    struct EngineConfiguration {
        std::uint16_t bpm = 120U;
        std::uint8_t beatsPerBar = 4U;
        std::uint8_t beatUnit = 4U;
        ClockSource source = ClockSource::Internal;
        SyncLossMode syncLossMode = SyncLossMode::Freewheel;
        OperatingMode operatingMode = OperatingMode::Independent;
        std::uint16_t unifiedHumanizeUs = 0U;
        ChannelConfig channels[kChannelCount]{};
    };

    /** @brief Mutable scheduling state for one output channel. */
    struct ChannelRuntime {
        std::uint64_t nextEventQ32 = 0U;
        std::uint64_t scheduleEpochQ32 = 0U;
        std::uint64_t nextEventSerial = 0U;
        std::uint8_t step = 0U;
        std::uint8_t displayedStep = 0U;
        std::uint32_t gateOffTick = 0U;
        bool gateHigh = false;
        std::uint32_t randomState = 0x12345678U;
    };

    /** @brief Returns the tempo that currently drives the master timeline. */
    std::uint32_t effectiveBpmMilli() const;

    /** @brief Returns the shortest representable musical interval at the scheduler cadence. */
    std::uint64_t minimumOutputIntervalQ32() const;

    /** @brief Resolves one channel's effective musical rate into a reduced fraction. */
    void resolveChannelRate(
        std::size_t channelIndex,
        std::uint32_t& numerator,
        std::uint32_t& denominator) const;

    /** @brief Calculates one nominal interval without consuming the channel remainder accumulator. */
    std::uint64_t calculateNominalBaseIntervalQ32(std::size_t channelIndex) const;

    /** @brief Returns the exact cumulative base-grid offset for one event serial. */
    std::uint64_t calculateBaseEventOffsetQ32(
        std::size_t channelIndex,
        std::uint64_t eventSerial) const;

    /** @brief Returns the absolute epoch-anchored event position including phase, swing, and ONE CLOCK humanize. */
    std::uint64_t calculateEventPositionQ32(
        std::size_t channelIndex,
        std::uint64_t eventSerial) const;

    /** @brief Calculates one channel interval in real microseconds for gate-length limiting. */
    std::uint64_t calculateBaseIntervalUs(std::size_t channelIndex) const;

    /** @brief Returns the safely capped ONE CLOCK humanization amount for one output. */
    std::uint16_t effectiveHumanizeUs(std::size_t channelIndex) const;

    /** @brief Returns deterministic signed per-output timing jitter in Q32 master-beat units. */
    std::int64_t calculateHumanizeOffsetQ32(
        std::size_t channelIndex,
        std::uint64_t eventSerial) const;

    /** @brief Drives one output only when its logical state changes. */
    void setGateState(std::size_t channelIndex, bool high);

    /** @brief Evaluates and emits one scheduled event for one channel. */
    void fireChannelEvent(std::size_t channelIndex);

    /** @brief Rebuilds one channel on its deterministic epoch-anchored musical grid. */
    void scheduleChannelFromCurrentPosition(
        std::size_t channelIndex,
        bool includeCurrentBoundary = false);

    /** @brief Returns the active local cycle length used to derive GLOBAL step phase. */
    std::uint8_t channelCycleLength(std::size_t channelIndex) const;

    /** @brief Re-establishes GLOBAL pattern phase or bounds FREE local phase after edits. */
    void synchronizeChannelStepPhase(std::size_t channelIndex);

    /** @brief Returns the swing displacement used by the epoch-anchored event grid. */
    std::uint64_t calculateSwingDeltaQ32(
        std::size_t channelIndex,
        std::uint64_t baseIntervalQ32) const;

    /** @brief Applies one external reference without manipulating global interrupt state. */
    void acceptExternalPulseUnsafe(
        std::uint32_t bpmMilli,
        std::uint8_t pulsesPerQuarterNote);

    /** @brief Shifts pending GLOBAL schedules by an observed external phase correction. */
    void alignGlobalSchedulesToExternalPulse(std::uint64_t expectedPulsePositionQ32);

    /** @brief Resets master/runtime state while respecting each channel's GLOBAL/FREE policy. */
    void resetRuntime();

    /** @brief Copies master and channel configuration without entering a critical section. */
    void copyConfigurationUnsafe(const ClockState& state);

    hal::GateOutputDriver& gateOutputs_;
    EngineConfiguration configuration_{};
    volatile bool playing_ = true;
    volatile bool restartPending_ = true;
    volatile std::uint64_t masterPositionQ32_ = 0U;
    volatile std::uint64_t globalScheduleEpochQ32_ = 0U;
    volatile std::uint64_t masterBeatPhaseQ32_ = 0U;
    volatile std::uint32_t masterBeatSerial_ = 0U;
    volatile std::uint8_t masterBeat_ = 1U;
    volatile std::uint16_t masterBar_ = 1U;
    volatile bool externalLocked_ = false;
    volatile bool externalResetGate_ = false;
    volatile std::uint32_t externalBpmMilli_ = 0U;
    volatile bool externalPhaseInitialized_ = false;
    volatile std::uint64_t externalReferenceQ32_ = 0U;
    volatile std::uint64_t externalMusicalPositionQ32_ = 0U;
    volatile std::uint32_t externalPulseRemainder_ = 0U;
    ChannelRuntime channelRuntime_[kChannelCount]{};
    volatile std::uint32_t schedulerTickCounter_ = 0U;
    volatile std::uint64_t masterRemainder_ = 0U;
};

}  // namespace clockfw::engine
