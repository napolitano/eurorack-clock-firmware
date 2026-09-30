/**
 * @file clock_vcv_runtime_tests.cpp
 * @brief Host tests for the Rack-independent CLOCK VCV runtime bridge.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <algorithm>
#include <array>
#include <cassert>
#include <cstdint>
#include <cmath>
#include <filesystem>
#include <iostream>
#include <limits>
#include <string>
#include <vector>

#include "clock_vcv_runtime.h"
#include "config.h"
#include "domain/default_configuration.h"
#include "hal/persistent_storage.h"
#include "services/persistent_state_service.h"

namespace {

void advance(clockfw::vcv::ClockVcvRuntime& runtime, const double seconds, const double sampleRate) {
    const std::size_t samples = static_cast<std::size_t>(seconds * sampleRate);
    const double sampleTime = 1.0 / sampleRate;
    for (std::size_t i = 0U; i < samples; ++i) {
        runtime.processSample(sampleTime, false, 0.0F, false, 0.0F);
    }
}

void advanceWithConnections(
    clockfw::vcv::ClockVcvRuntime& runtime,
    const double seconds,
    const double sampleRate,
    const bool syncConnected,
    const bool resetConnected) {
    const std::size_t samples = static_cast<std::size_t>(seconds * sampleRate);
    const double sampleTime = 1.0 / sampleRate;
    for (std::size_t i = 0U; i < samples; ++i) {
        runtime.processSample(sampleTime, syncConnected, 0.0F, resetConnected, 0.0F);
    }
}

void click(
    clockfw::vcv::ClockVcvRuntime& runtime,
    bool clockfw::vcv::PanelControls::* member,
    const double sampleRate) {
    clockfw::vcv::PanelControls controls{};
    controls.*member = true;
    runtime.setPanelControls(controls);
    advance(runtime, 0.040, sampleRate);
    controls.*member = false;
    runtime.setPanelControls(controls);
    advance(runtime, 0.040, sampleRate);
}

void oneSampleSyncPulse(clockfw::vcv::ClockVcvRuntime& runtime, const double sampleRate) {
    const double sampleTime = 1.0 / sampleRate;
    runtime.processSample(sampleTime, true, 5.0F, false, 0.0F);
    runtime.processSample(sampleTime, true, 0.0F, false, 0.0F);
}

void oneSampleResetPulse(clockfw::vcv::ClockVcvRuntime& runtime, const double sampleRate) {
    const double sampleTime = 1.0 / sampleRate;
    runtime.processSample(sampleTime, false, 0.0F, true, 5.0F);
    runtime.processSample(sampleTime, false, 0.0F, true, 0.0F);
}


std::array<std::uint8_t, clockfw::hal::PersistentStorage::kCapacityBytes>
makeUnifiedSwingPersistenceImage(const std::uint8_t swingPercent) {
    using namespace clockfw;
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::PersistentStateService persistent(storage);
    persistent.begin();

    ClockState state{};
    initializeFactoryDefaults(state);
    state.operatingMode = OperatingMode::UnifiedClock;
    state.source = ClockSource::Internal;
    state.bpm = 120U;
    state.unifiedClock.swingPercent = swingPercent;
    state.transport = TransportState::Stopped;
    persistent.requestCurrentState(state, 0U);
    persistent.service(config::kPersistenceCommitDelayMs, true);
    assert(persistent.hasStoredCurrentState());

    std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes> image{};
    assert(storage.readBytes(0U, image.data(), image.size()));
    return image;
}

void assertSwingOneClockGates(const double sampleRate, const char* suffix) {
    const auto statePath = std::filesystem::temp_directory_path() /
        (std::string("ssl-clock-vcv-swing-test-") + suffix + ".bin");
    std::error_code ignored;
    std::filesystem::remove(statePath, ignored);

    const auto swingImage = makeUnifiedSwingPersistenceImage(25U);
    clockfw::vcv::ClockVcvRuntime runtime(statePath);
    runtime.begin();
    runtime.restorePersistenceImage(swingImage);
    advance(runtime, 1.100, sampleRate);
    click(runtime, &clockfw::vcv::PanelControls::playPressed, sampleRate);

    std::vector<std::size_t> risingSamples{};
    bool previousHigh = false;
    const std::size_t sampleCount = static_cast<std::size_t>(3.100 * sampleRate);
    for (std::size_t sample = 0U; sample < sampleCount && risingSamples.size() < 6U; ++sample) {
        runtime.processSample(1.0 / sampleRate, false, 0.0F, false, 0.0F);
        const bool high = runtime.gateVoltage(0U) == 5.0F;
        if (high && !previousHigh) {
            risingSamples.push_back(sample);
        }
        previousHigh = high;
    }

    assert(risingSamples.size() >= 5U);
    const double expectedLongSamples = sampleRate * 0.625;
    const double expectedShortSamples = sampleRate * 0.375;
    const double toleranceSamples = std::max(4.0, sampleRate * 0.00015);
    std::array<double, 4U> intervals{};
    for (std::size_t index = 0U; index < intervals.size(); ++index) {
        intervals[index] = static_cast<double>(
            risingSamples[index + 1U] - risingSamples[index]);
    }

    const bool startsLong = std::abs(intervals[0] - expectedLongSamples) <= toleranceSamples;
    const bool startsShort = std::abs(intervals[0] - expectedShortSamples) <= toleranceSamples;
    assert(startsLong || startsShort);
    for (std::size_t index = 0U; index < intervals.size(); ++index) {
        const double expected = ((index % 2U) == 0U)
            ? (startsLong ? expectedLongSamples : expectedShortSamples)
            : (startsLong ? expectedShortSamples : expectedLongSamples);
        assert(std::abs(intervals[index] - expected) <= toleranceSamples);
    }

    const double pairSamples = intervals[0] + intervals[1];
    assert(std::abs(pairSamples - sampleRate) <= toleranceSamples * 2.0);
    std::filesystem::remove(statePath, ignored);
}

void assertRegularOneClockGates(const double sampleRate, const char* suffix) {
    const auto statePath = std::filesystem::temp_directory_path() /
        (std::string("ssl-clock-vcv-gate-test-") + suffix + ".bin");
    std::error_code ignored;
    std::filesystem::remove(statePath, ignored);

    clockfw::vcv::ClockVcvRuntime runtime(statePath);
    runtime.begin();
    advance(runtime, 1.100, sampleRate);
    click(runtime, &clockfw::vcv::PanelControls::playPressed, sampleRate);

    std::array<bool, 8U> previousHigh{};
    std::array<std::vector<std::size_t>, 8U> risingSamples{};
    const std::size_t sampleCount = static_cast<std::size_t>(2.100 * sampleRate);
    for (std::size_t sample = 0U; sample < sampleCount; ++sample) {
        runtime.processSample(1.0 / sampleRate, false, 0.0F, false, 0.0F);
        for (std::size_t channel = 0U; channel < 8U; ++channel) {
            const bool high = runtime.gateVoltage(channel) == 5.0F;
            if (high && !previousHigh[channel]) {
                risingSamples[channel].push_back(sample);
            }
            previousHigh[channel] = high;
        }
    }

    assert(risingSamples[0].size() >= 3U);
    for (std::size_t channel = 1U; channel < risingSamples.size(); ++channel) {
        assert(risingSamples[channel].size() == risingSamples[0].size());
        assert(risingSamples[channel] == risingSamples[0]);
    }

    const double expectedSamples = sampleRate * 0.5;
    const double toleranceSamples = std::max(4.0, sampleRate * 0.0001);
    for (std::size_t index = 1U; index < risingSamples[0].size(); ++index) {
        const double actualSamples = static_cast<double>(
            risingSamples[0][index] - risingSamples[0][index - 1U]);
        assert(std::abs(actualSamples - expectedSamples) <= toleranceSamples);
    }

    std::filesystem::remove(statePath, ignored);
}

void assertVcvPersistenceStaysOffAudioThreadAndSnapshotsLiveState(const double sampleRate) {
    const auto statePath = std::filesystem::temp_directory_path() /
        "ssl-clock-vcv-realtime-persistence-test.bin";
    std::error_code ignored;
    std::filesystem::remove(statePath, ignored);

    clockfw::vcv::ClockVcvRuntime runtime(statePath);
    runtime.begin();
    advance(runtime, 1.100, sampleRate);

    // A normal live edit schedules firmware persistence. More than the simulator's
    // historical 500-ms mirror period must still perform no filesystem write from processSample().
    runtime.rotateEncoder(5);
    advance(runtime, 4.000, sampleRate);
    assert(!std::filesystem::exists(statePath));

    // Rack serialization must include the accepted live state even though hardware-style
    // persistence would still be deferred/coalesced. Exporting the in-memory NVM snapshot
    // must not create the simulator mirror file either.
    const auto edited = runtime.persistenceImage();
    assert(!std::filesystem::exists(statePath));

    clockfw::vcv::ClockVcvRuntime baselineRuntime(
        std::filesystem::temp_directory_path() / "ssl-clock-vcv-realtime-baseline.bin");
    const auto baselinePath = std::filesystem::temp_directory_path() /
        "ssl-clock-vcv-realtime-baseline.bin";
    std::filesystem::remove(baselinePath, ignored);
    baselineRuntime.begin();
    advance(baselineRuntime, 1.100, sampleRate);
    const auto baseline = baselineRuntime.persistenceImage();
    assert(edited != baseline);
    assert(!std::filesystem::exists(baselinePath));

    // Restoring the exported Rack image must reproduce the exact logical NVM snapshot.
    runtime.restorePersistenceImage(edited);
    advance(runtime, 1.100, sampleRate);
    assert(runtime.persistenceImage() == edited);

    std::filesystem::remove(statePath, ignored);
    std::filesystem::remove(baselinePath, ignored);
}

}  // namespace

int main() {
    constexpr double kSampleRate = 48000.0;

    // The actual Rack output voltage must remain periodic independently of the GUI LED refresh.
    // Test common Rack sample rates so sample-to-50-us scheduler conversion cannot hide a drift.
    assertRegularOneClockGates(44100.0, "44100");
    assertRegularOneClockGates(48000.0, "48000");
    assertRegularOneClockGates(96000.0, "96000");
    assertSwingOneClockGates(44100.0, "44100");
    assertSwingOneClockGates(48000.0, "48000");
    assertSwingOneClockGates(96000.0, "96000");
    assertVcvPersistenceStaysOffAudioThreadAndSnapshotsLiveState(kSampleRate);
    const auto statePath = std::filesystem::temp_directory_path() / "ssl-clock-vcv-runtime-test.bin";
    std::error_code ignored;
    std::filesystem::remove(statePath, ignored);

    clockfw::vcv::ClockVcvRuntime runtime(statePath);
    runtime.begin();

    // The adapter must expose its host-running state. begin() powers the virtual module
    // immediately; the one-second boot animation is part of that powered runtime.
    assert(runtime.running());

    // Invalid/non-positive Rack sample times are ignored rather than perturbing scheduler state.
    runtime.processSample(0.0, false, 0.0F, false, 0.0F);
    runtime.processSample(-1.0, false, 0.0F, false, 0.0F);
    runtime.processSample(std::numeric_limits<double>::quiet_NaN(), false, 0.0F, false, 0.0F);

    // Real CLOCK boot duration is exercised rather than bypassed.
    advance(runtime, 1.100, kSampleRate);
    assert(runtime.running());
    const auto& frame = runtime.framebuffer();
    bool anyPixel = false;
    for (const std::uint8_t byte : frame) {
        anyPixel = anyPixel || byte != 0U;
    }
    assert(anyPixel);
    assert(runtime.gateVoltage(8U) == 0.0F);

    // Cover all physical momentary controls before entering menus.
    click(runtime, &clockfw::vcv::PanelControls::tapPressed, kSampleRate);
    click(runtime, &clockfw::vcv::PanelControls::stopPressed, kSampleRate);

    // Relative encoder rotation is a real detent path; zero is explicitly a no-op.
    const auto beforeRotate = runtime.framebuffer();
    runtime.rotateEncoder(0);
    runtime.rotateEncoder(1);
    advance(runtime, 0.050, kSampleRate);
    assert(runtime.framebuffer() != beforeRotate);
    runtime.rotateEncoder(-1);
    advance(runtime, 0.050, kSampleRate);

    // Force more Rack-sample input transitions than the fixed bridge can queue before one
    // scheduler quantum. Overflow must collapse to the latest sampled level without allocation.
    for (int transition = 0; transition < 24; ++transition) {
        const float volts = (transition % 2 == 0) ? 5.0F : 0.0F;
        runtime.processSample(0.000001, true, volts, false, 0.0F);
    }
    runtime.processSample(0.000050, true, 0.0F, false, 0.0F);
    runtime.processSample(0.000050, false, 0.0F, false, 0.0F);

    // Host shortcuts must enter the existing production General Settings page, not a parallel
    // Rack-specific settings model.
    assert(runtime.openGeneralSettings());
    advance(runtime, 0.020, kSampleRate);
    assert(runtime.generalSettingsOpenForTest());

    // Return to Performance before testing the physical long-press path.
    const auto generalSettingsImage = runtime.persistenceImage();
    runtime.restorePersistenceImage(generalSettingsImage);
    advance(runtime, 1.100, kSampleRate);

    // Holding encoder push must reach the production long-press threshold and change UI context.
    const auto performanceFrame = runtime.framebuffer();
    clockfw::vcv::PanelControls heldControls{};
    heldControls.encoderPressed = true;
    runtime.setPanelControls(heldControls);
    advance(runtime, 0.750, kSampleRate);
    heldControls.encoderPressed = false;
    runtime.setPanelControls(heldControls);
    advance(runtime, 0.050, kSampleRate);
    assert(runtime.framebuffer() != performanceFrame);

    // Return to a clean boot state before transport/gate checks.
    const auto cleanImage = runtime.persistenceImage();
    runtime.restorePersistenceImage(cleanImage);
    advance(runtime, 1.100, kSampleRate);

    // Physical PLAY must start the same engine and produce continuous hardware-faithful 5-V gates.
    click(runtime, &clockfw::vcv::PanelControls::playPressed, kSampleRate);
    std::array<std::uint32_t, 8U> risingEdges{};
    std::array<bool, 8U> previousHigh{};
    for (std::size_t sample = 0U; sample < static_cast<std::size_t>(2.100 * kSampleRate); ++sample) {
        runtime.processSample(1.0 / kSampleRate, false, 0.0F, false, 0.0F);
        for (std::size_t channel = 0U; channel < 8U; ++channel) {
            const float voltage = runtime.gateVoltage(channel);
            assert(voltage == 0.0F || voltage == 5.0F);
            const bool high = voltage == 5.0F;
            if (high && !previousHigh[channel]) {
                ++risingEdges[channel];
            }
            previousHigh[channel] = high;
        }
    }
    assert(risingEdges[0] >= 3U);
    for (std::size_t channel = 1U; channel < risingEdges.size(); ++channel) {
        assert(risingEdges[channel] == risingEdges[0]);
    }

    // Rack patch persistence transports the exact logical CLOCK image, not a second schema.
    const auto saved = runtime.persistenceImage();
    runtime.restorePersistenceImage(saved);
    advance(runtime, 1.100, kSampleRate);
    const auto restored = runtime.persistenceImage();
    assert(saved == restored);

    // One-Rack-sample trigger pulses must survive the 20-kHz firmware scheduler boundary.
    // At 48 kHz each HIGH sample is only ~20.8 us, shorter than one 50-us scheduler tick.
    const std::uint64_t syncBefore = runtime.syncPulseCountForTest();
    for (int pulse = 0; pulse < 4; ++pulse) {
        oneSampleSyncPulse(runtime, kSampleRate);
        advanceWithConnections(runtime, 0.050, kSampleRate, true, false);
    }
    assert(runtime.syncPulseCountForTest() == syncBefore + 4U);

    const std::uint64_t resetBefore = runtime.resetPulseCountForTest();
    for (int pulse = 0; pulse < 4; ++pulse) {
        oneSampleResetPulse(runtime, kSampleRate);
        advanceWithConnections(runtime, 0.010, kSampleRate, false, true);
    }
    assert(runtime.resetPulseCountForTest() == resetBefore + 4U);

    // Normal longer external pulses still feed the real firmware capture/lock path unchanged.
    for (int pulse = 0; pulse < 4; ++pulse) {
        for (std::size_t i = 0U; i < static_cast<std::size_t>(0.010 * kSampleRate); ++i) {
            runtime.processSample(1.0 / kSampleRate, true, 5.0F, false, 0.0F);
        }
        for (std::size_t i = 0U; i < static_cast<std::size_t>(0.490 * kSampleRate); ++i) {
            runtime.processSample(1.0 / kSampleRate, true, 0.0F, false, 0.0F);
        }
    }

    std::filesystem::remove(statePath, ignored);
    std::cout << "clock-vcv-runtime-tests: PASS\n";
    return 0;
}
