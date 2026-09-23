/**
 * @file ui_controller_navigation_activate.cpp
 * @brief Settings-row activation routing for UiController.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/ui_controller_navigation_activate.h"

#include <cstdio>

#include "ui/menu_model.h"
#include "ui/menu_model_channel.h"

namespace clockfw::ui {

void UiController::activateCurrentSetting() {
    // SettingsPage is the state-machine discriminator. Page-specific helpers keep
    // navigation commands separate from ordinary leaf-value editing.
    switch (navigation_.settingsPage) {
        case SettingsPage::Root:
            activateRootSetting();
            return;
        case SettingsPage::General:
            activateGeneralSetting();
            return;
        case SettingsPage::InputAssignments:
            if (navigation_.cursor == 2U) {
                openSettingsPage(SettingsPage::Sync);
            } else {
                toggleCurrentSettingEditing();
            }
            return;
        case SettingsPage::Hardware:
            toggleCurrentSettingEditing();
            return;
        case SettingsPage::Diagnostics:
            openSettingsPage(
                navigation_.cursor == 0U
                    ? SettingsPage::DiagnosticsInputs
                    : SettingsPage::DiagnosticsOutputs);
            return;
        case SettingsPage::Info:
            activateInfoSetting();
            return;
        case SettingsPage::Preferences:
            activatePreferencesSetting();
            return;
        case SettingsPage::Channel:
            activateChannelSetting();
            return;
        case SettingsPage::UnifiedClock:
            activateUnifiedClockSetting();
            return;
        case SettingsPage::ChannelTiming:
        case SettingsPage::UnifiedTiming:
            activateTimingSetting();
            return;
        case SettingsPage::Sequencer:
            if (navigation_.cursor == 2U) {
                openSettingsPage(SettingsPage::SequencerPattern);
            } else {
                toggleCurrentSettingEditing();
            }
            return;
        case SettingsPage::Groove:
            activateGrooveSetting();
            return;
        case SettingsPage::GrooveEditorMenu:
            activateGrooveEditorMenuSetting();
            return;
        case SettingsPage::GrooveRecordMenu:
            activateGrooveRecordMenuSetting();
            return;
        case SettingsPage::DividerBank:
            activateDividerBankSetting();
            return;
        case SettingsPage::SequencerPattern:
            activateSequencerPatternSetting();
            return;
        case SettingsPage::Master:
        case SettingsPage::Sync:
        case SettingsPage::Screensaver:
        case SettingsPage::ChannelOutput:
        case SettingsPage::Clock:
        case SettingsPage::Euclid:
        case SettingsPage::UnifiedOutput:
            toggleCurrentSettingEditing();
            return;
        case SettingsPage::DiagnosticsInputs:
        case SettingsPage::DiagnosticsOutputs:
        case SettingsPage::Licenses:
            openSelectedInformationPopover();
            return;
        case SettingsPage::Updates:
            // Read-only pages deliberately ignore encoder activation.
            return;
    }
}

void UiController::toggleCurrentSettingEditing() {
    navigation_.editing = !navigation_.editing;
    invalidate();
}

void UiController::activateRootSetting() {
    if (navigation_.cursor == 0U) {
        openSettingsPage(SettingsPage::General);
    } else if (navigation_.cursor == 1U) {
        // Screen::Settings is a parent marker here; backFromSettings() restores
        // the real Performance exit target once navigation returns to Root.
        navigation_.settingsExitScreen = Screen::Settings;
        if (state_.operatingMode == OperatingMode::UnifiedClock) {
            openSettingsPage(SettingsPage::UnifiedClock);
        } else if (state_.operatingMode == OperatingMode::DividerBank) {
            openSettingsPage(SettingsPage::DividerBank);
        } else {
            openSettingsPage(SettingsPage::Channel);
        }
    } else if (navigation_.cursor == 2U) {
        openSettingsPage(SettingsPage::Preferences);
    } else if (navigation_.cursor == 3U) {
        openSettingsPage(SettingsPage::Info);
    } else if (navigation_.highScoreResetAvailable && navigation_.cursor == 4U) {
        navigation_.screen = Screen::HighScoreClearConfirm;
        navigation_.cursor = 0U;  // Confirmation dialogs always default to NO.
        navigation_.editing = false;
        invalidate();
    } else {
        engine_.resetGlobalPhase();
        invalidate();
    }
}

void UiController::activateGeneralSetting() {
    switch (navigation_.cursor) {
        case 0U:
            openSettingsPage(SettingsPage::Master);
            break;
        case 1U:
            openSettingsPage(SettingsPage::InputAssignments);
            break;
        case 2U:
            openSettingsPage(SettingsPage::Screensaver);
            break;
        case 3U:
            openSettingsPage(SettingsPage::Diagnostics);
            break;
        default:
            openSettingsPage(SettingsPage::Hardware);
            break;
    }
}

void UiController::activateInfoSetting() {
    if (navigation_.cursor <= 2U) {
        openSelectedInformationPopover();
    } else if (navigation_.cursor == 3U) {
        openSettingsPage(SettingsPage::Licenses);
    } else if (navigation_.cursor == 4U) {
        openSettingsPage(SettingsPage::Updates);
    } else if (navigation_.cursor == 5U) {
        navigation_.screen = Screen::FactoryResetConfirm;
        navigation_.cursor = 0U;  // Confirmation dialogs always default to NO.
        navigation_.editing = false;
        invalidate();
    }
}

void UiController::openSelectedInformationPopover() {
    const MenuRow row = buildMenuRow(
        navigation_.settingsPage,
        navigation_.cursor,
        navigation_.selectedChannel,
        state_,
        navigation_.highScoreResetAvailable);
    if (!row.expandableInformation || row.value[0] == '\0' ||
        !renderer_.informationValueOverflows(row.label, row.value)) {
        return;
    }
    std::snprintf(
        navigation_.informationPopoverTitle.data(),
        navigation_.informationPopoverTitle.size(),
        "%s",
        row.label);
    std::snprintf(
        navigation_.informationPopoverValue.data(),
        navigation_.informationPopoverValue.size(),
        "%s",
        row.value);
    navigation_.editing = false;
    navigation_.screen = Screen::InformationPopover;
    invalidate();
}

void UiController::activatePreferencesSetting() {
    if (navigation_.cursor == 1U) {
        openPresetSlots(PresetSlotAction::Load);
    } else if (navigation_.cursor == 2U) {
        openPresetSlots(PresetSlotAction::Save);
    } else if (navigation_.cursor == 3U) {
        navigation_.screen = Screen::Templates;
        navigation_.cursor = 0U;
        navigation_.scrollOffset = 0U;
        invalidate();
    }
}

void UiController::activateChannelSetting() {
    const ChannelMode mode = state_.channels[navigation_.selectedChannel].common.mode;
    switch (channelMenuAction(mode, navigation_.cursor)) {
        case ChannelMenuAction::Mode:
            openModeSelect(Screen::Settings);
            break;
        case ChannelMenuAction::Timing:
            openSettingsPage(SettingsPage::ChannelTiming);
            break;
        case ChannelMenuAction::Clock:
            openSettingsPage(SettingsPage::Clock);
            break;
        case ChannelMenuAction::Euclid:
            openSettingsPage(SettingsPage::Euclid);
            break;
        case ChannelMenuAction::Sequencer:
            openSettingsPage(SettingsPage::Sequencer);
            break;
        case ChannelMenuAction::Output:
            openSettingsPage(SettingsPage::ChannelOutput);
            break;
    }
}

void UiController::activateUnifiedClockSetting() {
    if (navigation_.cursor == 0U) {
        openModeSelect(Screen::Settings);
    } else if (navigation_.cursor == 1U) {
        openSettingsPage(SettingsPage::UnifiedTiming);
    } else {
        openSettingsPage(SettingsPage::UnifiedOutput);
    }
}

void UiController::activateTimingSetting() {
    if (navigation_.cursor == 4U) {
        openSettingsPage(SettingsPage::Groove);
    } else {
        toggleCurrentSettingEditing();
    }
}

void UiController::activateGrooveSetting() {
    if (navigation_.cursor == 3U) {
        openGrooveSlots(GrooveSlotAction::Activate);
    } else if (navigation_.cursor == 4U) {
        openGrooveEditor();
    } else if (navigation_.cursor == 5U) {
        openGrooveRecorder();
    } else if (navigation_.cursor == 6U) {
        openGrooveSlots(GrooveSlotAction::Rename);
    } else if (navigation_.cursor == 7U) {
        openGrooveSlots(GrooveSlotAction::Delete);
    } else {
        toggleCurrentSettingEditing();
    }
}

void UiController::activateGrooveEditorMenuSetting() {
    if (navigation_.cursor == 0U) {
        openGrooveSlots(GrooveSlotAction::Save);
        return;
    }
    if (navigation_.cursor != 1U) {
        toggleCurrentSettingEditing();
        return;
    }

    // Loading replaces the live draft. Require an explicit discard decision first
    // so an accidental LOAD cannot destroy unsaved marker edits.
    if (grooveEditorDirty_) {
        grooveLoadPendingAfterDiscard_ = true;
        navigation_.screen = Screen::GrooveDiscardConfirm;
        navigation_.cursor = 0U;
        invalidate();
    } else {
        openGrooveSlots(GrooveSlotAction::LoadEditor);
    }
}

void UiController::activateGrooveRecordMenuSetting() {
    if (navigation_.cursor <= 3U) {
        toggleCurrentSettingEditing();
        return;
    }
    if (navigation_.cursor == 4U) {
        openGrooveSlots(GrooveSlotAction::Save);
        return;
    }
    if (navigation_.cursor == 5U) {
        grooveRecorder_.stop();
        navigation_.grooveRecordState = GrooveRecordState::Ready;
        navigation_.grooveRecordPlayheadStep = 0U;
        grooveWorkspaceScreen_ = Screen::GrooveEditor;
        navigation_.screen = Screen::GrooveEditor;
        navigation_.cursor = 0U;
        navigation_.scrollOffset = 0U;
        navigation_.editing = false;
        invalidate();
        return;
    }
    clearGrooveRecording();
}

void UiController::activateDividerBankSetting() {
    if (navigation_.cursor == 0U) {
        openModeSelect(Screen::Settings);
    } else {
        toggleCurrentSettingEditing();
    }
}

void UiController::activateSequencerPatternSetting() {
    if (navigation_.cursor == 0U) {
        navigation_.screen = Screen::SequencerEditor;
        navigation_.sequencerPage = 0U;
        navigation_.sequencerCursor = 0U;
        invalidate();
        return;
    }

    if (settingsEditor_.executeSequencerCommand(
            navigation_.selectedChannel, navigation_.cursor)) {
        invalidate();
    }
}

}  // namespace clockfw::ui
