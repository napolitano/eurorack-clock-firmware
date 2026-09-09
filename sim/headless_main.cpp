/**
 * @file headless_main.cpp
 * @brief Headless native simulator smoke executable used when SDL3 is unavailable.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <cstdint>
#include <filesystem>
#include <iostream>

#include "simulator_runtime.h"

int main() {
    const std::filesystem::path statePath = ".clock-simulator-headless-state.bin";
    clockfw::sim::SimulatorRuntime runtime(statePath);
    runtime.begin();
    runtime.advanceMicroseconds(1050000ULL);

    runtime.setButton(clockfw::sim::SimButton::Play, true);
    runtime.advanceMicroseconds(30000ULL);
    runtime.setButton(clockfw::sim::SimButton::Play, false);
    runtime.advanceMicroseconds(30000ULL);
    runtime.advanceMicroseconds(2000000ULL);

    std::uint64_t totalEdges = 0ULL;
    for (const auto& channel : runtime.telemetry()) {
        totalEdges += channel.risingEdges;
    }

    std::uint32_t framebufferChecksum = 2166136261UL;
    for (const std::uint8_t byte : runtime.framebuffer()) {
        framebufferChecksum ^= byte;
        framebufferChecksum *= 16777619UL;
    }

    runtime.flushPersistence();
    std::filesystem::remove(statePath);
    std::cout << "simulator-smoke bpm=" << runtime.state().bpm
              << " edges=" << totalEdges
              << " framebuffer=0x" << std::hex << framebufferChecksum << std::dec << '\n';

    return totalEdges > 0ULL ? 0 : 1;
}
