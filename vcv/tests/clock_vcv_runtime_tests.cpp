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
#include <string>
#include <vector>

#include "clock_vcv_runtime.h"

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

}  // namespace

int main() {
    constexpr double kSampleRate = 48000.0;

    // The actual Rack output voltage must remain periodic independently of the GUI LED refresh.
    // Test common Rack sample rates so sample-to-50-us scheduler conversion cannot hide a drift.
    assertRegularOneClockGates(44100.0, "44100");
    assertRegularOneClockGates(48000.0, "48000");
    assertRegularOneClockGates(96000.0, "96000");
    const auto statePath = std::filesystem::temp_directory_path() / "ssl-clock-vcv-runtime-test.bin";
    std::error_code ignored;
    std::filesystem::remove(statePath, ignored);

    clockfw::vcv::ClockVcvRuntime runtime(statePath);
    runtime.begin();

    // Real CLOCK boot duration is exercised rather than bypassed.
    advance(runtime, 1.100, kSampleRate);
    const auto& frame = runtime.framebuffer();
    bool anyPixel = false;
    for (const std::uint8_t byte : frame) {
        anyPixel = anyPixel || byte != 0U;
    }
    assert(anyPixel);

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
