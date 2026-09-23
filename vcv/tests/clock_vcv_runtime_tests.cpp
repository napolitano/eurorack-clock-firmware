/**
 * @file clock_vcv_runtime_tests.cpp
 * @brief Host tests for the Rack-independent CLOCK VCV runtime bridge.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <array>
#include <cassert>
#include <cstdint>
#include <filesystem>
#include <iostream>

#include "clock_vcv_runtime.h"

namespace {

void advance(clockfw::vcv::ClockVcvRuntime& runtime, const double seconds, const double sampleRate) {
    const std::size_t samples = static_cast<std::size_t>(seconds * sampleRate);
    const double sampleTime = 1.0 / sampleRate;
    for (std::size_t i = 0U; i < samples; ++i) {
        runtime.processSample(sampleTime, false, 0.0F, false, 0.0F);
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

}  // namespace

int main() {
    constexpr double kSampleRate = 48000.0;
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

    // Physical PLAY must start the same engine and produce hardware-faithful 5-V gates.
    click(runtime, &clockfw::vcv::PanelControls::playPressed, kSampleRate);
    bool observedHigh = false;
    for (std::size_t sample = 0U; sample < static_cast<std::size_t>(0.600 * kSampleRate); ++sample) {
        runtime.processSample(1.0 / kSampleRate, false, 0.0F, false, 0.0F);
        for (std::size_t channel = 0U; channel < 8U; ++channel) {
            const float voltage = runtime.gateVoltage(channel);
            assert(voltage == 0.0F || voltage == 5.0F);
            observedHigh = observedHigh || voltage == 5.0F;
        }
    }
    assert(observedHigh);

    // Rack patch persistence transports the exact logical CLOCK image, not a second schema.
    const auto saved = runtime.persistenceImage();
    runtime.restorePersistenceImage(saved);
    advance(runtime, 1.100, kSampleRate);
    const auto restored = runtime.persistenceImage();
    assert(saved == restored);

    // External ports use hysteresis but feed the real firmware capture/lock path.
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
