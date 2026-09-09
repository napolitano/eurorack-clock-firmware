/**
 * @file ui_controller.cpp
 * @brief Control-flow and editing logic for the OLED user interface.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/ui_controller.h"

#include <algorithm>

#include "config.h"
#include "services/template_service.h"
#include "ui/menu_model.h"
#include "ui/mode_functions.h"

namespace clockfw::ui {
namespace {

/** Returns true when one debounced front-panel sample represents user activity. */
bool hasUserActivity(const hal::ControlSample& controls) {
    return controls.encoderDelta != 0 ||
        controls.encoderButton.edge != hal::ButtonEdge::None ||
        controls.transportButton.edge != hal::ButtonEdge::None ||
        controls.tapButton.edge != hal::ButtonEdge::None ||
        controls.resetButton.edge != hal::ButtonEdge::None;
}

}  // namespace

UiController::UiController(
    ClockState& state,
    engine::ClockEngine& engine,
    UiRenderer& renderer,
    services::PersistentStateService& persistentState)
    : state_(state),
      engine_(engine),
      renderer_(renderer),
      persistentState_(persistentState),
      settingsEditor_(state, engine) {}

void UiController::invalidate() {
    renderDirty_ = true;
}

void UiController::processControls(
    const hal::ControlSample& controls,
    const std::uint32_t nowMs) {
    if (!activityClockInitialized_) {
        lastUserActivityAtMs_ = nowMs;
        activityClockInitialized_ = true;
    }
    if (hasUserActivity(controls)) {
        noteUserActivity(nowMs);
    }

    const bool settingsChordPressed =
        navigation_.screen == Screen::Performance &&
        controls.tapButton.pressed &&
        controls.encoderButton.edge == hal::ButtonEdge::Pressed;

    if (settingsChordPressed) {
        // TAP + encoder push is the explicit entry point for the combined
        // settings tree. A plain push remains dedicated to channel overview.
        settingsChordActive_ = true;
        navigation_.settingsExitScreen = Screen::Performance;
        openSettingsPage(SettingsPage::Root);
    }

    if (controls.encoderDelta != 0 && !settingsChordActive_) {
        handleEncoderDelta(controls.encoderDelta, controls.tapButton.pressed);
        persistCurrentState(nowMs);
    }

    handleEncoderButton(controls.encoderButton, nowMs);
    handleTransportButton(controls.transportButton, nowMs);
    handleTapButton(controls.tapButton, nowMs);
    handleResetButton(controls.resetButton, nowMs);

    if (settingsChordActive_ &&
        !controls.tapButton.pressed &&
        !controls.encoderButton.pressed) {
        settingsChordActive_ = false;
    }
}

void UiController::serviceRendering(const std::uint32_t nowMs) {
    if (serviceStopModeDisplay(nowMs)) {
        return;
    }

    const engine::EngineSnapshot snapshot = engine_.snapshot();

    if (navigation_.screen == Screen::Performance) {
        const ChannelMode mode = state_.operatingMode == OperatingMode::Independent
            ? state_.channels[navigation_.selectedChannel].common.mode
            : ChannelMode::Clock;
        const std::uint8_t currentStep = snapshot.channelStep[navigation_.selectedChannel];
        const bool playbackStepIsVisible =
            state_.operatingMode == OperatingMode::Independent &&
            (mode == ChannelMode::Euclid || mode == ChannelMode::Sequencer);

        if (!hasRenderedEngineStatus_ ||
            snapshot.externalLocked != lastRenderedExternalLocked_ ||
            (playbackStepIsVisible && currentStep != lastRenderedChannelStep_)) {
            invalidate();
        }
        lastRenderedExternalLocked_ = snapshot.externalLocked;
        lastRenderedChannelStep_ = currentStep;
        hasRenderedEngineStatus_ = true;
    }

    if (!renderDirty_ || nowMs - lastRenderAtMs_ < config::kDisplayRefreshMinimumMs) {
        return;
    }

    renderer_.render(state_, navigation_, snapshot);
    lastRenderAtMs_ = nowMs;
    renderDirty_ = false;
}

