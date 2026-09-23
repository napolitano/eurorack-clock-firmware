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
 * 50-us CLOCK scheduler quanta, captures Rack input transitions before that lower-rate boundary,
 * forwards panel gestures, and exposes the real gate states and OLED framebuffer.
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

    /**
     * @brief Advances the runtime by one Rack audio sample.
     *
     * Input edges are first captured at Rack sample rate and then replayed in order at the
     * firmware's 20-kHz scheduler boundary. This prevents short Rack trigger pulses from being
     * lost merely because they fall between adjacent 50-us firmware scheduler ticks.
     */
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

    /** @brief Returns SYNC comparator rising edges observed by the shared simulator boundary. */
    std::uint64_t syncPulseCountForTest() const;

    /** @brief Returns RST comparator rising edges observed by the shared simulator boundary. */
    std::uint64_t resetPulseCountForTest() const;

private:
    /**
     * @brief Fixed-capacity transition bridge from Rack sample time to scheduler time.
     *
     * The bridge is allocation-free because processSample() runs on Rack's audio thread. Normal
     * clock/trigger signals produce at most two queued transitions per pulse. Input transition
     * rates above the 20-kHz CLOCK scheduler boundary cannot be represented faithfully by the
     * production firmware and therefore collapse to the latest sampled level on overflow.
     */
    struct InputTransitionBridge final {
        static constexpr std::size_t kCapacity = 16U;

        /** @brief Captures one new Rack-sample input state. */
        void update(bool nextConnected, bool nextHigh) noexcept;

        /** @brief Returns the next level that must be presented to one scheduler tick. */
        bool nextSchedulerLevel() noexcept;

        /** @brief Clears queued transitions and returns to disconnected LOW. */
        void disconnect() noexcept;

        std::array<bool, kCapacity> pending{};
        std::size_t head = 0U;
        std::size_t count = 0U;
        bool connected = false;
        bool sampledHigh = false;
        bool schedulerHigh = false;
    };

    /** @brief Updates one hysteretic Rack voltage input into a conditioned logic level. */
    static bool updateInputLevel(float voltage, bool previousLevel);

    sim::SimulatorRuntime runtime_;
    PanelControls controls_{};
    InputTransitionBridge syncBridge_{};
    InputTransitionBridge resetBridge_{};
    double pendingSchedulerUs_ = 0.0;
};

}  // namespace clockfw::vcv
