/**
 * @file persistent_storage.cpp
 * @brief Power-loss-safe A/B Flash storage implementation for STM32F401 and host tests.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/persistent_storage.h"

#include <algorithm>
#include <array>
#include <cstdint>
#include <cstring>

#ifndef CLOCK_HOST_TEST
#include <stm32f4xx_hal.h>
#endif

namespace clockfw::hal {
namespace {

constexpr std::uint8_t kErasedByte = 0xFFU;
constexpr std::uint32_t kSlotMagic = 0x434C4B50UL;      // "CLKP"
constexpr std::uint32_t kSlotFormatVersion = 1U;
constexpr std::uint32_t kCommitMarker = 0xC10C17EDUL;
constexpr std::uint32_t kCrcPolynomial = 0xEDB88320UL;
constexpr std::uint32_t kCrcInitialValue = 0xFFFFFFFFUL;
constexpr std::size_t kSlotHeaderBytes = 32U;
constexpr std::size_t kHeaderWordCount = kSlotHeaderBytes / sizeof(std::uint32_t);
constexpr std::size_t kCommitWordIndex = kHeaderWordCount - 1U;
constexpr std::size_t kPayloadOffset = kSlotHeaderBytes;
constexpr std::size_t kLegacyPayloadBytes = 4096U;

static_assert(
    kSlotHeaderBytes + PersistentStorage::kMaximumImageBytes <=
        PersistentStorage::kPhysicalSlotBytes,
    "12-KiB persistent-image policy must fit inside one 16-KiB slot");
static_assert((PersistentStorage::kCapacityBytes % 4U) == 0U, "Flash image must be word-aligned");

struct SlotHeader final {
    std::uint32_t magic = kSlotMagic;
    std::uint32_t formatVersion = kSlotFormatVersion;
    std::uint32_t generation = 0U;
    std::uint32_t payloadSize = static_cast<std::uint32_t>(PersistentStorage::kCapacityBytes);
    std::uint32_t payloadCrc32 = 0U;
    std::uint32_t headerCrc32 = 0U;
    std::uint32_t reserved = 0xFFFFFFFFUL;
    std::uint32_t commitMarker = 0xFFFFFFFFUL;
};
static_assert(sizeof(SlotHeader) == kSlotHeaderBytes, "Unexpected slot-header padding");

std::uint32_t calculateCrc32(const std::uint8_t* const data, const std::size_t size) {
    std::uint32_t crc = kCrcInitialValue;
    for (std::size_t index = 0U; index < size; ++index) {
        crc ^= data[index];
        for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
            const bool lowBit = (crc & 1U) != 0U;
            crc >>= 1U;
            if (lowBit) {
                crc ^= kCrcPolynomial;
            }
        }
    }
    return crc ^ kCrcInitialValue;
}

std::uint32_t headerCrc32(const SlotHeader& header) {
    // The first five words are immutable metadata. headerCrc/reserved/commit are excluded.
    return calculateCrc32(
        reinterpret_cast<const std::uint8_t*>(&header),
        5U * sizeof(std::uint32_t));
}

bool generationIsNewer(const std::uint32_t candidate, const std::uint32_t reference) {
    return static_cast<std::int32_t>(candidate - reference) > 0;
}

#ifdef CLOCK_HOST_TEST
std::array<std::array<std::uint8_t, PersistentStorage::kPhysicalSlotBytes>, 2U> gHostSlots = [] {
    std::array<std::array<std::uint8_t, PersistentStorage::kPhysicalSlotBytes>, 2U> slots{};
    for (auto& slot : slots) {
        slot.fill(kErasedByte);
    }
    return slots;
}();
std::uint32_t gHostWriteCommitCount = 0U;
bool gFailNextRead = false;
bool gFailNextWrite = false;
bool gPowerLossBeforeCommit = false;

const std::uint8_t* slotBase(const std::size_t slotIndex) {
    return gHostSlots[slotIndex].data();
}

std::uint8_t* mutableSlotBase(const std::size_t slotIndex) {
    return gHostSlots[slotIndex].data();
}
#else
constexpr std::uintptr_t kSlotAddresses[2U] = {0x08004000UL, 0x08008000UL};
constexpr std::uint32_t kSlotSectors[2U] = {FLASH_SECTOR_1, FLASH_SECTOR_2};

const std::uint8_t* slotBase(const std::size_t slotIndex) {
    return reinterpret_cast<const std::uint8_t*>(kSlotAddresses[slotIndex]);
}
#endif

bool readHeader(const std::size_t slotIndex, SlotHeader& header) {
    std::memcpy(&header, slotBase(slotIndex), sizeof(header));
    return true;
}

bool slotIsValid(const std::size_t slotIndex, SlotHeader& header) {
    (void)readHeader(slotIndex, header);
    if (header.magic != kSlotMagic ||
        header.formatVersion != kSlotFormatVersion ||
        (header.payloadSize != PersistentStorage::kCapacityBytes && header.payloadSize != kLegacyPayloadBytes) ||
        header.commitMarker != kCommitMarker ||
        header.headerCrc32 != headerCrc32(header)) {
        return false;
    }
    return header.payloadCrc32 == calculateCrc32(
        slotBase(slotIndex) + kPayloadOffset,
        static_cast<std::size_t>(header.payloadSize));
}

int newestValidSlot(SlotHeader* const newestHeader = nullptr) {
    SlotHeader first{};
    SlotHeader second{};
    const bool firstValid = slotIsValid(0U, first);
    const bool secondValid = slotIsValid(1U, second);
    if (!firstValid && !secondValid) {
        return -1;
    }

    std::size_t newest = 0U;
    SlotHeader selected = first;
    if (!firstValid || (secondValid && generationIsNewer(second.generation, first.generation))) {
        newest = 1U;
        selected = second;
    }
    if (newestHeader != nullptr) {
        *newestHeader = selected;
    }
    return static_cast<int>(newest);
}

#ifndef CLOCK_HOST_TEST
bool eraseSlotUnlocked(const std::size_t slotIndex) {
    FLASH_EraseInitTypeDef erase{};
    erase.TypeErase = FLASH_TYPEERASE_SECTORS;
    erase.Sector = kSlotSectors[slotIndex];
    erase.NbSectors = 1U;
    erase.VoltageRange = FLASH_VOLTAGE_RANGE_3;
    std::uint32_t sectorError = 0U;
    return HAL_FLASHEx_Erase(&erase, &sectorError) == HAL_OK;
}

bool programWordUnlocked(const std::uintptr_t address, const std::uint32_t value) {
    return HAL_FLASH_Program(
        FLASH_TYPEPROGRAM_WORD,
        static_cast<std::uint32_t>(address),
        static_cast<std::uint64_t>(value)) == HAL_OK;
}

bool programWordsUnlocked(
    const std::uintptr_t address,
    const std::uint8_t* const bytes,
    const std::size_t size) {
    if ((size % sizeof(std::uint32_t)) != 0U) {
        return false;
    }
    for (std::size_t offset = 0U; offset < size; offset += sizeof(std::uint32_t)) {
        std::uint32_t word = 0U;
        std::memcpy(&word, bytes + offset, sizeof(word));
        if (!programWordUnlocked(address + offset, word)) {
            return false;
        }
    }
    return true;
}
#endif

bool commitImage(
    const std::size_t targetSlot,
    const std::uint32_t generation,
    const std::array<std::uint8_t, PersistentStorage::kCapacityBytes>& image) {
    SlotHeader header{};
    header.generation = generation;
    header.payloadCrc32 = calculateCrc32(image.data(), image.size());
    header.headerCrc32 = headerCrc32(header);

#ifdef CLOCK_HOST_TEST
    auto& slot = gHostSlots[targetSlot];
    slot.fill(kErasedByte);
    std::memcpy(slot.data(), &header, kCommitWordIndex * sizeof(std::uint32_t));
    std::copy(image.begin(), image.end(), slot.begin() + static_cast<std::ptrdiff_t>(kPayloadOffset));
    if (gPowerLossBeforeCommit) {
        gPowerLossBeforeCommit = false;
        return false;
    }
    std::memcpy(
        slot.data() + static_cast<std::ptrdiff_t>(kCommitWordIndex * sizeof(std::uint32_t)),
        &kCommitMarker,
        sizeof(kCommitMarker));
#else
    if (HAL_FLASH_Unlock() != HAL_OK) {
        return false;
    }
    const std::uintptr_t base = kSlotAddresses[targetSlot];
    const bool programmed =
        eraseSlotUnlocked(targetSlot) &&
        programWordsUnlocked(
            base,
            reinterpret_cast<const std::uint8_t*>(&header),
            kCommitWordIndex * sizeof(std::uint32_t)) &&
        programWordsUnlocked(base + kPayloadOffset, image.data(), image.size()) &&
        // COMMIT is deliberately the last programmed word. Until this succeeds,
        // boot validation must continue to prefer the previous generation.
        programWordUnlocked(base + kCommitWordIndex * sizeof(std::uint32_t), kCommitMarker);
    (void)HAL_FLASH_Lock();
    if (!programmed) {
        return false;
    }
#endif

    SlotHeader verified{};
    return slotIsValid(targetSlot, verified) && verified.generation == generation;
}

}  // namespace

bool PersistentStorage::readBytes(
    const std::size_t offset,
    std::uint8_t* const destination,
    const std::size_t size) const {
    if (destination == nullptr || !isRangeValid(offset, size)) {
        return false;
    }
#ifdef CLOCK_HOST_TEST
    if (gFailNextRead) {
        gFailNextRead = false;
        return false;
    }
#endif

    const int activeSlot = newestValidSlot();
    if (activeSlot < 0) {
        std::fill_n(destination, size, kErasedByte);
        return true;
    }
    SlotHeader activeHeader{};
    (void)readHeader(static_cast<std::size_t>(activeSlot), activeHeader);
    const std::size_t payloadSize = static_cast<std::size_t>(activeHeader.payloadSize);
    std::fill_n(destination, size, kErasedByte);
    if (offset < payloadSize) {
        const std::size_t readable = std::min<std::size_t>(size, payloadSize - offset);
        std::memcpy(
            destination,
            slotBase(static_cast<std::size_t>(activeSlot)) + kPayloadOffset + offset,
            readable);
    }
    return true;
}

bool PersistentStorage::writeBytes(
    const std::size_t offset,
    const std::uint8_t* const source,
    const std::size_t size) {
    if (source == nullptr || !isRangeValid(offset, size)) {
        return false;
    }
#ifdef CLOCK_HOST_TEST
    if (gFailNextWrite) {
        gFailNextWrite = false;
        return false;
    }
#endif

    std::array<std::uint8_t, kCapacityBytes> image{};
    SlotHeader activeHeader{};
    const int activeSlot = newestValidSlot(&activeHeader);
    if (activeSlot < 0) {
        image.fill(kErasedByte);
    } else {
        image.fill(kErasedByte);
        const std::size_t previousSize = std::min<std::size_t>(
            image.size(), static_cast<std::size_t>(activeHeader.payloadSize));
        std::memcpy(
            image.data(),
            slotBase(static_cast<std::size_t>(activeSlot)) + kPayloadOffset,
            previousSize);
    }

    if (std::equal(source, source + size, image.begin() + static_cast<std::ptrdiff_t>(offset))) {
        return true;
    }
    std::copy_n(source, size, image.begin() + static_cast<std::ptrdiff_t>(offset));

    const std::size_t targetSlot = activeSlot == 0 ? 1U : 0U;
    const std::uint32_t generation =
        activeSlot < 0 ? 1U : activeHeader.generation + 1U;
    if (!commitImage(targetSlot, generation, image)) {
        return false;
    }
#ifdef CLOCK_HOST_TEST
    ++gHostWriteCommitCount;
#endif
    return true;
}

#ifdef CLOCK_HOST_TEST
void PersistentStorage::resetForTest() {
    for (auto& slot : gHostSlots) {
        slot.fill(kErasedByte);
    }
    gHostWriteCommitCount = 0U;
    gFailNextRead = false;
    gFailNextWrite = false;
    gPowerLossBeforeCommit = false;
}

std::uint32_t PersistentStorage::writeCommitCountForTest() {
    return gHostWriteCommitCount;
}

void PersistentStorage::failNextReadForTest() {
    gFailNextRead = true;
}

void PersistentStorage::failNextWriteForTest() {
    gFailNextWrite = true;
}

void PersistentStorage::powerLossBeforeCommitForTest() {
    gPowerLossBeforeCommit = true;
}

void PersistentStorage::corruptNewestPayloadByteForTest(const std::size_t offset) {
    if (offset >= kCapacityBytes) {
        return;
    }
    const int activeSlot = newestValidSlot();
    if (activeSlot >= 0) {
        mutableSlotBase(static_cast<std::size_t>(activeSlot))[kPayloadOffset + offset] ^= 0x01U;
    }
}

void PersistentStorage::corruptNewestHeaderWordForTest(
    const std::size_t wordIndex,
    const std::uint32_t value) {
    if (wordIndex >= kHeaderWordCount) {
        return;
    }
    const int activeSlot = newestValidSlot();
    if (activeSlot < 0) {
        return;
    }
    std::memcpy(
        mutableSlotBase(static_cast<std::size_t>(activeSlot)) +
            wordIndex * sizeof(std::uint32_t),
        &value,
        sizeof(value));
}
bool PersistentStorage::seedLegacyImageForTest(const std::uint8_t* const image, const std::size_t size) {
    if (image == nullptr || size != kLegacyPayloadBytes) {
        return false;
    }
    resetForTest();
    SlotHeader header{};
    header.generation = 7U;
    header.payloadSize = static_cast<std::uint32_t>(kLegacyPayloadBytes);
    header.payloadCrc32 = calculateCrc32(image, size);
    header.headerCrc32 = headerCrc32(header);
    header.commitMarker = kCommitMarker;
    auto& slot = gHostSlots[0U];
    slot.fill(kErasedByte);
    std::memcpy(slot.data(), &header, sizeof(header));
    std::memcpy(slot.data() + static_cast<std::ptrdiff_t>(kPayloadOffset), image, size);
    SlotHeader verified{};
    return slotIsValid(0U, verified) && verified.payloadSize == kLegacyPayloadBytes;
}

#endif

bool PersistentStorage::isRangeValid(const std::size_t offset, const std::size_t size) {
    return offset <= kCapacityBytes && size <= kCapacityBytes - offset;
}

}  // namespace clockfw::hal