const NavigationState& UiController::navigation() const {
    return navigation_;
}

void UiController::handleEncoderDelta(
    const std::int8_t delta,
    const bool tapPressed) {
    if (navigation_.screen == Screen::ChannelQuickSelect && tapPressed) {
        if (!modeTapTurnActive_) {
            if (state_.operatingMode == OperatingMode::Independent) {
                navigation_.selectedChannel = navigation_.cursor;
            }
            openModeSelect(Screen::ChannelQuickSelect);
            modeTapTurnActive_ = true;
        }
        const int optionCount = static_cast<int>(kModeFunctions.size());
        navigation_.cursor = static_cast<std::uint8_t>(
            (static_cast<int>(navigation_.cursor) + optionCount + delta) % optionCount);
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::ChannelQuickSelect) {
        if (state_.operatingMode == OperatingMode::Independent) {
            navigation_.cursor = static_cast<std::uint8_t>(
                (static_cast<int>(navigation_.cursor) + static_cast<int>(kChannelCount) + delta) %
                static_cast<int>(kChannelCount));
            invalidate();
        }
        return;
    }

    if (navigation_.screen == Screen::Performance) {
        settingsEditor_.changeMasterTempo(delta);
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::ModeSelect) {
        const int optionCount = static_cast<int>(kModeFunctions.size());
        navigation_.cursor = static_cast<std::uint8_t>(
            (static_cast<int>(navigation_.cursor) + optionCount + delta) % optionCount);
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::Templates) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta,
            0,
            static_cast<int>(services::TemplateService::kTemplateCount - 1U)));
        if (navigation_.cursor < navigation_.scrollOffset) {
            navigation_.scrollOffset = navigation_.cursor;
        }
        if (navigation_.cursor >= navigation_.scrollOffset + 5U) {
            navigation_.scrollOffset = static_cast<std::uint8_t>(navigation_.cursor - 4U);
        }
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::PresetSlots) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta,
            0,
            static_cast<int>(services::PersistentStateService::kUserPresetSlotCount - 1U)));
        if (navigation_.cursor < navigation_.scrollOffset) {
            navigation_.scrollOffset = navigation_.cursor;
        }
        if (navigation_.cursor >= navigation_.scrollOffset + 5U) {
            navigation_.scrollOffset = static_cast<std::uint8_t>(navigation_.cursor - 4U);
        }
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::NameEntry) {
        adjustPresetNameCharacter(delta);
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::OverwriteConfirm) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta, 0, 1));
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::ModeChangeConfirm) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta, 0, 1));
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::Settings) {
        if (navigation_.editing) {
            settingsEditor_.adjust(
                navigation_.settingsPage,
                navigation_.cursor,
                navigation_.selectedChannel,
                delta);
            invalidate();
        } else {
            navigation_.cursor = static_cast<std::uint8_t>(clampInt(
                static_cast<int>(navigation_.cursor) + delta,
                0,
                static_cast<int>(settingsPageItemCount(
                    navigation_.settingsPage,
                    state_.channels[navigation_.selectedChannel].common.mode)) - 1));
            normalizeScrollOffset();
            invalidate();
        }
        return;
    }

    if (navigation_.screen == Screen::SequencerEditor) {
        const SequencerSettings& sequencer = state_.channels[navigation_.selectedChannel].sequencer;
        int absoluteStep = navigation_.sequencerPage * 16 + navigation_.sequencerCursor + delta;
        absoluteStep = clampInt(absoluteStep, 0, static_cast<int>(sequencer.length) - 1);
        navigation_.sequencerPage = static_cast<std::uint8_t>(absoluteStep / 16);
        navigation_.sequencerCursor = static_cast<std::uint8_t>(absoluteStep % 16);
        invalidate();
    }
}

