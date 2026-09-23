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
         navigation_.screen == Screen::ChannelQuickSelect ||
         navigation_.screen == Screen::GrooveEditor ||
         navigation_.screen == Screen::GrooveRecorder ||
         navigation_.screen == Screen::NameEntry ||
         navigation_.screen == Screen::GrooveNameEntry)) {
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
    // Screen is the state-machine discriminator. A switch keeps each press action
    // self-contained and makes intentionally unsupported screens explicit.
    switch (navigation_.screen) {
        case Screen::Performance:
            navigation_.screen = Screen::ChannelQuickSelect;
            navigation_.cursor = navigation_.selectedChannel;
            navigation_.editing = false;
            invalidate();
            return;

        case Screen::ChannelQuickSelect:
            if (state_.operatingMode == OperatingMode::Independent) {
                navigation_.selectedChannel = navigation_.cursor;
            }
            navigation_.screen = Screen::Performance;
            navigation_.cursor = navigation_.selectedChannel;
            navigation_.editing = false;
            invalidate();
            return;

        case Screen::ModeSelect:
            commitSelectedMode(nowMs);
            return;

        case Screen::ModeChangeConfirm:
            if (navigation_.cursor == 0U) {
                navigation_.screen = Screen::ModeSelect;
                navigation_.cursor = modeFunctionIndexForState(state_, navigation_.selectedChannel);
                invalidate();
            } else {
                applyConfirmedMode(nowMs);
            }
            return;

        case Screen::Templates:
            applySelectedTemplate(nowMs);
            return;

        case Screen::PresetSlots:
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
            return;

        case Screen::OverwriteConfirm:
            if (navigation_.cursor == 0U) {
                navigation_.screen = Screen::PresetSlots;
                navigation_.cursor = navigation_.selectedPresetSlot;
                invalidate();
            } else {
                preparePresetName();
                navigation_.screen = Screen::NameEntry;
                invalidate();
            }
            return;

        case Screen::HighScoreClearConfirm:
            confirmHighScoreClear(nowMs);
            return;

        case Screen::FactoryResetConfirm:
            confirmFactoryReset(nowMs);
            return;

        case Screen::NameEntry:
            if (navigation_.nameCharacterIndex <
                services::PersistentStateService::kPresetNameLength - 1U) {
                ++navigation_.nameCharacterIndex;
                invalidate();
            } else {
                saveNamedPreset(nowMs);
            }
            return;

        case Screen::GrooveSlots:
            navigation_.selectedGrooveSlot = navigation_.cursor;
            if (navigation_.grooveSlotAction == GrooveSlotAction::LoadEditor) {
                loadSelectedGroove();
            } else if (navigation_.grooveSlotAction == GrooveSlotAction::Activate) {
                activateSelectedGroove(nowMs);
            } else if (navigation_.grooveSlotAction == GrooveSlotAction::Rename) {
                if (prepareExistingGrooveName()) {
                    navigation_.screen = Screen::GrooveNameEntry;
                    invalidate();
                }
            } else if (navigation_.grooveSlotAction == GrooveSlotAction::Delete) {
                if (customGrooveStore_ != nullptr &&
                    customGrooveStore_->exists(navigation_.selectedGrooveSlot)) {
                    navigation_.screen = Screen::GrooveDeleteConfirm;
                    navigation_.cursor = 0U;
                    invalidate();
                }
            } else if (customGrooveStore_ != nullptr &&
                       customGrooveStore_->exists(navigation_.selectedGrooveSlot)) {
                navigation_.screen = Screen::GrooveOverwriteConfirm;
                navigation_.cursor = 0U;
                invalidate();
            } else {
                prepareGrooveName(nowMs);
                navigation_.screen = Screen::GrooveNameEntry;
                invalidate();
            }
            return;

        case Screen::GrooveOverwriteConfirm:
            if (navigation_.cursor == 0U) {
                navigation_.screen = Screen::GrooveSlots;
                navigation_.cursor = navigation_.selectedGrooveSlot;
                invalidate();
            } else {
                saveCustomGroove(nowMs, true);
            }
            return;

        case Screen::GrooveDeleteConfirm:
            if (navigation_.cursor == 0U) {
                navigation_.screen = Screen::GrooveSlots;
                navigation_.cursor = navigation_.selectedGrooveSlot;
                invalidate();
            } else {
                removeSelectedGroove(nowMs);
            }
            return;

        case Screen::GrooveDiscardConfirm:
            if (navigation_.cursor == 0U) {
                navigation_.screen = grooveWorkspaceScreen_;
                grooveLoadPendingAfterDiscard_ = false;
                invalidate();
            } else if (grooveLoadPendingAfterDiscard_) {
                grooveEditorDirty_ = false;
                grooveLoadPendingAfterDiscard_ = false;
                openGrooveSlots(GrooveSlotAction::LoadEditor);
            } else {
                leaveGrooveEditor(true);
            }
            return;

        case Screen::GrooveNameEntry:
            if (navigation_.nameCharacterIndex < services::CustomGrooveStore::kNameLength - 1U) {
                ++navigation_.nameCharacterIndex;
                invalidate();
            } else if (navigation_.grooveSlotAction == GrooveSlotAction::Rename) {
                renameSelectedGroove();
            } else {
                saveCustomGroove(nowMs, false);
            }
            return;

        case Screen::InformationPopover:
            navigation_.screen = Screen::Settings;
            invalidate();
            return;

        case Screen::Settings:
            activateCurrentSetting();
            persistCurrentState(nowMs);
            return;

        case Screen::SequencerEditor: {
            const std::uint8_t absoluteStep = static_cast<std::uint8_t>(
                navigation_.sequencerPage * 16U + navigation_.sequencerCursor);
            settingsEditor_.toggleSequencerStep(navigation_.selectedChannel, absoluteStep);
            persistCurrentState(nowMs);
            invalidate();
            return;
        }

        case Screen::GrooveEditor:
        case Screen::GrooveRecorder:
            // Short press has no direct workspace action. Long press opens the
            // context menu; TAP/PLAY own musical interaction in each workspace.
            return;
    }
}


void UiController::handleLongEncoderPress(const std::uint32_t nowMs) {
    const Screen origin = navigation_.screen;
    if (origin == Screen::GrooveNameEntry) {
        if (navigation_.grooveSlotAction == GrooveSlotAction::Rename) {
            renameSelectedGroove();
        } else {
            saveCustomGroove(nowMs, false);
        }
        return;
    }
    if (origin == Screen::NameEntry) {
        saveNamedPreset(nowMs);
        return;
    }
    if (origin == Screen::GrooveEditor) {
        navigation_.settingsExitScreen = Screen::GrooveEditor;
        openSettingsPage(SettingsPage::GrooveEditorMenu);
        return;
    }
    if (origin == Screen::GrooveRecorder) {
        grooveRecorder_.stop();
        navigation_.grooveRecordState = GrooveRecordState::Ready;
        navigation_.grooveRecordPlayheadStep = 0U;
        navigation_.grooveRecordPlayheadPhase256 = 0U;
        navigation_.settingsExitScreen = Screen::GrooveRecorder;
        openSettingsPage(SettingsPage::GrooveRecordMenu);
        return;
    }
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
