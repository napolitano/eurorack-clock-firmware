/**
 * @file simulator_tests.cpp
 * @brief Deterministic smoke/regression tests for the native simulator runtime boundary.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <array>
#include <cstdint>
#include <filesystem>
#include <iostream>

#include "simulator_contract_tests.h"
#include "scope_session.h"
#include "simulator_runtime.h"

namespace {

int fail(const char* const message) {
    std::cerr << "simulator test failed: " << message << '\n';
    return 1;
}

bool framebufferPixel(
    const std::array<std::uint8_t, clockfw::hal::OledDisplay::kFramebufferSize>& framebuffer,
    const std::int16_t x,
    const std::int16_t y) {
    if (x < 0 || x >= clockfw::hal::OledDisplay::kWidth || y < 0 || y >= clockfw::hal::OledDisplay::kHeight) {
        return false;
    }
    const std::size_t index = static_cast<std::size_t>(x) +
        static_cast<std::size_t>(y / 8) * static_cast<std::size_t>(clockfw::hal::OledDisplay::kWidth);
    return (framebuffer[index] & static_cast<std::uint8_t>(1U << (y & 7))) != 0U;
}

}  // namespace

int main() {
    const std::filesystem::path statePath = ".clock-simulator-test-state.bin";
    std::filesystem::remove(statePath);

    using namespace clockfw::sim;

    if (clockfw::sim::tests::runStaticContracts() != 0) {
        return 1;
    }


    SimulatorRuntime runtime(statePath);
    runtime.begin();
    if (!runtime.poweredOn() || runtime.outputStageEnabled()) {
        return fail("power-on must expose the real boot interval with the gate buffer disabled");
    }
    bool bootPixel = false;
    for (const std::uint8_t byte : runtime.framebuffer()) bootPixel = bootPixel || byte != 0U;
    if (!bootPixel) {
        return fail("boot screen must be visible before the non-blocking boot interval completes");
    }
    runtime.advanceMicroseconds(1050000ULL);
    if (runtime.state().transport != clockfw::TransportState::Stopped) {
        return fail("boot must enter STOP");
    }
    if (!runtime.outputStageEnabled()) {
        return fail("gate buffer should be enabled only after safe boot completes");
    }

    runtime.setPower(false);
    if (runtime.poweredOn() || runtime.outputStageEnabled()) {
        return fail("POWER OFF must disable MCU/output-stage state");
    }
    for (const std::uint8_t byte : runtime.framebuffer()) {
        if (byte != 0U) return fail("POWER OFF must blank the OLED framebuffer");
    }
    runtime.setButton(SimButton::Encoder, true);
    runtime.setPower(true);
    runtime.advanceMicroseconds(1050000ULL);
    if (!runtime.pixelRaidActive() || runtime.outputStageEnabled()) {
        return fail("holding encoder across power-on boot must launch Pixel Raid with outputs disabled");
    }
    runtime.setButton(SimButton::Encoder, false);
    runtime.advanceMicroseconds(35000ULL);
    runtime.setButton(SimButton::Tap, true);
    runtime.advanceMicroseconds(45000ULL);
    if (!framebufferPixel(runtime.framebuffer(), 64, 55) &&
        !framebufferPixel(runtime.framebuffer(), 64, 56) &&
        !framebufferPixel(runtime.framebuffer(), 64, 57)) {
        return fail("Pixel Raid projectile must survive long enough to appear in a rendered OLED frame");
    }
    runtime.setButton(SimButton::Tap, false);
    runtime.setPower(false);
    runtime.setButton(SimButton::Encoder, false);
    runtime.setPower(true);
    runtime.advanceMicroseconds(1050000ULL);
    if (runtime.pixelRaidActive() || !runtime.outputStageEnabled()) {
        return fail("normal power-on without boot chord must return to the clock application");
    }

    runtime.setSyncBpmMilli(90000U);
    runtime.setSyncCableConnected(true);
    runtime.advanceMicroseconds(1500000ULL);
    const SyncInputTelemetry lockedSync = runtime.syncInputTelemetry();
    if (!lockedSync.cableConnected || !lockedSync.generatorRunning ||
        !lockedSync.locked || lockedSync.pulseCount < 2ULL || lockedSync.bpmMilli != 90000U) {
        return fail("virtual SYNC IN generator must acquire lock from generated pulses");
    }
    const std::uint8_t oldPpqn = lockedSync.ppqn;
    runtime.cycleSyncPpqn();
    runtime.advanceMicroseconds(1500000ULL);
    const SyncInputTelemetry mismatchedSync = runtime.syncInputTelemetry();
    if (mismatchedSync.ppqn == oldPpqn) {
        return fail("virtual SYNC IN PPQN control must cycle supported values");
    }
    if (mismatchedSync.firmwarePpqn != runtime.state().externalSync.pulsesPerQuarterNote ||
        mismatchedSync.firmwarePpqn == mismatchedSync.ppqn) {
        return fail("generator and firmware PPQN must remain independently observable");
    }
    const std::uint32_t expectedInterpretedBpm = static_cast<std::uint32_t>(
        static_cast<std::uint64_t>(mismatchedSync.bpmMilli) * mismatchedSync.ppqn /
        mismatchedSync.firmwarePpqn);
    const std::uint32_t interpretedError = mismatchedSync.engineBpmMilli > expectedInterpretedBpm
        ? mismatchedSync.engineBpmMilli - expectedInterpretedBpm
        : expectedInterpretedBpm - mismatchedSync.engineBpmMilli;
    // Comparator transitions are sampled on the 50-us simulator scheduler grid, so a
    // few milli-BPM of deterministic quantization error are expected after filtering.
    if (!mismatchedSync.locked || interpretedError > 50U) {
        return fail("firmware PPQN must determine generated pulse interpretation within 0.05 BPM");
    }
    runtime.setSyncGeneratorRunning(false);
    runtime.advanceMicroseconds(2000000ULL);
    if (runtime.syncInputTelemetry().locked) {
        return fail("virtual SYNC IN must lose lock after the firmware timeout");
    }
    runtime.setSyncCableConnected(false);

    // At 20 BPM / 1 PPQN, valid pulses are 3 s apart. The simulator must not impose
    // its former fixed 1.5-s timeout over the firmware's adaptive slow-clock policy.
    runtime.setSyncBpmMilli(20000U);
    while (runtime.syncInputTelemetry().ppqn != 1U) runtime.cycleSyncPpqn();
    runtime.setSyncCableConnected(true);
    runtime.advanceMicroseconds(6500000ULL);
    const SyncInputTelemetry slowSync = runtime.syncInputTelemetry();
    if (!slowSync.locked || slowSync.pulseCount < 2ULL) {
        return fail("20 BPM / 1 PPQN SYNC must remain locked between valid 3-second pulses");
    }
    runtime.setSyncCableConnected(false);

    const SignalWaveform initialSyncWaveform = runtime.syncInputTelemetry().waveform;
    runtime.cycleSyncWaveform();
    if (runtime.syncInputTelemetry().waveform == initialSyncWaveform) {
        return fail("SYNC source must cycle ideal comparator input waveforms");
    }
    runtime.setResetCableConnected(true);
    runtime.adjustResetPeriod(-5);
    const std::uint32_t resetPeriodMs = runtime.resetInputTelemetry().periodMs;
    runtime.advanceMicroseconds(static_cast<std::uint64_t>(resetPeriodMs) * 2200ULL);
    const ResetInputTelemetry generatedReset = runtime.resetInputTelemetry();
    if (!generatedReset.cableConnected || !generatedReset.generatorRunning ||
        generatedReset.resetCount < 2ULL) {
        return fail("continuous RST generator must emit conditioned reset edges");
    }
    const std::uint64_t resetCountBeforeSingle = generatedReset.resetCount;
    runtime.triggerResetPulse();
    if (runtime.resetInputTelemetry().resetCount != resetCountBeforeSingle + 1ULL) {
        return fail("single RST injection must reach the real engine boundary");
    }
    runtime.setResetCableConnected(false);

    const std::uint16_t initialBpm = runtime.state().bpm;
    const std::int64_t initialEncoderVisualPosition = runtime.encoderVisualPosition();
    runtime.rotateEncoder(2);
    runtime.advanceMicroseconds(20000ULL);
    if (runtime.state().bpm <= initialBpm) {
        return fail("encoder detents must reach the real UI controller");
    }
    if (runtime.encoderVisualPosition() != initialEncoderVisualPosition + 2) {
        return fail("simulator encoder feedback must track requested detents");
    }

    scope::Session scopeSession{};
    scopeSession.setWindowUs(32000000ULL);
    scopeSession.update(runtime);
    if (scopeSession.view().started || scopeSession.view().referenceUs != 0ULL ||
        scopeSession.view().windowUs != 32000000ULL) {
        return fail("developer scope must remain armed at t=0 until transport starts");
    }

    runtime.setButton(clockfw::sim::SimButton::Play, true);
    runtime.advanceMicroseconds(35000ULL);
    runtime.setButton(clockfw::sim::SimButton::Play, false);
    runtime.advanceMicroseconds(35000ULL);
    if (runtime.state().transport != clockfw::TransportState::Playing) {
        return fail("PLAY button must start the real transport");
    }
    scopeSession.update(runtime);
    const scope::SessionView startedScope = scopeSession.view();
    if (!startedScope.started || startedScope.epochSimulatorUs == 0ULL ||
        startedScope.referenceUs > 100000ULL) {
        return fail("scope t=0 must be anchored to the exact STOP-to-PLAY transport transition");
    }

    runtime.advanceMicroseconds(1200000ULL);
    scopeSession.update(runtime);
    if (scopeSession.view().referenceUs < 1200000ULL) {
        return fail("scope reference time must advance from the transport start epoch");
    }
    std::uint64_t edges = 0ULL;
    for (const auto& channel : runtime.telemetry()) {
        edges += channel.risingEdges;
    }
    if (edges == 0ULL) {
        return fail("real scheduler must create observable gate edges");
    }
    bool measuredPulse = false;
    for (const auto& channel : runtime.telemetry()) {
        if (channel.lastPulseWidthUs != 0ULL) {
            measuredPulse = true;
            if (channel.lastPulseWidthUs < 9500ULL || channel.lastPulseWidthUs > 10500ULL) {
                return fail("simulator telemetry must report the real default 10 ms gate width");
            }
        }
    }
    if (!measuredPulse) {
        return fail("simulator telemetry must retain a completed gate-pulse width");
    }

    bool anyPixel = false;
    for (const std::uint8_t byte : runtime.framebuffer()) {
        anyPixel = anyPixel || byte != 0U;
    }
    if (!anyPixel) {
        return fail("real OLED renderer must populate the simulator framebuffer");
    }

    runtime.setButton(clockfw::sim::SimButton::Stop, true);
    runtime.advanceMicroseconds(35000ULL);
    runtime.setButton(clockfw::sim::SimButton::Stop, false);
    runtime.advanceMicroseconds(35000ULL);
    if (runtime.state().transport != clockfw::TransportState::Stopped) {
        return fail("STOP button must stop the real transport");
    }
    scopeSession.update(runtime);
    const std::uint64_t frozenReferenceUs = scopeSession.view().referenceUs;
    runtime.advanceMicroseconds(500000ULL);
    scopeSession.update(runtime);
    if (scopeSession.view().referenceUs != frozenReferenceUs) {
        return fail("checked FREEZE ON STOP must hold the scope reference time after STOP");
    }
    scopeSession.setFreezeOnStop(false);
    runtime.advanceMicroseconds(250000ULL);
    scopeSession.update(runtime);
    if (scopeSession.view().referenceUs <= frozenReferenceUs) {
        return fail("disabled FREEZE ON STOP must let the started scope timebase continue");
    }

    runtime.setPower(false);
    scopeSession.update(runtime);
    if (scopeSession.view().started || scopeSession.view().referenceUs != 0ULL) {
        return fail("POWER OFF must disarm the developer scope and clear its timebase");
    }
    runtime.setPower(true);
    runtime.advanceMicroseconds(1050000ULL);
    scopeSession.update(runtime);
    runtime.setButton(clockfw::sim::SimButton::Play, true);
    runtime.advanceMicroseconds(35000ULL);
    runtime.setButton(clockfw::sim::SimButton::Play, false);
    runtime.advanceMicroseconds(35000ULL);
    scopeSession.update(runtime);
    if (!scopeSession.view().started || scopeSession.view().epochSimulatorUs == 0ULL ||
        scopeSession.view().referenceUs > 100000ULL) {
        return fail("scope must re-arm against the first PLAY edge after a virtual power cycle");
    }
    runtime.setButton(clockfw::sim::SimButton::Stop, true);
    runtime.advanceMicroseconds(35000ULL);
    runtime.setButton(clockfw::sim::SimButton::Stop, false);
    runtime.advanceMicroseconds(35000ULL);

    // Firmware coalesces durable CURRENT writes for three seconds and never
    // commits while PLAYING; advance STOP time so the real persistence service flushes.
    runtime.advanceMicroseconds(3200000ULL);
    runtime.flushPersistence();
    if (!std::filesystem::exists(statePath) || std::filesystem::file_size(statePath) == 0U) {
        return fail("simulator persistence mirror must create durable state");
    }

    const std::uint16_t persistedBpm = runtime.state().bpm;
    SimulatorRuntime restoredRuntime(statePath);
    restoredRuntime.begin();
    if (restoredRuntime.state().bpm != persistedBpm) {
        return fail("simulator must reload the firmware persistence image on restart");
    }
    if (restoredRuntime.state().transport != clockfw::TransportState::Stopped) {
        return fail("restored simulator must preserve the firmware's safe STOP-on-boot rule");
    }

    std::filesystem::remove(statePath);

    std::cout << "simulator runtime tests passed\n";
    return 0;
}
