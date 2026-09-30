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

PersistentStateService::PersistentStateService(hal::PersistentStorage& storage) : storage_(storage) {}

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
    bool anyPendingPreset = false;
    for (const bool pending : pendingPresetWrites_) {
        anyPendingPreset = anyPendingPreset || pending;
    }
    if (anyPendingPreset) {
        // An explicit user save is already a synchronization point. Include a
        // pending CURRENT record in the same whole-sector Flash commit rather
        // than causing a second erase/program cycle a few seconds later.
        currentRecordReady = writePending_;
    }
    if (!currentRecordReady && !anyPendingPreset) {
        return;
    }

    if (!storage_.beginUpdate()) {
        return;
    }

    bool staged = true;
    if (currentRecordReady) {
        const std::array<std::uint8_t, kCurrentRecordSize> currentRecord =
            serializeCurrentRecord(pendingCurrentState_);
        staged = storage_.stageBytes(
            kCurrentRecordOffset, currentRecord.data(), currentRecord.size());
    }
    if (staged) {
        for (std::uint8_t slotIndex = 0U; slotIndex < kUserPresetSlotCount; ++slotIndex) {
            if (!pendingPresetWrites_[slotIndex]) {
                continue;
            }
            staged = storage_.stageBytes(
                presetOffset(slotIndex),
                pendingPresetRecords_[slotIndex].data(),
                pendingPresetRecords_[slotIndex].size());
            if (!staged) {
                break;
            }
        }
    }

    if (!staged || !storage_.commitUpdate()) {
        storage_.cancelUpdate();
        return;
    }

    if (currentRecordReady) {
        storedCurrentState_ = pendingCurrentState_;
        persistedTransportPreference_ = storedCurrentState_.transport;
        hasStoredCurrentState_ = true;
        writePending_ = false;
    }
    for (std::uint8_t slotIndex = 0U; slotIndex < kUserPresetSlotCount; ++slotIndex) {
        if (!pendingPresetWrites_[slotIndex]) {
            continue;
        }
        presetValid_[slotIndex] = pendingPresetWillExist_[slotIndex];
        presetNames_[slotIndex].fill('\0');
        if (pendingPresetWillExist_[slotIndex]) {
            std::copy_n(
                pendingPresetNames_[slotIndex].begin(),
                kPresetNameLength,
                presetNames_[slotIndex].begin());
        }
        pendingPresetWrites_[slotIndex] = false;
    }
}


#ifdef CLOCK_SIMULATOR
void PersistentStateService::flushPendingForSimulator() {
    const std::uint32_t forcedNowMs =
        pendingSinceMs_ + static_cast<std::uint32_t>(config::kPersistenceCommitDelayMs);
    service(forcedNowMs, true);
}
#endif

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

    pendingPresetRecords_[slotIndex] = serializePresetRecord(name, state);
    pendingPresetWillExist_[slotIndex] = true;
    pendingPresetWrites_[slotIndex] = true;

    char normalizedName[kPresetNameLength + 1U]{};
    normalizePresetName(name, normalizedName);
    pendingPresetNames_[slotIndex].fill('\0');
    std::copy_n(normalizedName, kPresetNameLength, pendingPresetNames_[slotIndex].begin());
    return true;
}

bool PersistentStateService::loadPreset(
    const std::uint8_t slotIndex,
    ClockState& state) const {
    if (slotIndex >= kUserPresetSlotCount || !presetExists(slotIndex)) {
        return false;
    }

    std::array<std::uint8_t, kPresetRecordSize> record{};
    if (pendingPresetWrites_[slotIndex]) {
        record = pendingPresetRecords_[slotIndex];
    } else if (!storage_.readBytes(presetOffset(slotIndex), record.data(), record.size())) {
        return false;
    }
    ClockState loaded{};
    char ignoredName[kPresetNameLength + 1U]{};
    if (!deserializePresetRecord(record, ignoredName, loaded)) {
        return false;
    }

    // Loading a musical preset must never cause an implicit transport transition
    // or change device-local panel/display orientation preferences.
    const TransportState liveTransport = state.transport;
    const DevicePreferences liveDevicePreferences = state.device;
    state = loaded;
    state.transport = liveTransport;
    state.device = liveDevicePreferences;
    return true;
}

bool PersistentStateService::renamePreset(
    const std::uint8_t slotIndex,
    const char* const name) {
    if (slotIndex >= kUserPresetSlotCount || !presetExists(slotIndex) || name == nullptr) {
        return false;
    }

    std::array<std::uint8_t, kPresetRecordSize> record{};
    if (pendingPresetWrites_[slotIndex]) {
        record = pendingPresetRecords_[slotIndex];
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

    pendingPresetRecords_[slotIndex].fill(0xFFU);
    pendingPresetNames_[slotIndex].fill('\0');
    pendingPresetWillExist_[slotIndex] = false;
    pendingPresetWrites_[slotIndex] = true;
    return true;
}

bool PersistentStateService::presetExists(const std::uint8_t slotIndex) const {
    if (slotIndex >= kUserPresetSlotCount) {
        return false;
    }
    if (pendingPresetWrites_[slotIndex]) {
        return pendingPresetWillExist_[slotIndex];
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

    if (pendingPresetWrites_[slotIndex]) {
        const std::size_t copyLength = std::min(kPresetNameLength, destinationSize - 1U);
        std::copy_n(pendingPresetNames_[slotIndex].data(), copyLength, destination);
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
