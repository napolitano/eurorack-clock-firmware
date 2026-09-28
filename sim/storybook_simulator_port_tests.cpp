/**
 * @file storybook_simulator_port_tests.cpp
 * @brief Exercises Storybook semantic boundaries against the real native simulator runtime.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <cstdint>
#include <cstdlib>
#include <filesystem>
#include <iostream>

#include "domain/clock_types.h"
#include "tutorial/story_contract.h"
#include "tutorial/story_simulator_port.h"
#include "virtual_input_signal.h"

namespace {

using clockfw::ClockSource;
using clockfw::TransportState;
using clockfw::sim::SignalWaveform;
using clockfw::sim::SimulatorRuntime;
using clockfw::sim::tutorial::ExternalStimulus;
using clockfw::sim::tutorial::ModuleControl;
using clockfw::sim::tutorial::PatchAction;
using clockfw::sim::tutorial::StoryPortError;
using clockfw::sim::tutorial::StorySimulatorPort;

bool require(const bool condition, const char* const message) {
    if (!condition) {
        std::cerr << "Storybook simulator-port failure: " << message << '\n';
        return false;
    }
    return true;
}

bool accepted(const clockfw::sim::tutorial::StoryPortResult result) {
    return static_cast<bool>(result);
}

void pressAndRelease(
    StorySimulatorPort& port,
    const ModuleControl control) {
    static_cast<void>(port.setModuleControl(control, true));
    port.advanceFirmwareMicroseconds(35000ULL);
    static_cast<void>(port.setModuleControl(control, false));
    port.advanceFirmwareMicroseconds(35000ULL);
}

}  // namespace

int main() {
    const std::filesystem::path statePath = ".clock-storybook-port-test-state.bin";
    std::filesystem::remove(statePath);

    SimulatorRuntime runtime(statePath);
    runtime.begin();
    StorySimulatorPort port(runtime);

    bool ok = true;
    ok &= require(port.poweredOn(), "runtime must begin powered on");
    port.advanceFirmwareMicroseconds(1050000ULL);
    ok &= require(port.clockSource() == ClockSource::Auto, "factory CLOCK SOURCE must remain AUTO");

    const auto syncWithoutCable = port.setGeneratorRunning(ExternalStimulus::SyncGenerator, true);
    ok &= require(syncWithoutCable.error == StoryPortError::CableRequired,
                  "SYNC generator run without cable must be rejected");
    ok &= require(!port.syncTelemetry().cableConnected,
                  "rejected SYNC generator run must not auto-connect the cable");

    const auto resetWithoutCable = port.setGeneratorRunning(ExternalStimulus::ResetGenerator, true);
    ok &= require(resetWithoutCable.error == StoryPortError::CableRequired,
                  "RST generator run without cable must be rejected");
    ok &= require(!port.resetTelemetry().cableConnected,
                  "rejected RST generator run must not auto-connect the cable");

    const ClockSource sourceBeforeEnvironment = port.clockSource();
    ok &= require(accepted(port.configureSyncSource(120000U, 1U, SignalWaveform::Square)),
                  "valid SYNC source configuration must succeed");
    ok &= require(accepted(port.setPatchConnected(PatchAction::SyncCable, true)),
                  "SYNC cable connect must succeed");
    ok &= require(port.syncTelemetry().cableConnected && port.syncTelemetry().generatorRunning,
                  "SYNC connect must preserve existing simulator semantics and start its generator");
    ok &= require(port.clockSource() == sourceBeforeEnvironment,
                  "SYNC connect/configuration must not mutate CLOCK SOURCE");

    port.advanceFirmwareMicroseconds(10000ULL);
    const auto firstEdge = port.syncTelemetry();
    ok &= require(firstEdge.pulseCount >= 1ULL && !firstEdge.locked,
                  "first accepted SYNC edge must acquire but not yet establish lock");
    port.advanceFirmwareMicroseconds(550000ULL);
    ok &= require(port.syncTelemetry().pulseCount >= 2ULL && port.syncTelemetry().locked,
                  "second accepted SYNC edge must establish measurable lock");
    ok &= require(port.clockSource() == ClockSource::Auto,
                  "external lock must not rewrite factory AUTO source selection");
    ok &= require(port.transportState() == TransportState::Playing,
                  "AUTO lock must exercise production firmware auto-start semantics");

    ok &= require(accepted(port.setGeneratorRunning(ExternalStimulus::SyncGenerator, false)),
                  "SYNC hold must succeed while cable remains connected");
    ok &= require(port.syncTelemetry().cableConnected && !port.syncTelemetry().generatorRunning,
                  "SYNC hold must preserve cable state");
    ok &= require(port.clockSource() == ClockSource::Auto,
                  "SYNC hold must not change CLOCK SOURCE");
    ok &= require(accepted(port.setGeneratorRunning(ExternalStimulus::SyncGenerator, true)),
                  "SYNC run must resume while cable stays connected");
    port.advanceFirmwareMicroseconds(1100000ULL);
    ok &= require(port.syncTelemetry().locked, "resumed SYNC generator must reacquire lock");

    pressAndRelease(port, ModuleControl::StopBack);
    ok &= require(port.transportState() == TransportState::Stopped,
                  "real STOP/BACK control must stop production transport");
    port.advanceFirmwareMicroseconds(1200000ULL);
    ok &= require(port.syncTelemetry().locked,
                  "external SYNC may remain locked after explicit STOP");
    ok &= require(port.transportState() == TransportState::Stopped,
                  "incoming locked SYNC must not override explicit manual STOP");
    ok &= require(port.clockSource() == ClockSource::Auto,
                  "manual STOP test must leave CLOCK SOURCE AUTO");

    ok &= require(accepted(port.setPatchConnected(PatchAction::ResetCable, true)),
                  "RST cable connect must succeed");
    ok &= require(port.resetTelemetry().cableConnected && port.resetTelemetry().generatorRunning,
                  "RST connect must start the configured generator under simulator semantics");
    ok &= require(accepted(port.setGeneratorRunning(ExternalStimulus::ResetGenerator, false)),
                  "RST hold must succeed while patched");
    const std::uint64_t resetCountBefore = port.resetTelemetry().resetCount;
    ok &= require(accepted(port.triggerResetPulse()), "one-shot RST pulse must succeed while cable is connected");
    ok &= require(port.resetTelemetry().resetCount == resetCountBefore + 1ULL,
                  "one-shot RST pulse must traverse the real simulator reset boundary");
    ok &= require(port.clockSource() == ClockSource::Auto,
                  "RST environment actions must not mutate CLOCK SOURCE");

    ok &= require(accepted(port.setPatchConnected(PatchAction::ResetCable, false)),
                  "RST disconnect must succeed");
    const auto resetPulseWithoutCable = port.triggerResetPulse();
    ok &= require(resetPulseWithoutCable.error == StoryPortError::CableRequired,
                  "RST pulse without cable must be rejected by Storybook semantics");

    ok &= require(accepted(port.setPower(false)), "POWER OFF must use the simulator power boundary");
    ok &= require(!port.poweredOn(), "POWER OFF must actually power the simulated module down");
    const auto encoderWhileOff = port.rotateEncoderDetent(1);
    ok &= require(encoderWhileOff.error == StoryPortError::ModulePoweredOff,
                  "recorded encoder movement while module is off must be rejected");
    ok &= require(accepted(port.setPower(true)), "POWER ON must use the simulator power boundary");
    ok &= require(port.poweredOn(), "POWER ON must reconstruct the simulated module");
    port.advanceFirmwareMicroseconds(1050000ULL);
    ok &= require(port.clockSource() == ClockSource::Auto,
                  "power cycle must preserve production/persistence source semantics");

    const auto badDirection = port.rotateEncoderDetent(2);
    ok &= require(badDirection.error == StoryPortError::InvalidParameter,
                  "one Storybook encoder operation must represent exactly one detent");
    ok &= require(accepted(port.rotateEncoderDetent(1)), "one clockwise detent must be accepted");
    port.advanceFirmwareMicroseconds(20000ULL);

    runtime.flushPersistence();
    std::filesystem::remove(statePath);

    if (!ok) {
        return EXIT_FAILURE;
    }

    std::cout << "CLOCK Storybook simulator boundary: PASS\n";
    return EXIT_SUCCESS;
}
