/**
 * @file SPI.h
 * @brief Native-simulator no-op SPI transport shim for the SSD1306 HAL.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "Arduino.h"

inline constexpr std::uint8_t MSBFIRST = 1U;
inline constexpr bool SPI_TRANSMITONLY = true;
inline constexpr std::uint8_t SPI_MODE0 = 0U;

class SPISettings {
public:
    SPISettings(std::uint32_t, std::uint8_t, std::uint8_t) {}
};

class SPIClass {
public:
    void setSCLK(PinName) {}
    void setMOSI(PinName) {}
    void begin() {}
    void beginTransaction(const SPISettings&) {}
    std::uint8_t transfer(std::uint8_t value, bool = false) { return value; }
    void endTransaction() {}
};

inline SPIClass SPI{};
