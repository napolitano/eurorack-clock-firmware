/**
 * @file external_sync_controller.cpp
 * @brief Deterministic scheduler-side processing for configurable external inputs.
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

/** Microseconds in one minute, used for period/BPM conversion. */
constexpr std::uint64_t kMicrosPerMinute = 60000000ULL;

/** Microseconds-per-minute scaled by 1000 for milli-BPM calculations. */
constexpr std::uint64_t kMicrosPerMinuteMilliBpm = kMicrosPerMinute * 1000ULL;

bool settingsEqual(
    const ExternalSyncSettings& first,
    const ExternalSyncSettings& second) {
    return first.pulsesPerQuarterNote == second.pulsesPerQuarterNote &&
        first.edge == second.edge &&
        first.lossMode == second.lossMode &&
        first.resetMode == second.resetMode &&
        first.smoothing == second.smoothing &&
        first.glitchFilterUs == second.glitchFilterUs &&
        first.timeoutMs == second.timeoutMs;
}

bool inputsEqual(
    const ExternalInputAssignments& first,
    const ExternalInputAssignments& second) {
    return first.input1 == second.input1 && first.input2 == second.input2;
}

int assignmentIndex(
    const ExternalInputAssignments& inputs,
    const InputFunction function) {
    if (inputs.input1 == function) {
        return 0;
    }
    if (inputs.input2 == function) {
        return 1;
    }
    return -1;
}

}  // namespace

ExternalSyncController::ExternalSyncController(
    hal::ExternalInputCapture& inputCapture,
    engine::ClockEngine& engine)
    : inputCapture_(inputCapture), engine_(engine) {}

void ExternalSyncController::begin(const ClockState& state) {
    foregroundSettings_ = state.externalSync;
    foregroundInputs_ = state.inputs;
    foregroundFallbackBpmMilli_ = static_cast<std::uint32_t>(state.bpm) * 1000U;
    foregroundMinimumBpm_ = state.tempoRange.minimumBpm != 0U
        ? state.tempoRange.minimumBpm
        : 1U;
    foregroundSource_ = state.source;

    settings_ = foregroundSettings_;
    inputs_ = foregroundInputs_;
    fallbackBpmMilli_ = foregroundFallbackBpmMilli_;
    minimumBpm_ = foregroundMinimumBpm_;
    source_ = foregroundSource_;

    // Initial level-sensitive roles must be applied, but startup must not discard
    // a real edge that arrived after ExternalInputCapture::begin().
    configurationDirty_ = true;
    inputRolesChanged_ = false;
    resetGateApplied_ = false;
    const int runInput = assignmentIndex(inputs_, InputFunction::Run);
    runLevelInitialized_ = runInput >= 0;
    runLevelApplied_ = runInput >= 0 ? inputLevelHigh(runInput) : false;

    haveReferencePulse_ = false;
    externalLocked_ = false;
    lastAcceptedPulseUs_ = 0U;
    filteredPeriodQ8_ = 0U;
    filteredBpmMilli_ = 0U;
    autoTransportArmed_ = true;
    pendingTransportTransition_ = kNoPendingTransportTransition;
    pendingTap_ = false;
    pendingTapTimestampUs_ = 0U;
}

void ExternalSyncController::updateConfiguration(const ClockState& state) {
    const std::uint32_t requestedFallbackBpmMilli =
        static_cast<std::uint32_t>(state.bpm) * 1000U;
    const std::uint16_t requestedMinimumBpm = state.tempoRange.minimumBpm != 0U
        ? state.tempoRange.minimumBpm
        : 1U;

    if (settingsEqual(state.externalSync, foregroundSettings_) &&
        inputsEqual(state.inputs, foregroundInputs_) &&
        requestedFallbackBpmMilli == foregroundFallbackBpmMilli_ &&
        requestedMinimumBpm == foregroundMinimumBpm_ &&
        state.source == foregroundSource_) {
        return;
    }

    const bool timingInterpretationChanged =
        state.externalSync.pulsesPerQuarterNote != foregroundSettings_.pulsesPerQuarterNote ||
        state.externalSync.edge != foregroundSettings_.edge ||
        assignmentIndex(state.inputs, InputFunction::Sync) !=
            assignmentIndex(foregroundInputs_, InputFunction::Sync);
    const bool rolesChanged = !inputsEqual(state.inputs, foregroundInputs_);

    foregroundSettings_ = state.externalSync;
    foregroundInputs_ = state.inputs;
    foregroundFallbackBpmMilli_ = requestedFallbackBpmMilli;
    foregroundMinimumBpm_ = requestedMinimumBpm;
    foregroundSource_ = state.source;

    hal::InterruptLock interruptLock;
    const bool resetModeChanged = foregroundSettings_.resetMode != settings_.resetMode;
    settings_ = foregroundSettings_;
    inputs_ = foregroundInputs_;
    fallbackBpmMilli_ = foregroundFallbackBpmMilli_;
    minimumBpm_ = foregroundMinimumBpm_;
    source_ = foregroundSource_;
    configurationDirty_ = configurationDirty_ || resetModeChanged || rolesChanged;
    inputRolesChanged_ = inputRolesChanged_ || rolesChanged;

    // A role change can turn already queued electrical edges into a different
    // musical command. The scheduler discards those stale edges before interpreting
    // the new assignment. Timing-role changes also invalidate clock acquisition.
    if (timingInterpretationChanged) {
        haveReferencePulse_ = false;
        externalLocked_ = false;
        lastAcceptedPulseUs_ = 0U;
        filteredPeriodQ8_ = 0U;
        filteredBpmMilli_ = fallbackBpmMilli_;
        engine_.setExternalLockFromIsr(false, filteredBpmMilli_);
    }
}

