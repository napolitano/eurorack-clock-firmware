/**
 * @file external_sync_controller.cpp
 * @brief Deterministic scheduler-side processing for external SYNC and RST inputs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/external_sync_controller.h"

#include <cstdint>

#include "hal/interrupt_lock.h"

namespace clockfw::services {
namespace {

constexpr std::uint64_t kMicrosPerMinuteMilliBpm = 60000000000ULL;

}  // namespace

ExternalSyncController::ExternalSyncController(
    hal::ExternalInputCapture& inputCapture,
    engine::ClockEngine& engine)
    : inputCapture_(inputCapture), engine_(engine) {}

void ExternalSyncController::begin(const ClockState& state) {
    settings_ = state.externalSync;
    fallbackBpmMilli_ = static_cast<std::uint32_t>(state.bpm) * 1000U;
    minimumBpm_ = state.tempoRange.minimumBpm != 0U ? state.tempoRange.minimumBpm : 1U;
    configurationDirty_ = true;
    resetGateApplied_ = false;
    haveAcceptedPulse_ = false;
    externalLocked_ = false;
    lastAcceptedPulseUs_ = 0U;
    filteredPeriodQ8_ = 0U;
    filteredBpmMilli_ = 0U;
}

void ExternalSyncController::updateConfiguration(const ClockState& state) {
    hal::InterruptLock interruptLock;
    const bool resetModeChanged = state.externalSync.resetMode != settings_.resetMode;
    settings_ = state.externalSync;
    fallbackBpmMilli_ = static_cast<std::uint32_t>(state.bpm) * 1000U;
    minimumBpm_ = state.tempoRange.minimumBpm != 0U ? state.tempoRange.minimumBpm : 1U;
    configurationDirty_ = configurationDirty_ || resetModeChanged;
}

void ExternalSyncController::processSchedulerTick(const std::uint32_t nowUs) {
    applyResetModeChange();
    processResetEdges();
    processSyncEdges();

    if (!externalLocked_ || !haveAcceptedPulse_) {
        return;
    }
    const std::uint32_t timeoutUs = effectiveTimeoutUs();
    if (static_cast<std::uint32_t>(nowUs - lastAcceptedPulseUs_) < timeoutUs) {
        return;
    }

    externalLocked_ = false;
    haveAcceptedPulse_ = false;
    filteredPeriodQ8_ = 0U;
    engine_.setExternalLockFromIsr(false, filteredBpmMilli_);
}

void ExternalSyncController::clearExternalLock() {
    hal::InterruptLock interruptLock;
    externalLocked_ = false;
    haveAcceptedPulse_ = false;
    filteredPeriodQ8_ = 0U;
    engine_.setExternalLockFromIsr(false, filteredBpmMilli_);
}

std::uint32_t ExternalSyncController::filteredBpmMilli() const {
    return filteredBpmMilli_;
}

void ExternalSyncController::processResetEdges() {
    hal::ExternalInputEdge edge{};
    while (inputCapture_.popResetEdge(edge)) {
        if (settings_.resetMode == ExternalResetMode::Trigger) {
            if (edge.high) {
                engine_.resetGlobalPhaseFromIsr();
            }
            continue;
        }
        resetGateApplied_ = edge.high;
        engine_.setExternalResetGateFromIsr(edge.high);
    }
}

void ExternalSyncController::processSyncEdges() {
    hal::ExternalInputEdge edge{};
    while (inputCapture_.popSyncEdge(edge)) {
        if (!selectedSyncEdge(edge.high)) {
            continue;
        }

        if (edge.continuityLost) {
            // We know that at least one selected edge may have been skipped. Never
            // interpret the resulting multi-period gap as a real tempo change. Keep
            // the last trustworthy tempo, re-anchor phase, and reacquire period on
            // the following clean edge.
            haveAcceptedPulse_ = false;
            filteredPeriodQ8_ = 0U;
        }

        if (!haveAcceptedPulse_) {
            haveAcceptedPulse_ = true;
            externalLocked_ = true;
            lastAcceptedPulseUs_ = edge.timestampUs;
            if (filteredBpmMilli_ == 0U) {
                filteredBpmMilli_ = fallbackBpmMilli_;
            }
            engine_.acceptExternalPulseFromIsr(
                filteredBpmMilli_,
                settings_.pulsesPerQuarterNote);
            continue;
        }

        const std::uint32_t periodUs = edge.timestampUs - lastAcceptedPulseUs_;
        if (periodUs == 0U || periodUs < settings_.glitchFilterUs) {
            continue;
        }
        lastAcceptedPulseUs_ = edge.timestampUs;

        const std::uint64_t periodQ8 = static_cast<std::uint64_t>(periodUs) << 8U;
        if (filteredPeriodQ8_ == 0U) {
            filteredPeriodQ8_ = periodQ8;
        } else {
            // 25% new sample: enough smoothing for comparator/IRQ jitter without
            // making deliberate tempo changes feel sluggish.
            filteredPeriodQ8_ = (filteredPeriodQ8_ * 3U + periodQ8) / 4U;
        }
        const std::uint32_t filteredPeriodUs = static_cast<std::uint32_t>(
            (filteredPeriodQ8_ + 128U) >> 8U);
        filteredBpmMilli_ = calculateBpmMilli(filteredPeriodUs);
        externalLocked_ = true;
        engine_.acceptExternalPulseFromIsr(
            filteredBpmMilli_,
            settings_.pulsesPerQuarterNote);
    }
}

void ExternalSyncController::applyResetModeChange() {
    if (!configurationDirty_) {
        return;
    }
    configurationDirty_ = false;
    if (settings_.resetMode == ExternalResetMode::Gate) {
        resetGateApplied_ = inputCapture_.resetLevelHigh();
        engine_.setExternalResetGateFromIsr(resetGateApplied_);
    } else if (resetGateApplied_) {
        resetGateApplied_ = false;
        engine_.setExternalResetGateFromIsr(false);
    }
}

bool ExternalSyncController::selectedSyncEdge(const bool high) const {
    return settings_.edge == SyncEdge::Rising ? high : !high;
}

std::uint32_t ExternalSyncController::calculateBpmMilli(const std::uint32_t periodUs) const {
    if (periodUs == 0U) {
        return 0U;
    }
    const std::uint32_t ppqn = settings_.pulsesPerQuarterNote != 0U
        ? settings_.pulsesPerQuarterNote
        : 1U;
    const std::uint64_t denominator = static_cast<std::uint64_t>(periodUs) * ppqn;
    const std::uint64_t value = kMicrosPerMinuteMilliBpm / denominator;
    return value > 0xFFFFFFFFULL ? 0xFFFFFFFFU : static_cast<std::uint32_t>(value);
}

std::uint32_t ExternalSyncController::effectiveTimeoutUs() const {
    constexpr std::uint64_t kMicrosPerMinute = 60000000ULL;
    constexpr std::uint64_t kMissedPulseTolerance = 2ULL;
    const std::uint64_t configuredUs = static_cast<std::uint64_t>(settings_.timeoutMs) * 1000ULL;
    const std::uint32_t ppqn = settings_.pulsesPerQuarterNote != 0U
        ? settings_.pulsesPerQuarterNote
        : 1U;

    std::uint64_t referencePeriodUs = 0U;
    if (filteredPeriodQ8_ != 0U) {
        referencePeriodUs = (filteredPeriodQ8_ + 128U) >> 8U;
    } else {
        const std::uint32_t minimumBpm = minimumBpm_ != 0U ? minimumBpm_ : 1U;
        referencePeriodUs = kMicrosPerMinute /
            (static_cast<std::uint64_t>(minimumBpm) * ppqn);
    }
    const std::uint64_t adaptiveUs = referencePeriodUs * kMissedPulseTolerance;
    const std::uint64_t timeoutUs = configuredUs > adaptiveUs ? configuredUs : adaptiveUs;
    return timeoutUs > 0xFFFFFFFFULL
        ? 0xFFFFFFFFU
        : static_cast<std::uint32_t>(timeoutUs);
}

}  // namespace clockfw::services
