/**
 * @file external_sync_controller.cpp
 * @brief Deterministic scheduler-side processing for external SYNC and RST inputs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/external_sync_controller.h"

#include <cstdint>

#include "config.h"
#include "hal/interrupt_lock.h"

namespace clockfw::services {
namespace {

constexpr std::uint64_t kMicrosPerMinuteMilliBpm = 60000000000ULL;

bool settingsEqual(
    const ExternalSyncSettings& first,
    const ExternalSyncSettings& second) {
    return first.pulsesPerQuarterNote == second.pulsesPerQuarterNote &&
        first.edge == second.edge &&
        first.lossMode == second.lossMode &&
        first.resetMode == second.resetMode &&
        first.glitchFilterUs == second.glitchFilterUs &&
        first.timeoutMs == second.timeoutMs;
}

}  // namespace

ExternalSyncController::ExternalSyncController(
    hal::ExternalInputCapture& inputCapture,
    engine::ClockEngine& engine)
    : inputCapture_(inputCapture), engine_(engine) {}

void ExternalSyncController::begin(const ClockState& state) {
    foregroundSettings_ = state.externalSync;
    foregroundFallbackBpmMilli_ = static_cast<std::uint32_t>(state.bpm) * 1000U;
    foregroundMinimumBpm_ = state.tempoRange.minimumBpm != 0U
        ? state.tempoRange.minimumBpm
        : 1U;
    settings_ = foregroundSettings_;
    fallbackBpmMilli_ = foregroundFallbackBpmMilli_;
    minimumBpm_ = foregroundMinimumBpm_;
    configurationDirty_ = true;
    resetGateApplied_ = false;
    haveAcceptedPulse_ = false;
    externalLocked_ = false;
    lastAcceptedPulseUs_ = 0U;
    filteredPeriodQ8_ = 0U;
    filteredBpmMilli_ = 0U;
}

void ExternalSyncController::updateConfiguration(const ClockState& state) {
    const std::uint32_t requestedFallbackBpmMilli =
        static_cast<std::uint32_t>(state.bpm) * 1000U;
    const std::uint16_t requestedMinimumBpm = state.tempoRange.minimumBpm != 0U
        ? state.tempoRange.minimumBpm
        : 1U;
    if (settingsEqual(state.externalSync, foregroundSettings_) &&
        requestedFallbackBpmMilli == foregroundFallbackBpmMilli_ &&
        requestedMinimumBpm == foregroundMinimumBpm_) {
        return;
    }

    foregroundSettings_ = state.externalSync;
    foregroundFallbackBpmMilli_ = requestedFallbackBpmMilli;
    foregroundMinimumBpm_ = requestedMinimumBpm;

    hal::InterruptLock interruptLock;
    const bool resetModeChanged = foregroundSettings_.resetMode != settings_.resetMode;
    settings_ = foregroundSettings_;
    fallbackBpmMilli_ = foregroundFallbackBpmMilli_;
    minimumBpm_ = foregroundMinimumBpm_;
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
    bool sawEdge = false;
    bool triggerRequested = false;
    while (inputCapture_.popResetEdge(edge)) {
        sawEdge = true;
        if (settings_.resetMode == ExternalResetMode::Trigger) {
            // Several comparator transitions may accumulate while TIM3 is delayed.
            // They are not musically distinguishable inside one 50-us scheduler
            // quantum, so collapse them to at most one expensive engine reset.
            // Continuity loss is handled conservatively: a rising edge may have
            // been among the dropped transitions, therefore perform one reset.
            triggerRequested = triggerRequested || edge.high || edge.continuityLost;
        }
    }

    if (settings_.resetMode == ExternalResetMode::Trigger) {
        if (triggerRequested) {
            engine_.resetGlobalPhaseFromIsr();
        }
        return;
    }

    if (!sawEdge) {
        return;
    }
    // Gate reset is level-sensitive. The separately captured current comparator
    // level is authoritative even if the edge queue overflowed and collapsed
    // intermediate transitions.
    const bool high = inputCapture_.resetLevelHigh();
    if (high != resetGateApplied_) {
        resetGateApplied_ = high;
        engine_.setExternalResetGateFromIsr(high);
    }
}

void ExternalSyncController::processSyncEdges() {
    hal::ExternalInputEdge edge{};
    while (inputCapture_.popSyncEdge(edge)) {
        if (edge.continuityLost) {
            // A loss marker describes a boundary, not a trustworthy pulse. It may
            // also have the opposite polarity from the selected edge because the
            // queue captures CHANGE transitions. Reset acquisition and wait for the
            // next real selected edge instead of manufacturing a long period across
            // transitions that were dropped while the queue was saturated.
            haveAcceptedPulse_ = false;
            filteredPeriodQ8_ = 0U;
            continue;
        }

        if (!selectedSyncEdge(edge.high)) {
            continue;
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
        const std::uint32_t ppqn = settings_.pulsesPerQuarterNote != 0U
            ? settings_.pulsesPerQuarterNote
            : 1U;
        const std::uint32_t minimumSupportedPeriodUs = static_cast<std::uint32_t>(
            60000000ULL /
            (static_cast<std::uint64_t>(config::kSupportedMaximumBpm) * ppqn));
        const std::uint32_t effectiveGlitchFloorUs =
            settings_.glitchFilterUs > minimumSupportedPeriodUs
                ? settings_.glitchFilterUs
                : minimumSupportedPeriodUs;
        if (periodUs == 0U || periodUs < effectiveGlitchFloorUs) {
            continue;
        }

        const std::uint64_t maximumSupportedPeriodUs = 60000000ULL /
            (static_cast<std::uint64_t>(config::kSupportedMinimumBpm) * ppqn);
        if (static_cast<std::uint64_t>(periodUs) > maximumSupportedPeriodUs) {
            // Treat an out-of-range slow gap as a fresh acquisition boundary,
            // never as a valid ultra-low tempo. This also keeps the arithmetic
            // contract aligned with the documented 1..999 BPM technical range.
            haveAcceptedPulse_ = true;
            filteredPeriodQ8_ = 0U;
            lastAcceptedPulseUs_ = edge.timestampUs;
            filteredBpmMilli_ = fallbackBpmMilli_;
            engine_.acceptExternalPulseFromIsr(
                filteredBpmMilli_,
                settings_.pulsesPerQuarterNote);
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
    const std::uint64_t maximumSupportedMilliBpm =
        static_cast<std::uint64_t>(config::kSupportedMaximumBpm) * 1000ULL;
    return static_cast<std::uint32_t>(
        value > maximumSupportedMilliBpm ? maximumSupportedMilliBpm : value);
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
