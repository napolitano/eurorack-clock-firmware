/**
 * @file simulator_runtime.cpp
 * @brief Native host runtime that advances the real firmware against simulated hardware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "simulator_runtime.h"

#include <algorithm>
#include <array>
#include <stdexcept>
#include <utility>

#include <Arduino.h>
#include <HardwareTimer.h>

#include "config.h"
#include "domain/clock_options.h"
#include "domain/default_configuration.h"
#include "pin_map.h"
#include "scope_timeline.h"

namespace clockfw::sim {
namespace {

constexpr std::uint64_t kForegroundPeriodUs = 1000ULL;
constexpr std::uint64_t kPersistenceFlushPeriodUs = 500000ULL;
constexpr std::uint64_t kMinimumButtonPulseUs = 30000ULL;

constexpr std::array<std::uint8_t, 5U> kClockwisePhases{{3U, 1U, 0U, 2U, 3U}};
constexpr std::array<std::uint8_t, 5U> kCounterClockwisePhases{{3U, 2U, 0U, 1U, 3U}};

}  // namespace

SimulatorRuntime::SimulatorRuntime(std::filesystem::path persistencePath)
    : persistence_(std::move(persistencePath)) {
    initializeFactoryDefaults(poweredOffState_);
    poweredOffState_.transport = TransportState::Stopped;
}

void SimulatorRuntime::begin() {
    persistence_.load();
    setPower(true);
}

void SimulatorRuntime::advanceMicroseconds(std::uint64_t durationUs) {
    if (!poweredOn_ || application_ == nullptr) {
        return;
    }
    const std::uint64_t schedulerPeriodUs = config::kSchedulerTickUs;
    if (schedulerPeriodUs == 0ULL) {
        throw std::runtime_error("Scheduler period must be non-zero");
    }

    while (durationUs >= schedulerPeriodUs) {
        simfw::advanceMicroseconds(schedulerPeriodUs);
        serviceExternalSync();
        serviceExternalReset();
        simfw::fireTimer();
        durationUs -= schedulerPeriodUs;
        foregroundAccumulatorUs_ += schedulerPeriodUs;
        persistenceAccumulatorUs_ += schedulerPeriodUs;

        if (foregroundAccumulatorUs_ >= kForegroundPeriodUs) {
            foregroundAccumulatorUs_ -= kForegroundPeriodUs;
            serviceEncoderSequence();
            serviceDeferredButtonReleases();
            application_->runOnce();
            serviceTransportTelemetry();
        }
        if (persistenceAccumulatorUs_ >= kPersistenceFlushPeriodUs) {
            persistenceAccumulatorUs_ -= kPersistenceFlushPeriodUs;
            persistence_.flushIfChanged();
        }
    }

    if (durationUs > 0ULL) {
        simfw::advanceMicroseconds(durationUs);
        serviceExternalSync();
        serviceExternalReset();
        foregroundAccumulatorUs_ += durationUs;
        persistenceAccumulatorUs_ += durationUs;
    }

    collectGateTransitions();
    pruneTelemetry();
}

void SimulatorRuntime::rotateEncoder(const int detents) {
    encoderVisualPosition_ += static_cast<std::int64_t>(detents);
    if (!poweredOn_) {
        return;
    }
    pendingEncoderDetents_ += detents;
    pendingEncoderDetents_ = std::clamp(pendingEncoderDetents_, -64, 64);
}

void SimulatorRuntime::setButton(const SimButton button, const bool pressed) {
    const std::size_t index = static_cast<std::size_t>(button);
    buttonPressed_[index] = pressed;
    if (!poweredOn_) {
        return;
    }
    if (pressed) {
        simfw::setPin(buttonPin(button), LOW);
        minimumButtonReleaseUs_[index] = simfw::nowUs + kMinimumButtonPulseUs;
        deferredButtonRelease_[index] = false;
        return;
    }

    if (simfw::nowUs < minimumButtonReleaseUs_[index]) {
        deferredButtonRelease_[index] = true;
        return;
    }
    simfw::setPin(buttonPin(button), HIGH);
}

void SimulatorRuntime::flushPersistence() {
    persistence_.flushIfChanged();
}

const std::array<std::uint8_t, hal::OledDisplay::kFramebufferSize>&
SimulatorRuntime::framebuffer() const {
    if (!poweredOn_ || application_ == nullptr) {
        return poweredOffFramebuffer_;
    }
    return application_->displayForSimulator().framebufferForTest();
}

const ClockState& SimulatorRuntime::state() const {
    return application_ != nullptr ? application_->stateForSimulator() : poweredOffState_;
}

const std::array<ChannelTelemetry, kChannelCount>& SimulatorRuntime::telemetry() const {
    return telemetry_;
}

std::uint64_t SimulatorRuntime::nowMicroseconds() const {
    return simfw::nowUs;
}

TransportTelemetry SimulatorRuntime::transportTelemetry() const {
    return {observedTransport_, transportStartSequence_, lastTransportStartUs_, lastTransportStopUs_};
}

void SimulatorRuntime::setTelemetryHistoryFrozen(const bool frozen) {
    telemetryHistoryFrozen_ = frozen;
}

std::int64_t SimulatorRuntime::encoderVisualPosition() const {
    return encoderVisualPosition_;
}

bool SimulatorRuntime::outputStageEnabled() const {
    return poweredOn_ &&
        simfw::pinValue(pinmap::kGateBufferOutputEnablePin) == pinmap::kGateBufferEnabledLevel;
}

bool SimulatorRuntime::poweredOn() const {
    return poweredOn_;
}

void SimulatorRuntime::setPower(const bool powered) {
    if (powered == poweredOn_) {
        return;
    }
    if (!powered) {
        if (application_ != nullptr) {
            poweredOffState_ = application_->stateForSimulator();
            poweredOffState_.transport = TransportState::Stopped;
            application_->powerOffForSimulator();
            collectGateTransitions();
        }
        for (ChannelTelemetry& channel : telemetry_) {
            channel.logicHigh = false;
        }
        persistence_.flushIfChanged();
        application_.reset();
        simfw::resetTimer();
        poweredOn_ = false;
        resetExternalSyncAcquisition();
        resetExternalResetAcquisition();
        return;
    }

    simfw::resetArduino();
    simfw::resetTimer();
    setEncoderPhase(3U);
    applyStoredButtonLevels();
    persistence_.load();
    application_ = std::make_unique<app::ClockApplication>();
    poweredOn_ = true;
    telemetry_ = {};
    processedWriteCount_ = simfw::writes.size();
    foregroundAccumulatorUs_ = 0ULL;
    persistenceAccumulatorUs_ = 0ULL;
    pendingEncoderDetents_ = 0;
    encoderSequenceIndex_ = 0U;
    activeEncoderDirection_ = 0;
    minimumButtonReleaseUs_.fill(0ULL);
    deferredButtonRelease_.fill(false);
    observedTransport_ = TransportState::Stopped;
    transportStartSequence_ = 0U;
    lastTransportStartUs_ = 0ULL;
    lastTransportStopUs_ = 0ULL;
    telemetryHistoryFrozen_ = false;
    resetExternalSyncAcquisition();
    resetExternalResetAcquisition();
    application_->begin();
    observedTransport_ = application_->stateForSimulator().transport;
}

void SimulatorRuntime::togglePower() {
    setPower(!poweredOn_);
}

bool SimulatorRuntime::pixelRaidActive() const {
    return application_ != nullptr && application_->pixelRaidActiveForSimulator();
}

void SimulatorRuntime::serviceEncoderSequence() {
    if (activeEncoderDirection_ == 0) {
        if (pendingEncoderDetents_ == 0) {
            return;
        }
        activeEncoderDirection_ = pendingEncoderDetents_ > 0 ? 1 : -1;
        pendingEncoderDetents_ -= activeEncoderDirection_;
        encoderSequenceIndex_ = 1U;
    }

    const auto& phases = activeEncoderDirection_ > 0 ? kClockwisePhases : kCounterClockwisePhases;
    setEncoderPhase(phases[encoderSequenceIndex_]);
    ++encoderSequenceIndex_;
    if (encoderSequenceIndex_ >= phases.size()) {
        activeEncoderDirection_ = 0;
        encoderSequenceIndex_ = 0U;
    }
}

void SimulatorRuntime::serviceDeferredButtonReleases() {
    for (std::size_t index = 0U; index < deferredButtonRelease_.size(); ++index) {
        if (!deferredButtonRelease_[index] || simfw::nowUs < minimumButtonReleaseUs_[index]) {
            continue;
        }
        deferredButtonRelease_[index] = false;
        simfw::setPin(buttonPin(static_cast<SimButton>(index)), HIGH);
    }
}

void SimulatorRuntime::collectGateTransitions() {
    while (processedWriteCount_ < simfw::writes.size()) {
        const simfw::PinWrite& write = simfw::writes[processedWriteCount_++];
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            if (write.pin != pinmap::kGateChannelPins[channelIndex]) {
                continue;
            }
            ChannelTelemetry& channel = telemetry_[channelIndex];
            const bool high = write.value == HIGH;
            if (!channel.logicHigh && high) {
                ++channel.risingEdges;
                channel.lastRisingUs = write.timestampUs;
            } else if (channel.logicHigh && !high && channel.lastRisingUs <= write.timestampUs) {
                channel.lastPulseWidthUs = write.timestampUs - channel.lastRisingUs;
            }
            channel.logicHigh = high;
            channel.lastTransitionUs = write.timestampUs;
            channel.transitions.push_back({write.timestampUs, high});
            break;
        }
    }

    if (processedWriteCount_ >= 4096U && processedWriteCount_ == simfw::writes.size()) {
        simfw::writes.clear();
        processedWriteCount_ = 0U;
    }
}

void SimulatorRuntime::pruneTelemetry() {
    if (telemetryHistoryFrozen_) {
        return;
    }
    const std::uint64_t cutoff = simfw::nowUs > scope::kMaximumWindowUs
        ? simfw::nowUs - scope::kMaximumWindowUs
        : 0ULL;
    for (ChannelTelemetry& channel : telemetry_) {
        while (!channel.transitions.empty() && channel.transitions.front().timestampUs < cutoff) {
            channel.transitions.pop_front();
        }
    }
}

void SimulatorRuntime::serviceTransportTelemetry() {
    if (application_ == nullptr || !application_->runningForSimulator()) {
        return;
    }
    const TransportState current = application_->stateForSimulator().transport;
    if (current == observedTransport_) {
        return;
    }

    if (observedTransport_ == TransportState::Stopped && current == TransportState::Playing) {
        lastTransportStartUs_ = simfw::nowUs;
        ++transportStartSequence_;
    }
    if (current == TransportState::Stopped) {
        lastTransportStopUs_ = simfw::nowUs;
    }
    observedTransport_ = current;
}

void SimulatorRuntime::applyStoredButtonLevels() {
    for (std::size_t index = 0U; index < buttonPressed_.size(); ++index) {
        simfw::setPin(
            buttonPin(static_cast<SimButton>(index)),
            buttonPressed_[index] ? LOW : HIGH);
    }
}

std::uint32_t SimulatorRuntime::buttonPin(const SimButton button) {
    switch (button) {
        case SimButton::Encoder: return pinmap::kEncoderPushButtonPin;
        case SimButton::Play: return pinmap::kPlayPauseButtonPin;
        case SimButton::Tap: return pinmap::kTapTempoButtonPin;
        case SimButton::Stop: return pinmap::kResetBackButtonPin;
    }
    return pinmap::kEncoderPushButtonPin;
}

void SimulatorRuntime::setEncoderPhase(const std::uint8_t phaseState) {
    simfw::setPin(pinmap::kEncoderPhaseAPin, (phaseState & 0x02U) != 0U ? HIGH : LOW);
    simfw::setPin(pinmap::kEncoderPhaseBPin, (phaseState & 0x01U) != 0U ? HIGH : LOW);
}

}  // namespace clockfw::sim
