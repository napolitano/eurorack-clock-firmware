/**
 * @file persistent_state_service.cpp
 * @brief Durable CURRENT-state and named-preset storage orchestration.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_service.h"

#include <algorithm>
#include <cstring>

#include "config.h"

namespace clockfw::services {

PersistentStateService::PersistentStateService(hal::PersistentStorage& storage)
    : storage_(storage) {}

void PersistentStateService::begin() {
    pendingPresetWrite_ = false;
    pendingPresetWillExist_ = false;
    pendingPresetName_.fill('\0');

    bool migrationNeeded = false;
    std::array<bool, kUserPresetSlotCount> migratedPresetLoaded{};
    std::array<ClockState, kUserPresetSlotCount> migratedPresetStates{};
    std::array<std::array<char, kPresetNameLength + 1U>, kUserPresetSlotCount>
        migratedPresetNames{};

    std::array<std::uint8_t, kCurrentRecordSize> currentRecord{};
    ClockState loadedCurrent{};
    hasStoredCurrentState_ = storage_.readBytes(
        kCurrentRecordOffset,
        currentRecord.data(),
        currentRecord.size()) && deserializeCurrentRecord(currentRecord, loadedCurrent);

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

    for (std::uint8_t slotIndex = 0U; slotIndex < kUserPresetSlotCount; ++slotIndex) {
        std::array<std::uint8_t, kPresetRecordSize> record{};
        ClockState loadedPreset{};
        char name[kPresetNameLength + 1U]{};
        presetValid_[slotIndex] = storage_.readBytes(
            presetOffset(slotIndex),
            record.data(),
            record.size()) && deserializePresetRecord(record, name, loadedPreset);

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kV5PresetRecordSize> v5Record{};
            presetValid_[slotIndex] = storage_.readBytes(
                v5PresetOffset(slotIndex),
                v5Record.data(),
                v5Record.size()) &&
                deserializeV5PresetRecord(v5Record, name, loadedPreset);
            if (presetValid_[slotIndex]) {
                migrationNeeded = true;
                migratedPresetLoaded[slotIndex] = true;
            }
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kPreviousPresetRecordSize> previousRecord{};
            presetValid_[slotIndex] = storage_.readBytes(
                previousPresetOffset(slotIndex),
                previousRecord.data(),
                previousRecord.size()) &&
                deserializePreviousPresetRecord(previousRecord, name, loadedPreset);
            if (presetValid_[slotIndex]) {
                migrationNeeded = true;
                migratedPresetLoaded[slotIndex] = true;
            }
        }

        if (!presetValid_[slotIndex]) {
            std::array<std::uint8_t, kLegacyPresetRecordSize> legacyRecord{};
            presetValid_[slotIndex] = storage_.readBytes(
                legacyPresetOffset(slotIndex),
                legacyRecord.data(),
                legacyRecord.size()) &&
                deserializeLegacyPresetRecord(legacyRecord, name, loadedPreset);
            if (presetValid_[slotIndex]) {
                migrationNeeded = true;
                migratedPresetLoaded[slotIndex] = true;
            }
        }

        presetNames_[slotIndex].fill('\0');
        if (presetValid_[slotIndex]) {
            std::copy_n(name, kPresetNameLength, presetNames_[slotIndex].begin());
            presetNames_[slotIndex][kPresetNameLength] = '\0';
            migratedPresetLoaded[slotIndex] = true;
            migratedPresetStates[slotIndex] = loadedPreset;
            std::copy_n(name, kPresetNameLength + 1U, migratedPresetNames[slotIndex].begin());
        }
    }

    if (!migrationNeeded) {
        return;
    }

    // Rewrite every recoverable prior-schema record at v6 offsets in one logical
    // storage-image commit. Bytes outside CURRENT/presets (notably Pixel Raid score)
    // remain untouched.
    std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes> storageImage{};
    if (!storage_.readBytes(0U, storageImage.data(), storageImage.size())) {
        return;
    }
    constexpr std::size_t kRecordsEnd =
        kCurrentRecordSize + kUserPresetSlotCount * kPresetRecordSize;
    std::fill_n(storageImage.begin(), kRecordsEnd, 0xFFU);
    if (hasStoredCurrentState_) {
        const auto migrated = serializeCurrentRecord(storedCurrentState_);
        std::copy(migrated.begin(), migrated.end(), storageImage.begin());
    }
    for (std::uint8_t slotIndex = 0U; slotIndex < kUserPresetSlotCount; ++slotIndex) {
        if (!migratedPresetLoaded[slotIndex]) {
            continue;
        }
        const auto migrated = serializePresetRecord(
            migratedPresetNames[slotIndex].data(), migratedPresetStates[slotIndex]);
        std::copy(
            migrated.begin(),
            migrated.end(),
            storageImage.begin() + static_cast<std::ptrdiff_t>(presetOffset(slotIndex)));
    }
    (void)storage_.writeBytes(0U, storageImage.data(), storageImage.size());
}

bool PersistentStateService::restoreCurrentState(ClockState& state) const {
    if (!hasStoredCurrentState_) {
        return false;
    }
    state = storedCurrentState_;
    return true;
}

void PersistentStateService::requestCurrentState(
    const ClockState& state,
    const std::uint32_t nowMs) {
    ClockState durableState = state;
    durableState.transport = persistedTransportPreference_;

    if (hasStoredCurrentState_ && statesEqual(durableState, storedCurrentState_)) {
        pendingCurrentState_ = durableState;
        writePending_ = false;
        return;
    }

    if (!writePending_ || !statesEqual(durableState, pendingCurrentState_)) {
        pendingCurrentState_ = durableState;
        pendingSinceMs_ = nowMs;
        writePending_ = true;
    }
}

void PersistentStateService::requestTransportState(
    const TransportState transport,
    const std::uint32_t nowMs) {
    const bool preferenceChanged = transport != persistedTransportPreference_;
    persistedTransportPreference_ = transport;
    pendingCurrentState_.transport = transport;

    if (hasStoredCurrentState_ && transport == storedCurrentState_.transport &&
        statesEqual(pendingCurrentState_, storedCurrentState_)) {
        writePending_ = false;
        return;
    }

    // Repeating an identical pending transport command must not postpone the
    // wear-coalescing deadline indefinitely. Restart only when the value changed.
    if (!writePending_ || preferenceChanged) {
        pendingSinceMs_ = nowMs;
    }
    writePending_ = true;
}

void PersistentStateService::service(
    const std::uint32_t nowMs,
    const bool allowFlashCommit) {
    if (!allowFlashCommit) {
        return;
    }

    bool currentRecordReady = writePending_ &&
        nowMs - pendingSinceMs_ >= config::kPersistenceCommitDelayMs;
    if (pendingPresetWrite_) {
        // An explicit user save is already a synchronization point. Include a
        // pending CURRENT record in the same whole-sector Flash commit rather
        // than causing a second erase/program cycle a few seconds later.
        currentRecordReady = writePending_;
    }
    if (!currentRecordReady && !pendingPresetWrite_) {
        return;
    }

    std::array<std::uint8_t, hal::PersistentStorage::kCapacityBytes> storageImage{};
    if (!storage_.readBytes(0U, storageImage.data(), storageImage.size())) {
        return;
    }

    if (currentRecordReady) {
        const std::array<std::uint8_t, kCurrentRecordSize> currentRecord =
            serializeCurrentRecord(pendingCurrentState_);
        std::copy(
            currentRecord.begin(),
            currentRecord.end(),
            storageImage.begin() + static_cast<std::ptrdiff_t>(kCurrentRecordOffset));
    }
    if (pendingPresetWrite_) {
        std::copy(
            pendingPresetRecord_.begin(),
            pendingPresetRecord_.end(),
            storageImage.begin() + static_cast<std::ptrdiff_t>(presetOffset(pendingPresetSlot_)));
    }

    if (!storage_.writeBytes(0U, storageImage.data(), storageImage.size())) {
        return;
    }

    if (currentRecordReady) {
        storedCurrentState_ = pendingCurrentState_;
        persistedTransportPreference_ = storedCurrentState_.transport;
        hasStoredCurrentState_ = true;
        writePending_ = false;
    }
    if (pendingPresetWrite_) {
        presetValid_[pendingPresetSlot_] = pendingPresetWillExist_;
        presetNames_[pendingPresetSlot_].fill('\0');
        if (pendingPresetWillExist_) {
            std::copy_n(
                pendingPresetName_.begin(),
                kPresetNameLength,
                presetNames_[pendingPresetSlot_].begin());
        }
        pendingPresetWrite_ = false;
    }
}

bool PersistentStateService::hasStoredCurrentState() const {
    return hasStoredCurrentState_;
}

TransportState PersistentStateService::storedTransportState() const {
    return hasStoredCurrentState_ ? storedCurrentState_.transport : TransportState::Stopped;
}

bool PersistentStateService::hasStoredTransportState() const {
    return hasStoredCurrentState();
}

bool PersistentStateService::savePreset(
    const std::uint8_t slotIndex,
    const char* const name,
    const ClockState& state) {
    if (slotIndex >= kUserPresetSlotCount || name == nullptr || !isStateValid(state)) {
        return false;
    }

    pendingPresetRecord_ = serializePresetRecord(name, state);
    pendingPresetSlot_ = slotIndex;
    pendingPresetWillExist_ = true;
    pendingPresetWrite_ = true;

    char normalizedName[kPresetNameLength + 1U]{};
    normalizePresetName(name, normalizedName);
    pendingPresetName_.fill('\0');
    std::copy_n(normalizedName, kPresetNameLength, pendingPresetName_.begin());
    return true;
}

bool PersistentStateService::loadPreset(
    const std::uint8_t slotIndex,
    ClockState& state) const {
    if (slotIndex >= kUserPresetSlotCount || !presetExists(slotIndex)) {
        return false;
    }

    std::array<std::uint8_t, kPresetRecordSize> record{};
    if (pendingPresetWrite_ && pendingPresetSlot_ == slotIndex) {
        record = pendingPresetRecord_;
    } else if (!storage_.readBytes(presetOffset(slotIndex), record.data(), record.size())) {
        return false;
    }
    ClockState loaded{};
    char ignoredName[kPresetNameLength + 1U]{};
    if (!deserializePresetRecord(record, ignoredName, loaded)) {
        return false;
    }

    // Loading a configuration must never cause an implicit transport transition.
    const TransportState liveTransport = state.transport;
    state = loaded;
    state.transport = liveTransport;
    return true;
}

bool PersistentStateService::renamePreset(
    const std::uint8_t slotIndex,
    const char* const name) {
    if (slotIndex >= kUserPresetSlotCount || !presetExists(slotIndex) || name == nullptr) {
        return false;
    }

    std::array<std::uint8_t, kPresetRecordSize> record{};
    if (pendingPresetWrite_ && pendingPresetSlot_ == slotIndex) {
        record = pendingPresetRecord_;
    } else if (!storage_.readBytes(presetOffset(slotIndex), record.data(), record.size())) {
        return false;
    }

    ClockState state{};
    char ignoredName[kPresetNameLength + 1U]{};
    if (!deserializePresetRecord(record, ignoredName, state)) {
        return false;
    }
    return savePreset(slotIndex, name, state);
}

bool PersistentStateService::clearPreset(const std::uint8_t slotIndex) {
    if (slotIndex >= kUserPresetSlotCount) {
        return false;
    }

    pendingPresetRecord_.fill(0xFFU);
    pendingPresetName_.fill('\0');
    pendingPresetSlot_ = slotIndex;
    pendingPresetWillExist_ = false;
    pendingPresetWrite_ = true;
    return true;
}

bool PersistentStateService::presetExists(const std::uint8_t slotIndex) const {
    if (slotIndex >= kUserPresetSlotCount) {
        return false;
    }
    if (pendingPresetWrite_ && pendingPresetSlot_ == slotIndex) {
        return pendingPresetWillExist_;
    }
    return presetValid_[slotIndex];
}

void PersistentStateService::presetName(
    const std::uint8_t slotIndex,
    char* const destination,
    const std::size_t destinationSize) const {
    if (destination == nullptr || destinationSize == 0U) {
        return;
    }
    destination[0] = '\0';
    if (!presetExists(slotIndex)) {
        return;
    }

    if (pendingPresetWrite_ && pendingPresetSlot_ == slotIndex) {
        const std::size_t copyLength = std::min(kPresetNameLength, destinationSize - 1U);
        std::copy_n(pendingPresetName_.data(), copyLength, destination);
        destination[copyLength] = '\0';
        std::size_t end = std::strlen(destination);
        while (end > 0U && destination[end - 1U] == ' ') {
            destination[--end] = '\0';
        }
        return;
    }

    const std::size_t copyLength = std::min(kPresetNameLength, destinationSize - 1U);
    std::copy_n(presetNames_[slotIndex].data(), copyLength, destination);
    destination[copyLength] = '\0';

    // Remove right-padding spaces for a compact OLED list representation.
    std::size_t end = std::strlen(destination);
    while (end > 0U && destination[end - 1U] == ' ') {
        destination[--end] = '\0';
    }
}


}  // namespace clockfw::services
