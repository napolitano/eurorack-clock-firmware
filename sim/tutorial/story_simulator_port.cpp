/**
 * @file story_simulator_port.cpp
 * @brief Implements the strict Storybook semantic adapter over SimulatorRuntime.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_simulator_port.h"

#include <array>
#include <cstdint>

#include "domain/clock_options.h"

namespace clockfw::sim::tutorial {
namespace {

constexpr std::uint32_t kMinimumSyncBpmMilli = 1000U;
constexpr std::uint32_t kMaximumSyncBpmMilli = 999000U;
constexpr std::uint32_t kMinimumResetPeriodMs = 100U;
constexpr std::uint32_t kMaximumResetPeriodMs = 60000U;
constexpr std::uint32_t kResetPeriodStepMs = 100U;
constexpr std::size_t kWaveformCount = 3U;

bool isSupportedPpqn(const std::uint8_t ppqn) {
    for (const std::uint8_t value : clockfw::kExternalPpqnOptions) {
        if (value == ppqn) {
            return true;
        }
    }
    return false;
}

}  // namespace

StoryPortResult::operator bool() const {
    return error == StoryPortError::None;
}

StorySimulatorPort::StorySimulatorPort(SimulatorRuntime& runtime)
    : runtime_(runtime) {}

StoryPortResult StorySimulatorPort::rotateEncoderDetent(const int direction) {
    if (!runtime_.poweredOn()) {
        return {StoryPortError::ModulePoweredOff};
    }
    if (direction != -1 && direction != 1) {
        return {StoryPortError::InvalidParameter};
    }
    runtime_.rotateEncoder(direction);
    return {};
}

StoryPortResult StorySimulatorPort::setModuleControl(
    const ModuleControl control,
    const bool pressed) {
    if (control == ModuleControl::Power) {
        return {StoryPortError::UnsupportedAction};
    }
    if (!runtime_.poweredOn()) {
        return {StoryPortError::ModulePoweredOff};
    }

    SimButton button = SimButton::Encoder;
    const StoryPortResult mapping = mapButton(control, button);
    if (!mapping) {
        return mapping;
    }
    runtime_.setButton(button, pressed);
    return {};
}

StoryPortResult StorySimulatorPort::setPower(const bool powered) {
    runtime_.setPower(powered);
    return {};
}

StoryPortResult StorySimulatorPort::resetPersistenceToFactory() {
    auto erased = runtime_.persistenceImage();
    erased.fill(0xFFU);
    runtime_.restorePersistenceImage(erased);
    return {};
}

StoryPortResult StorySimulatorPort::setPatchConnected(
    const PatchAction action,
    const bool connected) {
    switch (action) {
        case PatchAction::SyncCable:
            runtime_.setSyncCableConnected(connected);
            return {};
        case PatchAction::ResetCable:
            runtime_.setResetCableConnected(connected);
            return {};
    }
    return {StoryPortError::UnsupportedAction};
}

StoryPortResult StorySimulatorPort::setGeneratorRunning(
    const ExternalStimulus stimulus,
    const bool running) {
    switch (stimulus) {
        case ExternalStimulus::SyncGenerator:
            if (!runtime_.syncInputTelemetry().cableConnected) {
                return {StoryPortError::CableRequired};
            }
            runtime_.setSyncGeneratorRunning(running);
            return {};
        case ExternalStimulus::ResetGenerator:
            if (!runtime_.resetInputTelemetry().cableConnected) {
                return {StoryPortError::CableRequired};
            }
            runtime_.setResetGeneratorRunning(running);
            return {};
        case ExternalStimulus::SyncSource:
        case ExternalStimulus::ResetPulse:
            return {StoryPortError::UnsupportedAction};
    }
    return {StoryPortError::UnsupportedAction};
}

StoryPortResult StorySimulatorPort::configureSyncSource(
    const std::uint32_t bpmMilli,
    const std::uint8_t ppqn,
    const SignalWaveform waveform) {
    if (bpmMilli < kMinimumSyncBpmMilli || bpmMilli > kMaximumSyncBpmMilli ||
        !isSupportedPpqn(ppqn)) {
        return {StoryPortError::InvalidParameter};
    }

    runtime_.setSyncBpmMilli(bpmMilli);
    for (std::size_t attempt = 0U;
         attempt < clockfw::kExternalPpqnOptions.size() && runtime_.syncInputTelemetry().ppqn != ppqn;
         ++attempt) {
        runtime_.cycleSyncPpqn();
    }
    if (runtime_.syncInputTelemetry().ppqn != ppqn) {
        return {StoryPortError::InvalidParameter};
    }
    return setSyncWaveform(waveform);
}

StoryPortResult StorySimulatorPort::configureResetSource(
    const std::uint32_t periodMs,
    const SignalWaveform waveform) {
    if (periodMs < kMinimumResetPeriodMs || periodMs > kMaximumResetPeriodMs ||
        (periodMs % kResetPeriodStepMs) != 0U) {
        return {StoryPortError::InvalidParameter};
    }

    const std::uint32_t currentPeriodMs = runtime_.resetInputTelemetry().periodMs;
    const std::int64_t deltaMs = static_cast<std::int64_t>(periodMs) -
        static_cast<std::int64_t>(currentPeriodMs);
    runtime_.adjustResetPeriod(static_cast<int>(deltaMs / static_cast<std::int64_t>(kResetPeriodStepMs)));
    if (runtime_.resetInputTelemetry().periodMs != periodMs) {
        return {StoryPortError::InvalidParameter};
    }
    return setResetWaveform(waveform);
}

StoryPortResult StorySimulatorPort::triggerResetPulse() {
    if (!runtime_.resetInputTelemetry().cableConnected) {
        return {StoryPortError::CableRequired};
    }
    if (!runtime_.poweredOn()) {
        return {StoryPortError::ModulePoweredOff};
    }
    runtime_.triggerResetPulse();
    return {};
}

void StorySimulatorPort::advanceFirmwareMicroseconds(const std::uint64_t durationUs) {
    runtime_.advanceMicroseconds(durationUs);
}

bool StorySimulatorPort::poweredOn() const {
    return runtime_.poweredOn();
}

ClockSource StorySimulatorPort::clockSource() const {
    return runtime_.state().source;
}

TransportState StorySimulatorPort::transportState() const {
    return runtime_.state().transport;
}

SyncInputTelemetry StorySimulatorPort::syncTelemetry() const {
    return runtime_.syncInputTelemetry();
}

ResetInputTelemetry StorySimulatorPort::resetTelemetry() const {
    return runtime_.resetInputTelemetry();
}

const SimulatorRuntime& StorySimulatorPort::runtime() const {
    return runtime_;
}

StoryPortResult StorySimulatorPort::mapButton(
    const ModuleControl control,
    SimButton& button) {
    switch (control) {
        case ModuleControl::EncoderPush:
            button = SimButton::Encoder;
            return {};
        case ModuleControl::Play:
            button = SimButton::Play;
            return {};
        case ModuleControl::Tap:
            button = SimButton::Tap;
            return {};
        case ModuleControl::StopBack:
            button = SimButton::Stop;
            return {};
        case ModuleControl::Encoder:
        case ModuleControl::Power:
            return {StoryPortError::UnsupportedAction};
    }
    return {StoryPortError::UnsupportedAction};
}

StoryPortResult StorySimulatorPort::setSyncWaveform(const SignalWaveform waveform) {
    for (std::size_t attempt = 0U;
         attempt < kWaveformCount && runtime_.syncInputTelemetry().waveform != waveform;
         ++attempt) {
        runtime_.cycleSyncWaveform();
    }
    return runtime_.syncInputTelemetry().waveform == waveform
        ? StoryPortResult{}
        : StoryPortResult{StoryPortError::InvalidParameter};
}

StoryPortResult StorySimulatorPort::setResetWaveform(const SignalWaveform waveform) {
    for (std::size_t attempt = 0U;
         attempt < kWaveformCount && runtime_.resetInputTelemetry().waveform != waveform;
         ++attempt) {
        runtime_.cycleResetWaveform();
    }
    return runtime_.resetInputTelemetry().waveform == waveform
        ? StoryPortResult{}
        : StoryPortResult{StoryPortError::InvalidParameter};
}

}  // namespace clockfw::sim::tutorial
