/**
 * @file custom_groove_store.cpp
 * @brief Fixed-record CRC-protected persistent Custom Groove library.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/custom_groove_store.h"

#include <algorithm>
#include <array>
#include <cstring>

namespace clockfw::services {
namespace {
constexpr std::uint8_t kVersion = 1U;
constexpr std::uint32_t kMagic = 0x56524743UL;  // "CGRV" little-endian.
constexpr std::size_t kHeaderBytes = 8U;
constexpr std::size_t kNameOffset = kHeaderBytes;
constexpr std::size_t kOffsetsOffset = kNameOffset + CustomGrooveStore::kNameLength;
constexpr std::size_t kCrcOffset = CustomGrooveStore::kRecordBytes - 4U;
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
}  // namespace

CustomGrooveStore::CustomGrooveStore(hal::PersistentStorage& storage) : storage_(storage) {}

std::size_t CustomGrooveStore::slotOffset(const std::uint8_t slotIndex) {
    return kStorageOffset + static_cast<std::size_t>(slotIndex) * kRecordBytes;
}

bool CustomGrooveStore::validNameCharacter(const char character) {
    return character >= 0x20 && character <= 0x7E;
}

void CustomGrooveStore::normalizeName(const char* const source, char* const destination) {
    std::fill_n(destination, kNameLength + 1U, '\0');
    if (source == nullptr) {
        return;
    }
    for (std::size_t index = 0U; index < kNameLength && source[index] != '\0'; ++index) {
        destination[index] = validNameCharacter(source[index]) ? source[index] : ' ';
    }
}

bool CustomGrooveStore::load(
    const std::uint8_t slotIndex,
    CustomGroovePattern& pattern,
    char* const nameDestination,
    const std::size_t nameSize) const {
    if (slotIndex >= kCustomGrooveSlotCount) {
        return false;
    }
    std::array<std::uint8_t, kRecordBytes> record{};
    if (!storage_.readBytes(slotOffset(slotIndex), record.data(), record.size()) ||
        read32(record.data()) != kMagic || record[4] != kVersion ||
        read32(record.data() + kCrcOffset) != calculateCrc32(record.data(), kCrcOffset)) {
        return false;
    }

    CustomGroovePattern candidate{};
    candidate.length = record[5];
    for (std::size_t index = 0U; index < kCustomGrooveMaximumSteps; ++index) {
        candidate.offsets256[index] = static_cast<std::int8_t>(record[kOffsetsOffset + index]);
    }
    if (!isCustomGroovePatternValid(candidate)) {
        return false;
    }

    if (nameDestination != nullptr && nameSize > 0U) {
        const std::size_t count = std::min<std::size_t>(kNameLength, nameSize - 1U);
        for (std::size_t index = 0U; index < count; ++index) {
            const char character = static_cast<char>(record[kNameOffset + index]);
            nameDestination[index] = validNameCharacter(character) ? character : ' ';
        }
        nameDestination[count] = '\0';
        for (std::size_t index = count; index > 0U; --index) {
            if (nameDestination[index - 1U] != ' ') {
                break;
            }
            nameDestination[index - 1U] = '\0';
        }
    }

    pattern = candidate;
    return true;
}

bool CustomGrooveStore::save(
    const std::uint8_t slotIndex,
    const char* const nameSource,
    const CustomGroovePattern& pattern) {
    if (slotIndex >= kCustomGrooveSlotCount || !isCustomGroovePatternValid(pattern)) {
        return false;
    }

    std::array<std::uint8_t, kRecordBytes> record{};
    write32(record.data(), kMagic);
    record[4] = kVersion;
    record[5] = pattern.length;
    record[6] = 0U;
    record[7] = 0U;
    char normalized[kNameLength + 1U]{};
    normalizeName(nameSource, normalized);
    std::copy_n(
        reinterpret_cast<const std::uint8_t*>(normalized),
        kNameLength,
        record.begin() + static_cast<std::ptrdiff_t>(kNameOffset));
    for (std::size_t index = 0U; index < kCustomGrooveMaximumSteps; ++index) {
        record[kOffsetsOffset + index] = static_cast<std::uint8_t>(pattern.offsets256[index]);
    }
    write32(record.data() + kCrcOffset, calculateCrc32(record.data(), kCrcOffset));
    return storage_.writeBytes(slotOffset(slotIndex), record.data(), record.size());
}

bool CustomGrooveStore::clear(const std::uint8_t slotIndex) {
    if (slotIndex >= kCustomGrooveSlotCount) {
        return false;
    }
    std::array<std::uint8_t, kRecordBytes> erased{};
    erased.fill(0xFFU);
    return storage_.writeBytes(slotOffset(slotIndex), erased.data(), erased.size());
}

bool CustomGrooveStore::exists(const std::uint8_t slotIndex) const {
    CustomGroovePattern ignored{};
    return load(slotIndex, ignored);
}

void CustomGrooveStore::name(
    const std::uint8_t slotIndex,
    char* const destination,
    const std::size_t destinationSize) const {
    if (destination == nullptr || destinationSize == 0U) {
        return;
    }
    destination[0] = '\0';
    CustomGroovePattern ignored{};
    (void)load(slotIndex, ignored, destination, destinationSize);
}

std::uint32_t CustomGrooveStore::calculateCrc32(
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

}  // namespace clockfw::services
