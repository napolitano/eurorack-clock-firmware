/**
 * @file simulator_persistence.cpp
 * @brief File-backed persistence adapter for the native clock simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "simulator_persistence.h"

#include <algorithm>
#include <fstream>
#include <stdexcept>

namespace clockfw::sim {

SimulatorPersistence::SimulatorPersistence(std::filesystem::path path)
    : path_(std::move(path)) {
    lastFlushedImage_.fill(0xFFU);
}

void SimulatorPersistence::load() {
    hal::PersistentStorage::resetForTest();

    std::ifstream stream(path_, std::ios::binary);
    if (!stream) {
        lastFlushedImage_ = readImage();
        hasLastImage_ = true;
        return;
    }

    std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes> image{};
    image.fill(0xFFU);
    stream.read(reinterpret_cast<char*>(image.data()), static_cast<std::streamsize>(image.size()));
    const std::streamsize bytesRead = stream.gcount();
    if (bytesRead < 0) {
        throw std::runtime_error("Could not read simulator persistence file");
    }

    hal::PersistentStorage storage;
    if (!storage.writeBytes(0U, image.data(), image.size())) {
        throw std::runtime_error("Could not seed simulator persistence storage");
    }
    lastFlushedImage_ = image;
    hasLastImage_ = true;
}

void SimulatorPersistence::flushIfChanged() {
    const auto image = readImage();
    if (hasLastImage_ && image == lastFlushedImage_) {
        return;
    }
    flush();
}

void SimulatorPersistence::flush() {
    const auto image = readImage();
    if (!path_.parent_path().empty()) {
        std::filesystem::create_directories(path_.parent_path());
    }

    const std::filesystem::path temporaryPath = path_.string() + ".tmp";
    {
        std::ofstream stream(temporaryPath, std::ios::binary | std::ios::trunc);
        if (!stream) {
            throw std::runtime_error("Could not open simulator persistence file for writing");
        }
        stream.write(reinterpret_cast<const char*>(image.data()), static_cast<std::streamsize>(image.size()));
        if (!stream) {
            throw std::runtime_error("Could not write simulator persistence file");
        }
    }

    std::error_code error;
    std::filesystem::rename(temporaryPath, path_, error);
    if (error) {
        std::filesystem::remove(path_, error);
        error.clear();
        std::filesystem::rename(temporaryPath, path_, error);
        if (error) {
            throw std::runtime_error("Could not replace simulator persistence file");
        }
    }

    lastFlushedImage_ = image;
    hasLastImage_ = true;
}

const std::filesystem::path& SimulatorPersistence::path() const {
    return path_;
}

std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes>
SimulatorPersistence::readImage() const {
    std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes> image{};
    hal::PersistentStorage storage;
    if (!storage.readBytes(0U, image.data(), image.size())) {
        throw std::runtime_error("Could not read simulator persistence storage");
    }
    return image;
}

}  // namespace clockfw::sim
