/**
 * @file clock_vcv_runtime.h
 * @brief Rack-independent bridge between VCV Rack and the real CLOCK simulator runtime.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>

#include "hal/oled_display.h"
#include "hal/persistent_storage.h"
#include "simulator_runtime.h"

namespace clockfw::vcv {

/** @brief Host-facing control state matching the physical CLOCK panel. */
struct PanelControls final {
    bool encoderPressed = false;
    bool playPressed = false;
    bool tapPressed = false;
    bool stopPressed = false;
};

/**
 * @brief Adapts Rack sample time and voltages to the same CLOCK application used by the simulator.
 *
 * This class contains no musical timing implementation. It advances SimulatorRuntime in exact
 * 50-us CLOCK scheduler quanta, feeds conditioned SYNC/RST logic levels, forwards panel gestures,
 * and exposes the real gate states and OLED framebuffer.
 */
class ClockVcvRuntime final {
public:
    static constexpr float kGateHighVoltage = 5.0F;
    static constexpr float kInputHighThresholdVolts = 1.5F;
    static constexpr float kInputLowThresholdVolts = 0.5F;

    /** @brief Creates one runtime using a temporary host persistence mirror. */
    explicit ClockVcvRuntime(std::filesystem::path persistencePath);

    /** @brief Boots the real CLOCK application. */
    void begin();

    /** @brief Advances the runtime by one Rack audio sample. */
    void processSample(
        double sampleTimeSeconds,
        bool syncConnected,
        float syncVoltage,
        bool resetConnected,
        float resetVoltage);

    /** @brief Applies current momentary panel-control states. */
    void setPanelControls(const PanelControls& controls);

    /** @brief Injects signed physical encoder detents. */
    void rotateEncoder(int detents);

    /** @brief Returns one hardware-faithful 0/+5-V gate output. */
    float gateVoltage(std::size_t channelIndex) const;

    /** @brief Returns the exact 128x64 CLOCK framebuffer produced by the real renderer. */
    const std::array<std::uint8_t, hal::OledDisplay::kFramebufferSize>& framebuffer() const;

    /** @brief Exports CLOCK's complete logical persistent image for Rack patch storage. */
    std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes> persistenceImage() const;

    /** @brief Restores CLOCK's complete logical persistent image and reboots from it. */
    void restorePersistenceImage(
        const std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes>& image);

    /** @brief Returns true once the application has completed its real boot sequence. */
    bool running() const;

private:
    /** @brief Updates one hysteretic Rack voltage input into a conditioned logic level. */
    static bool updateInputLevel(float voltage, bool previousLevel);

    sim::SimulatorRuntime runtime_;
    PanelControls controls_{};
    double pendingSchedulerUs_ = 0.0;
    bool syncHigh_ = false;
    bool resetHigh_ = false;
};

}  // namespace clockfw::vcv
