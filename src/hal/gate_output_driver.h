/**
 * @file gate_output_driver.h
 * @brief HAL driver for the eight gate/LED logic outputs and HCT244 enable line.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>

namespace clockfw::hal {

/** @brief Owns all physical gate-output GPIO operations. */
class GateOutputDriver final {
public:
    /**
     * @brief Configures output GPIOs and leaves the external buffer disabled.
     *
     * This method guarantees that all eight logic signals are LOW before the
     * 74HCT244 output stage may be enabled.
     */
    void beginDisabled();

    /** @brief Enables the external gate buffer after all source signals are known LOW. */
    void enableOutputStage();

    /** @brief Immediately disables the external gate buffer. */
    void disableOutputStage();

    /** @brief Drives every channel source signal LOW. */
    void setAllChannelsLow();

    /**
     * @brief Updates one gate/LED source signal.
     * @param channelIndex Zero-based channel index in the range 0..7.
     * @param high True for an active gate, false for LOW.
     */
    void setChannelState(std::size_t channelIndex, bool high);
};

}  // namespace clockfw::hal
