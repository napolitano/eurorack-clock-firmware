/**
 * @file ui_controller_groove.cpp
 * @brief Custom Groove editor navigation, live preview, and fixed-slot persistence.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/ui_controller.h"

#include <algorithm>
#include <cstdio>

#include "ui/groove_name_generator.h"
#include "ui/preset_name_alphabet.h"

namespace clockfw::ui {
namespace {
GrooveSettings& activeGrooveSettings(ClockState& state, const std::uint8_t channel) {
    return state.operatingMode == OperatingMode::UnifiedClock
        ? state.unifiedClock.groove
        : state.channels[channel].common.groove;
}
}  // namespace

void UiController::openGrooveEditor() {
    grooveEditorOriginalSettings_ = activeGrooveSettings(state_, navigation_.selectedChannel);
    navigation_.grooveDraft = CustomGroovePattern{};
    if (customGrooveStore_ != nullptr &&
        grooveEditorOriginalSettings_.preset == GroovePreset::Custom &&
        grooveEditorOriginalSettings_.customSlot < kCustomGrooveSlotCount) {
        (void)customGrooveStore_->load(
            grooveEditorOriginalSettings_.customSlot,
            navigation_.grooveDraft);
    }
    if (!isCustomGroovePatternValid(navigation_.grooveDraft)) {
        navigation_.grooveDraft = CustomGroovePattern{};
    }
    navigation_.grooveCursor = 0U;
    navigation_.grooveZoomSteps = 0U;
    navigation_.screen = Screen::GrooveEditor;
    navigation_.editing = false;
    grooveEditorDirty_ = false;
    grooveLoadPendingAfterDiscard_ = false;
    transportPressConsumedByGrooveZoom_ = false;
    updateGroovePreview();
    invalidate();
}

void UiController::updateGroovePreview() {
    const GrooveSettings& settings = activeGrooveSettings(state_, navigation_.selectedChannel);
    const std::uint8_t rotation = settings.rotation < navigation_.grooveDraft.length
        ? settings.rotation
        : 0U;
    if (state_.operatingMode == OperatingMode::UnifiedClock) {
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            engine_.setCustomGroovePreview(
                channelIndex,
                navigation_.grooveDraft,
                settings.amountPercent,
                rotation,
                true);
        }
    } else {
        engine_.setCustomGroovePreview(
            navigation_.selectedChannel,
            navigation_.grooveDraft,
            settings.amountPercent,
            rotation,
            true);
    }
}

void UiController::clearGroovePreview() {
    if (state_.operatingMode == OperatingMode::UnifiedClock) {
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            engine_.clearCustomGroovePreview(channelIndex, true);
        }
    } else {
        engine_.clearCustomGroovePreview(navigation_.selectedChannel, true);
    }
}

void UiController::adjustGrooveOffset(const std::int8_t delta, const bool coarse) {
    const int scale = coarse ? 4 : 1;
    const int current = navigation_.grooveDraft.offsets256[navigation_.grooveCursor];
    navigation_.grooveDraft.offsets256[navigation_.grooveCursor] = static_cast<std::int8_t>(clampInt(
        current + static_cast<int>(delta) * scale,
        kCustomGrooveMinimumOffset256,
        kCustomGrooveMaximumOffset256));
    grooveEditorDirty_ = true;
    updateGroovePreview();
    invalidate();
}

void UiController::adjustGrooveZoom(const std::int8_t direction) {
    if (direction == 0) return;
    const std::uint8_t length = navigation_.grooveDraft.length;
    if (direction > 0) {
        if (navigation_.grooveZoomSteps == 0U) {
            const std::uint8_t capped = std::min<std::uint8_t>(32U, length);
            navigation_.grooveZoomSteps = capped > 4U
                ? static_cast<std::uint8_t>((capped / 4U) * 4U)
                : 4U;
            if (navigation_.grooveZoomSteps >= length && length > 4U) {
                navigation_.grooveZoomSteps = static_cast<std::uint8_t>(
                    ((length - 1U) / 4U) * 4U);
            }
        } else if (navigation_.grooveZoomSteps > 4U) {
            navigation_.grooveZoomSteps = static_cast<std::uint8_t>(navigation_.grooveZoomSteps - 4U);
        }
    } else if (navigation_.grooveZoomSteps != 0U) {
        const std::uint8_t next = static_cast<std::uint8_t>(navigation_.grooveZoomSteps + 4U);
        navigation_.grooveZoomSteps = next >= length ? 0U : next;
    }
    invalidate();
}

void UiController::openGrooveSlots(const GrooveSlotAction action) {
    if (customGrooveStore_ == nullptr) {
        navigation_.screen = Screen::GrooveEditor;
        invalidate();
        return;
    }
    navigation_.grooveSlotAction = action;
    navigation_.screen = Screen::GrooveSlots;
    navigation_.cursor = 0U;
    navigation_.scrollOffset = 0U;
    invalidate();
}

void UiController::loadSelectedGroove() {
    if (customGrooveStore_ == nullptr) {
        navigation_.screen = Screen::GrooveEditor;
        invalidate();
        return;
    }
    CustomGroovePattern pattern{};
    if (!customGrooveStore_->load(navigation_.selectedGrooveSlot, pattern)) {
        navigation_.screen = Screen::GrooveSlots;
        invalidate();
        return;
    }
    navigation_.grooveDraft = pattern;
    navigation_.grooveCursor = static_cast<std::uint8_t>(std::min<std::uint8_t>(
        navigation_.grooveCursor,
        static_cast<std::uint8_t>(pattern.length - 1U)));
    navigation_.grooveZoomSteps = 0U;
    grooveEditorDirty_ = false;
    grooveEditorOriginalSettings_ = activeGrooveSettings(state_, navigation_.selectedChannel);
    updateGroovePreview();
    navigation_.screen = Screen::GrooveEditor;
    invalidate();
}

void UiController::activateSelectedGroove(const std::uint32_t nowMs) {
    if (customGrooveStore_ == nullptr) {
        openSettingsPage(SettingsPage::Groove, 3U);
        return;
    }

    CustomGroovePattern pattern{};
    if (!customGrooveStore_->load(navigation_.selectedGrooveSlot, pattern)) {
        navigation_.screen = Screen::GrooveSlots;
        invalidate();
        return;
    }

    // Refresh the real-time library before making the slot active. LOAD from the
    // normal Groove page is an immediate preset recall, not an editor preview.
    engine_.updateCustomGrooveSlot(navigation_.selectedGrooveSlot, pattern, false);
    GrooveSettings& settings = activeGrooveSettings(state_, navigation_.selectedChannel);
    settings.preset = GroovePreset::Custom;
    settings.customSlot = navigation_.selectedGrooveSlot;
    settings.amountPercent = 100U;
    settings.rotation = 0U;
    if (state_.operatingMode == OperatingMode::UnifiedClock) {
        engine_.updateConfiguration(state_, true);
    } else {
        engine_.updateChannel(
            navigation_.selectedChannel,
            state_.channels[navigation_.selectedChannel],
            true);
    }
    persistCurrentState(nowMs);
    openSettingsPage(SettingsPage::Groove, 3U);
}

void UiController::prepareGrooveName(const std::uint32_t nowMs) {
    navigation_.grooveNameBuffer.fill(' ');
    navigation_.grooveNameBuffer.back() = '\0';

    char generated[services::CustomGrooveStore::kNameLength + 1U]{};
    groovename::generateDefaultName(
        generatedNameSeed_,
        generatedNameSequence_++,
        nowMs ^ static_cast<std::uint32_t>(navigation_.selectedGrooveSlot),
        generated,
        sizeof(generated));
    for (std::size_t index = 0U;
         index < services::CustomGrooveStore::kNameLength && generated[index] != '\0';
         ++index) {
        navigation_.grooveNameBuffer[index] = generated[index];
    }
    navigation_.nameCharacterIndex = 0U;
}

void UiController::adjustGrooveNameCharacter(const std::int8_t delta) {
    char& character = navigation_.grooveNameBuffer[navigation_.nameCharacterIndex];
    const int current = static_cast<int>(presetname::characterIndex(character));
    character = presetname::characterAtWrapped(current + delta);
}

void UiController::saveCustomGroove(
    const std::uint32_t nowMs,
    const bool keepExistingName) {
    if (customGrooveStore_ == nullptr) {
        navigation_.screen = Screen::GrooveEditor;
        invalidate();
        return;
    }
    char name[services::CustomGrooveStore::kNameLength + 1U]{};
    if (keepExistingName) {
        customGrooveStore_->name(navigation_.selectedGrooveSlot, name, sizeof(name));
    } else {
        std::copy_n(
            navigation_.grooveNameBuffer.data(),
            services::CustomGrooveStore::kNameLength,
            name);
        name[services::CustomGrooveStore::kNameLength] = '\0';
    }
    if (!customGrooveStore_->save(
            navigation_.selectedGrooveSlot,
            name,
            navigation_.grooveDraft)) {
        navigation_.screen = Screen::GrooveEditor;
        invalidate();
        return;
    }
    engine_.updateCustomGrooveSlot(
        navigation_.selectedGrooveSlot,
        navigation_.grooveDraft,
        false);
    activateSavedCustomGroove(nowMs);
}

void UiController::activateSavedCustomGroove(const std::uint32_t nowMs) {
    clearGroovePreview();
    GrooveSettings& settings = activeGrooveSettings(state_, navigation_.selectedChannel);
    settings.preset = GroovePreset::Custom;
    settings.customSlot = navigation_.selectedGrooveSlot;
    settings.amountPercent = 100U;
    settings.rotation = 0U;
    if (state_.operatingMode == OperatingMode::UnifiedClock) {
        engine_.updateConfiguration(state_, true);
    } else {
        engine_.updateChannel(navigation_.selectedChannel, state_.channels[navigation_.selectedChannel], true);
    }
    persistCurrentState(nowMs);
    grooveEditorOriginalSettings_ = settings;
    grooveEditorDirty_ = false;
    navigation_.settingsExitScreen = Screen::Performance;
    openSettingsPage(SettingsPage::Groove, 4U);
}

void UiController::leaveGrooveEditor(const bool discardChanges) {
    clearGroovePreview();
    if (discardChanges) {
        GrooveSettings& settings = activeGrooveSettings(state_, navigation_.selectedChannel);
        settings = grooveEditorOriginalSettings_;
        if (state_.operatingMode == OperatingMode::UnifiedClock) {
            engine_.updateConfiguration(state_, true);
        } else {
            engine_.updateChannel(navigation_.selectedChannel, state_.channels[navigation_.selectedChannel], true);
        }
    }
    grooveEditorDirty_ = false;
    navigation_.settingsExitScreen = Screen::Performance;
    openSettingsPage(SettingsPage::Groove, 4U);
}

}  // namespace clockfw::ui
