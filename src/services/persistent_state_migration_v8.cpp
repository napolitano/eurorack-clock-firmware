/**
 * @file persistent_state_migration_v8.cpp
 * @brief Stable 1.0.x schema-v8 to schema-v9 persistence migration.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_service.h"

#include <algorithm>

#include "defaults.h"

namespace clockfw::services {
namespace {

std::uint16_t readUint16LeV8(const std::uint8_t* const source) {
    return static_cast<std::uint16_t>(source[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(source[1]) << 8U);
}

std::uint32_t readUint32LeV8(const std::uint8_t* const source) {
    return static_cast<std::uint32_t>(source[0]) |
        (static_cast<std::uint32_t>(source[1]) << 8U) |
        (static_cast<std::uint32_t>(source[2]) << 16U) |
        (static_cast<std::uint32_t>(source[3]) << 24U);
}

bool isAllowedV8PresetCharacter(const char character) {
    return character == ' ' || character == '-' || character == '_' ||
        (character >= '0' && character <= '9') ||
        (character >= 'A' && character <= 'Z');
}

}  // namespace

bool PersistentStateService::deserializeV8State(
    const std::array<std::uint8_t, kV8StatePayloadSize>& payload,
    ClockState& state) {
    static_assert(kV9StatePayloadSize == kV8StatePayloadSize + 1U);

    std::array<std::uint8_t, kV9StatePayloadSize> upgraded{};
    std::copy(payload.begin(), payload.end(), upgraded.begin());
    upgraded[kV8StatePayloadSize] = defaults::kPreCountSteps;
    return deserializeV9State(upgraded, state);
}

bool PersistentStateService::deserializeV8CurrentRecord(
    const std::array<std::uint8_t, kV8CurrentRecordSize>& record,
    ClockState& state) {
    constexpr std::uint32_t kCurrentMagicValue = 0x38525543UL;  // "CUR8"
    if (readUint32LeV8(record.data()) != kCurrentMagicValue ||
        record[4] != kV8SchemaVersion ||
        record[5] != 0U ||
        readUint16LeV8(record.data() + 6U) != kV8StatePayloadSize ||
        readUint32LeV8(record.data() + kV8CurrentRecordSize - 4U) !=
            calculateCrc32(record.data(), kV8CurrentRecordSize - 4U)) {
        return false;
    }

    constexpr std::size_t kPayloadOffset = 8U;
    std::array<std::uint8_t, kV8StatePayloadSize> payload{};
    std::copy_n(record.begin() + static_cast<std::ptrdiff_t>(kPayloadOffset), payload.size(), payload.begin());
    return deserializeV8State(payload, state);
}

bool PersistentStateService::deserializeV8PresetRecord(
    const std::array<std::uint8_t, kV8PresetRecordSize>& record,
    char* const name,
    ClockState& state) {
    constexpr std::uint32_t kPresetMagicValue = 0x38455250UL;  // "PRE8"
    constexpr std::size_t kNameOffset = 8U;
    constexpr std::size_t kPayloadOffset = kNameOffset + kPresetNameLength;
    if (readUint32LeV8(record.data()) != kPresetMagicValue ||
        record[4] != kV8SchemaVersion ||
        record[5] != 1U ||
        readUint16LeV8(record.data() + 6U) != kV8StatePayloadSize ||
        readUint32LeV8(record.data() + kV8PresetRecordSize - 4U) !=
            calculateCrc32(record.data(), kV8PresetRecordSize - 4U)) {
        return false;
    }
    for (std::size_t index = 0U; index < kPresetNameLength; ++index) {
        const char character = static_cast<char>(record[kNameOffset + index]);
        if (!isAllowedV8PresetCharacter(character)) {
            return false;
        }
        name[index] = character;
    }
    name[kPresetNameLength] = '\0';

    std::array<std::uint8_t, kV8StatePayloadSize> payload{};
    std::copy_n(record.begin() + static_cast<std::ptrdiff_t>(kPayloadOffset), payload.size(), payload.begin());
    return deserializeV8State(payload, state);
}


}  // namespace clockfw::services
