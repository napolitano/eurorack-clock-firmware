/**
 * @file gate_output_driver.cpp
 * @brief HAL driver for the eight gate/LED logic outputs and HCT244 enable line.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/gate_output_driver.h"

#include <Arduino.h>

#include "domain/clock_types.h"
#include "pin_map.h"

namespace clockfw::hal {

void GateOutputDriver::beginDisabled() {
    pinMode(pinmap::kGateBufferOutputEnablePin, OUTPUT);
    disableOutputStage();

    for (const std::uint32_t pin : pinmap::kGateChannelPins) {
        pinMode(pin, OUTPUT);
        digitalWrite(pin, LOW);
    }
}

void GateOutputDriver::enableOutputStage() {
    digitalWrite(pinmap::kGateBufferOutputEnablePin, pinmap::kGateBufferEnabledLevel);
}

void GateOutputDriver::disableOutputStage() {
    digitalWrite(pinmap::kGateBufferOutputEnablePin, pinmap::kGateBufferDisabledLevel);
}

void GateOutputDriver::setAllChannelsLow() {
    for (const std::uint32_t pin : pinmap::kGateChannelPins) {
        digitalWrite(pin, LOW);
    }
}

void GateOutputDriver::setChannelState(const std::size_t channelIndex, const bool high) {
    if (channelIndex >= kChannelCount) {
        return;
    }

    digitalWrite(pinmap::kGateChannelPins[channelIndex], high ? HIGH : LOW);
}

}  // namespace clockfw::hal
