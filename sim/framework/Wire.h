/**
 * @file Wire.h
 * @brief Native-simulator no-op I2C transport shim for the SSD1306 HAL.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstddef>
#include <cstdint>

#include "Arduino.h"

class TwoWire {
public:
    void setSDA(PinName value) { sda_ = value; }
    void setSCL(PinName value) { scl_ = value; }
    void begin() { begun_ = true; }
    void setClock(std::uint32_t value) { frequencyHz_ = value; }
    void beginTransmission(std::uint8_t address) { currentAddress_ = address; }
    std::size_t write(std::uint8_t) { return 1U; }
    std::size_t write(const std::uint8_t*, std::size_t size) { return size; }
    std::uint8_t endTransmission() const { return currentAddress_ == 0x3CU ? 0U : 4U; }

private:
    PinName sda_ = 0U;
    PinName scl_ = 0U;
    std::uint32_t frequencyHz_ = 0U;
    std::uint8_t currentAddress_ = 0U;
    bool begun_ = false;
};

inline TwoWire Wire{};
