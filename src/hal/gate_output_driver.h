/**
 * @file gate_output_driver.h
 * @brief HAL driver for the eight gate/LED logic outputs and optional buffer-enable line.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstddef>

namespace clockfw::hal {

/** @brief Owns all physical gate-output GPIO operations. */
class GateOutputDriver final {
public:
    /**
     * @brief Configures output GPIOs and leaves logical gate output muted.
     *
     * This method guarantees that all eight logic signals are LOW before the
     * firmware may allow gate HIGH requests.
     */
    void beginDisabled();

    /** @brief Enables logical gate output after all source signals are known LOW. */
    void enableOutputStage();

    /** @brief Immediately disables logical gate output and forces all source GPIOs LOW. */
    void disableOutputStage();

    /** @brief Drives every channel source signal LOW. */
    void setAllChannelsLow();

    /**
     * @brief Updates one gate/LED source signal.
     * @param channelIndex Zero-based channel index in the range 0..7.
     * @param high True for an active gate, false for LOW.
     */
    void setChannelState(std::size_t channelIndex, bool high);

    /** @brief Returns the last source level written for one gate channel. */
    bool channelStateHigh(std::size_t channelIndex) const;

    /** @brief Returns whether the logical gate-output stage currently accepts HIGH requests. */
    bool outputStageEnabled() const;

private:
    bool outputStageEnabled_{false};
    std::array<bool, 8U> channelStates_{};
};

}  // namespace clockfw::hal
