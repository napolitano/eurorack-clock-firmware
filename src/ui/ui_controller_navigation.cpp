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
#include "ui/mode_functions.h"

namespace clockfw::ui {

void UiController::openSettingsPage(
    const SettingsPage page,
    const std::uint8_t initialCursor) {
    navigation_.settingsPage = page;
    const ChannelMode mode = state_.channels[navigation_.selectedChannel].common.mode;
    const std::uint8_t itemCount = settingsPageItemCount(page, mode);
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
    if (navigation_.settingsPage == SettingsPage::Root) {
        navigation_.screen = navigation_.settingsExitScreen;
    } else if (navigation_.settingsPage == SettingsPage::General ||
               navigation_.settingsPage == SettingsPage::Preferences ||
               navigation_.settingsPage == SettingsPage::Info) {
        navigation_.settingsPage = SettingsPage::Root;
    } else if (navigation_.settingsPage == SettingsPage::Master ||
               navigation_.settingsPage == SettingsPage::Sync ||
               navigation_.settingsPage == SettingsPage::Screensaver) {
        navigation_.settingsPage = SettingsPage::General;
    } else if (navigation_.settingsPage == SettingsPage::Licenses ||
               navigation_.settingsPage == SettingsPage::Updates) {
        navigation_.settingsPage = SettingsPage::Info;
    } else if (navigation_.settingsPage == SettingsPage::Rate ||
               navigation_.settingsPage == SettingsPage::Clock ||
               navigation_.settingsPage == SettingsPage::Euclid ||
               navigation_.settingsPage == SettingsPage::Sequencer) {
        navigation_.settingsPage = SettingsPage::Channel;
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

    navigation_.cursor = navigation_.screen == Screen::ChannelQuickSelect
        ? navigation_.selectedChannel
        : 0U;
    navigation_.scrollOffset = 0U;
    navigation_.editing = false;
    invalidate();
}

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
            openSettingsPage(SettingsPage::Sync);
        } else {
            openSettingsPage(SettingsPage::Screensaver);
        }
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Info) {
        if (navigation_.cursor == 3U) {
            openSettingsPage(SettingsPage::Licenses);
        } else if (navigation_.cursor == 4U) {
            openSettingsPage(SettingsPage::Updates);
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
        if (navigation_.cursor == 0U) {
            openModeSelect(Screen::Settings);
        } else if (navigation_.cursor == 1U) {
            openSettingsPage(SettingsPage::Rate);
        } else if (navigation_.cursor == 2U) {
            if (channel.common.mode == ChannelMode::Clock) {
                openSettingsPage(SettingsPage::Clock);
            } else if (channel.common.mode == ChannelMode::Euclid) {
                openSettingsPage(SettingsPage::Euclid);
            } else if (channel.common.mode == ChannelMode::Sequencer) {
                openSettingsPage(SettingsPage::Sequencer);
            }
            // OFF has no mode-specific parameters. Keep the user on the common
            // channel page rather than opening an unrelated editor.
        } else {
            navigation_.editing = !navigation_.editing;
            invalidate();
        }
        return;
    }

    if ((navigation_.settingsPage == SettingsPage::UnifiedClock ||
         navigation_.settingsPage == SettingsPage::DividerBank) &&
        navigation_.cursor == 0U) {
        openModeSelect(Screen::Settings);
        return;
    }

    if (navigation_.settingsPage == SettingsPage::UnifiedClock ||
        navigation_.settingsPage == SettingsPage::DividerBank) {
        navigation_.editing = !navigation_.editing;
        invalidate();
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Sequencer && navigation_.cursor == 0U) {
        navigation_.screen = Screen::SequencerEditor;
        navigation_.sequencerPage = 0U;
        navigation_.sequencerCursor = 0U;
        invalidate();
        return;
    }

    if (navigation_.settingsPage == SettingsPage::Sequencer && navigation_.cursor >= 3U) {
        if (settingsEditor_.executeSequencerCommand(
                navigation_.selectedChannel, navigation_.cursor)) {
            invalidate();
        }
        return;
    }

    if (navigation_.settingsPage != SettingsPage::Info &&
        navigation_.settingsPage != SettingsPage::Licenses &&
        navigation_.settingsPage != SettingsPage::Updates &&
        navigation_.settingsPage != SettingsPage::General) {
        navigation_.editing = !navigation_.editing;
        invalidate();
    }
}


void UiController::normalizeScrollOffset() {
    constexpr std::uint8_t visibleRows = 5U;
    const ChannelMode mode = state_.channels[navigation_.selectedChannel].common.mode;
    const std::uint8_t itemCount = settingsPageItemCount(navigation_.settingsPage, mode);
    if (navigation_.cursor < navigation_.scrollOffset) {
        navigation_.scrollOffset = navigation_.cursor;
    }
    if (navigation_.cursor >= navigation_.scrollOffset + visibleRows) {
        navigation_.scrollOffset = static_cast<std::uint8_t>(navigation_.cursor - (visibleRows - 1U));
    }
    if (itemCount <= visibleRows) {
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
