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
namespace {

/**
 * @brief Reads one CURRENT record shape and decodes it into the current in-memory model.
 * @param storage Persistent byte store containing the record.
 * @param offset First byte of the record in the logical image.
 * @param decoder Schema-specific decoder and migration function.
 * @param state Destination state receiving the decoded record.
 * @return True only when both the bounded read and schema validation succeed.
 *
 * Keeping the record-size-dependent buffer here makes the migration order in begin()
 * readable without hiding any schema-specific validation inside PersistentStorage.
 */
template <std::size_t RecordSize>
bool loadCurrentRecord(
    hal::PersistentStorage& storage,
    const std::size_t offset,
    bool (*decoder)(const std::array<std::uint8_t, RecordSize>&, ClockState&),
    ClockState& state) {
    std::array<std::uint8_t, RecordSize> record{};
    return storage.readBytes(offset, record.data(), record.size()) && decoder(record, state);
}

/**
 * @brief Reads one named preset record shape and decodes it into the current model.
 * @param storage Persistent byte store containing the record.
 * @param offset First byte of the record in the logical image.
 * @param decoder Schema-specific preset decoder and migration function.
 * @param name Destination buffer for the normalized preset name.
 * @param state Destination state receiving the decoded record.
 * @return True only when both the bounded read and schema validation succeed.
 */
template <std::size_t RecordSize>
bool loadPresetRecord(
    hal::PersistentStorage& storage,
    const std::size_t offset,
    bool (*decoder)(const std::array<std::uint8_t, RecordSize>&, char*, ClockState&),
    char* name,
    ClockState& state) {
    std::array<std::uint8_t, RecordSize> record{};
    return storage.readBytes(offset, record.data(), record.size()) && decoder(record, name, state);
}

}  // namespace

void PersistentStateService::begin() {
    pendingPresetWrite_ = false;
    pendingPresetWillExist_ = false;
    pendingPresetName_.fill('\0');

    ClockState loadedCurrent{};
    hasStoredCurrentState_ = loadCurrentRecord(
        storage_, kCurrentRecordOffset, deserializeCurrentRecord, loadedCurrent);

    // Migration always probes newest-to-oldest. The first valid record wins, so a
    // newer schema can never be shadowed by stale bytes that happen to resemble an
    // older record layout.
    bool migrationNeeded = false;
    if (!hasStoredCurrentState_) {
        migrationNeeded =
            loadCurrentRecord(storage_, kCurrentRecordOffset, deserializeV11CurrentRecord, loadedCurrent) ||
            loadCurrentRecord(storage_, kCurrentRecordOffset, deserializeV10CurrentRecord, loadedCurrent) ||
            loadCurrentRecord(storage_, kCurrentRecordOffset, deserializeV9CurrentRecord, loadedCurrent) ||
            loadCurrentRecord(storage_, kCurrentRecordOffset, deserializeV8CurrentRecord, loadedCurrent) ||
            loadCurrentRecord(storage_, kCurrentRecordOffset, deserializeV7CurrentRecord, loadedCurrent) ||
            loadCurrentRecord(storage_, kCurrentRecordOffset, deserializeV6CurrentRecord, loadedCurrent) ||
            loadCurrentRecord(storage_, kCurrentRecordOffset, deserializeV5CurrentRecord, loadedCurrent) ||
            loadCurrentRecord(storage_, kCurrentRecordOffset, deserializePreviousCurrentRecord, loadedCurrent) ||
            loadCurrentRecord(storage_, kCurrentRecordOffset, deserializeLegacyCurrentRecord, loadedCurrent);
        hasStoredCurrentState_ = migrationNeeded;
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

    // Build the canonical v12 records area in PersistentStorage's bounded staging
    // image while old records are inspected. If no old schema is found, the staged
    // transaction is discarded. This avoids keeping eight migrated ClockState
    // objects or a complete 8-KiB persistence image on the embedded stack.
    constexpr std::size_t kRecordsEnd =
        kCurrentRecordSize + kUserPresetSlotCount * kPresetRecordSize;
    bool migrationStagingReady = storage_.beginUpdate() &&
        storage_.stageFill(0U, kRecordsEnd, static_cast<std::uint8_t>(0xFFU));
    if (migrationStagingReady && hasStoredCurrentState_) {
        const auto migratedRecord = serializeCurrentRecord(storedCurrentState_);
        migrationStagingReady = storage_.stageBytes(
            kCurrentRecordOffset, migratedRecord.data(), migratedRecord.size());
    }

    for (std::uint8_t slotIndex = 0U; slotIndex < kUserPresetSlotCount; ++slotIndex) {
        ClockState loadedPreset{};
        char name[kPresetNameLength + 1U]{};
        presetValid_[slotIndex] = loadPresetRecord(
            storage_, presetOffset(slotIndex), deserializePresetRecord, name, loadedPreset);

        bool loadedPriorSchema = false;
        if (!presetValid_[slotIndex]) {
            loadedPriorSchema =
                loadPresetRecord(storage_, v11PresetOffset(slotIndex), deserializeV11PresetRecord, name, loadedPreset) ||
                loadPresetRecord(storage_, v10PresetOffset(slotIndex), deserializeV10PresetRecord, name, loadedPreset) ||
                loadPresetRecord(storage_, v9PresetOffset(slotIndex), deserializeV9PresetRecord, name, loadedPreset) ||
                loadPresetRecord(storage_, v8PresetOffset(slotIndex), deserializeV8PresetRecord, name, loadedPreset) ||
                loadPresetRecord(storage_, v7PresetOffset(slotIndex), deserializeV7PresetRecord, name, loadedPreset) ||
                loadPresetRecord(storage_, v6PresetOffset(slotIndex), deserializeV6PresetRecord, name, loadedPreset) ||
                loadPresetRecord(storage_, v5PresetOffset(slotIndex), deserializeV5PresetRecord, name, loadedPreset) ||
                loadPresetRecord(storage_, previousPresetOffset(slotIndex), deserializePreviousPresetRecord, name, loadedPreset) ||
                loadPresetRecord(storage_, legacyPresetOffset(slotIndex), deserializeLegacyPresetRecord, name, loadedPreset);
            presetValid_[slotIndex] = loadedPriorSchema;
        }

        migrationNeeded = migrationNeeded || loadedPriorSchema;
        presetNames_[slotIndex].fill('\0');
        if (!presetValid_[slotIndex]) {
            continue;
        }

        std::copy_n(name, kPresetNameLength, presetNames_[slotIndex].begin());
        presetNames_[slotIndex][kPresetNameLength] = '\0';
        if (migrationStagingReady) {
            const auto migratedRecord = serializePresetRecord(name, loadedPreset);
            migrationStagingReady = storage_.stageBytes(
                presetOffset(slotIndex), migratedRecord.data(), migratedRecord.size());
        }
    }

    if (migrationNeeded && migrationStagingReady) {
        (void)storage_.commitUpdate();
    } else {
        storage_.cancelUpdate();
    }
}

}  // namespace clockfw::services
