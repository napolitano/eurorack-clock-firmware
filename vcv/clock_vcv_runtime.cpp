/**
 * @file clock_vcv_runtime.cpp
 * @brief Rack-independent bridge implementation for the experimental VCV Rack port.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "clock_vcv_runtime.h"

#include <algorithm>
#include <cmath>

#include "config.h"

namespace clockfw::vcv {

ClockVcvRuntime::ClockVcvRuntime(std::filesystem::path persistencePath)
    : runtime_(std::move(persistencePath)) {}

void ClockVcvRuntime::begin() {
    runtime_.begin();
}

void ClockVcvRuntime::InputTransitionBridge::disconnect() noexcept {
    head = 0U;
    count = 0U;
    connected = false;
    sampledHigh = false;
    schedulerHigh = false;
}

void ClockVcvRuntime::InputTransitionBridge::update(
    const bool nextConnected,
    const bool nextHigh) noexcept {
    if (!nextConnected) {
        disconnect();
        return;
    }

    const bool effectiveHigh = nextHigh;
    if (!connected) {
        connected = true;
        sampledHigh = false;
        schedulerHigh = false;
        head = 0U;
        count = 0U;
    }

    if (effectiveHigh == sampledHigh) {
        return;
    }
    sampledHigh = effectiveHigh;

    if (count < kCapacity) {
        const std::size_t tail = (head + count) % kCapacity;
        pending[tail] = effectiveHigh;
        ++count;
        return;
    }

    // The production scheduler cannot consume more than one input transition per 50-us tick.
    // If a pathological Rack signal outruns that boundary, discard stale backlog and preserve
    // the latest sampled level rather than allocating or blocking on the audio thread.
    head = 0U;
    count = 1U;
    pending[0] = effectiveHigh;
}

bool ClockVcvRuntime::InputTransitionBridge::nextSchedulerLevel() noexcept {
    if (!connected) {
        schedulerHigh = false;
        return false;
    }
    if (count == 0U) {
        schedulerHigh = sampledHigh;
        return schedulerHigh;
    }

    schedulerHigh = pending[head];
    head = (head + 1U) % kCapacity;
    --count;
    return schedulerHigh;
}

void ClockVcvRuntime::processSample(
    const double sampleTimeSeconds,
    const bool syncConnected,
    const float syncVoltage,
    const bool resetConnected,
    const float resetVoltage) {
    const bool previousSyncHigh = syncBridge_.sampledHigh;
    const bool previousResetHigh = resetBridge_.sampledHigh;
    const bool nextSyncHigh = syncConnected && updateInputLevel(syncVoltage, previousSyncHigh);
    const bool nextResetHigh = resetConnected && updateInputLevel(resetVoltage, previousResetHigh);
    syncBridge_.update(syncConnected, nextSyncHigh);
    resetBridge_.update(resetConnected, nextResetHigh);

    // Preserve the simulator's immediate cable-removal semantics even when this Rack sample does
    // not advance a complete 50-us scheduler quantum.
    if (!syncConnected) {
        runtime_.setExternalSyncInput(false, false);
    }
    if (!resetConnected) {
        runtime_.setExternalResetInput(false, false);
    }

    if (sampleTimeSeconds <= 0.0 || !std::isfinite(sampleTimeSeconds)) {
        return;
    }

    pendingSchedulerUs_ += sampleTimeSeconds * 1000000.0;
    const double schedulerQuantumUs = static_cast<double>(config::kSchedulerTickUs);
    while (pendingSchedulerUs_ >= schedulerQuantumUs) {
        runtime_.setExternalSyncInput(syncBridge_.connected, syncBridge_.nextSchedulerLevel());
        runtime_.setExternalResetInput(resetBridge_.connected, resetBridge_.nextSchedulerLevel());
        runtime_.advanceMicroseconds(config::kSchedulerTickUs);
        pendingSchedulerUs_ -= schedulerQuantumUs;
    }
}

void ClockVcvRuntime::setPanelControls(const PanelControls& controls) {
    if (controls.encoderPressed != controls_.encoderPressed) {
        runtime_.setButton(sim::SimButton::Encoder, controls.encoderPressed);
    }
    if (controls.playPressed != controls_.playPressed) {
        runtime_.setButton(sim::SimButton::Play, controls.playPressed);
    }
    if (controls.tapPressed != controls_.tapPressed) {
        runtime_.setButton(sim::SimButton::Tap, controls.tapPressed);
    }
    if (controls.stopPressed != controls_.stopPressed) {
        runtime_.setButton(sim::SimButton::Stop, controls.stopPressed);
    }
    controls_ = controls;
}

void ClockVcvRuntime::rotateEncoder(const int detents) {
    if (detents != 0) {
        runtime_.rotateEncoder(detents);
    }
}

bool ClockVcvRuntime::openGeneralSettings() {
    return runtime_.openGeneralSettings();
}

float ClockVcvRuntime::gateVoltage(const std::size_t channelIndex) const {
    if (channelIndex >= runtime_.telemetry().size()) {
        return 0.0F;
    }
    return runtime_.telemetry()[channelIndex].logicHigh ? kGateHighVoltage : 0.0F;
}

const std::array<std::uint8_t, hal::OledDisplay::kFramebufferSize>&
ClockVcvRuntime::framebuffer() const {
    return runtime_.framebuffer();
}

std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes>
ClockVcvRuntime::persistenceImage() const {
    return runtime_.persistenceImage();
}

void ClockVcvRuntime::restorePersistenceImage(
    const std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes>& image) {
    runtime_.restorePersistenceImage(image);
    pendingSchedulerUs_ = 0.0;
}

bool ClockVcvRuntime::running() const {
    return runtime_.poweredOn() && !runtime_.easterEggActive();
}

std::uint64_t ClockVcvRuntime::syncPulseCountForTest() const {
    return runtime_.syncInputTelemetry().pulseCount;
}

std::uint64_t ClockVcvRuntime::resetPulseCountForTest() const {
    return runtime_.resetInputTelemetry().resetCount;
}

bool ClockVcvRuntime::generalSettingsOpenForTest() const {
    return runtime_.generalSettingsOpen();
}

bool ClockVcvRuntime::updateInputLevel(const float voltage, const bool previousLevel) {
    if (previousLevel) {
        return voltage > kInputLowThresholdVolts;
    }
    return voltage >= kInputHighThresholdVolts;
}

}  // namespace clockfw::vcv
