/**
 * @file persistent_state_factory_reset.cpp
 * @brief Atomic whole-image factory reset for CLOCK persistence.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_factory_reset.h"

namespace clockfw::services {
bool PersistentStateService::factoryReset(const ClockState& factoryState) {
    if (!isStateValid(factoryState) || !storage_.beginUpdate()) {
        return false;
    }

    const auto currentRecord = serializeCurrentRecord(factoryState);
    const bool staged =
        storage_.stageFill(0U, hal::PersistentStorage::kCapacityBytes, 0xFFU) &&
        storage_.stageBytes(kCurrentRecordOffset, currentRecord.data(), currentRecord.size());
    if (!staged || !storage_.commitUpdate()) {
        storage_.cancelUpdate();
        return false;
    }

    storedCurrentState_ = factoryState;
    pendingCurrentState_ = factoryState;
    persistedTransportPreference_ = factoryState.transport;
    hasStoredCurrentState_ = true;
    writePending_ = false;
    pendingPresetWrite_ = false;
    pendingPresetWillExist_ = false;
    pendingPresetName_.fill('\0');
    pendingPresetRecord_.fill(0xFFU);
    for (std::size_t slot = 0U; slot < kUserPresetSlotCount; ++slot) {
        presetValid_[slot] = false;
        presetNames_[slot].fill('\0');
    }
    return true;
}

}  // namespace clockfw::services
