/**
 * @file sequencer_pattern_store.h
 * @brief Wear-coalesced persistent Sequencer 2.0 pattern bank.
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
#include "hal/persistent_storage.h"

namespace clockfw::services {

/**
 * @brief Stores 8 patterns for each of CLOCK's 8 physical channels.
 *
 * The bank occupies the first 2 KiB of the post-V1 8..12-KiB persistence extension.
 * The remaining 2 KiB stays reserved for sparse step metadata / Song data. The bank
 * is global user data rather than being duplicated inside CURRENT/eight presets.
 */
class SequencerPatternStore final {
public:
    static constexpr std::size_t kStorageOffset = hal::persistent_layout::kSequencer2RegionOffset;
    static constexpr std::size_t kStorageBytes = hal::persistent_layout::kSequencerPatternBankBytes;
    static constexpr std::size_t kHeaderBytes = 16U;
    static constexpr std::size_t kRecordBytes = 24U;
    static constexpr std::size_t kPatternCount =
        static_cast<std::size_t>(kChannelCount) * kSequencerPatternSlotsPerChannel;

    /** @brief Creates a pattern store over the shared durable persistence image. */
    explicit SequencerPatternStore(hal::PersistentStorage& storage);

    /** @brief Loads the durable bank or initializes clean in-memory defaults when absent/corrupt. */
    bool begin();

    /** @brief Returns one immutable slot, or slot 0/channel 0 defaults for invalid indices. */
    const SequencerPatternV2& pattern(std::uint8_t channelIndex, std::uint8_t slotIndex) const;

    /** @brief Returns the persisted active pattern slot for one channel, defaulting to P1. */
    std::uint8_t activeSlot(std::uint8_t channelIndex) const;

    /** @brief Selects P1..P8 for one channel and schedules a coalesced persistent commit. */
    bool setActiveSlot(std::uint8_t channelIndex, std::uint8_t slotIndex, std::uint32_t nowMs);

    /**
     * @brief Seeds P1 of every channel from the legacy 1.x SequencerSettings.
     *
     * This is accepted only while no durable Sequencer 2.0 bank exists; slots P2..P8
     * retain their clean defaults. It provides the one-way migration bridge from the
     * historical per-channel 64-bit sequence into the new global pattern bank.
     */
    bool seedLegacyPatternOnes(const ClockState& state, std::uint32_t nowMs);

    /** @brief Replaces one validated slot and schedules a coalesced persistent commit. */
    bool updatePattern(
        std::uint8_t channelIndex,
        std::uint8_t slotIndex,
        const SequencerPatternV2& pattern,
        std::uint32_t nowMs);

    /** @brief Mutates one binary gate bit and schedules persistence. */
    bool setGate(
        std::uint8_t channelIndex,
        std::uint8_t slotIndex,
        std::uint8_t step,
        bool enabled,
        std::uint32_t nowMs);

    /** @brief Commits pending changes once the normal delay elapsed and Flash writes are allowed. */
    bool service(std::uint32_t nowMs, bool allowFlashWrite);

    /** @brief Forces an immediate commit of a dirty bank; primarily useful for controlled tests/tools. */
    bool flush();

    /** @brief Returns whether RAM differs from the durable pattern bank. */
    bool dirty() const { return dirty_; }
    /** @brief Returns whether a valid Sequencer 2.0 bank was loaded or committed. */
    bool hasDurableBank() const { return hasDurableBank_; }

private:
    static constexpr std::uint8_t kFormatVersion = 2U;
    static constexpr std::uint8_t kLegacyFormatVersion = 1U;
    static constexpr std::size_t kActiveSlotsOffset =
        kHeaderBytes + kPatternCount * kRecordBytes;

    /** @brief Maps channel and slot indices onto the fixed bank array. */
    static std::size_t flatIndex(std::uint8_t channelIndex, std::uint8_t slotIndex);
    /** @brief Computes the bank payload CRC-32. */
    static std::uint32_t calculateCrc32(const std::uint8_t* data, std::size_t size);
    /** @brief Writes one little-endian 32-bit field. */
    static void write32(std::uint8_t* destination, std::uint32_t value);
    /** @brief Reads one little-endian 32-bit field. */
    static std::uint32_t read32(const std::uint8_t* source);
    /** @brief Encodes one pattern into its compiler-independent fixed record. */
    static void encodePattern(const SequencerPatternV2& pattern, std::uint8_t* record);
    /** @brief Decodes and validates one fixed pattern record. */
    static bool decodePattern(const std::uint8_t* record, SequencerPatternV2& pattern);
    /** @brief Restores clean in-memory defaults for all 64 slots. */
    void resetDefaults();
    /** @brief Decodes and validates the complete pattern bank image. */
    bool decodeBank(const std::array<std::uint8_t, kStorageBytes>& bytes);
    /** @brief Encodes the complete pattern bank with header and CRC. */
    void encodeBank(std::array<std::uint8_t, kStorageBytes>& bytes) const;

    hal::PersistentStorage& storage_;
    std::array<SequencerPatternV2, kPatternCount> patterns_{};
    std::array<std::uint8_t, kChannelCount> activeSlots_{};
    std::uint32_t dirtySinceMs_ = 0U;
    bool dirty_ = false;
    bool hasDurableBank_ = false;
};

static_assert(
    SequencerPatternStore::kHeaderBytes +
            SequencerPatternStore::kPatternCount * SequencerPatternStore::kRecordBytes <=
        SequencerPatternStore::kStorageBytes,
    "Sequencer pattern bank must fit inside its 2-KiB extension region.");
static_assert(
    SequencerPatternStore::kStorageOffset + SequencerPatternStore::kStorageBytes <=
        hal::PersistentStorage::kCapacityBytes,
    "Sequencer pattern bank must fit inside the current logical persistence image.");

}  // namespace clockfw::services
