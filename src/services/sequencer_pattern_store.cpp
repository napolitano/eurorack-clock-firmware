/**
 * @file sequencer_pattern_store.cpp
 * @brief Wear-coalesced CRC-protected Sequencer 2.0 pattern persistence.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/sequencer_pattern_store.h"

#include <algorithm>

#include "config.h"

namespace clockfw::services {
namespace {
constexpr std::uint32_t kMagic = 0x50325153UL;  // "SQ2P" little-endian.
constexpr std::size_t kCrcOffset = 8U;
constexpr std::size_t kPayloadOffset = SequencerPatternStore::kHeaderBytes;
constexpr std::uint32_t kCrcPolynomial = 0xEDB88320UL;
constexpr std::uint32_t kCrcInitialValue = 0xFFFFFFFFUL;
}

SequencerPatternStore::SequencerPatternStore(hal::PersistentStorage& storage) : storage_(storage) {
    resetDefaults();
}

std::size_t SequencerPatternStore::flatIndex(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex) {
    return static_cast<std::size_t>(channelIndex) * kSequencerPatternSlotsPerChannel + slotIndex;
}

void SequencerPatternStore::write32(std::uint8_t* const destination, const std::uint32_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    destination[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    destination[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

std::uint32_t SequencerPatternStore::read32(const std::uint8_t* const source) {
    return static_cast<std::uint32_t>(source[0]) |
        (static_cast<std::uint32_t>(source[1]) << 8U) |
        (static_cast<std::uint32_t>(source[2]) << 16U) |
        (static_cast<std::uint32_t>(source[3]) << 24U);
}

std::uint32_t SequencerPatternStore::calculateCrc32(
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

void SequencerPatternStore::encodePattern(
    const SequencerPatternV2& pattern,
    std::uint8_t* const record) {
    record[0] = pattern.length;
    record[1] = pattern.rotation;
    record[2] = static_cast<std::uint8_t>(pattern.direction);
    record[3] = static_cast<std::uint8_t>(pattern.loopMode);
    for (std::size_t wordIndex = 0U; wordIndex < kSequencerGateWordCount; ++wordIndex) {
        const std::uint64_t word = pattern.gates[wordIndex];
        for (std::size_t byteIndex = 0U; byteIndex < 8U; ++byteIndex) {
            record[4U + wordIndex * 8U + byteIndex] = static_cast<std::uint8_t>(
                (word >> static_cast<unsigned>(byteIndex * 8U)) & 0xFFULL);
        }
    }
    // Bytes 20..23 are deliberately reserved for a future compact metadata pointer/profile.
    std::fill(record + 20U, record + kRecordBytes, 0xFFU);
}

bool SequencerPatternStore::decodePattern(
    const std::uint8_t* const record,
    SequencerPatternV2& pattern) {
    SequencerPatternV2 candidate{};
    candidate.length = record[0];
    candidate.rotation = record[1];
    candidate.direction = static_cast<SequencerPlayDirection>(record[2]);
    candidate.loopMode = static_cast<SequencerLoopMode>(record[3]);
    for (std::size_t wordIndex = 0U; wordIndex < kSequencerGateWordCount; ++wordIndex) {
        std::uint64_t word = 0ULL;
        for (std::size_t byteIndex = 0U; byteIndex < 8U; ++byteIndex) {
            word |= static_cast<std::uint64_t>(record[4U + wordIndex * 8U + byteIndex])
                << static_cast<unsigned>(byteIndex * 8U);
        }
        candidate.gates[wordIndex] = word;
    }
    if (!isSequencerPatternValid(candidate)) {
        return false;
    }
    clampSequencerPattern(candidate);
    pattern = candidate;
    return true;
}

void SequencerPatternStore::resetDefaults() {
    for (SequencerPatternV2& pattern : patterns_) {
        pattern = SequencerPatternV2{};
    }
    activeSlots_.fill(0U);
    dirty_ = false;
    dirtySinceMs_ = 0U;
    hasDurableBank_ = false;
}

bool SequencerPatternStore::decodeBank(const std::array<std::uint8_t, kStorageBytes>& bytes) {
    const std::uint8_t formatVersion = bytes[4];
    if (read32(bytes.data()) != kMagic ||
        (formatVersion != kFormatVersion && formatVersion != kLegacyFormatVersion) ||
        bytes[5] != kChannelCount || bytes[6] != kSequencerPatternSlotsPerChannel ||
        bytes[7] != kSequencerMaximumSteps ||
        read32(bytes.data() + kCrcOffset) !=
            calculateCrc32(bytes.data() + kPayloadOffset, bytes.size() - kPayloadOffset)) {
        return false;
    }

    std::array<SequencerPatternV2, kPatternCount> decoded{};
    for (std::size_t index = 0U; index < decoded.size(); ++index) {
        const std::size_t offset = kPayloadOffset + index * kRecordBytes;
        if (!decodePattern(bytes.data() + offset, decoded[index])) {
            return false;
        }
    }
    std::array<std::uint8_t, kChannelCount> decodedActive{};
    if (formatVersion == kFormatVersion) {
        for (std::size_t channelIndex = 0U; channelIndex < decodedActive.size(); ++channelIndex) {
            const std::uint8_t slot = bytes[kActiveSlotsOffset + channelIndex];
            if (slot >= kSequencerPatternSlotsPerChannel) {
                return false;
            }
            decodedActive[channelIndex] = slot;
        }
    }
    patterns_ = decoded;
    activeSlots_ = decodedActive;
    return true;
}

void SequencerPatternStore::encodeBank(std::array<std::uint8_t, kStorageBytes>& bytes) const {
    bytes.fill(0xFFU);
    write32(bytes.data(), kMagic);
    bytes[4] = kFormatVersion;
    bytes[5] = kChannelCount;
    bytes[6] = kSequencerPatternSlotsPerChannel;
    bytes[7] = kSequencerMaximumSteps;
    for (std::size_t index = 0U; index < patterns_.size(); ++index) {
        encodePattern(patterns_[index], bytes.data() + kPayloadOffset + index * kRecordBytes);
    }
    for (std::size_t channelIndex = 0U; channelIndex < activeSlots_.size(); ++channelIndex) {
        bytes[kActiveSlotsOffset + channelIndex] = activeSlots_[channelIndex];
    }
    write32(
        bytes.data() + kCrcOffset,
        calculateCrc32(bytes.data() + kPayloadOffset, bytes.size() - kPayloadOffset));
}

bool SequencerPatternStore::begin() {
    std::array<std::uint8_t, kStorageBytes> bytes{};
    if (!storage_.readBytes(kStorageOffset, bytes.data(), bytes.size())) {
        resetDefaults();
        return false;
    }
    if (!decodeBank(bytes)) {
        resetDefaults();
        return true;
    }
    dirty_ = false;
    dirtySinceMs_ = 0U;
    hasDurableBank_ = true;
    return true;
}

const SequencerPatternV2& SequencerPatternStore::pattern(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex) const {
    if (channelIndex >= kChannelCount || slotIndex >= kSequencerPatternSlotsPerChannel) {
        return patterns_[0U];
    }
    return patterns_[flatIndex(channelIndex, slotIndex)];
}


std::uint8_t SequencerPatternStore::activeSlot(const std::uint8_t channelIndex) const {
    return channelIndex < kChannelCount ? activeSlots_[channelIndex] : 0U;
}

bool SequencerPatternStore::setActiveSlot(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex,
    const std::uint32_t nowMs) {
    if (channelIndex >= kChannelCount || slotIndex >= kSequencerPatternSlotsPerChannel) {
        return false;
    }
    if (activeSlots_[channelIndex] == slotIndex) {
        return true;
    }
    activeSlots_[channelIndex] = slotIndex;
    if (!dirty_) {
        dirtySinceMs_ = nowMs;
    }
    dirty_ = true;
    return true;
}

bool SequencerPatternStore::seedLegacyPatternOnes(
    const ClockState& state,
    const std::uint32_t nowMs) {
    if (hasDurableBank_ || dirty_) {
        return false;
    }
    for (std::uint8_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        const SequencerSettings& legacy = state.channels[channelIndex].sequencer;
        SequencerPatternV2 migrated{};
        migrated.length = legacy.length == 0U
            ? 1U
            : static_cast<std::uint8_t>(std::min<std::uint16_t>(legacy.length, 64U));
        migrated.rotation = legacy.rotation < migrated.length ? legacy.rotation : 0U;
        migrated.direction = SequencerPlayDirection::Forward;
        migrated.loopMode = SequencerLoopMode::Loop;
        migrated.gates = {{legacy.pattern, 0ULL}};
        clampSequencerPattern(migrated);
        patterns_[flatIndex(channelIndex, 0U)] = migrated;
    }
    dirtySinceMs_ = nowMs;
    dirty_ = true;
    return true;
}

bool SequencerPatternStore::updatePattern(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex,
    const SequencerPatternV2& patternValue,
    const std::uint32_t nowMs) {
    if (channelIndex >= kChannelCount || slotIndex >= kSequencerPatternSlotsPerChannel ||
        !isSequencerPatternValid(patternValue)) {
        return false;
    }
    SequencerPatternV2 candidate = patternValue;
    clampSequencerPattern(candidate);
    patterns_[flatIndex(channelIndex, slotIndex)] = candidate;
    if (!dirty_) {
        dirtySinceMs_ = nowMs;
    }
    dirty_ = true;
    return true;
}

bool SequencerPatternStore::setGate(
    const std::uint8_t channelIndex,
    const std::uint8_t slotIndex,
    const std::uint8_t step,
    const bool enabled,
    const std::uint32_t nowMs) {
    if (channelIndex >= kChannelCount || slotIndex >= kSequencerPatternSlotsPerChannel ||
        step >= kSequencerMaximumSteps) {
        return false;
    }
    SequencerPatternV2 candidate = patterns_[flatIndex(channelIndex, slotIndex)];
    (void)setSequencerPatternGate(candidate, step, enabled);
    return updatePattern(channelIndex, slotIndex, candidate, nowMs);
}

bool SequencerPatternStore::service(const std::uint32_t nowMs, const bool allowFlashWrite) {
    if (!dirty_ || !allowFlashWrite ||
        nowMs - dirtySinceMs_ < config::kPersistenceCommitDelayMs) {
        return true;
    }
    return flush();
}

bool SequencerPatternStore::flush() {
    if (!dirty_) {
        return true;
    }
    std::array<std::uint8_t, kStorageBytes> bytes{};
    encodeBank(bytes);
    if (!storage_.writeBytes(kStorageOffset, bytes.data(), bytes.size())) {
        return false;
    }
    dirty_ = false;
    dirtySinceMs_ = 0U;
    hasDurableBank_ = true;
    return true;
}

}  // namespace clockfw::services
