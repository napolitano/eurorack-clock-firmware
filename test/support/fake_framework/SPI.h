/**
 * @file SPI.h
 * @brief Host-test fake for STM32duino SPI display transport.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once
#include <cstdint>
#include <vector>
#include "Arduino.h"
inline constexpr std::uint8_t MSBFIRST=1U;
inline constexpr bool SPI_TRANSMITONLY = true;
inline constexpr std::uint8_t SPI_MODE0=0U;
class SPISettings {
public: SPISettings(std::uint32_t clock, std::uint8_t order, std::uint8_t mode):clockHz(clock),bitOrder(order),dataMode(mode){}
std::uint32_t clockHz; std::uint8_t bitOrder; std::uint8_t dataMode;
};
class SPIClass {
public:
 void reset(){sclk=0;mosi=0;begun=false;inTransaction=false;transfers.clear();beginCount=0;endCount=0;lastClock=0;}
 void setSCLK(PinName p){sclk=p;} void setMOSI(PinName p){mosi=p;} void begin(){begun=true;}
 void beginTransaction(const SPISettings& s){inTransaction=true;++beginCount;lastClock=s.clockHz;}
 std::uint8_t transfer(std::uint8_t v, bool = false){transfers.push_back(v);return v;}
 void endTransaction(){inTransaction=false;++endCount;}
 PinName sclk=0,mosi=0; bool begun=false,inTransaction=false; std::vector<std::uint8_t> transfers{}; unsigned beginCount=0,endCount=0; std::uint32_t lastClock=0;
};
inline SPIClass SPI{};
