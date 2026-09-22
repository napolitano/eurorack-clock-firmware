/**
 * @file persistent_state_service_begin.cpp
 * @brief Startup validation and migration orchestration for persistent CLOCK state.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_service.h"

#include <algorithm>

namespace clockfw::services {

void PersistentStateService::begin() {
    pendingPresetWrite_ = false;
    pendingPresetWillExist_ = false;
    pendingPresetName_.fill('\0');

    bool migrationNeeded = false;
    std::array<std::uint8_t, kCurrentRecordSize> currentRecord{};
    ClockState loadedCurrent{};
    hasStoredCurrentState_ = storage_.readBytes(
        kCurrentRecordOffset,
        currentRecord.data(),
        currentRecord.size()) && deserializeCurrentRecord(currentRecord, loadedCurrent);

    if (!hasStoredCurrentState_) {
        std::array<std::uint8_t, kV11CurrentRecordSize> v11Record{};
        if (storage_.readBytes(
                kCurrentRecordOffset,
                v11Record.data(),
                v11Record.size()) &&
            deserializeV11CurrentRecord(v11Record, loadedCurrent)) {
            hasStoredCurrentState_ = true;
            migrationNeeded = true;
        }
    }

    if (!hasStoredCurrentState_) {
        std::array<std::uint8_t, kV10CurrentRecordSize> v10Record{};
        if (storage_.readBytes(
                kCurrentRecordOffset,
                v10Record.data(),
                v10Record.size()) &&
            deserializeV10CurrentRecord(v10Record, loadedCurrent)) {
            hasStoredCurrentState_ = true;
            migrationNeeded = true;
        }
    }

    if (!hasStoredCurrentState_) {
        std::array<std::uint8_t, kV9CurrentRecordSize> v9Record{};
        if (storage_.readBytes(
                kCurrentRecordOffset,
                v9Record.data(),
                v9Record.size()) &&
            deserializeV9CurrentRecord(v9Record, loadedCurrent)) {
            hasStoredCurrentState_ = true;
            migrationNeeded = true;
        }
    }

    if (!hasStoredCurrentState_) {
        std::array<std::uint8_t, kV8CurrentRecordSize> v8Record{};
        if (storage_.readBytes(
                kCurrentRecordOffset,
                v8Record.data(),
                v8Record.size()) &&
            deserializeV8CurrentRecord(v8Record, loadedCurrent)) {
            hasStoredCurrentState_ = true;
            migrationNeeded = true;
        }
    }

    if (!hasStoredCurrentState_) {
        std::array<std::uint8_t, kV7CurrentRecordSize> v7Record{};
        if (storage_.readBytes(
                kCurrentRecordOffset,
                v7Record.data(),
                v7Record.size()) &&
            deserializeV7CurrentRecord(v7Record, loadedCurrent)) {
            hasStoredCurrentState_ = true;
            migrationNeeded = true;
        }
    }

    if (!hasStoredCurrentState_) {
        std::array<std::uint8_t, kV6CurrentRecordSize> v6Record{};
        if (storage_.readBytes(
                kCurrentRecordOffset,
                v6Record.data(),
                v6Record.size()) &&
            deserializeV6CurrentRecord(v6Record, loadedCurrent)) {
            hasStoredCurrentState_ = true;
            migrationNeeded = true;
        }
    }

    if (!hasStoredCurrentState_) {
        std::array<std::uint8_t, kV5CurrentRecordSize> v5Record{};
        if (storage_.readBytes(
                kCurrentRecordOffset,
                v5Record.data(),
                v5Record.size()) &&
            deserializeV5CurrentRecord(v5Record, loadedCurrent)) {
            hasStoredCurrentState_ = true;
            migrationNeeded = true;
        }
    }

    if (!hasStoredCurrentState_) {
        std::array<std::uint8_t, kPreviousCurrentRecordSize> previousRecord{};
        if (storage_.readBytes(
                kCurrentRecordOffset,
                previousRecord.data(),
                previousRecord.size()) &&
            deserializePreviousCurrentRecord(previousRecord, loadedCurrent)) {
            hasStoredCurrentState_ = true;
            migrationNeeded = true;
        }
    }

    if (!hasStoredCurrentState_) {
        std::array<std::uint8_t, kLegacyCurrentRecordSize> legacyRecord{};
        if (storage_.readBytes(
                kCurrentRecordOffset,
                legacyRecord.data(),
                legacyRecord.size()) &&
            deserializeLegacyCurrentRecord(legacyRecord, loadedCurrent)) {
            hasStoredCurrentState_ = true;
            migrationNeeded = true;
        }
    }

    if (hasStoredCurrentState_) {
        storedCurrentState_ = loadedCurrent;
        pendingCurrentState_ = loadedCurrent;
        persistedTransportPreference_ = loadedCurrent.transport;
    } else {
        // GCC 12.3 for ARM can ICE while lowering assignment from an anonymous
        // aggregate temporary (ClockState{}). A named default-initialized object
        // produces identical C++ semantics without exercising that compiler path.
        const ClockState defaultState{};
        storedCurrentState_ = defaultState;
        pendingCurrentState_ = defaultState;
        persistedTransportPreference_ = TransportState::Stopped;
    }
    writePending_ = false;

    // Build a canonical v12 records area in PersistentStorage's bounded staging
    // buffer while records are being inspected. If no old schema is found the
    // transaction is simply discarded. This avoids keeping migrated copies of all
    // eight ClockState objects or an 8-KiB storage image on the call stack.
    constexpr std::size_t kRecordsEnd =
        kCurrentRecordSize + kUserPresetSlotCount * kPresetRecordSize;
    bool migrationStagingReady = storage_.beginUpdate() &&
        storage_.stageFill(0U, kRecordsEnd, static_cast<std::uint8_t>(0xFFU));
    if (migrationStagingReady && hasStoredCurrentState_) {
        const auto migrated = serializeCurrentRecord(storedCurrentState_);
        migrationStagingReady = storage_.stageBytes(
            kCurrentRecordOffset, migrated.data(), migrated.size());
    }

    for (std::uint8_t slotIndex = 0U; slotIndex < kUserPresetSlotCount; ++slotIndex) {
        std::array<std::uint8_t, kPresetRecordSize> record{};
        ClockState loadedPreset{};
        char name[kPresetNameLength + 1U]{};
        bool loadedPriorSchema = false;
        presetValid_[slotIndex] = storage_.readBytes(
            presetOffset(slotIndex),
            record.data(),
            record.size()) && deserializePresetRecord(record, name, loadedPreset);

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kV11PresetRecordSize> v11Record{};
            presetValid_[slotIndex] = storage_.readBytes(
                kV11CurrentRecordSize + static_cast<std::size_t>(slotIndex) * kV11PresetRecordSize,
                v11Record.data(),
                v11Record.size()) &&
                deserializeV11PresetRecord(v11Record, name, loadedPreset);
            loadedPriorSchema = presetValid_[slotIndex];
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kV10PresetRecordSize> v10Record{};
            presetValid_[slotIndex] = storage_.readBytes(
                v10PresetOffset(slotIndex),
                v10Record.data(),
                v10Record.size()) &&
                deserializeV10PresetRecord(v10Record, name, loadedPreset);
            loadedPriorSchema = presetValid_[slotIndex];
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kV9PresetRecordSize> v9Record{};
            presetValid_[slotIndex] = storage_.readBytes(
                kV9CurrentRecordSize + static_cast<std::size_t>(slotIndex) * kV9PresetRecordSize,
                v9Record.data(),
                v9Record.size()) &&
                deserializeV9PresetRecord(v9Record, name, loadedPreset);
            loadedPriorSchema = presetValid_[slotIndex];
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kV8PresetRecordSize> v8Record{};
            presetValid_[slotIndex] = storage_.readBytes(
                v8PresetOffset(slotIndex),
                v8Record.data(),
                v8Record.size()) &&
                deserializeV8PresetRecord(v8Record, name, loadedPreset);
            loadedPriorSchema = presetValid_[slotIndex];
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kV7PresetRecordSize> v7Record{};
            presetValid_[slotIndex] = storage_.readBytes(
                v7PresetOffset(slotIndex),
                v7Record.data(),
                v7Record.size()) &&
                deserializeV7PresetRecord(v7Record, name, loadedPreset);
            loadedPriorSchema = presetValid_[slotIndex];
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kV6PresetRecordSize> v6Record{};
            presetValid_[slotIndex] = storage_.readBytes(
                v6PresetOffset(slotIndex),
                v6Record.data(),
                v6Record.size()) &&
                deserializeV6PresetRecord(v6Record, name, loadedPreset);
            loadedPriorSchema = presetValid_[slotIndex];
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kV5PresetRecordSize> v5Record{};
            presetValid_[slotIndex] = storage_.readBytes(
                v5PresetOffset(slotIndex),
                v5Record.data(),
                v5Record.size()) &&
                deserializeV5PresetRecord(v5Record, name, loadedPreset);
            loadedPriorSchema = presetValid_[slotIndex];
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kPreviousPresetRecordSize> previousRecord{};
            presetValid_[slotIndex] = storage_.readBytes(
                previousPresetOffset(slotIndex),
                previousRecord.data(),
                previousRecord.size()) &&
                deserializePreviousPresetRecord(previousRecord, name, loadedPreset);
            loadedPriorSchema = presetValid_[slotIndex];
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kLegacyPresetRecordSize> legacyRecord{};
            presetValid_[slotIndex] = storage_.readBytes(
                legacyPresetOffset(slotIndex),
                legacyRecord.data(),
                legacyRecord.size()) &&
                deserializeLegacyPresetRecord(legacyRecord, name, loadedPreset);
            loadedPriorSchema = presetValid_[slotIndex];
        }

        migrationNeeded = migrationNeeded || loadedPriorSchema;
        presetNames_[slotIndex].fill('\0');
        if (!presetValid_[slotIndex]) {
            continue;
        }

        std::copy_n(name, kPresetNameLength, presetNames_[slotIndex].begin());
        presetNames_[slotIndex][kPresetNameLength] = '\0';
        if (migrationStagingReady) {
            const auto migrated = serializePresetRecord(name, loadedPreset);
            migrationStagingReady = storage_.stageBytes(
                presetOffset(slotIndex), migrated.data(), migrated.size());
        }
    }

    if (migrationNeeded && migrationStagingReady) {
        (void)storage_.commitUpdate();
    } else {
        storage_.cancelUpdate();
    }
}


}  // namespace clockfw::services
