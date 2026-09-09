/**
 * @file Wire.h
 * @brief Host-test fake for STM32duino TwoWire display transport.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once
#include <cstddef>
#include <cstdint>
#include <map>
#include <vector>
#include "Arduino.h"

namespace fakefw {
struct I2cTransmission { std::uint8_t address; std::vector<std::uint8_t> bytes; };
}

class TwoWire {
public:
    void reset() { sda=0; scl=0; frequency=0; begun=false; currentAddress=0; current.clear(); transmissions.clear(); ack.clear(); }
    void setSDA(PinName value) { sda=value; }
    void setSCL(PinName value) { scl=value; }
    void begin() { begun=true; }
    void setClock(std::uint32_t value) { frequency=value; }
    void beginTransmission(std::uint8_t address) { currentAddress=address; current.clear(); }
    std::size_t write(std::uint8_t value) { current.push_back(value); return 1U; }
    std::size_t write(const std::uint8_t* data, std::size_t size) { current.insert(current.end(),data,data+size); return size; }
    std::uint8_t endTransmission() {
        transmissions.push_back({currentAddress,current});
        const auto it=ack.find(currentAddress); return it==ack.end()?4U:it->second;
    }
    PinName sda=0, scl=0; std::uint32_t frequency=0; bool begun=false;
    std::uint8_t currentAddress=0; std::vector<std::uint8_t> current{};
    std::vector<fakefw::I2cTransmission> transmissions{}; std::map<std::uint8_t,std::uint8_t> ack{};
};
inline TwoWire Wire{};
