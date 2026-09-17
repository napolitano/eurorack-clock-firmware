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

    for (std::size_t index = 0U; index < pinmap::kGateChannelPins.size(); ++index) {
        platform::configureOutput(pinmap::kGateChannelPins[index]);
        platform::write(pinmap::kGateChannelPins[index], false);
        channelStates_[index] = false;
    }
}

void GateOutputDriver::enableOutputStage() {
    platform::write(pinmap::kGateBufferOutputEnablePin, pinmap::kGateBufferEnabledLevel);
}

void GateOutputDriver::disableOutputStage() {
    platform::write(pinmap::kGateBufferOutputEnablePin, pinmap::kGateBufferDisabledLevel);
}

void GateOutputDriver::setAllChannelsLow() {
    for (std::size_t index = 0U; index < pinmap::kGateChannelPins.size(); ++index) {
        platform::write(pinmap::kGateChannelPins[index], false);
        channelStates_[index] = false;
    }
}

void GateOutputDriver::setChannelState(const std::size_t channelIndex, const bool high) {
    if (channelIndex >= kChannelCount) {
        return;
    }

    platform::write(pinmap::kGateChannelPins[channelIndex], high);
    channelStates_[channelIndex] = high;
}

bool GateOutputDriver::channelStateHigh(const std::size_t channelIndex) const {
    return channelIndex < channelStates_.size() ? channelStates_[channelIndex] : false;
}

}  // namespace clockfw::hal
