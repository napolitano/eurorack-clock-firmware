/**
 * @file ui_controller_display.cpp
 * @brief STOP-mode display protection, screensaver timing, and wake-up handling.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/ui_controller.h"

#include "config.h"

namespace clockfw::ui {
namespace {

/** Converts an editable minute timeout to monotonic milliseconds. */
std::uint32_t minutesToMilliseconds(const std::uint8_t minutes) {
    return static_cast<std::uint32_t>(minutes) * 60'000UL;
}

}  // namespace

/** Restores the OLED to its active state and restarts the inactivity timer. */
void UiController::noteUserActivity(const std::uint32_t nowMs) {
    lastUserActivityAtMs_ = nowMs;
    activityClockInitialized_ = true;
    screensaverFrameIndex_ = 0U;
    lastScreensaverFrameAtMs_ = nowMs;

    if (displayPoweredOff_) {
        renderer_.setDisplayPower(true);
        displayPoweredOff_ = false;
    }
    if (displayDimmed_) {
        renderer_.setDisplayDimmed(false);
        displayDimmed_ = false;
    }
    if (screensaverActive_) {
        screensaverActive_ = false;
    }
    invalidate();
}

void UiController::serviceRendering(const std::uint32_t nowMs) {
    serviceTapTempoFeedback(nowMs);
    if (serviceStopModeDisplay(nowMs)) {
        return;
    }

    const engine::EngineSnapshot snapshot = engine_.snapshot();
    serviceGrooveRecorder(snapshot);

    if (navigation_.screen == Screen::Performance) {
        const ChannelMode mode = state_.operatingMode == OperatingMode::Independent
            ? state_.channels[navigation_.selectedChannel].common.mode
            : ChannelMode::Clock;
        const std::uint8_t currentStep = snapshot.channelStep[navigation_.selectedChannel];
        const bool playbackStepIsVisible =
            state_.operatingMode == OperatingMode::Independent &&
            (mode == ChannelMode::Euclid || mode == ChannelMode::Sequencer);

        const bool preCountStateChanged =
            snapshot.preCountActive != lastRenderedPreCountActive_ ||
            snapshot.preCountRemaining != lastRenderedPreCountRemaining_;

        // The Pre-Count popover is intentionally static between beat boundaries.
        // Redraw only when active/remaining state changes; this updates the number and
        // meter-progress cells and guarantees one final frame that clears the popover.
        if (!hasRenderedEngineStatus_ ||
            snapshot.externalLocked != lastRenderedExternalLocked_ ||
            (playbackStepIsVisible && currentStep != lastRenderedChannelStep_) ||
            preCountStateChanged) {
            invalidate();
        }
        lastRenderedExternalLocked_ = snapshot.externalLocked;
        lastRenderedChannelStep_ = currentStep;
        lastRenderedPreCountActive_ = snapshot.preCountActive;
        lastRenderedPreCountRemaining_ = snapshot.preCountRemaining;
        hasRenderedEngineStatus_ = true;
    }

    DiagnosticSnapshot diagnostics{};
    const bool diagnosticsVisible =
        navigation_.screen == Screen::Settings &&
        (navigation_.settingsPage == SettingsPage::DiagnosticsInputs ||
         navigation_.settingsPage == SettingsPage::DiagnosticsOutputs);
    if (diagnosticsVisible) {
        if (externalInputs_ != nullptr) {
            diagnostics.syncHigh = externalInputs_->syncLevelHigh();
            diagnostics.resetHigh = externalInputs_->resetLevelHigh();
        }
        if (gateOutputs_ != nullptr) {
            for (std::size_t index = 0U; index < diagnostics.outputs.size(); ++index) {
                diagnostics.outputs[index] = gateOutputs_->channelStateHigh(index);
            }
        }
        if (!hasRenderedDiagnosticSnapshot_ || diagnostics != lastDiagnosticSnapshot_) {
            invalidate();
        }
        lastDiagnosticSnapshot_ = diagnostics;
        hasRenderedDiagnosticSnapshot_ = true;
    } else {
        hasRenderedDiagnosticSnapshot_ = false;
    }

    if (!renderDirty_ || nowMs - lastRenderAtMs_ < config::kDisplayRefreshMinimumMs) {
        return;
    }

    renderer_.render(state_, navigation_, snapshot, diagnostics);
    lastRenderAtMs_ = nowMs;
    renderDirty_ = false;
}


/**
 * Applies STOP-mode screensaver, dimming, and panel power-off policy.
 *
 * @return true when normal UI rendering must remain suppressed for this cycle.
 */
bool UiController::serviceStopModeDisplay(const std::uint32_t nowMs) {
    if (externalInputs_ != nullptr) {
        const std::uint32_t activitySequence = externalInputs_->activitySequence();
        if (activitySequence != lastExternalInputActivitySequence_) {
            lastExternalInputActivitySequence_ = activitySequence;
            noteUserActivity(nowMs);
        }
    }

    if (!activityClockInitialized_) {
        lastUserActivityAtMs_ = nowMs;
        activityClockInitialized_ = true;
    }

    if (state_.transport != TransportState::Stopped) {
        if (displayPoweredOff_ || displayDimmed_ || screensaverActive_) {
            noteUserActivity(nowMs);
        }
        return false;
    }

    const std::uint32_t idleMs = nowMs - lastUserActivityAtMs_;
    const std::uint32_t screensaverAtMs =
        minutesToMilliseconds(state_.display.screensaverAfterMinutes);
    const std::uint32_t dimAtMs = minutesToMilliseconds(state_.display.dimAfterMinutes);
    const std::uint32_t offAtMs = minutesToMilliseconds(state_.display.offAfterMinutes);

    if (idleMs >= offAtMs) {
        if (!displayPoweredOff_) {
            renderer_.setDisplayPower(false);
            displayPoweredOff_ = true;
        }
        return true;
    }

    const bool shouldDim = idleMs >= dimAtMs;
    if (shouldDim != displayDimmed_) {
        renderer_.setDisplayDimmed(shouldDim);
        displayDimmed_ = shouldDim;
    }

    const bool shouldAnimate =
        state_.display.screensaverMode != ScreensaverMode::None &&
        idleMs >= screensaverAtMs;
    if (!shouldAnimate) {
        return false;
    }

    const bool firstFrame = !screensaverActive_;
    screensaverActive_ = true;
    if (firstFrame || nowMs - lastScreensaverFrameAtMs_ >= config::kScreensaverFrameIntervalMs) {
        renderer_.renderScreensaver(state_.display.screensaverMode, screensaverFrameIndex_++);
        lastScreensaverFrameAtMs_ = nowMs;
    }
    return true;
}

}  // namespace clockfw::ui
