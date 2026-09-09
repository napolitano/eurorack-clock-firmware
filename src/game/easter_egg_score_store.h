/**
 * @file easter_egg_score_store.h
 * @brief CRC-protected persistent high scores for boot Easter eggs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstdint>

#include "hal/persistent_storage.h"

namespace clockfw::game {

/** @brief Identifies the independent durable score slot used by one Easter egg. */
enum class EasterEggScoreId : std::uint8_t { PixelRaid = 0U, Formula1 = 1U, Breakout = 2U, MoonBuggy = 3U };

/** @brief Persistent three-letter arcade high-score entry. */
struct HighScoreEntry {
    std::uint32_t score = 0U;
    std::array<char, 4U> initials{{'A', 'A', 'A', '\0'}};
};

/** @brief Stores one Easter-egg high score in its own dedicated NVM slot. */
class EasterEggScoreStore final {
public:
    /** @brief Constructs a store for the selected game; Pixel Raid remains the compatibility default. */
    explicit EasterEggScoreStore(
        hal::PersistentStorage& storage,
        EasterEggScoreId scoreId = EasterEggScoreId::PixelRaid);

    /** @brief Loads a validated score record, returning the default zero score when absent. */
    HighScoreEntry load() const;

    /** @brief Persists a validated score entry immediately. Safe because boot games run before clocks. */
    bool save(const HighScoreEntry& entry);

private:
    static constexpr std::size_t kStorageBaseOffset = 3072U;
    static constexpr std::size_t kRecordSize = 16U;
    static constexpr std::size_t kSlotCount = 4U;

    /** @brief Calculates the standard reflected CRC-32 used throughout firmware persistence. */
    static std::uint32_t calculateCrc32(const std::uint8_t* data, std::size_t size);
    /** @brief Returns the first magic byte identifying the configured score slot. */
    std::uint8_t magicGameByte() const;
    /** @brief Returns the logical storage offset assigned to the configured score slot. */
    std::size_t storageOffset() const;

    hal::PersistentStorage& storage_;
    EasterEggScoreId scoreId_;
};

static_assert(
    3072U + 16U * 4U <= hal::PersistentStorage::kCapacityBytes,
    "Persistent storage is too small for Easter-egg high scores.");

}  // namespace clockfw::game
