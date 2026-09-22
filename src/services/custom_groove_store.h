/**
 * @file custom_groove_store.h
 * @brief Fixed-record persistent library for user-authored Custom Grooves.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/custom_groove.h"
#include "hal/persistent_storage.h"

namespace clockfw::services {

/**
 * @brief Stores ten bounded Custom Groove records in the unused V1 legacy-score window.
 *
 * The store is deliberately separate from the repeated CURRENT/eight-preset ClockState
 * records. Ten 96-byte records exactly occupy bytes 3136..4095, preserving the four
 * historical 16-byte score records at 3072..3135 and the Top-100 region at 4096.
 */
class CustomGrooveStore final {
public:
    static constexpr std::size_t kNameLength = 16U;
    static constexpr std::size_t kRecordBytes = 96U;
    static constexpr std::size_t kStorageOffset = 3136U;

    /** @brief Constructs the fixed-record store around the shared persistent image. */
    explicit CustomGrooveStore(hal::PersistentStorage& storage);

    /** @brief Loads one validated slot; returns false for empty/corrupt/out-of-range slots. */
    bool load(
        std::uint8_t slotIndex,
        CustomGroovePattern& pattern,
        char* name = nullptr,
        std::size_t nameSize = 0U) const;

    /** @brief Saves one named validated pattern with CRC protection. */
    bool save(std::uint8_t slotIndex, const char* name, const CustomGroovePattern& pattern);

    /** @brief Erases one slot back to the Flash-erased representation. */
    bool clear(std::uint8_t slotIndex);

    /** @brief Returns true when a slot contains one valid current-format record. */
    bool exists(std::uint8_t slotIndex) const;

    /** @brief Copies a slot name or an empty string for absent/corrupt slots. */
    void name(std::uint8_t slotIndex, char* destination, std::size_t destinationSize) const;

private:
    /** @brief Calculates the record CRC over the supplied byte range. */
    static std::uint32_t calculateCrc32(const std::uint8_t* data, std::size_t size);
    /** @brief Returns the byte offset of one fixed Custom Groove slot. */
    static std::size_t slotOffset(std::uint8_t slotIndex);
    /** @brief Returns true for printable ASCII accepted by the persistent name field. */
    static bool validNameCharacter(char character);
    /** @brief Copies and sanitizes one bounded persistent Custom Groove name. */
    static void normalizeName(const char* source, char* destination);

    hal::PersistentStorage& storage_;
};

static_assert(
    CustomGrooveStore::kStorageOffset +
            static_cast<std::size_t>(kCustomGrooveSlotCount) * CustomGrooveStore::kRecordBytes ==
        hal::persistent_layout::kLeaderboardRegionOffset,
    "Custom Groove records must end exactly at the Top-100 region boundary.");

}  // namespace clockfw::services
