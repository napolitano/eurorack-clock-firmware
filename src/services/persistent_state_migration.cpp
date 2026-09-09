/**
 * @file persistent_state_migration.cpp
 * @brief Backward-compatible v3/v4/v5 to v6 persistence migration helpers.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_service.h"

#include <algorithm>

#include "defaults.h"

namespace clockfw::services {
namespace {

std::uint16_t readUint16LeMigration(const std::uint8_t* const source) {
    return static_cast<std::uint16_t>(source[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(source[1]) << 8U);
}

void writeUint16LeMigration(std::uint8_t* const destination, const std::uint16_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>(value >> 8U);
}

std::uint32_t readUint32LeMigration(const std::uint8_t* const source) {
    return static_cast<std::uint32_t>(source[0]) |
        (static_cast<std::uint32_t>(source[1]) << 8U) |
        (static_cast<std::uint32_t>(source[2]) << 16U) |
        (static_cast<std::uint32_t>(source[3]) << 24U);
}

bool isAllowedLegacyPresetCharacter(const char character) {
    return character == ' ' || character == '-' || character == '_' ||
        (character >= '0' && character <= '9') ||
        (character >= 'A' && character <= 'Z');
}

}  // namespace


bool PersistentStateService::deserializeV5State(
    const std::array<std::uint8_t, kV5StatePayloadSize>& payload,
    ClockState& state) {
    constexpr std::size_t kResetModeOffsetV6 = 14U;
    static_assert(kStatePayloadSize == kV5StatePayloadSize + 1U);

    std::array<std::uint8_t, kStatePayloadSize> upgraded{};
    std::copy_n(payload.begin(), kResetModeOffsetV6, upgraded.begin());
    upgraded[kResetModeOffsetV6] = static_cast<std::uint8_t>(defaults::kExternalResetMode);
    std::copy(
        payload.begin() + static_cast<std::ptrdiff_t>(kResetModeOffsetV6),
        payload.end(),
        upgraded.begin() + static_cast<std::ptrdiff_t>(kResetModeOffsetV6 + 1U));
    return deserializeState(upgraded, state);
}

bool PersistentStateService::deserializeV5CurrentRecord(
    const std::array<std::uint8_t, kV5CurrentRecordSize>& record,
    ClockState& state) {
    constexpr std::uint32_t kCurrentMagicValue = 0x35525543UL;  // "CUR5"
    if (readUint32LeMigration(record.data()) != kCurrentMagicValue ||
        record[4] != kV5SchemaVersion ||
        record[5] != 0U ||
        readUint16LeMigration(record.data() + 6U) != kV5StatePayloadSize ||
        readUint32LeMigration(record.data() + kV5CurrentRecordSize - 4U) !=
            calculateCrc32(record.data(), kV5CurrentRecordSize - 4U)) {
        return false;
    }

    constexpr std::size_t kPayloadOffset = 8U;
    std::array<std::uint8_t, kV5StatePayloadSize> payload{};
    std::copy_n(record.begin() + static_cast<std::ptrdiff_t>(kPayloadOffset), payload.size(), payload.begin());
    return deserializeV5State(payload, state);
}

bool PersistentStateService::deserializeV5PresetRecord(
    const std::array<std::uint8_t, kV5PresetRecordSize>& record,
    char* const name,
    ClockState& state) {
    constexpr std::uint32_t kPresetMagicValue = 0x35455250UL;  // "PRE5"
    constexpr std::size_t kNameOffset = 8U;
    constexpr std::size_t kPayloadOffset = kNameOffset + kPresetNameLength;
    if (readUint32LeMigration(record.data()) != kPresetMagicValue ||
        record[4] != kV5SchemaVersion ||
        record[5] != 1U ||
        readUint16LeMigration(record.data() + 6U) != kV5StatePayloadSize ||
        readUint32LeMigration(record.data() + kV5PresetRecordSize - 4U) !=
            calculateCrc32(record.data(), kV5PresetRecordSize - 4U)) {
        return false;
    }
    for (std::size_t index = 0U; index < kPresetNameLength; ++index) {
        const char character = static_cast<char>(record[kNameOffset + index]);
        if (!isAllowedLegacyPresetCharacter(character)) {
            return false;
        }
        name[index] = character;
    }
    name[kPresetNameLength] = '\0';

    std::array<std::uint8_t, kV5StatePayloadSize> payload{};
    std::copy_n(record.begin() + static_cast<std::ptrdiff_t>(kPayloadOffset), payload.size(), payload.begin());
    return deserializeV5State(payload, state);
}

bool PersistentStateService::deserializePreviousState(
    const std::array<std::uint8_t, kPreviousStatePayloadSize>& payload,
    ClockState& state) {
    constexpr std::size_t kTempoRangeOffset = 2U;
    constexpr std::size_t kAfterUnifiedPhaseOffsetV4 = 22U;
    constexpr std::size_t kAfterTempoRangeOffsetV5 = 6U;
    constexpr std::size_t kHumanizeOffsetV5 = 26U;
    constexpr std::size_t kAfterHumanizeOffsetV5 = 28U;
    static_assert(kV5StatePayloadSize == kPreviousStatePayloadSize + 6U);

    std::array<std::uint8_t, kV5StatePayloadSize> upgraded{};
    std::copy_n(payload.begin(), kTempoRangeOffset, upgraded.begin());

    const std::uint16_t legacyBpm = readUint16LeMigration(payload.data());
    const std::uint16_t minimumBpm = std::min(defaults::kMinimumBpm, legacyBpm);
    const std::uint16_t maximumBpm = std::max(defaults::kMaximumBpm, legacyBpm);
    writeUint16LeMigration(upgraded.data() + kTempoRangeOffset, minimumBpm);
    writeUint16LeMigration(upgraded.data() + kTempoRangeOffset + 2U, maximumBpm);

    std::copy(
        payload.begin() + static_cast<std::ptrdiff_t>(kTempoRangeOffset),
        payload.begin() + static_cast<std::ptrdiff_t>(kAfterUnifiedPhaseOffsetV4),
        upgraded.begin() + static_cast<std::ptrdiff_t>(kAfterTempoRangeOffsetV5));
    writeUint16LeMigration(upgraded.data() + kHumanizeOffsetV5, defaults::kUnifiedClockHumanizeUs);
    std::copy(
        payload.begin() + static_cast<std::ptrdiff_t>(kAfterUnifiedPhaseOffsetV4),
        payload.end(),
        upgraded.begin() + static_cast<std::ptrdiff_t>(kAfterHumanizeOffsetV5));
    return deserializeV5State(upgraded, state);
}

bool PersistentStateService::deserializePreviousCurrentRecord(
    const std::array<std::uint8_t, kPreviousCurrentRecordSize>& record,
    ClockState& state) {
    constexpr std::uint32_t kCurrentMagicValue = 0x34525543UL;  // "CUR4"
    if (readUint32LeMigration(record.data()) != kCurrentMagicValue ||
        record[4] != kPreviousSchemaVersion ||
        record[5] != 0U ||
        readUint16LeMigration(record.data() + 6U) != kPreviousStatePayloadSize ||
        readUint32LeMigration(record.data() + kPreviousCurrentRecordSize - 4U) !=
            calculateCrc32(record.data(), kPreviousCurrentRecordSize - 4U)) {
        return false;
    }

    constexpr std::size_t kCurrentPayloadOffsetValue = 8U;
    std::array<std::uint8_t, kPreviousStatePayloadSize> payload{};
    std::copy_n(
        record.begin() + static_cast<std::ptrdiff_t>(kCurrentPayloadOffsetValue),
        payload.size(),
        payload.begin());
    return deserializePreviousState(payload, state);
}

bool PersistentStateService::deserializePreviousPresetRecord(
    const std::array<std::uint8_t, kPreviousPresetRecordSize>& record,
    char* const name,
    ClockState& state) {
    constexpr std::uint32_t kPresetMagicValue = 0x34455250UL;  // "PRE4"
    constexpr std::size_t kPresetNameOffsetValue = 8U;
    constexpr std::size_t kPresetPayloadOffsetValue =
        kPresetNameOffsetValue + kPresetNameLength;
    if (readUint32LeMigration(record.data()) != kPresetMagicValue ||
        record[4] != kPreviousSchemaVersion ||
        record[5] != 1U ||
        readUint16LeMigration(record.data() + 6U) != kPreviousStatePayloadSize ||
        readUint32LeMigration(record.data() + kPreviousPresetRecordSize - 4U) !=
            calculateCrc32(record.data(), kPreviousPresetRecordSize - 4U)) {
        return false;
    }

    for (std::size_t index = 0U; index < kPresetNameLength; ++index) {
        const char character = static_cast<char>(record[kPresetNameOffsetValue + index]);
        if (!isAllowedLegacyPresetCharacter(character)) {
            return false;
        }
        name[index] = character;
    }
    name[kPresetNameLength] = '\0';

    std::array<std::uint8_t, kPreviousStatePayloadSize> payload{};
    std::copy_n(
        record.begin() + static_cast<std::ptrdiff_t>(kPresetPayloadOffsetValue),
        payload.size(),
        payload.begin());
    return deserializePreviousState(payload, state);
}

bool PersistentStateService::deserializeLegacyState(
    const std::array<std::uint8_t, kLegacyStatePayloadSize>& payload,
    ClockState& state) {
    constexpr std::size_t kDisplayPreferencesOffset = 25U;
    static_assert(kPreviousStatePayloadSize == kLegacyStatePayloadSize + 4U);

    std::array<std::uint8_t, kPreviousStatePayloadSize> upgradedV4{};
    std::copy_n(payload.begin(), kDisplayPreferencesOffset, upgradedV4.begin());
    upgradedV4[kDisplayPreferencesOffset] =
        static_cast<std::uint8_t>(defaults::kScreensaverMode);
    upgradedV4[kDisplayPreferencesOffset + 1U] = defaults::kScreensaverAfterMinutes;
    upgradedV4[kDisplayPreferencesOffset + 2U] = defaults::kScreensaverDimAfterMinutes;
    upgradedV4[kDisplayPreferencesOffset + 3U] = defaults::kScreensaverOffAfterMinutes;
    std::copy(
        payload.begin() + static_cast<std::ptrdiff_t>(kDisplayPreferencesOffset),
        payload.end(),
        upgradedV4.begin() + static_cast<std::ptrdiff_t>(kDisplayPreferencesOffset + 4U));
    return deserializePreviousState(upgradedV4, state);
}

bool PersistentStateService::deserializeLegacyCurrentRecord(
    const std::array<std::uint8_t, kLegacyCurrentRecordSize>& record,
    ClockState& state) {
    constexpr std::uint32_t kCurrentMagicValue = 0x33525543UL;  // "CUR3"
    if (readUint32LeMigration(record.data()) != kCurrentMagicValue ||
        record[4] != kLegacySchemaVersion ||
        record[5] != 0U ||
        readUint16LeMigration(record.data() + 6U) != kLegacyStatePayloadSize ||
        readUint32LeMigration(record.data() + kLegacyCurrentRecordSize - 4U) !=
            calculateCrc32(record.data(), kLegacyCurrentRecordSize - 4U)) {
        return false;
    }

    constexpr std::size_t kCurrentPayloadOffsetValue = 8U;
    std::array<std::uint8_t, kLegacyStatePayloadSize> payload{};
    std::copy_n(
        record.begin() + static_cast<std::ptrdiff_t>(kCurrentPayloadOffsetValue),
        payload.size(),
        payload.begin());
    return deserializeLegacyState(payload, state);
}

bool PersistentStateService::deserializeLegacyPresetRecord(
    const std::array<std::uint8_t, kLegacyPresetRecordSize>& record,
    char* const name,
    ClockState& state) {
    constexpr std::uint32_t kPresetMagicValue = 0x33455250UL;  // "PRE3"
    constexpr std::size_t kPresetNameOffsetValue = 8U;
    constexpr std::size_t kPresetPayloadOffsetValue =
        kPresetNameOffsetValue + kPresetNameLength;
    if (readUint32LeMigration(record.data()) != kPresetMagicValue ||
        record[4] != kLegacySchemaVersion ||
        record[5] != 1U ||
        readUint16LeMigration(record.data() + 6U) != kLegacyStatePayloadSize ||
        readUint32LeMigration(record.data() + kLegacyPresetRecordSize - 4U) !=
            calculateCrc32(record.data(), kLegacyPresetRecordSize - 4U)) {
        return false;
    }

    for (std::size_t index = 0U; index < kPresetNameLength; ++index) {
        const char character = static_cast<char>(record[kPresetNameOffsetValue + index]);
        if (!isAllowedLegacyPresetCharacter(character)) {
            return false;
        }
        name[index] = character;
    }
    name[kPresetNameLength] = '\0';

    std::array<std::uint8_t, kLegacyStatePayloadSize> payload{};
    std::copy_n(
        record.begin() + static_cast<std::ptrdiff_t>(kPresetPayloadOffsetValue),
        payload.size(),
        payload.begin());
    return deserializeLegacyState(payload, state);
}

}  // namespace clockfw::services
