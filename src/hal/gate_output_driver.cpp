/**
 * @file gate_output_driver.cpp
 * @brief HAL driver for the eight gate/LED logic outputs and optional shared buffer-enable line.
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
    outputStageEnabled_ = false;
    if (pinmap::kGateBufferOutputEnablePin != pinmap::kUnassignedDigitalPin) {
        platform::configureOutput(pinmap::kGateBufferOutputEnablePin);
        platform::write(pinmap::kGateBufferOutputEnablePin, pinmap::kGateBufferDisabledLevel);
    }

    for (std::size_t index = 0U; index < pinmap::kGateChannelPins.size(); ++index) {
        platform::configureOutput(pinmap::kGateChannelPins[index]);
        platform::write(pinmap::kGateChannelPins[index], false);
        channelStates_[index] = false;
    }
}

void GateOutputDriver::enableOutputStage() {
    // Source GPIOs remain LOW until the engine deliberately raises a channel.
    outputStageEnabled_ = true;
    if (pinmap::kGateBufferOutputEnablePin != pinmap::kUnassignedDigitalPin) {
        platform::write(pinmap::kGateBufferOutputEnablePin, pinmap::kGateBufferEnabledLevel);
    }
}

void GateOutputDriver::disableOutputStage() {
    outputStageEnabled_ = false;
    setAllChannelsLow();
    if (pinmap::kGateBufferOutputEnablePin != pinmap::kUnassignedDigitalPin) {
        platform::write(pinmap::kGateBufferOutputEnablePin, pinmap::kGateBufferDisabledLevel);
    }
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

    const bool effectiveHigh = outputStageEnabled_ && high;
    platform::write(pinmap::kGateChannelPins[channelIndex], effectiveHigh);
    channelStates_[channelIndex] = effectiveHigh;
}

bool GateOutputDriver::channelStateHigh(const std::size_t channelIndex) const {
    return channelIndex < channelStates_.size() ? channelStates_[channelIndex] : false;
}

bool GateOutputDriver::outputStageEnabled() const {
    return outputStageEnabled_;
}

}  // namespace clockfw::hal
