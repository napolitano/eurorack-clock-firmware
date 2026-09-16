/**
 * @file gate_output_driver.cpp
 * @brief HAL driver for the eight gate/LED logic outputs and HCT244 enable line.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/gate_output_driver.h"

#include "hal/platform_io.h"

#include "domain/clock_types.h"
#include "pin_map.h"

namespace clockfw::hal {

void GateOutputDriver::beginDisabled() {
    platform::configureOutput(pinmap::kGateBufferOutputEnablePin);
    disableOutputStage();

    for (const std::uint32_t pin : pinmap::kGateChannelPins) {
        platform::configureOutput(pin);
        platform::write(pin, false);
    }
}

void GateOutputDriver::enableOutputStage() {
    platform::write(pinmap::kGateBufferOutputEnablePin, pinmap::kGateBufferEnabledLevel);
}

void GateOutputDriver::disableOutputStage() {
    platform::write(pinmap::kGateBufferOutputEnablePin, pinmap::kGateBufferDisabledLevel);
}

void GateOutputDriver::setAllChannelsLow() {
    for (const std::uint32_t pin : pinmap::kGateChannelPins) {
        platform::write(pin, false);
    }
}

void GateOutputDriver::setChannelState(const std::size_t channelIndex, const bool high) {
    if (channelIndex >= kChannelCount) {
        return;
    }

    platform::write(pinmap::kGateChannelPins[channelIndex], high);
}

}  // namespace clockfw::hal