void ExternalSyncController::processSchedulerTick(const std::uint32_t nowUs) {
    applyInputConfiguration();

    // Preserve the historical reset-before-clock rule independently of which
    // physical jack currently owns RESET or SYNC.
    processResetEdges();

    // Edge-controlled transport commands are deterministic; STOP is processed
    // after START/RESTART so a simultaneous STOP wins. RUN is applied last and is
    // level-authoritative when assigned.
    processTransportEdge(InputFunction::Start);
    processTransportEdge(InputFunction::Restart);
    processTransportEdge(InputFunction::Tap);
    processTransportEdge(InputFunction::Stop);
    processSyncEdges();
    processRunLevel();
    drainInactiveInputs();

    if (!externalLocked_ || !haveReferencePulse_) {
        return;
    }
    const std::uint32_t timeoutUs = effectiveTimeoutUs();
    if (static_cast<std::uint32_t>(nowUs - lastAcceptedPulseUs_) < timeoutUs) {
        return;
    }
    loseExternalLockFromIsr();
}

void ExternalSyncController::clearExternalLock() {
    hal::InterruptLock interruptLock;
    loseExternalLockFromIsr();
}

std::uint32_t ExternalSyncController::filteredBpmMilli() const {
    return filteredBpmMilli_;
}

void ExternalSyncController::notifyManualTransportState(const TransportState transport) {
    hal::InterruptLock interruptLock;
    autoTransportArmed_ = transport == TransportState::Playing;
    if (assignedInput(InputFunction::Run) >= 0) {
        // A wired RUN level remains authoritative over a front-panel transport
        // action; re-apply it at the next scheduler boundary.
        runLevelInitialized_ = false;
    }
}

bool ExternalSyncController::consumeTransportTransition(TransportState& transport) {
    hal::InterruptLock interruptLock;
    if (pendingTransportTransition_ == kNoPendingTransportTransition) {
        return false;
    }
    transport = static_cast<TransportState>(pendingTransportTransition_);
    pendingTransportTransition_ = kNoPendingTransportTransition;
    return true;
}

bool ExternalSyncController::consumeTapRequest(std::uint32_t& timestampUs) {
    hal::InterruptLock interruptLock;
    if (!pendingTap_) {
        return false;
    }
    timestampUs = pendingTapTimestampUs_;
    pendingTap_ = false;
    return true;
}

