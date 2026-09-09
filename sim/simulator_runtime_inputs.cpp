/**
 * @file simulator_runtime_inputs.cpp
 * @brief Virtual SYNC/RST source and comparator behavior for the native simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "simulator_runtime.h"

#include <algorithm>
#include <cstdint>


#include "domain/clock_options.h"

namespace clockfw::sim {
namespace {

constexpr std::uint32_t kResetPeriodStepMs = 100U;
constexpr std::uint32_t kResetMinimumPeriodMs = 100U;
constexpr std::uint32_t kResetMaximumPeriodMs = 60000U;

}  // namespace

void SimulatorRuntime::setSyncCableConnected(const bool connected) {
    syncCableConnected_ = connected;
    if (connected) syncGeneratorRunning_ = true;
    resetExternalSyncAcquisition();
}

void SimulatorRuntime::setSyncGeneratorRunning(const bool running) {
    if (syncGeneratorRunning_ == running) return;
    syncGeneratorRunning_ = running;
    if (running) {
        if (!syncCableConnected_) syncCableConnected_ = true;
        resetExternalSyncAcquisition();
        return;
    }

    // Stopping the source while the cable remains connected models clock loss, not
    // cable removal. Preserve the firmware lock until its own adaptive timeout expires.
    if (syncComparatorHigh_ && application_ != nullptr && application_->runningForSimulator()) {
        application_->injectExternalSyncLevelForSimulator(false);
    }
    syncComparatorHigh_ = false;
}

void SimulatorRuntime::setSyncBpmMilli(const std::uint32_t bpmMilli) {
    syncBpmMilli_ = std::clamp<std::uint32_t>(bpmMilli, 1000U, 999000U);
    resetExternalSyncAcquisition();
}

void SimulatorRuntime::adjustSyncBpm(const int deltaBpm) {
    const int currentBpm = static_cast<int>(syncBpmMilli_ / 1000U);
    const int adjustedBpm = std::clamp(currentBpm + deltaBpm, 1, 999);
    setSyncBpmMilli(static_cast<std::uint32_t>(adjustedBpm) * 1000U);
}

void SimulatorRuntime::cycleSyncPpqn() {
    const auto& values = clockfw::kExternalPpqnOptions;
    std::size_t index = 0U;
    for (; index < values.size(); ++index) {
        if (values[index] == syncPpqn_) break;
    }
    syncPpqn_ = values[(index + 1U) % values.size()];
    resetExternalSyncAcquisition();
}

void SimulatorRuntime::cycleSyncWaveform() {
    syncWaveform_ = nextSignalWaveform(syncWaveform_);
    resetExternalSyncAcquisition();
}

SyncInputTelemetry SimulatorRuntime::syncInputTelemetry() const {
    engine::EngineSnapshot engineSnapshot{};
    if (application_ != nullptr && application_->runningForSimulator()) {
        engineSnapshot = application_->engineSnapshotForSimulator();
    }
    return {
        syncCableConnected_, syncGeneratorRunning_, syncComparatorHigh_, syncLocked_, syncWaveform_,
        syncBpmMilli_, syncPpqn_, state().externalSync.pulsesPerQuarterNote,
        engineSnapshot.externalBpmMilli, syncPulseCount_};
}

void SimulatorRuntime::setResetCableConnected(const bool connected) {
    resetCableConnected_ = connected;
    if (connected) resetGeneratorRunning_ = true;
    resetExternalResetAcquisition();
}

void SimulatorRuntime::setResetGeneratorRunning(const bool running) {
    resetGeneratorRunning_ = running;
    if (running && !resetCableConnected_) resetCableConnected_ = true;
    resetExternalResetAcquisition();
}

void SimulatorRuntime::adjustResetPeriod(const int deltaSteps) {
    const std::int64_t adjusted = static_cast<std::int64_t>(resetPeriodMs_) +
        static_cast<std::int64_t>(deltaSteps) * kResetPeriodStepMs;
    resetPeriodMs_ = static_cast<std::uint32_t>(std::clamp<std::int64_t>(
        adjusted, kResetMinimumPeriodMs, kResetMaximumPeriodMs));
    resetExternalResetAcquisition();
}

void SimulatorRuntime::cycleResetWaveform() {
    resetWaveform_ = nextSignalWaveform(resetWaveform_);
    resetExternalResetAcquisition();
}

void SimulatorRuntime::triggerResetPulse() {
    if (application_ == nullptr || !application_->runningForSimulator()) return;
    application_->injectExternalResetLevelForSimulator(true);
    application_->injectExternalResetLevelForSimulator(false);
    ++resetCount_;
}

ResetInputTelemetry SimulatorRuntime::resetInputTelemetry() const {
    return {resetCableConnected_, resetGeneratorRunning_, resetComparatorHigh_,
        resetWaveform_, resetPeriodMs_, resetCount_};
}

void SimulatorRuntime::serviceExternalSync() {
    if (!syncCableConnected_ || !poweredOn_ || application_ == nullptr ||
        !application_->runningForSimulator()) {
        syncComparatorHigh_ = false;
        return;
    }

    if (syncGeneratorRunning_) {
        const std::uint64_t denominator = static_cast<std::uint64_t>(syncBpmMilli_) * syncPpqn_;
        const std::uint64_t periodUs = denominator != 0ULL ? 60000000000ULL / denominator : 60000000ULL;
        const bool high = comparatorHigh(
            syncWaveform_, nowMicroseconds(), syncEpochUs_, std::max<std::uint64_t>(periodUs, 1ULL));
        if (syncComparatorHigh_ != high) {
            application_->injectExternalSyncLevelForSimulator(high);
            if (high) {
                syncLastPulseUs_ = nowMicroseconds();
                ++syncPulseCount_;
            }
        }
        syncComparatorHigh_ = high;
    } else {
        syncComparatorHigh_ = false;
    }

    // The firmware owns lock-loss timing. Mirroring the engine snapshot here avoids a
    // second simulator-only timeout policy that can disagree with slow-clock behavior.
    syncLocked_ = application_->engineSnapshotForSimulator().externalLocked;
}

void SimulatorRuntime::serviceExternalReset() {
    if (!resetCableConnected_ || !resetGeneratorRunning_ || !poweredOn_ ||
        application_ == nullptr || !application_->runningForSimulator()) {
        resetComparatorHigh_ = false;
        return;
    }
    const std::uint64_t periodUs = static_cast<std::uint64_t>(resetPeriodMs_) * 1000ULL;
    const bool high = comparatorHigh(
        resetWaveform_, nowMicroseconds(), resetEpochUs_, std::max<std::uint64_t>(periodUs, 1ULL));
    if (resetComparatorHigh_ != high) {
        application_->injectExternalResetLevelForSimulator(high);
        if (high) {
            ++resetCount_;
        }
    }
    resetComparatorHigh_ = high;
}

void SimulatorRuntime::resetExternalSyncAcquisition() {
    if (syncLocked_ && application_ != nullptr && application_->runningForSimulator()) {
        application_->clearExternalSyncForSimulator();
    }
    syncLocked_ = false;
    syncComparatorHigh_ = false;
    syncEpochUs_ = nowMicroseconds();
    syncLastPulseUs_ = 0ULL;
    syncPulseCount_ = 0ULL;
}

void SimulatorRuntime::resetExternalResetAcquisition() {
    if (resetComparatorHigh_ && application_ != nullptr && application_->runningForSimulator()) {
        application_->injectExternalResetLevelForSimulator(false);
    }
    resetComparatorHigh_ = false;
    resetEpochUs_ = nowMicroseconds();
}

}  // namespace clockfw::sim
