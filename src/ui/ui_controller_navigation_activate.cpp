/**
 * @file ui_controller_navigation_activate.cpp
 * @brief Settings-row activation routing for UiController.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/ui_controller_navigation_activate.h"

#include "ui/menu_model_channel.h"

namespace clockfw::ui {

void UiController::activateCurrentSetting() {
    ChannelConfig& channel = state_.channels[navigation_.selectedChannel];

    if (navigation_.settingsPage == SettingsPage::Root) {
        if (navigation_.cursor == 0U) {
            openSettingsPage(SettingsPage::General);
        } else if (navigation_.cursor == 1U) {
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
            navigation_.cursor = 0U;  // Safe default: NO.
            navigation_.editing = false;
            invalidate();
        } else {
            engine_.resetGlobalPhase();
            invalidate();
        }
        return;
    }

    if (navigation_.settingsPage == SettingsPage::General) {
        if (navigation_.cursor == 0U) {
            openSettingsPage(SettingsPage::Master);
        } else if (navigation_.cursor == 1U) {
            openSettingsPage(SettingsPage::InputAssignments);
        } else if (navigation_.cursor == 2U) {
            openSettingsPage(SettingsPage::Screensaver);
        } else if (navigation_.cursor == 3U) {
            openSettingsPage(SettingsPage::Diagnostics);
        } else {
            openSettingsPage(SettingsPage::Hardware);
        }
        return;
    }

    if (navigation_.settingsPage == SettingsPage::InputAssignments) {
        if (navigation_.cursor == 2U) {
            openSettingsPage(SettingsPage::Sync);
        } else {
            navigation_.editing = !navigation_.editing;
            invalidate();
        }
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Hardware) {
        navigation_.editing = !navigation_.editing;
        invalidate();
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Diagnostics) {
        openSettingsPage(
            navigation_.cursor == 0U
                ? SettingsPage::DiagnosticsInputs
                : SettingsPage::DiagnosticsOutputs);
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Info) {
        if (navigation_.cursor == 3U) {
            openSettingsPage(SettingsPage::Licenses);
        } else if (navigation_.cursor == 4U) {
            openSettingsPage(SettingsPage::Updates);
        } else if (navigation_.cursor == 5U) {
            navigation_.screen = Screen::FactoryResetConfirm;
            navigation_.cursor = 0U;  // Safe default: NO.
            navigation_.editing = false;
            invalidate();
        }
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Preferences) {
        if (navigation_.cursor == 1U) openPresetSlots(PresetSlotAction::Load);
        else if (navigation_.cursor == 2U) openPresetSlots(PresetSlotAction::Save);
        else if (navigation_.cursor == 3U) {
            navigation_.screen = Screen::Templates;
            navigation_.cursor = 0U;
            navigation_.scrollOffset = 0U;
            invalidate();
        }
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Channel) {
        switch (channelMenuAction(channel.common.mode, navigation_.cursor)) {
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
        return;
    }

    if (navigation_.settingsPage == SettingsPage::UnifiedClock) {
        if (navigation_.cursor == 0U) {
            openModeSelect(Screen::Settings);
        } else if (navigation_.cursor == 1U) {
            openSettingsPage(SettingsPage::UnifiedTiming);
        } else {
            openSettingsPage(SettingsPage::UnifiedOutput);
        }
        return;
    }

    if (navigation_.settingsPage == SettingsPage::ChannelTiming && navigation_.cursor == 4U) {
        openSettingsPage(SettingsPage::Groove);
        return;
    }

    if (navigation_.settingsPage == SettingsPage::UnifiedTiming && navigation_.cursor == 4U) {
        openSettingsPage(SettingsPage::Groove);
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Sequencer && navigation_.cursor == 2U) {
        openSettingsPage(SettingsPage::SequencerPattern);
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Groove) {
        if (navigation_.cursor == 3U) {
            openGrooveEditor();
        } else {
            navigation_.editing = !navigation_.editing;
            invalidate();
        }
        return;
    }

    if (navigation_.settingsPage == SettingsPage::GrooveEditorMenu) {
        if (navigation_.cursor == 0U) {
            openGrooveSlots(GrooveSlotAction::Save);
        } else if (navigation_.cursor == 1U) {
            if (grooveEditorDirty_) {
                grooveLoadPendingAfterDiscard_ = true;
                navigation_.screen = Screen::GrooveDiscardConfirm;
                navigation_.cursor = 0U;
                invalidate();
            } else {
                openGrooveSlots(GrooveSlotAction::Load);
            }
        } else {
            navigation_.editing = !navigation_.editing;
            invalidate();
        }
        return;
    }

    if (navigation_.settingsPage == SettingsPage::DividerBank && navigation_.cursor == 0U) {
        openModeSelect(Screen::Settings);
        return;
    }

    if (navigation_.settingsPage == SettingsPage::DividerBank) {
        navigation_.editing = !navigation_.editing;
        invalidate();
        return;
    }

    if (navigation_.settingsPage == SettingsPage::SequencerPattern && navigation_.cursor == 0U) {
        navigation_.screen = Screen::SequencerEditor;
        navigation_.sequencerPage = 0U;
        navigation_.sequencerCursor = 0U;
        invalidate();
        return;
    }

    if (navigation_.settingsPage == SettingsPage::SequencerPattern && navigation_.cursor >= 1U) {
        if (settingsEditor_.executeSequencerCommand(
                navigation_.selectedChannel, navigation_.cursor)) {
            invalidate();
        }
        return;
    }

    if (navigation_.settingsPage != SettingsPage::Info &&
        navigation_.settingsPage != SettingsPage::Licenses &&
        navigation_.settingsPage != SettingsPage::Updates &&
        navigation_.settingsPage != SettingsPage::General &&
        navigation_.settingsPage != SettingsPage::Diagnostics &&
        navigation_.settingsPage != SettingsPage::DiagnosticsInputs &&
        navigation_.settingsPage != SettingsPage::DiagnosticsOutputs) {
        navigation_.editing = !navigation_.editing;
        invalidate();
    }
}


}  // namespace clockfw::ui