void ExternalSyncController::processSyncEdges() {
    const int inputIndex = assignedInput(InputFunction::Sync);
    if (inputIndex < 0) {
        return;
    }

    hal::ExternalInputEdge edge{};
    while (popInputEdge(inputIndex, edge)) {
        if (edge.continuityLost) {
            if (externalLocked_) {
                loseExternalLockFromIsr();
            } else {
                haveReferencePulse_ = false;
                filteredPeriodQ8_ = 0U;
            }
            continue;
        }

        if (!selectedSyncEdge(edge.high)) {
            continue;
        }

        if (!haveReferencePulse_) {
            haveReferencePulse_ = true;
            lastAcceptedPulseUs_ = edge.timestampUs;
            continue;
        }

        const std::uint32_t periodUs = edge.timestampUs - lastAcceptedPulseUs_;
        const std::uint32_t ppqn = settings_.pulsesPerQuarterNote != 0U
            ? settings_.pulsesPerQuarterNote
            : 1U;
        const std::uint32_t minimumSupportedPeriodUs = static_cast<std::uint32_t>(
            kMicrosPerMinute /
            (static_cast<std::uint64_t>(config::kSupportedMaximumBpm) * ppqn));
        const std::uint32_t effectiveGlitchFloorUs =
            settings_.glitchFilterUs > minimumSupportedPeriodUs
                ? settings_.glitchFilterUs
                : minimumSupportedPeriodUs;

        // Rejected short edges do not move lastAcceptedPulseUs_. A comparator
        // glitch therefore cannot become the reference for the next valid period.
        if (periodUs == 0U || periodUs < effectiveGlitchFloorUs) {
            continue;
        }

        const std::uint64_t maximumSupportedPeriodUs = kMicrosPerMinute /
            (static_cast<std::uint64_t>(config::kSupportedMinimumBpm) * ppqn);
        if (static_cast<std::uint64_t>(periodUs) > maximumSupportedPeriodUs) {
            if (externalLocked_) {
                loseExternalLockFromIsr();
            }
            haveReferencePulse_ = true;
            filteredPeriodQ8_ = 0U;
            lastAcceptedPulseUs_ = edge.timestampUs;
            continue;
        }

        lastAcceptedPulseUs_ = edge.timestampUs;
        const std::uint64_t periodQ8 = static_cast<std::uint64_t>(periodUs) << 8U;
        if (filteredPeriodQ8_ == 0U) {
            filteredPeriodQ8_ = periodQ8;
        } else {
            // Integer Q8 filters keep the scheduler free of floating-point state.
            // Low weights the new period 75%, Medium 50%, Full 25%.
            switch (settings_.smoothing) {
                case SyncSmoothing::Off:
                    filteredPeriodQ8_ = periodQ8;
                    break;
                case SyncSmoothing::Low:
                    filteredPeriodQ8_ = (filteredPeriodQ8_ + periodQ8 * 3U) / 4U;
                    break;
                case SyncSmoothing::Medium:
                    filteredPeriodQ8_ = (filteredPeriodQ8_ + periodQ8) / 2U;
                    break;
                case SyncSmoothing::Full:
                default:
                    filteredPeriodQ8_ = (filteredPeriodQ8_ * 3U + periodQ8) / 4U;
                    break;
            }
        }

        const std::uint32_t filteredPeriodUs = static_cast<std::uint32_t>(
            (filteredPeriodQ8_ + 128U) >> 8U);
        filteredBpmMilli_ = calculateBpmMilli(filteredPeriodUs);
        const bool acquiredLock = !externalLocked_;
        externalLocked_ = true;
        engine_.acceptExternalPulseFromIsr(
            filteredBpmMilli_,
            settings_.pulsesPerQuarterNote);
        if (acquiredLock) {
            startOnLockAcquisitionFromIsr();
        }
    }
}

int ExternalSyncController::assignedInput(const InputFunction function) const {
    return assignmentIndex(inputs_, function);
}

bool ExternalSyncController::popInputEdge(
    const int inputIndex,
    hal::ExternalInputEdge& edge) {
    if (inputIndex == 0) {
        return inputCapture_.popSyncEdge(edge);
    }
    if (inputIndex == 1) {
        return inputCapture_.popResetEdge(edge);
    }
    return false;
}

bool ExternalSyncController::inputLevelHigh(const int inputIndex) const {
    return inputIndex == 0
        ? inputCapture_.syncLevelHigh()
        : (inputIndex == 1 && inputCapture_.resetLevelHigh());
}

void ExternalSyncController::drainInput(const int inputIndex) {
    hal::ExternalInputEdge edge{};
    while (popInputEdge(inputIndex, edge)) {
    }
}

void ExternalSyncController::requestPlayFromInput() {
    autoTransportArmed_ = true;
    engine_.play();
    pendingTransportTransition_ = static_cast<std::uint8_t>(TransportState::Playing);
}

void ExternalSyncController::requestStopFromInput() {
    autoTransportArmed_ = false;
    engine_.stop();
    pendingTransportTransition_ = static_cast<std::uint8_t>(TransportState::Stopped);
}

void ExternalSyncController::requestRestartFromInput() {
    autoTransportArmed_ = true;
    engine_.stop();
    engine_.play();
    pendingTransportTransition_ = static_cast<std::uint8_t>(TransportState::Playing);
}

void ExternalSyncController::loseExternalLockFromIsr() {
    const bool wasLocked = externalLocked_;
    externalLocked_ = false;
    haveReferencePulse_ = false;
    filteredPeriodQ8_ = 0U;
    engine_.setExternalLockFromIsr(false, filteredBpmMilli_);

    if (!wasLocked ||
        settings_.lossMode != SyncLossMode::Stop ||
        (source_ != ClockSource::External && source_ != ClockSource::Auto) ||
        !autoTransportArmed_) {
        return;
    }

    engine_.stop();
    pendingTransportTransition_ = static_cast<std::uint8_t>(TransportState::Stopped);
}

void ExternalSyncController::startOnLockAcquisitionFromIsr() {
    if ((source_ != ClockSource::External && source_ != ClockSource::Auto) ||
        !autoTransportArmed_) {
        return;
    }

    engine_.play();
    pendingTransportTransition_ = static_cast<std::uint8_t>(TransportState::Playing);
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
    constexpr std::uint64_t kMissedPulseTolerance = 2ULL;
    const std::uint64_t configuredUs =
        static_cast<std::uint64_t>(settings_.timeoutMs) * 1000ULL;
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
