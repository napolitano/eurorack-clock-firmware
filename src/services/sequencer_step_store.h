/**
 * @file sequencer_step_store.h
 * @brief Sparse persistent Sequencer 2.0 per-step expression store.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "domain/clock_types.h"
#include "domain/sequencer_pattern.h"
#include "domain/sequencer_step_metadata.h"
#include "hal/persistent_storage.h"

namespace clockfw::services {

/**
 * @brief Stores only exceptional per-step expression records across all 64 patterns.
 *
 * Each persistent record is exactly four bytes: a compiler-independent 13-bit
 * channel/slot/step key plus one stable 16-bit metadata word. Untouched steps
 * consume no record. The last 512 bytes of the 12-KiB image remain reserved for
 * the later Song Mode rather than being consumed by step expression.
 */
class SequencerStepStore final {
public:
    static constexpr std::size_t kStorageOffset = hal::persistent_layout::kSequencerStepMetadataOffset;
    static constexpr std::size_t kStorageBytes = hal::persistent_layout::kSequencerStepMetadataBytes;
    static constexpr std::size_t kHeaderBytes = 16U;
    static constexpr std::size_t kRecordBytes = 4U;
    static constexpr std::size_t kMaximumOverrides = (kStorageBytes - kHeaderBytes) / kRecordBytes;

    /** @brief Creates the sparse store over the shared A/B persistence image. */
    explicit SequencerStepStore(hal::PersistentStorage& storage);

    /** @brief Loads a valid sparse image or initializes an empty in-memory store. */
    bool begin();

    /** @brief Returns one override, or DEFAULT metadata when no explicit record exists. */
    SequencerStepMetadata metadata(
        std::uint8_t channelIndex,
        std::uint8_t slotIndex,
        std::uint8_t step) const;

    /** @brief Expands one active pattern into the fixed 128-word scheduler snapshot. */
    void loadPatternWords(
        std::uint8_t channelIndex,
        std::uint8_t slotIndex,
        std::array<SequencerStepMetadataWord, kSequencerMaximumSteps>& destination) const;

    /** @brief Adds, replaces, or removes one sparse override and schedules persistence. */
    bool updateMetadata(
        std::uint8_t channelIndex,
        std::uint8_t slotIndex,
        std::uint8_t step,
        const SequencerStepMetadata& metadata,
        std::uint32_t nowMs);

    /** @brief Removes every expression override owned by one pattern slot. */
    bool clearPattern(std::uint8_t channelIndex, std::uint8_t slotIndex, std::uint32_t nowMs);

    /** @brief Replaces one pattern's overrides from a complete 128-word snapshot. */
    bool replacePatternWords(
        std::uint8_t channelIndex,
        std::uint8_t slotIndex,
        const std::array<SequencerStepMetadataWord, kSequencerMaximumSteps>& words,
        std::uint32_t nowMs);

    /** @brief Commits pending changes after the normal coalescing delay when Flash is safe. */
    bool service(std::uint32_t nowMs, bool allowFlashWrite);

    /** @brief Forces an immediate durable commit of pending step expression. */
    bool flush();

    /** @brief Returns the number of non-default sparse records currently held in RAM. */
    std::size_t overrideCount() const { return count_; }

    /** @brief Returns whether RAM differs from the durable sparse image. */
    bool dirty() const { return dirty_; }

private:
    static constexpr std::uint8_t kFormatVersion = 1U;
    static constexpr std::uint16_t kUnusedKey = 0xFFFFU;

    /** @brief Builds the stable 13-bit channel/slot/step record key. */
    static std::uint16_t makeKey(std::uint8_t channelIndex, std::uint8_t slotIndex, std::uint8_t step);
    /** @brief Decodes and validates one stable record key. */
    static bool decodeKey(
        std::uint16_t key,
        std::uint8_t& channelIndex,
        std::uint8_t& slotIndex,
        std::uint8_t& step);
    /** @brief Computes the payload CRC-32. */
    static std::uint32_t calculateCrc32(const std::uint8_t* data, std::size_t size);
    /** @brief Writes one little-endian 16-bit field. */
    static void write16(std::uint8_t* destination, std::uint16_t value);
    /** @brief Reads one little-endian 16-bit field. */
    static std::uint16_t read16(const std::uint8_t* source);
    /** @brief Writes one little-endian 32-bit field. */
    static void write32(std::uint8_t* destination, std::uint32_t value);
    /** @brief Reads one little-endian 32-bit field. */
    static std::uint32_t read32(const std::uint8_t* source);
    /** @brief Finds one record index or returns kMaximumOverrides when absent. */
    std::size_t findKey(std::uint16_t key) const;
    /** @brief Marks the in-memory image dirty while retaining the first-edit timestamp. */
    void markDirty(std::uint32_t nowMs);
    /** @brief Restores an empty sparse store. */
    void resetDefaults();
    /** @brief Decodes and validates the complete sparse store image. */
    bool decodeStore(const std::array<std::uint8_t, kStorageBytes>& bytes);
    /** @brief Encodes the complete sparse store image with CRC. */
    void encodeStore(std::array<std::uint8_t, kStorageBytes>& bytes) const;

    hal::PersistentStorage& storage_;
    std::array<std::uint16_t, kMaximumOverrides> keys_{};
    std::array<SequencerStepMetadataWord, kMaximumOverrides> words_{};
    std::size_t count_ = 0U;
    std::uint32_t dirtySinceMs_ = 0U;
    bool dirty_ = false;
};

static_assert(
    SequencerStepStore::kStorageOffset + SequencerStepStore::kStorageBytes <=
        hal::persistent_layout::kSequencerSongRegionOffset,
    "Step metadata must not consume the reserved Song Mode region.");
static_assert(
    SequencerStepStore::kMaximumOverrides == 380U,
    "Sparse step metadata capacity changed unexpectedly.");

}  // namespace clockfw::services