void UiController::handleTransportButton(
    const hal::ButtonSample& button,
    const std::uint32_t nowMs) {
    if (button.edge != hal::ButtonEdge::Pressed) {
        return;
    }

    if (navigation_.screen == Screen::Performance) {
        toggleTransport(nowMs);
    } else if (navigation_.screen == Screen::SequencerEditor && navigation_.sequencerPage < 3U) {
        ++navigation_.sequencerPage;
        navigation_.sequencerCursor = 0U;
        invalidate();
    }
}

void UiController::handleTapButton(
    const hal::ButtonSample& button,
    const std::uint32_t nowMs) {
    if (button.edge != hal::ButtonEdge::Released) {
        return;
    }

    if (modeTapTurnActive_ && navigation_.screen == Screen::ModeSelect) {
        modeTapTurnActive_ = false;
        commitSelectedMode(nowMs);
        return;
    }

    if (settingsChordActive_) {
        return;
    }

    if (navigation_.screen == Screen::Performance) {
        registerTapTempo(nowMs);
    } else if (navigation_.screen == Screen::SequencerEditor && navigation_.sequencerPage > 0U) {
        --navigation_.sequencerPage;
        navigation_.sequencerCursor = 0U;
        invalidate();
    }
}

void UiController::handleResetButton(
    const hal::ButtonSample& button,
    const std::uint32_t nowMs) {
    if (button.edge != hal::ButtonEdge::Pressed) {
        return;
    }

    if (navigation_.screen == Screen::Performance) {
        stopTransport(nowMs);
    } else if (navigation_.screen == Screen::Settings) {
        backFromSettings();
    } else if (navigation_.screen == Screen::SequencerEditor) {
        openSettingsPage(SettingsPage::Sequencer);
    } else if (navigation_.screen == Screen::Templates) {
        openSettingsPage(SettingsPage::Preferences);
    } else if (navigation_.screen == Screen::PresetSlots) {
        openSettingsPage(SettingsPage::Preferences);
    } else if (navigation_.screen == Screen::NameEntry ||
               navigation_.screen == Screen::OverwriteConfirm) {
        navigation_.screen = Screen::PresetSlots;
        invalidate();
    } else if (navigation_.screen == Screen::ModeChangeConfirm) {
        navigation_.screen = Screen::ModeSelect;
        navigation_.cursor = modeFunctionIndexForState(state_, navigation_.selectedChannel);
        invalidate();
    } else if (navigation_.screen == Screen::ModeSelect) {
        modeTapTurnActive_ = false;
        navigation_.screen = navigation_.modeSelectReturnScreen;
        invalidate();
    } else if (navigation_.screen == Screen::ChannelQuickSelect) {
        navigation_.screen = Screen::Performance;
        invalidate();
    } else {
        navigation_.screen = Screen::Performance;
        invalidate();
    }
}

void UiController::toggleTransport(const std::uint32_t nowMs) {
    if (state_.transport == TransportState::Playing) {
        state_.transport = TransportState::Paused;
        engine_.pause();
    } else {
        state_.transport = TransportState::Playing;
        engine_.play();
    }
    persistentState_.requestTransportState(state_.transport, nowMs);
    persistCurrentState(nowMs);
    invalidate();
}

void UiController::stopTransport(const std::uint32_t nowMs) {
    state_.transport = TransportState::Stopped;
    engine_.stop();
    persistentState_.requestTransportState(state_.transport, nowMs);
    persistCurrentState(nowMs);
    invalidate();
}

void UiController::registerTapTempo(const std::uint32_t nowMs) {
    const std::uint16_t estimatedBpm = tapTempo_.registerTap(nowMs, state_.tempoRange.minimumBpm, state_.tempoRange.maximumBpm);
    if (estimatedBpm == 0U) {
        return;
    }

    state_.bpm = estimatedBpm;
    state_.source = ClockSource::Internal;
    engine_.updateConfiguration(state_, false);
    persistCurrentState(nowMs);
    invalidate();
}

int UiController::clampInt(const int value, const int minimum, const int maximum) {
    return std::max(minimum, std::min(value, maximum));
}

}  // namespace clockfw::ui
