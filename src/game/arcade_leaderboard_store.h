/**
 * @file arcade_leaderboard_store.h
 * @brief Compact CRC-protected Top-100 leaderboards for ranked boot Easter eggs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "hal/persistent_layout.h"
#include "hal/persistent_storage.h"

namespace clockfw::game {

enum class ArcadeGameId : std::uint8_t { PixelRaid = 0U, Formula1 = 1U, Breakout = 2U, EggJourney = 3U };

struct LeaderboardEntry final {
    std::uint32_t score = 0U;
    std::array<char, 4U> initials{{'A', 'A', 'A', '\0'}};
};

struct LeaderboardTable final {
    static constexpr std::size_t kMaximumEntries = 100U;
    std::array<LeaderboardEntry, kMaximumEntries> entries{};
    std::uint8_t count = 0U;
};

class ArcadeLeaderboardStore final {
public:
    /** @brief Constructs one leaderboard view for a specific ranked Easter egg. */
    ArcadeLeaderboardStore(hal::PersistentStorage& storage, ArcadeGameId gameId);
    /** @brief Loads and validates the Top-100 table, migrating a historical single score when needed. */
    LeaderboardTable load() const;
    /** @brief Returns the zero-based insertion rank for a score, or -1 when it does not qualify. */
    std::int16_t qualifyingRank(std::uint32_t score, const LeaderboardTable& table) const;
    /** @brief Inserts one named score in sorted order and commits the updated table atomically. */
    std::int16_t insertAndSave(std::uint32_t score, const std::array<char, 4U>& initials);

private:
    static constexpr std::size_t kStorageBaseOffset = hal::persistent_layout::kLeaderboardRegionOffset;
    static constexpr std::size_t kHeaderBytes = 8U;
    static constexpr std::size_t kEntryBytes = 8U;
    static constexpr std::size_t kCrcBytes = 4U;
    static constexpr std::size_t kRecordBytes =
        kHeaderBytes + LeaderboardTable::kMaximumEntries * kEntryBytes + kCrcBytes;
    static constexpr std::size_t kSlotCount = 4U;

    /** @brief Returns the logical persistent-image offset assigned to this game. */
    std::size_t storageOffset() const;
    /** @brief Returns the compact game identifier embedded in the durable record. */
    std::uint8_t gameMagic() const;
    /** @brief Reads the prerelease single-score slot as a migration fallback. */
    LeaderboardTable loadLegacyFallback() const;
    /** @brief Serializes and commits one complete validated leaderboard record. */
    bool save(const LeaderboardTable& table);
    /** @brief Calculates reflected CRC-32 for durable leaderboard validation. */
    static std::uint32_t calculateCrc32(const std::uint8_t* data, std::size_t size);

    hal::PersistentStorage& storage_;
    ArcadeGameId gameId_;
};

static_assert(
    hal::persistent_layout::kLeaderboardRegionOffset +
            (8U + 100U * 8U + 4U) * 4U <=
        hal::PersistentStorage::kCapacityBytes,
    "Persistent storage is too small for four Top-100 leaderboards.");

}  // namespace clockfw::game
