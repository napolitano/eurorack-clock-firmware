/**
 * @file external_sync_controller.h
 * @brief Deterministic scheduler-side processing for external SYNC and RST inputs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "domain/clock_types.h"
#include "engine/clock_engine.h"
#include "hal/external_input_capture.h"

namespace clockfw::services {

/**
 * @brief Converts captured comparator edges into phase references and reset behavior.
 *
 * EXTI/input-capture callbacks only enqueue timestamped edges. All engine mutation
 * occurs here at the deterministic 20-kHz scheduler boundary, so RST and SYNC cannot
 * race the clock engine from unrelated interrupt contexts.
 */
class ExternalSyncController final {
public:
    /** @brief Constructs the controller around the edge-capture HAL and clock engine. */
    ExternalSyncController(
        hal::ExternalInputCapture& inputCapture,
        engine::ClockEngine& engine);

    /** @brief Initializes filtering/reset semantics from the active application state. */
    void begin(const ClockState& state);

    /** @brief Atomically updates user-editable synchronization settings. */
    void updateConfiguration(const ClockState& state);

    /** @brief Consumes queued edges and applies timeout/reset behavior before one engine tick. */
    void processSchedulerTick(std::uint32_t nowUs);

    /** @brief Clears lock/period acquisition after a known cable or generator loss. */
    void clearExternalLock();

    /** @brief Returns the most recent filtered external tempo in milli-BPM. */
    std::uint32_t filteredBpmMilli() const;

private:
    /** @brief Consumes RST edges before SYNC so reset wins at identical scheduler boundaries. */
    void processResetEdges();

    /** @brief Consumes selected SYNC edges and updates period/phase estimation. */
    void processSyncEdges();

    /** @brief Applies a pending RST-mode change using the currently observed input level. */
    void applyResetModeChange();

    /** @brief Returns true when one captured level corresponds to the selected SYNC edge. */
    bool selectedSyncEdge(bool high) const;

    /** @brief Converts one filtered pulse period into bounded milli-BPM. */
    std::uint32_t calculateBpmMilli(std::uint32_t periodUs) const;

    /** @brief Returns the lock-loss timeout derived from user floor and observed/minimum tempo. */
    std::uint32_t effectiveTimeoutUs() const;

    hal::ExternalInputCapture& inputCapture_;
    engine::ClockEngine& engine_;
    // Foreground-only publication shadow. It lets the hot runOnce() path reject
    // unchanged settings without entering a global interrupt critical section.
    ExternalSyncSettings foregroundSettings_{};
    std::uint32_t foregroundFallbackBpmMilli_ = 120000U;
    std::uint16_t foregroundMinimumBpm_ = 20U;

    ExternalSyncSettings settings_{};
    std::uint32_t fallbackBpmMilli_ = 120000U;
    std::uint16_t minimumBpm_ = 20U;
    bool configurationDirty_ = false;
    bool resetGateApplied_ = false;
    bool haveAcceptedPulse_ = false;
    bool externalLocked_ = false;
    std::uint32_t lastAcceptedPulseUs_ = 0U;
    std::uint64_t filteredPeriodQ8_ = 0U;
    std::uint32_t filteredBpmMilli_ = 0U;
};

}  // namespace clockfw::services
