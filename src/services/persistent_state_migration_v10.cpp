/**
 * @file persistent_state_migration_v10.cpp
 * @brief Backward-compatible v10 to v11 persistence migration for input roles.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_service.h"

#include <algorithm>

#include "defaults.h"

namespace clockfw::services {
namespace {

std::uint16_t readUint16LeV10(const std::uint8_t* const source) {
    return static_cast<std::uint16_t>(source[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(source[1]) << 8U);
}

std::uint32_t readUint32LeV10(const std::uint8_t* const source) {
    return static_cast<std::uint32_t>(source[0]) |
        (static_cast<std::uint32_t>(source[1]) << 8U) |
        (static_cast<std::uint32_t>(source[2]) << 16U) |
        (static_cast<std::uint32_t>(source[3]) << 24U);
}

bool isAllowedV10PresetCharacter(const char character) {
    return character == ' ' || character == '-' || character == '_' ||
        (character >= '0' && character <= '9') ||
        (character >= 'A' && character <= 'Z');
}

}  // namespace

bool PersistentStateService::deserializeV10State(
    const std::array<std::uint8_t, kV10StatePayloadSize>& payload,
    ClockState& state) {
    static_assert(kStatePayloadSize == kV10StatePayloadSize + 11U);
    std::array<std::uint8_t, kStatePayloadSize> upgraded{};
    std::copy(payload.begin(), payload.end(), upgraded.begin());
    upgraded[kV10StatePayloadSize] = static_cast<std::uint8_t>(defaults::kInput1Function);
    upgraded[kV10StatePayloadSize + 1U] = static_cast<std::uint8_t>(defaults::kInput2Function);
    return deserializeState(upgraded, state);
}

bool PersistentStateService::deserializeV10CurrentRecord(
    const std::array<std::uint8_t, kV10CurrentRecordSize>& record,
    ClockState& state) {
    constexpr std::uint32_t kCurrentMagicValue = 0x30315543UL;  // "CU10"
    if (readUint32LeV10(record.data()) != kCurrentMagicValue ||
        record[4] != kV10SchemaVersion || record[5] != 0U ||
        readUint16LeV10(record.data() + 6U) != kV10StatePayloadSize ||
        readUint32LeV10(record.data() + kV10CurrentRecordSize - 4U) !=
            calculateCrc32(record.data(), kV10CurrentRecordSize - 4U)) {
        return false;
    }
    std::array<std::uint8_t, kV10StatePayloadSize> payload{};
    std::copy_n(record.begin() + 8, payload.size(), payload.begin());
    return deserializeV10State(payload, state);
}

bool PersistentStateService::deserializeV10PresetRecord(
    const std::array<std::uint8_t, kV10PresetRecordSize>& record,
    char* const name,
    ClockState& state) {
    constexpr std::uint32_t kPresetMagicValue = 0x30315250UL;  // "PR10"
    constexpr std::size_t kNameOffset = 8U;
    constexpr std::size_t kPayloadOffset = kNameOffset + kPresetNameLength;
    if (readUint32LeV10(record.data()) != kPresetMagicValue ||
        record[4] != kV10SchemaVersion || record[5] != 1U ||
        readUint16LeV10(record.data() + 6U) != kV10StatePayloadSize ||
        readUint32LeV10(record.data() + kV10PresetRecordSize - 4U) !=
            calculateCrc32(record.data(), kV10PresetRecordSize - 4U)) {
        return false;
    }
    for (std::size_t index = 0U; index < kPresetNameLength; ++index) {
        const char character = static_cast<char>(record[kNameOffset + index]);
        if (!isAllowedV10PresetCharacter(character)) {
            return false;
        }
        name[index] = character;
    }
    name[kPresetNameLength] = '\0';
    std::array<std::uint8_t, kV10StatePayloadSize> payload{};
    std::copy_n(
        record.begin() + static_cast<std::ptrdiff_t>(kPayloadOffset),
        payload.size(),
        payload.begin());
    return deserializeV10State(payload, state);
}

}  // namespace clockfw::services
