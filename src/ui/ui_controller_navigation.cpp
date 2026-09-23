/**
 * @file ui_controller_navigation.cpp
 * @brief Settings hierarchy, shortcuts, templates, and navigation transitions for UiController.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/ui_controller.h"

#include <algorithm>

#include "domain/clock_options.h"
#include "services/template_service.h"
#include "ui/menu_model.h"
#include "ui/menu_model_channel.h"
#include "ui/mode_functions.h"

namespace clockfw::ui {

void UiController::openSettingsPage(
    const SettingsPage page,
    const std::uint8_t initialCursor) {
    navigation_.settingsPage = page;
    const ChannelMode mode = state_.channels[navigation_.selectedChannel].common.mode;
    if (page == SettingsPage::Root) {
        navigation_.highScoreResetAvailable = leaderboard_ != nullptr && leaderboard_->hasPersistentRecord();
    }
    const std::uint8_t itemCount = settingsPageItemCount(
        page, mode, navigation_.highScoreResetAvailable);
    navigation_.cursor = static_cast<std::uint8_t>(std::min<std::uint8_t>(
        initialCursor, static_cast<std::uint8_t>(itemCount - 1U)));
    navigation_.scrollOffset = 0U;
    navigation_.editing = false;
    navigation_.screen = Screen::Settings;
    normalizeScrollOffset();
    invalidate();
}

void UiController::openModeSelect(const Screen returnScreen) {
    navigation_.screen = Screen::ModeSelect;
    navigation_.modeSelectReturnScreen = returnScreen;
    navigation_.cursor = modeFunctionIndexForState(state_, navigation_.selectedChannel);
    navigation_.editing = false;
    invalidate();
}

void UiController::commitSelectedMode(const std::uint32_t nowMs) {
    const ModeFunction selectedFunction = kModeFunctions[navigation_.cursor];
    if (selectedModeIsCurrent(selectedFunction)) {
        navigateAfterModeSelection(selectedFunction);
        return;
    }

    (void)nowMs;
    navigation_.pendingModeFunction = selectedFunction;
    navigation_.screen = Screen::ModeChangeConfirm;
    navigation_.cursor = 0U;  // Safe default: NO.
    navigation_.editing = false;
    invalidate();
}

bool UiController::selectedModeIsCurrent(const ModeFunction modeFunction) const {
    const std::uint8_t currentIndex = modeFunctionIndexForState(
        state_, navigation_.selectedChannel);
    return currentIndex < kModeFunctions.size() &&
        kModeFunctions[currentIndex] == modeFunction;
}

void UiController::applyConfirmedMode(const std::uint32_t nowMs) {
    applyModeFunction(
        state_,
        navigation_.selectedChannel,
        navigation_.pendingModeFunction);
    engine_.updateConfiguration(state_, true);
    persistCurrentState(nowMs);
    navigateAfterModeSelection(navigation_.pendingModeFunction);
}

void UiController::navigateAfterModeSelection(const ModeFunction modeFunction) {
    navigation_.editing = false;
    navigation_.scrollOffset = 0U;

    switch (modeFunction) {
        case ModeFunction::Off:
        case ModeFunction::Clock:
            navigation_.screen = Screen::Performance;
            navigation_.cursor = navigation_.selectedChannel;
            invalidate();
            return;
        case ModeFunction::Euclid:
            navigation_.settingsExitScreen = Screen::Performance;
            openSettingsPage(SettingsPage::Euclid);
            return;
        case ModeFunction::Sequencer:
            navigation_.screen = Screen::SequencerEditor;
            navigation_.sequencerPage = 0U;
            navigation_.sequencerCursor = 0U;
            invalidate();
            return;
        case ModeFunction::UnifiedClock:
            navigation_.settingsExitScreen = Screen::Performance;
            openSettingsPage(SettingsPage::UnifiedClock, 1U);
            return;
        case ModeFunction::DividerBank:
            navigation_.settingsExitScreen = Screen::Performance;
            // BANK is the relevant first choice after entering divider mode.
            openSettingsPage(SettingsPage::DividerBank, 1U);
            return;
        default:
            navigation_.screen = Screen::Performance;
            invalidate();
            return;
    }
}

void UiController::backFromSettings() {
    const SettingsPage previousPage = navigation_.settingsPage;
    if (navigation_.settingsPage == SettingsPage::Root) {
        navigation_.screen = navigation_.settingsExitScreen;
    } else if (navigation_.settingsPage == SettingsPage::General ||
               navigation_.settingsPage == SettingsPage::Preferences ||
               navigation_.settingsPage == SettingsPage::Info) {
        navigation_.settingsPage = SettingsPage::Root;
    } else if (navigation_.settingsPage == SettingsPage::Sync) {
        navigation_.settingsPage = SettingsPage::InputAssignments;
    } else if (navigation_.settingsPage == SettingsPage::Master ||
               navigation_.settingsPage == SettingsPage::InputAssignments ||
               navigation_.settingsPage == SettingsPage::Hardware ||
               navigation_.settingsPage == SettingsPage::Screensaver ||
               navigation_.settingsPage == SettingsPage::Diagnostics) {
        navigation_.settingsPage = SettingsPage::General;
    } else if (navigation_.settingsPage == SettingsPage::DiagnosticsInputs ||
               navigation_.settingsPage == SettingsPage::DiagnosticsOutputs) {
        navigation_.settingsPage = SettingsPage::Diagnostics;
    } else if (navigation_.settingsPage == SettingsPage::Licenses ||
               navigation_.settingsPage == SettingsPage::Updates) {
        navigation_.settingsPage = SettingsPage::Info;
    } else if (navigation_.settingsPage == SettingsPage::ChannelTiming ||
               navigation_.settingsPage == SettingsPage::ChannelOutput ||
               navigation_.settingsPage == SettingsPage::Clock ||
               navigation_.settingsPage == SettingsPage::Euclid ||
               navigation_.settingsPage == SettingsPage::Sequencer) {
        navigation_.settingsPage = SettingsPage::Channel;
    } else if (navigation_.settingsPage == SettingsPage::SequencerPattern) {
        navigation_.settingsPage = SettingsPage::Sequencer;
    } else if (navigation_.settingsPage == SettingsPage::UnifiedTiming ||
               navigation_.settingsPage == SettingsPage::UnifiedOutput) {
        navigation_.settingsPage = SettingsPage::UnifiedClock;
    } else if (navigation_.settingsPage == SettingsPage::GrooveEditorMenu) {
        navigation_.screen = Screen::GrooveEditor;
    } else if (navigation_.settingsPage == SettingsPage::GrooveRecordMenu) {
        navigation_.screen = Screen::GrooveRecorder;
    } else if (navigation_.settingsPage == SettingsPage::Groove) {
        navigation_.settingsPage = state_.operatingMode == OperatingMode::UnifiedClock
            ? SettingsPage::UnifiedTiming
            : SettingsPage::ChannelTiming;
    } else if (navigation_.settingsPage == SettingsPage::Channel ||
               navigation_.settingsPage == SettingsPage::UnifiedClock ||
               navigation_.settingsPage == SettingsPage::DividerBank) {
        if (navigation_.settingsExitScreen == Screen::Settings) {
            navigation_.settingsPage = SettingsPage::Root;
            // Screen::Settings is used only as a parent marker while entering
            // channel/global settings from the root tree. Once back at Root,
            // restore its real exit target so BACK leaves settings normally.
            navigation_.settingsExitScreen = Screen::Performance;
        } else {
            navigation_.screen = navigation_.settingsExitScreen;
        }
    } else {
        navigation_.settingsPage = SettingsPage::Root;
    }

    if ((previousPage == SettingsPage::GrooveEditorMenu && navigation_.screen == Screen::GrooveEditor) ||
        (previousPage == SettingsPage::GrooveRecordMenu && navigation_.screen == Screen::GrooveRecorder)) {
        navigation_.cursor = 0U;
        navigation_.scrollOffset = 0U;
        navigation_.editing = false;
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::ChannelQuickSelect) {
        navigation_.cursor = navigation_.selectedChannel;
    } else if (previousPage == SettingsPage::Groove &&
               (navigation_.settingsPage == SettingsPage::ChannelTiming ||
                navigation_.settingsPage == SettingsPage::UnifiedTiming)) {
        navigation_.cursor = 4U;
    } else if (previousPage == SettingsPage::SequencerPattern &&
               navigation_.settingsPage == SettingsPage::Sequencer) {
        navigation_.cursor = 2U;
    } else if (navigation_.settingsPage == SettingsPage::InputAssignments &&
               previousPage == SettingsPage::Sync) {
        navigation_.cursor = 2U;
    } else if (navigation_.settingsPage == SettingsPage::General) {
        if (previousPage == SettingsPage::Master) {
            navigation_.cursor = 0U;
        } else if (previousPage == SettingsPage::InputAssignments) {
            navigation_.cursor = 1U;
        } else if (previousPage == SettingsPage::Screensaver) {
            navigation_.cursor = 2U;
        } else if (previousPage == SettingsPage::Diagnostics) {
            navigation_.cursor = 3U;
        } else if (previousPage == SettingsPage::Hardware) {
            navigation_.cursor = 4U;
        } else {
            navigation_.cursor = 0U;
        }
    } else if (navigation_.settingsPage == SettingsPage::Channel) {
        const ChannelMode mode = state_.channels[navigation_.selectedChannel].common.mode;
        if (previousPage == SettingsPage::ChannelTiming) {
            navigation_.cursor = channelMenuIndexForAction(mode, ChannelMenuAction::Timing);
        } else if (previousPage == SettingsPage::ChannelOutput) {
            navigation_.cursor = channelMenuIndexForAction(mode, ChannelMenuAction::Output);
        } else if (previousPage == SettingsPage::Clock) {
            navigation_.cursor = channelMenuIndexForAction(mode, ChannelMenuAction::Clock);
        } else if (previousPage == SettingsPage::Euclid) {
            navigation_.cursor = channelMenuIndexForAction(mode, ChannelMenuAction::Euclid);
        } else if (previousPage == SettingsPage::Sequencer) {
            navigation_.cursor = channelMenuIndexForAction(mode, ChannelMenuAction::Sequencer);
        } else {
            navigation_.cursor = 0U;
        }
    } else if (navigation_.settingsPage == SettingsPage::UnifiedClock) {
        navigation_.cursor = previousPage == SettingsPage::UnifiedOutput ? 2U : 1U;
    } else {
        navigation_.cursor = 0U;
    }
    navigation_.scrollOffset = 0U;
    navigation_.editing = false;
    normalizeScrollOffset();
    invalidate();
}

void UiController::confirmHighScoreClear(const std::uint32_t nowMs) {
    if (navigation_.cursor == 0U || leaderboard_ == nullptr) {
        openSettingsPage(SettingsPage::Root, navigation_.highScoreResetAvailable ? 4U : 0U);
        return;
    }

    // Flash erase/program stalls instruction fetch on STM32F4. Force STOP before
    // the immediate leaderboard commit so this maintenance action can never
    // disturb active musical timing.
    stopTransport(nowMs);
    (void)leaderboard_->clear();
    navigation_.highScoreResetAvailable = leaderboard_->hasPersistentRecord();
    openSettingsPage(SettingsPage::Root, navigation_.highScoreResetAvailable ? 4U : 0U);
}


void UiController::normalizeScrollOffset() {
    constexpr std::uint8_t kVisibleRows = 5U;
    const ChannelMode mode = state_.channels[navigation_.selectedChannel].common.mode;
    const std::uint8_t itemCount = settingsPageItemCount(
        navigation_.settingsPage, mode, navigation_.highScoreResetAvailable);

    if (navigation_.cursor < navigation_.scrollOffset) {
        navigation_.scrollOffset = navigation_.cursor;
    }
    if (navigation_.cursor >= navigation_.scrollOffset + kVisibleRows) {
        navigation_.scrollOffset = static_cast<std::uint8_t>(
            navigation_.cursor - (kVisibleRows - 1U));
    }
    if (itemCount <= kVisibleRows) {
        navigation_.scrollOffset = 0U;
    }
}

void UiController::applySelectedTemplate(const std::uint32_t nowMs) {
    (void)services::TemplateService::apply(navigation_.cursor, state_);
    engine_.updateConfiguration(state_, true);
    persistCurrentState(nowMs);
    navigation_.screen = Screen::Performance;
    invalidate();
}


}  // namespace clockfw::ui
