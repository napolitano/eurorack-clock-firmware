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

void ClockVcvRuntime::processSample(
    const double sampleTimeSeconds,
    const bool syncConnected,
    const float syncVoltage,
    const bool resetConnected,
    const float resetVoltage) {
    const bool nextSyncHigh = updateInputLevel(syncVoltage, syncHigh_);
    const bool nextResetHigh = updateInputLevel(resetVoltage, resetHigh_);
    syncHigh_ = nextSyncHigh;
    resetHigh_ = nextResetHigh;
    runtime_.setExternalSyncInput(syncConnected, syncHigh_);
    runtime_.setExternalResetInput(resetConnected, resetHigh_);

    if (sampleTimeSeconds <= 0.0 || !std::isfinite(sampleTimeSeconds)) {
        return;
    }

    pendingSchedulerUs_ += sampleTimeSeconds * 1000000.0;
    const double schedulerQuantumUs = static_cast<double>(config::kSchedulerTickUs);
    while (pendingSchedulerUs_ >= schedulerQuantumUs) {
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

bool ClockVcvRuntime::updateInputLevel(const float voltage, const bool previousLevel) {
    if (previousLevel) {
        return voltage > kInputLowThresholdVolts;
    }
    return voltage >= kInputHighThresholdVolts;
}

}  // namespace clockfw::vcv
