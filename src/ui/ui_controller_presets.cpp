/**
 * @file ui_controller_presets.cpp
 * @brief Named-preset and high-score-style name-entry workflow for UiController.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/ui_controller.h"

#include <algorithm>
#include <cstdio>
#include <cstring>

#include "ui/preset_name_alphabet.h"
#include "ui_text.h"

namespace clockfw::ui {
void UiController::openPresetSlots(const PresetSlotAction action) {
    navigation_.screen = Screen::PresetSlots;
    navigation_.presetSlotAction = action;
    navigation_.cursor = navigation_.selectedPresetSlot;
    navigation_.scrollOffset = navigation_.cursor >= 5U
        ? static_cast<std::uint8_t>(navigation_.cursor - 4U)
        : 0U;
    navigation_.editing = false;
    invalidate();
}

void UiController::preparePresetName() {
    navigation_.presetNameBuffer.fill(' ');
    navigation_.presetNameBuffer.back() = '\0';

    char existingName[services::PersistentStateService::kPresetNameLength + 1U]{};
    persistentState_.presetName(navigation_.selectedPresetSlot, existingName, sizeof(existingName));
    if (existingName[0] != '\0') {
        const std::size_t length = std::min<std::size_t>(
            std::strlen(existingName), services::PersistentStateService::kPresetNameLength);
        std::copy_n(existingName, length, navigation_.presetNameBuffer.begin());
    } else {
        char defaultName[services::PersistentStateService::kPresetNameLength + 1U]{};
        std::snprintf(
            defaultName,
            sizeof(defaultName),
            text::get(text::TextId::PresetDefaultFormat),
            static_cast<unsigned>(navigation_.selectedPresetSlot + 1U));
        std::copy_n(
            defaultName,
            std::min<std::size_t>(std::strlen(defaultName), services::PersistentStateService::kPresetNameLength),
            navigation_.presetNameBuffer.begin());
    }
    navigation_.nameCharacterIndex = 0U;
}

void UiController::adjustPresetNameCharacter(const std::int8_t delta) {
    const std::uint8_t characterPosition = navigation_.nameCharacterIndex;
    const std::size_t currentIndex = presetname::characterIndex(
        navigation_.presetNameBuffer[characterPosition]);
    navigation_.presetNameBuffer[characterPosition] = presetname::characterAtWrapped(
        static_cast<int>(currentIndex) + delta);
}

void UiController::saveNamedPreset(const std::uint32_t nowMs) {
    if (persistentState_.savePreset(
            navigation_.selectedPresetSlot,
            navigation_.presetNameBuffer.data(),
            state_)) {
        persistCurrentState(nowMs);
    }
    openPresetSlots(PresetSlotAction::Save);
    navigation_.cursor = navigation_.selectedPresetSlot;
}

void UiController::loadSelectedPreset(const std::uint32_t nowMs) {
    if (!persistentState_.loadPreset(navigation_.selectedPresetSlot, state_)) {
        invalidate();
        return;
    }

    engine_.updateConfiguration(state_, true);
    persistCurrentState(nowMs);
    navigation_.screen = Screen::Performance;
    invalidate();
}

void UiController::persistCurrentState(const std::uint32_t nowMs) {
    persistentState_.requestCurrentState(state_, nowMs);
}

}  // namespace clockfw::ui
