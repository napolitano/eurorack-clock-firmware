/**
 * @file ui_controller_encoder.cpp
 * @brief Encoder push gesture handling and channel-menu navigation for UiController.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/ui_controller_encoder.h"

#include "config.h"
#include "ui/mode_functions.h"

namespace clockfw::ui {

void UiController::handleEncoderButton(
    const hal::ButtonSample& button,
    const std::uint32_t nowMs) {
    if (settingsChordActive_ || modeTapTurnActive_) {
        encoderPressTracking_ = false;
        encoderLongPressHandled_ = false;
        return;
    }

    if (button.edge == hal::ButtonEdge::Pressed) {
        encoderPressTracking_ = true;
        encoderLongPressHandled_ = false;
        encoderPressedAtMs_ = nowMs;
        return;
    }

    if (button.pressed && encoderPressTracking_ && !encoderLongPressHandled_ &&
        nowMs - encoderPressedAtMs_ >= config::kEncoderLongPressMs &&
        (navigation_.screen == Screen::Performance ||
         navigation_.screen == Screen::ChannelQuickSelect)) {
        encoderLongPressHandled_ = true;
        handleLongEncoderPress(nowMs);
        return;
    }

    if (button.edge == hal::ButtonEdge::Released) {
        const bool wasLongPress = encoderLongPressHandled_;
        encoderPressTracking_ = false;
        encoderLongPressHandled_ = false;
        if (!wasLongPress) {
            handleShortEncoderPress(nowMs);
        }
    }
}

void UiController::handleShortEncoderPress(const std::uint32_t nowMs) {
    if (navigation_.screen == Screen::Performance) {
        navigation_.screen = Screen::ChannelQuickSelect;
        navigation_.cursor = navigation_.selectedChannel;
        navigation_.editing = false;
        invalidate();
    } else if (navigation_.screen == Screen::ChannelQuickSelect) {
        if (state_.operatingMode == OperatingMode::Independent) {
            navigation_.selectedChannel = navigation_.cursor;
        }
        navigation_.screen = Screen::Performance;
        navigation_.cursor = navigation_.selectedChannel;
        navigation_.editing = false;
        invalidate();
    } else if (navigation_.screen == Screen::ModeSelect) {
        commitSelectedMode(nowMs);
    } else if (navigation_.screen == Screen::ModeChangeConfirm) {
        if (navigation_.cursor == 0U) {
            navigation_.screen = Screen::ModeSelect;
            navigation_.cursor = modeFunctionIndexForState(state_, navigation_.selectedChannel);
            invalidate();
        } else {
            applyConfirmedMode(nowMs);
        }
    } else if (navigation_.screen == Screen::Templates) {
        applySelectedTemplate(nowMs);
    } else if (navigation_.screen == Screen::PresetSlots) {
        navigation_.selectedPresetSlot = navigation_.cursor;
        if (navigation_.presetSlotAction == PresetSlotAction::Load) {
            loadSelectedPreset(nowMs);
        } else if (persistentState_.presetExists(navigation_.selectedPresetSlot)) {
            navigation_.screen = Screen::OverwriteConfirm;
            navigation_.cursor = 0U;
            invalidate();
        } else {
            preparePresetName();
            navigation_.screen = Screen::NameEntry;
            invalidate();
        }
    } else if (navigation_.screen == Screen::OverwriteConfirm) {
        if (navigation_.cursor == 0U) {
            navigation_.screen = Screen::PresetSlots;
            navigation_.cursor = navigation_.selectedPresetSlot;
            invalidate();
        } else {
            preparePresetName();
            navigation_.screen = Screen::NameEntry;
            invalidate();
        }
    } else if (navigation_.screen == Screen::NameEntry) {
        if (navigation_.nameCharacterIndex < services::PersistentStateService::kPresetNameLength - 1U) {
            ++navigation_.nameCharacterIndex;
            invalidate();
        } else {
            saveNamedPreset(nowMs);
        }
    } else if (navigation_.screen == Screen::Settings) {
        activateCurrentSetting();
        persistCurrentState(nowMs);
    } else if (navigation_.screen == Screen::SequencerEditor) {
        const std::uint8_t absoluteStep = static_cast<std::uint8_t>(
            navigation_.sequencerPage * 16U + navigation_.sequencerCursor);
        settingsEditor_.toggleSequencerStep(navigation_.selectedChannel, absoluteStep);
        persistCurrentState(nowMs);
        invalidate();
    }
}


void UiController::handleLongEncoderPress(const std::uint32_t nowMs) {
    (void)nowMs;
    const Screen origin = navigation_.screen;
    if (origin == Screen::ChannelQuickSelect &&
        state_.operatingMode == OperatingMode::Independent) {
        navigation_.selectedChannel = navigation_.cursor;
    }
    navigation_.settingsExitScreen = origin;

    if (state_.operatingMode == OperatingMode::UnifiedClock) {
        openSettingsPage(SettingsPage::UnifiedClock);
    } else if (state_.operatingMode == OperatingMode::DividerBank) {
        openSettingsPage(SettingsPage::DividerBank);
    } else {
        openSettingsPage(SettingsPage::Channel);
    }
}


}  // namespace clockfw::ui
