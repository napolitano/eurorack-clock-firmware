/**
 * @file sequencer_step_store.cpp
 * @brief CRC-protected sparse persistence for Sequencer 2.0 step expression.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/sequencer_step_store.h"

#include <algorithm>

#include "config.h"

namespace clockfw::services {
namespace {
constexpr std::uint32_t kMagic = 0x53513253UL;  // "S2QS" little-endian; format-scoped magic.
constexpr std::size_t kCrcOffset = 8U;
constexpr std::size_t kPayloadOffset = SequencerStepStore::kHeaderBytes;
constexpr std::uint32_t kCrcPolynomial = 0xEDB88320UL;
constexpr std::uint32_t kCrcInitialValue = 0xFFFFFFFFUL;
constexpr std::uint16_t kKeyReservedMask = 0xE000U;
}

SequencerStepStore::SequencerStepStore(hal::PersistentStorage& storage) : storage_(storage) {
    resetDefaults();
}

std::uint16_t SequencerStepStore::makeKey(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex,
    const std::uint8_t step) {
    return static_cast<std::uint16_t>(
        static_cast<std::uint16_t>(step) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(channelIndex) << 7U) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(slotIndex) << 10U));
}

bool SequencerStepStore::decodeKey(
    const std::uint16_t key,
    std::uint8_t& channelIndex,
    std::uint8_t& slotIndex,
    std::uint8_t& step) {
    if ((key & kKeyReservedMask) != 0U) {
        return false;
    }
    step = static_cast<std::uint8_t>(key & 0x007FU);
    channelIndex = static_cast<std::uint8_t>((key >> 7U) & 0x0007U);
    slotIndex = static_cast<std::uint8_t>((key >> 10U) & 0x0007U);
    return channelIndex < kChannelCount &&
        slotIndex < kSequencerPatternSlotsPerChannel && step < kSequencerMaximumSteps;
}

void SequencerStepStore::write16(std::uint8_t* const destination, const std::uint16_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

std::uint16_t SequencerStepStore::read16(const std::uint8_t* const source) {
    return static_cast<std::uint16_t>(source[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(source[1]) << 8U);
}

void SequencerStepStore::write32(std::uint8_t* const destination, const std::uint32_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    destination[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    destination[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

std::uint32_t SequencerStepStore::read32(const std::uint8_t* const source) {
    return static_cast<std::uint32_t>(source[0]) |
        (static_cast<std::uint32_t>(source[1]) << 8U) |
        (static_cast<std::uint32_t>(source[2]) << 16U) |
        (static_cast<std::uint32_t>(source[3]) << 24U);
}

std::uint32_t SequencerStepStore::calculateCrc32(
    const std::uint8_t* const data,
    const std::size_t size) {
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

std::size_t SequencerStepStore::findKey(const std::uint16_t key) const {
    for (std::size_t index = 0U; index < count_; ++index) {
        if (keys_[index] == key) {
            return index;
        }
    }
    return kMaximumOverrides;
}

void SequencerStepStore::markDirty(const std::uint32_t nowMs) {
    if (!dirty_) {
        dirtySinceMs_ = nowMs;
    }
    dirty_ = true;
}

void SequencerStepStore::resetDefaults() {
    keys_.fill(kUnusedKey);
    words_.fill(0U);
    count_ = 0U;
    dirtySinceMs_ = 0U;
    dirty_ = false;
}

bool SequencerStepStore::decodeStore(const std::array<std::uint8_t, kStorageBytes>& bytes) {
    if (read32(bytes.data()) != kMagic || bytes[4] != kFormatVersion ||
        bytes[7] != kSequencerMaximumSteps ||
        read32(bytes.data() + kCrcOffset) !=
            calculateCrc32(bytes.data() + kPayloadOffset, bytes.size() - kPayloadOffset)) {
        return false;
    }
    const std::size_t recordCount = read16(bytes.data() + 5U);
    if (recordCount > kMaximumOverrides) {
        return false;
    }

    std::array<std::uint16_t, kMaximumOverrides> decodedKeys{};
    std::array<SequencerStepMetadataWord, kMaximumOverrides> decodedWords{};
    decodedKeys.fill(kUnusedKey);
    for (std::size_t index = 0U; index < recordCount; ++index) {
        const std::size_t offset = kPayloadOffset + index * kRecordBytes;
        const std::uint16_t key = read16(bytes.data() + offset);
        const SequencerStepMetadataWord word = read16(bytes.data() + offset + 2U);
        std::uint8_t channel = 0U;
        std::uint8_t slot = 0U;
        std::uint8_t step = 0U;
        const SequencerStepMetadata metadata = unpackSequencerStepMetadata(word);
        if (!decodeKey(key, channel, slot, step) || isSequencerStepMetadataDefault(metadata) ||
            packSequencerStepMetadata(metadata) != word) {
            return false;
        }
        for (std::size_t previous = 0U; previous < index; ++previous) {
            if (decodedKeys[previous] == key) {
                return false;
            }
        }
        decodedKeys[index] = key;
        decodedWords[index] = word;
    }
    keys_ = decodedKeys;
    words_ = decodedWords;
    count_ = recordCount;
    return true;
}

void SequencerStepStore::encodeStore(std::array<std::uint8_t, kStorageBytes>& bytes) const {
    bytes.fill(0xFFU);
    write32(bytes.data(), kMagic);
    bytes[4] = kFormatVersion;
    write16(bytes.data() + 5U, static_cast<std::uint16_t>(count_));
    bytes[7] = kSequencerMaximumSteps;
    for (std::size_t index = 0U; index < count_; ++index) {
        const std::size_t offset = kPayloadOffset + index * kRecordBytes;
        write16(bytes.data() + offset, keys_[index]);
        write16(bytes.data() + offset + 2U, words_[index]);
    }
    write32(
        bytes.data() + kCrcOffset,
        calculateCrc32(bytes.data() + kPayloadOffset, bytes.size() - kPayloadOffset));
}

bool SequencerStepStore::begin() {
    std::array<std::uint8_t, kStorageBytes> bytes{};
    if (!storage_.readBytes(kStorageOffset, bytes.data(), bytes.size())) {
        resetDefaults();
        return false;
    }
    const bool erased = std::all_of(bytes.begin(), bytes.end(), [](const std::uint8_t value) {
        return value == 0xFFU;
    });
    if (erased) {
        resetDefaults();
        return true;
    }
    if (!decodeStore(bytes)) {
        resetDefaults();
        return true;
    }
    dirty_ = false;
    dirtySinceMs_ = 0U;
    return true;
}

SequencerStepMetadata SequencerStepStore::metadata(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex,
    const std::uint8_t step) const {
    if (channelIndex >= kChannelCount || slotIndex >= kSequencerPatternSlotsPerChannel ||
        step >= kSequencerMaximumSteps) {
        return SequencerStepMetadata{};
    }
    const std::size_t index = findKey(makeKey(channelIndex, slotIndex, step));
    return index < kMaximumOverrides
        ? unpackSequencerStepMetadata(words_[index])
        : SequencerStepMetadata{};
}

void SequencerStepStore::loadPatternWords(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex,
    std::array<SequencerStepMetadataWord, kSequencerMaximumSteps>& destination) const {
    destination.fill(0U);
    if (channelIndex >= kChannelCount || slotIndex >= kSequencerPatternSlotsPerChannel) {
        return;
    }
    for (std::size_t index = 0U; index < count_; ++index) {
        std::uint8_t recordChannel = 0U;
        std::uint8_t recordSlot = 0U;
        std::uint8_t recordStep = 0U;
        if (decodeKey(keys_[index], recordChannel, recordSlot, recordStep) &&
            recordChannel == channelIndex && recordSlot == slotIndex) {
            destination[recordStep] = words_[index];
        }
    }
}

bool SequencerStepStore::updateMetadata(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex,
    const std::uint8_t step,
    const SequencerStepMetadata& metadataValue,
    const std::uint32_t nowMs) {
    if (channelIndex >= kChannelCount || slotIndex >= kSequencerPatternSlotsPerChannel ||
        step >= kSequencerMaximumSteps || !isSequencerStepMetadataValid(metadataValue)) {
        return false;
    }
    const std::uint16_t key = makeKey(channelIndex, slotIndex, step);
    const std::size_t existing = findKey(key);
    if (isSequencerStepMetadataDefault(metadataValue)) {
        if (existing >= kMaximumOverrides) {
            return true;
        }
        const std::size_t last = count_ - 1U;
        keys_[existing] = keys_[last];
        words_[existing] = words_[last];
        keys_[last] = kUnusedKey;
        words_[last] = 0U;
        --count_;
        markDirty(nowMs);
        return true;
    }

    const SequencerStepMetadataWord word = packSequencerStepMetadata(metadataValue);
    if (existing < kMaximumOverrides) {
        if (words_[existing] == word) {
            return true;
        }
        words_[existing] = word;
        markDirty(nowMs);
        return true;
    }
    if (count_ >= kMaximumOverrides) {
        return false;
    }
    keys_[count_] = key;
    words_[count_] = word;
    ++count_;
    markDirty(nowMs);
    return true;
}

bool SequencerStepStore::clearPattern(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex,
    const std::uint32_t nowMs) {
    if (channelIndex >= kChannelCount || slotIndex >= kSequencerPatternSlotsPerChannel) {
        return false;
    }
    std::size_t writeIndex = 0U;
    bool changed = false;
    for (std::size_t index = 0U; index < count_; ++index) {
        std::uint8_t recordChannel = 0U;
        std::uint8_t recordSlot = 0U;
        std::uint8_t recordStep = 0U;
        (void)decodeKey(keys_[index], recordChannel, recordSlot, recordStep);
        if (recordChannel == channelIndex && recordSlot == slotIndex) {
            changed = true;
            continue;
        }
        keys_[writeIndex] = keys_[index];
        words_[writeIndex] = words_[index];
        ++writeIndex;
    }
    for (std::size_t index = writeIndex; index < count_; ++index) {
        keys_[index] = kUnusedKey;
        words_[index] = 0U;
    }
    count_ = writeIndex;
    if (changed) {
        markDirty(nowMs);
    }
    return true;
}

bool SequencerStepStore::replacePatternWords(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex,
    const std::array<SequencerStepMetadataWord, kSequencerMaximumSteps>& words,
    const std::uint32_t nowMs) {
    if (channelIndex >= kChannelCount || slotIndex >= kSequencerPatternSlotsPerChannel) {
        return false;
    }
    std::size_t required = 0U;
    for (const SequencerStepMetadataWord word : words) {
        const SequencerStepMetadata metadataValue = unpackSequencerStepMetadata(word);
        if (packSequencerStepMetadata(metadataValue) != word) {
            return false;
        }
        if (!isSequencerStepMetadataDefault(metadataValue)) {
            ++required;
        }
    }

    std::size_t existingTarget = 0U;
    for (std::size_t index = 0U; index < count_; ++index) {
        std::uint8_t recordChannel = 0U;
        std::uint8_t recordSlot = 0U;
        std::uint8_t recordStep = 0U;
        (void)decodeKey(keys_[index], recordChannel, recordSlot, recordStep);
        if (recordChannel == channelIndex && recordSlot == slotIndex) {
            ++existingTarget;
        }
    }
    if (count_ - existingTarget + required > kMaximumOverrides) {
        return false;
    }
    (void)clearPattern(channelIndex, slotIndex, nowMs);
    for (std::uint16_t step = 0U; step < kSequencerMaximumSteps; ++step) {
        const SequencerStepMetadata metadataValue = unpackSequencerStepMetadata(words[step]);
        if (!isSequencerStepMetadataDefault(metadataValue) &&
            !updateMetadata(
                channelIndex,
                slotIndex,
                static_cast<std::uint8_t>(step),
                metadataValue,
                nowMs)) {
            return false;
        }
    }
    return true;
}

bool SequencerStepStore::service(const std::uint32_t nowMs, const bool allowFlashWrite) {
    if (!dirty_ || !allowFlashWrite ||
        nowMs - dirtySinceMs_ < config::kPersistenceCommitDelayMs) {
        return true;
    }
    return flush();
}

bool SequencerStepStore::flush() {
    if (!dirty_) {
        return true;
    }
    std::array<std::uint8_t, kStorageBytes> bytes{};
    encodeStore(bytes);
    if (!storage_.writeBytes(kStorageOffset, bytes.data(), bytes.size())) {
        return false;
    }
    dirty_ = false;
    dirtySinceMs_ = 0U;
    return true;
}

}  // namespace clockfw::services
