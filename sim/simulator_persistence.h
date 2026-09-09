/**
 * @file simulator_persistence.h
 * @brief File-backed persistence adapter for the native clock simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstdint>
#include <filesystem>

#include "hal/persistent_storage.h"

namespace clockfw::sim {

/** @brief Mirrors the host-test persistence bytes to a normal desktop file. */
class SimulatorPersistence final {
public:
    /** @brief Constructs the adapter for one simulator-state file. */
    explicit SimulatorPersistence(std::filesystem::path path);

    /** @brief Loads an existing state file into the firmware's simulated NVM before boot. */
    void load();

    /** @brief Writes current simulated NVM to disk when contents changed. */
    void flushIfChanged();

    /** @brief Forces the current simulated NVM image to disk. */
    void flush();

    /** @brief Returns path used for durable simulator state. */
    const std::filesystem::path& path() const;

private:
    /** @brief Reads the complete host-storage image through the real persistence HAL API. */
    std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes> readImage() const;

    std::filesystem::path path_;
    std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes> lastFlushedImage_{};
    bool hasLastImage_ = false;
};

}  // namespace clockfw::sim
