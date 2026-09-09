/**
 * @file arcade_leaderboard_store.cpp
 * @brief Compact CRC-protected Top-100 leaderboard implementation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "game/arcade_leaderboard_store.h"

#include <algorithm>
#include <array>

#include "game/easter_egg_score_store.h"

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

ArcadeLeaderboardStore::ArcadeLeaderboardStore(hal::PersistentStorage& storage, const ArcadeGameId gameId)
    : storage_(storage), gameId_(gameId) {}

std::size_t ArcadeLeaderboardStore::storageOffset() const {
    return kStorageBaseOffset + static_cast<std::size_t>(gameId_) * kRecordBytes;
}

std::uint8_t ArcadeLeaderboardStore::gameMagic() const {
    switch (gameId_) {
        case ArcadeGameId::Formula1: return static_cast<std::uint8_t>('F');
        case ArcadeGameId::Breakout: return static_cast<std::uint8_t>('B');
        case ArcadeGameId::EggJourney: return static_cast<std::uint8_t>('E');
        case ArcadeGameId::PixelRaid:
        default: return static_cast<std::uint8_t>('P');
    }
}

LeaderboardTable ArcadeLeaderboardStore::loadLegacyFallback() const {
    EasterEggScoreId legacyId = EasterEggScoreId::PixelRaid;
    switch (gameId_) {
        case ArcadeGameId::Formula1: legacyId = EasterEggScoreId::Formula1; break;
        case ArcadeGameId::Breakout: legacyId = EasterEggScoreId::Breakout; break;
        case ArcadeGameId::EggJourney: legacyId = EasterEggScoreId::MoonBuggy; break;
        case ArcadeGameId::PixelRaid: default: break;
    }
    EasterEggScoreStore legacy(storage_, legacyId);
    const HighScoreEntry previous = legacy.load();
    LeaderboardTable table{};
    if (previous.score > 0U) {
        table.count = 1U;
        table.entries[0].score = previous.score;
        table.entries[0].initials = previous.initials;
    }
    return table;
}

LeaderboardTable ArcadeLeaderboardStore::load() const {
    std::array<std::uint8_t, kRecordBytes> record{};
    if (!storage_.readBytes(storageOffset(), record.data(), record.size())) return loadLegacyFallback();
    const std::uint32_t expectedCrc = read32(record.data() + kRecordBytes - kCrcBytes);
    if (record[0] != static_cast<std::uint8_t>('A') || record[1] != static_cast<std::uint8_t>('R') ||
        record[2] != gameMagic() || record[3] != static_cast<std::uint8_t>('1') || record[4] != kVersion ||
        record[5] > LeaderboardTable::kMaximumEntries ||
        expectedCrc != calculateCrc32(record.data(), kRecordBytes - kCrcBytes)) {
        return loadLegacyFallback();
    }

    LeaderboardTable table{};
    table.count = record[5];
    for (std::size_t index = 0U; index < table.count; ++index) {
        const std::size_t offset = kHeaderBytes + index * kEntryBytes;
        const char a = static_cast<char>(record[offset + 4U]);
        const char b = static_cast<char>(record[offset + 5U]);
        const char c = static_cast<char>(record[offset + 6U]);
        if (!validInitial(a) || !validInitial(b) || !validInitial(c)) return loadLegacyFallback();
        table.entries[index].score = read32(record.data() + offset);
        table.entries[index].initials = {{a, b, c, '\0'}};
        if (index > 0U && table.entries[index].score > table.entries[index - 1U].score) return loadLegacyFallback();
    }
    return table;
}

std::int16_t ArcadeLeaderboardStore::qualifyingRank(const std::uint32_t score, const LeaderboardTable& table) const {
    if (score == 0U) return -1;
    for (std::size_t index = 0U; index < table.count; ++index) {
        if (score > table.entries[index].score) return static_cast<std::int16_t>(index);
    }
    if (table.count < LeaderboardTable::kMaximumEntries) return static_cast<std::int16_t>(table.count);
    return -1;
}

std::int16_t ArcadeLeaderboardStore::insertAndSave(const std::uint32_t score, const std::array<char, 4U>& initials) {
    if (!validInitial(initials[0]) || !validInitial(initials[1]) || !validInitial(initials[2])) return -1;
    LeaderboardTable table = load();
    const std::int16_t rank = qualifyingRank(score, table);
    if (rank < 0) return -1;
    const std::size_t target = static_cast<std::size_t>(rank);
    const std::size_t oldCount = table.count;
    const std::size_t newCount = std::min<std::size_t>(LeaderboardTable::kMaximumEntries, oldCount + 1U);
    for (std::size_t index = newCount - 1U; index > target; --index) table.entries[index] = table.entries[index - 1U];
    table.entries[target].score = score;
    table.entries[target].initials = initials;
    table.count = static_cast<std::uint8_t>(newCount);
    return save(table) ? rank : -1;
}

bool ArcadeLeaderboardStore::save(const LeaderboardTable& table) {
    std::array<std::uint8_t, kRecordBytes> record{};
    record[0] = static_cast<std::uint8_t>('A'); record[1] = static_cast<std::uint8_t>('R');
    record[2] = gameMagic(); record[3] = static_cast<std::uint8_t>('1'); record[4] = kVersion; record[5] = table.count;
    record[6] = 0U; record[7] = 0U;
    for (std::size_t index = 0U; index < table.count; ++index) {
        const std::size_t offset = kHeaderBytes + index * kEntryBytes;
        write32(record.data() + offset, table.entries[index].score);
        record[offset + 4U] = static_cast<std::uint8_t>(table.entries[index].initials[0]);
        record[offset + 5U] = static_cast<std::uint8_t>(table.entries[index].initials[1]);
        record[offset + 6U] = static_cast<std::uint8_t>(table.entries[index].initials[2]);
        record[offset + 7U] = 0U;
    }
    write32(record.data() + kRecordBytes - kCrcBytes, calculateCrc32(record.data(), kRecordBytes - kCrcBytes));
    return storage_.writeBytes(storageOffset(), record.data(), record.size());
}

std::uint32_t ArcadeLeaderboardStore::calculateCrc32(const std::uint8_t* const data, const std::size_t size) {
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
