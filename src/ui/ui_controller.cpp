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
    services::PersistentStateService& persistentState,
    game::ArcadeLeaderboardStore* const leaderboard,
    const hal::ExternalInputCapture* const externalInputs,
    const hal::GateOutputDriver* const gateOutputs)
    : state_(state),
      engine_(engine),
      renderer_(renderer),
      persistentState_(persistentState),
      leaderboard_(leaderboard),
      externalInputs_(externalInputs),
      gateOutputs_(gateOutputs),
      settingsEditor_(state, engine) {}

UiController::UiController(
    ClockState& state,
    engine::ClockEngine& engine,
    UiRenderer& renderer,
    services::PersistentStateService& persistentState,
    services::CustomGrooveStore& customGrooveStore,
    game::ArcadeLeaderboardStore* const leaderboard,
    const hal::ExternalInputCapture* const externalInputs,
    const hal::GateOutputDriver* const gateOutputs)
    : state_(state),
      engine_(engine),
      renderer_(renderer),
      persistentState_(persistentState),
      customGrooveStore_(&customGrooveStore),
      leaderboard_(leaderboard),
      externalInputs_(externalInputs),
      gateOutputs_(gateOutputs),
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
        handleEncoderDelta(controls.encoderDelta, controls.tapButton.pressed, controls.transportButton.pressed);
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

const NavigationState& UiController::navigation() const {
    return navigation_;
}

void UiController::handleTransportButton(
    const hal::ButtonSample& button,
    const std::uint32_t nowMs) {
    if (navigation_.screen == Screen::GrooveEditor) {
        if (button.edge == hal::ButtonEdge::Released) {
            if (!transportPressConsumedByGrooveZoom_) {
                toggleTransport(nowMs);
            }
            transportPressConsumedByGrooveZoom_ = false;
        }
        return;
    }
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

    if (navigation_.screen == Screen::GrooveEditor) {
        navigation_.grooveCursor = static_cast<std::uint8_t>(
            (navigation_.grooveCursor + 1U) % navigation_.grooveDraft.length);
        invalidate();
    } else if (navigation_.screen == Screen::Performance) {
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
        openSettingsPage(SettingsPage::SequencerPattern);
    } else if (navigation_.screen == Screen::Templates) {
        openSettingsPage(SettingsPage::Preferences);
    } else if (navigation_.screen == Screen::PresetSlots) {
        openSettingsPage(SettingsPage::Preferences);
    } else if (navigation_.screen == Screen::GrooveEditor) {
        if (grooveEditorDirty_) {
            grooveLoadPendingAfterDiscard_ = false;
            navigation_.screen = Screen::GrooveDiscardConfirm;
            navigation_.cursor = 0U;
            invalidate();
        } else {
            leaveGrooveEditor(true);
        }
    } else if (navigation_.screen == Screen::GrooveSlots) {
        navigation_.screen = Screen::GrooveEditor;
        invalidate();
    } else if (navigation_.screen == Screen::GrooveNameEntry ||
               navigation_.screen == Screen::GrooveOverwriteConfirm) {
        navigation_.screen = Screen::GrooveSlots;
        navigation_.cursor = navigation_.selectedGrooveSlot;
        invalidate();
    } else if (navigation_.screen == Screen::GrooveDiscardConfirm) {
        navigation_.screen = Screen::GrooveEditor;
        invalidate();
    } else if (navigation_.screen == Screen::NameEntry ||
               navigation_.screen == Screen::OverwriteConfirm) {
        navigation_.screen = Screen::PresetSlots;
        invalidate();
    } else if (navigation_.screen == Screen::ModeChangeConfirm) {
        navigation_.screen = Screen::ModeSelect;
        navigation_.cursor = modeFunctionIndexForState(state_, navigation_.selectedChannel);
        invalidate();
    } else if (navigation_.screen == Screen::HighScoreClearConfirm) {
        openSettingsPage(SettingsPage::Root, navigation_.highScoreResetAvailable ? 4U : 0U);
    } else if (navigation_.screen == Screen::FactoryResetConfirm) {
        openSettingsPage(SettingsPage::Info, 5U);
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

void UiController::registerExternalTapTempo(const std::uint32_t timestampMs) {
    registerTapTempo(timestampMs);
}

void UiController::registerTapTempo(const std::uint32_t nowMs) {
    const std::uint32_t sequenceIntervalMs =
        services::TapTempo::maximumSequenceIntervalMs(state_.tempoRange.minimumBpm);
    const bool sequenceContinues =
        tapVisualSequenceActive_ && sequenceIntervalMs > 0U &&
        nowMs - lastTapVisualAtMs_ <= sequenceIntervalMs;

    lastTapVisualAtMs_ = nowMs;
    tapVisualSequenceActive_ = sequenceIntervalMs > 0U;
    if (sequenceContinues) {
        tapIndicatorStartedAtMs_ = nowMs;
        navigation_.tapIndicatorFrame = 1U;
        invalidate();
    } else {
        navigation_.tapIndicatorFrame = 0U;
    }

    const std::uint16_t estimatedBpm = tapTempo_.registerTap(
        nowMs, state_.tempoRange.minimumBpm, state_.tempoRange.maximumBpm);
    if (estimatedBpm == 0U) {
        return;
    }

    state_.bpm = estimatedBpm;
    // Tap Tempo updates only the internal/fallback BPM; SOURCE remains user-selected.
    engine_.updateConfiguration(state_, false);
    persistCurrentState(nowMs);
    invalidate();
}

void UiController::serviceTapTempoFeedback(const std::uint32_t nowMs) {
    const std::uint32_t sequenceIntervalMs =
        services::TapTempo::maximumSequenceIntervalMs(state_.tempoRange.minimumBpm);
    if (tapVisualSequenceActive_ &&
        (sequenceIntervalMs == 0U || nowMs - lastTapVisualAtMs_ > sequenceIntervalMs)) {
        tapVisualSequenceActive_ = false;
        tapTempo_.reset();
        if (navigation_.tapIndicatorFrame != 0U) {
            navigation_.tapIndicatorFrame = 0U;
            invalidate();
        }
    }

    if (navigation_.tapIndicatorFrame == 0U) {
        return;
    }

    const std::uint32_t elapsedMs = nowMs - tapIndicatorStartedAtMs_;
    const std::uint32_t frameIndex = elapsedMs / config::kTapIndicatorFrameDurationMs;
    const std::uint8_t nextFrame = frameIndex < config::kTapIndicatorFrameCount
        ? static_cast<std::uint8_t>(frameIndex + 1U)
        : 0U;
    if (nextFrame != navigation_.tapIndicatorFrame) {
        navigation_.tapIndicatorFrame = nextFrame;
        invalidate();
    }
}

int UiController::clampInt(const int value, const int minimum, const int maximum) {
    return std::max(minimum, std::min(value, maximum));
}

}  // namespace clockfw::ui
