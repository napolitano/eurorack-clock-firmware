/**
 * @file easter_egg_score_store.cpp
 * @brief Persistent high-score implementation for boot Easter eggs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "game/easter_egg_score_store.h"

#include <array>

namespace clockfw::game {
namespace {
constexpr std::uint8_t kVersion = 1U;
constexpr std::uint32_t kCrcPolynomial = 0xEDB88320UL;
constexpr std::uint32_t kCrcInitialValue = 0xFFFFFFFFUL;

void write32(std::uint8_t* const destination, const std::uint32_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    destination[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    destination[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

std::uint32_t read32(const std::uint8_t* const source) {
    return static_cast<std::uint32_t>(source[0]) |
        (static_cast<std::uint32_t>(source[1]) << 8U) |
        (static_cast<std::uint32_t>(source[2]) << 16U) |
        (static_cast<std::uint32_t>(source[3]) << 24U);
}

bool validInitial(const char character) { return character >= 'A' && character <= 'Z'; }
}  // namespace

EasterEggScoreStore::EasterEggScoreStore(
    hal::PersistentStorage& storage,
    const EasterEggScoreId scoreId)
    : storage_(storage), scoreId_(scoreId) {}

std::uint8_t EasterEggScoreStore::magicGameByte() const {
    switch (scoreId_) {
        case EasterEggScoreId::Formula1: return static_cast<std::uint8_t>('F');
        case EasterEggScoreId::Breakout: return static_cast<std::uint8_t>('B');
        case EasterEggScoreId::MoonBuggy: return static_cast<std::uint8_t>('M');
        case EasterEggScoreId::PixelRaid:
        default: return static_cast<std::uint8_t>('P');
    }
}

std::size_t EasterEggScoreStore::storageOffset() const {
    return kStorageBaseOffset + static_cast<std::size_t>(scoreId_) * kRecordSize;
}

HighScoreEntry EasterEggScoreStore::load() const {
    std::array<std::uint8_t, kRecordSize> record{};
    HighScoreEntry result{};
    if (!storage_.readBytes(storageOffset(), record.data(), record.size())) return result;

    if (record[0] != magicGameByte() || record[1] != static_cast<std::uint8_t>('R') ||
        record[2] != static_cast<std::uint8_t>('H') || record[3] != static_cast<std::uint8_t>('S') ||
        record[4] != kVersion || read32(record.data() + 12U) != calculateCrc32(record.data(), 12U)) {
        return result;
    }

    const char first = static_cast<char>(record[9]);
    const char second = static_cast<char>(record[10]);
    const char third = static_cast<char>(record[11]);
    if (!validInitial(first) || !validInitial(second) || !validInitial(third)) return result;

    result.score = read32(record.data() + 5U);
    result.initials = {{first, second, third, '\0'}};
    return result;
}

bool EasterEggScoreStore::save(const HighScoreEntry& entry) {
    if (!validInitial(entry.initials[0]) || !validInitial(entry.initials[1]) || !validInitial(entry.initials[2])) return false;

    std::array<std::uint8_t, kRecordSize> record{};
    record[0] = magicGameByte();
    record[1] = static_cast<std::uint8_t>('R');
    record[2] = static_cast<std::uint8_t>('H');
    record[3] = static_cast<std::uint8_t>('S');
    record[4] = kVersion;
    write32(record.data() + 5U, entry.score);
    record[9] = static_cast<std::uint8_t>(entry.initials[0]);
    record[10] = static_cast<std::uint8_t>(entry.initials[1]);
    record[11] = static_cast<std::uint8_t>(entry.initials[2]);
    write32(record.data() + 12U, calculateCrc32(record.data(), 12U));
    return storage_.writeBytes(storageOffset(), record.data(), record.size());
}

std::uint32_t EasterEggScoreStore::calculateCrc32(const std::uint8_t* const data, const std::size_t size) {
    std::uint32_t crc = kCrcInitialValue;
    for (std::size_t index = 0U; index < size; ++index) {
        crc ^= data[index];
        for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
            const bool lowBit = (crc & 1U) != 0U;
            crc >>= 1U;
            if (lowBit) crc ^= kCrcPolynomial;
        }
    }
    return crc ^ kCrcInitialValue;
}

}  // namespace clockfw::game
