/**
 * @file persistent_state_migration_v9.cpp
 * @brief Backward-compatible v9 to v10 persistence migration for Stage-1 Groove state.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_service.h"

#include <algorithm>

namespace clockfw::services {
namespace {

std::uint16_t readUint16LeV9(const std::uint8_t* const source) {
    return static_cast<std::uint16_t>(source[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(source[1]) << 8U);
}

std::uint32_t readUint32LeV9(const std::uint8_t* const source) {
    return static_cast<std::uint32_t>(source[0]) |
        (static_cast<std::uint32_t>(source[1]) << 8U) |
        (static_cast<std::uint32_t>(source[2]) << 16U) |
        (static_cast<std::uint32_t>(source[3]) << 24U);
}

bool isAllowedV9PresetCharacter(const char character) {
    return character == ' ' || character == '-' || character == '_' ||
        (character >= '0' && character <= '9') ||
        (character >= 'A' && character <= 'Z');
}

}  // namespace

bool PersistentStateService::deserializeV9State(
    const std::array<std::uint8_t, kV9StatePayloadSize>& payload,
    ClockState& state) {
    static_assert(kStatePayloadSize == kV9StatePayloadSize + 27U);
    std::array<std::uint8_t, kStatePayloadSize> upgraded{};
    std::copy(payload.begin(), payload.end(), upgraded.begin());
    std::size_t position = kV9StatePayloadSize;
    for (std::size_t index = 0U; index < 9U; ++index) {
        upgraded[position++] = static_cast<std::uint8_t>(GroovePreset::Off);
        upgraded[position++] = 100U;
        upgraded[position++] = 0U;
    }
    return deserializeState(upgraded, state);
}

bool PersistentStateService::deserializeV9CurrentRecord(
    const std::array<std::uint8_t, kV9CurrentRecordSize>& record,
    ClockState& state) {
    constexpr std::uint32_t kCurrentMagicValue = 0x39525543UL;  // "CUR9"
    if (readUint32LeV9(record.data()) != kCurrentMagicValue ||
        record[4] != kV9SchemaVersion || record[5] != 0U ||
        readUint16LeV9(record.data() + 6U) != kV9StatePayloadSize ||
        readUint32LeV9(record.data() + kV9CurrentRecordSize - 4U) !=
            calculateCrc32(record.data(), kV9CurrentRecordSize - 4U)) {
        return false;
    }
    std::array<std::uint8_t, kV9StatePayloadSize> payload{};
    std::copy_n(record.begin() + 8, payload.size(), payload.begin());
    return deserializeV9State(payload, state);
}

bool PersistentStateService::deserializeV9PresetRecord(
    const std::array<std::uint8_t, kV9PresetRecordSize>& record,
    char* const name,
    ClockState& state) {
    constexpr std::uint32_t kPresetMagicValue = 0x39455250UL;  // "PRE9"
    constexpr std::size_t kNameOffset = 8U;
    constexpr std::size_t kPayloadOffset = kNameOffset + kPresetNameLength;
    if (readUint32LeV9(record.data()) != kPresetMagicValue ||
        record[4] != kV9SchemaVersion || record[5] != 1U ||
        readUint16LeV9(record.data() + 6U) != kV9StatePayloadSize ||
        readUint32LeV9(record.data() + kV9PresetRecordSize - 4U) !=
            calculateCrc32(record.data(), kV9PresetRecordSize - 4U)) {
        return false;
    }
    for (std::size_t index = 0U; index < kPresetNameLength; ++index) {
        const char character = static_cast<char>(record[kNameOffset + index]);
        if (!isAllowedV9PresetCharacter(character)) return false;
        name[index] = character;
    }
    name[kPresetNameLength] = '\0';
    std::array<std::uint8_t, kV9StatePayloadSize> payload{};
    std::copy_n(record.begin() + static_cast<std::ptrdiff_t>(kPayloadOffset), payload.size(), payload.begin());
    return deserializeV9State(payload, state);
}

}  // namespace clockfw::services
