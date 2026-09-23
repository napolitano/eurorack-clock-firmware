/**
 * @file ui_controller_groove_record.cpp
 * @brief Live TAP-driven Custom Groove recorder control flow.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/ui_controller.h"

#include <cstdint>

namespace clockfw::ui {
namespace {

GrooveSettings& activeRecordGrooveSettings(ClockState& state, const std::uint8_t channel) {
    return state.operatingMode == OperatingMode::UnifiedClock
        ? state.unifiedClock.groove
        : state.channels[channel].common.groove;
}

std::uint64_t grooveRecordLengthMask(const std::uint8_t length) {
    if (length >= 64U) return UINT64_MAX;
    return length == 0U ? 0U : (1ULL << length) - 1ULL;
}

services::GrooveRecordMode serviceRecordMode(const GrooveRecordMode mode) {
    return mode == GrooveRecordMode::OneShot
        ? services::GrooveRecordMode::OneShot
        : services::GrooveRecordMode::Endless;
}

GrooveRecordState uiRecordState(const services::GrooveRecordState state) {
    switch (state) {
        case services::GrooveRecordState::PreCount: return GrooveRecordState::PreCount;
        case services::GrooveRecordState::Recording: return GrooveRecordState::Recording;
        case services::GrooveRecordState::Ready:
        default: return GrooveRecordState::Ready;
    }
}

}  // namespace

void UiController::openGrooveRecorder() {
    grooveEditorOriginalSettings_ = activeRecordGrooveSettings(state_, navigation_.selectedChannel);
    navigation_.grooveDraft = CustomGroovePattern{};
    bool loadedExisting = false;
    if (customGrooveStore_ != nullptr &&
        grooveEditorOriginalSettings_.preset == GroovePreset::Custom &&
        grooveEditorOriginalSettings_.customSlot < kCustomGrooveSlotCount) {
        loadedExisting = customGrooveStore_->load(
            grooveEditorOriginalSettings_.customSlot, navigation_.grooveDraft);
    }
    if (!isCustomGroovePatternValid(navigation_.grooveDraft)) {
        navigation_.grooveDraft = CustomGroovePattern{};
        loadedExisting = false;
    }
    navigation_.grooveCursor = 0U;
    navigation_.grooveZoomSteps = 0U;
    navigation_.grooveRecordState = GrooveRecordState::Ready;
    navigation_.grooveRecordPlayheadStep = 0U;
    navigation_.grooveRecordPlayheadPhase256 = 0U;
    navigation_.grooveRecordCountInRemaining = navigation_.grooveRecordCountInBeats;
    navigation_.grooveRecordCapturedMask = loadedExisting
        ? grooveRecordLengthMask(navigation_.grooveDraft.length)
        : 0U;
    grooveRecorder_.reset(
        serviceRecordMode(navigation_.grooveRecordMode),
        navigation_.grooveRecordCountInBeats,
        navigation_.grooveRecordCapturedMask);
    navigation_.screen = Screen::GrooveRecorder;
    grooveWorkspaceScreen_ = Screen::GrooveRecorder;
    navigation_.editing = false;
    grooveEditorDirty_ = false;
    grooveLoadPendingAfterDiscard_ = false;
    transportPressConsumedByGrooveZoom_ = false;
    updateGroovePreview();
    invalidate();
}

void UiController::toggleGrooveRecording(const std::uint32_t nowMs) {
    const services::GrooveRecorderView view = grooveRecorder_.view();
    if (view.state != services::GrooveRecordState::Ready) {
        grooveRecorder_.stop();
        navigation_.grooveRecordState = GrooveRecordState::Ready;
        navigation_.grooveRecordPlayheadStep = 0U;
        navigation_.grooveRecordPlayheadPhase256 = 0U;
        navigation_.grooveCursor = 0U;
        invalidate();
        return;
    }

    // Groove Record needs a moving musical timeline. Starting from STOP is the
    // only case where the recorder owns a transport transition; stopping the
    // recorder later does not pause the user's clock.
    if (state_.transport != TransportState::Playing) {
        state_.transport = TransportState::Playing;
        engine_.play();
        persistentState_.requestTransportState(state_.transport, nowMs);
        persistCurrentState(nowMs);
    }

    const std::size_t timingChannel = state_.operatingMode == OperatingMode::UnifiedClock
        ? 0U
        : navigation_.selectedChannel;
    grooveRecorder_.setMode(serviceRecordMode(navigation_.grooveRecordMode));
    grooveRecorder_.setCountInBeats(navigation_.grooveRecordCountInBeats);
    grooveRecorder_.start(
        engine_.snapshot().masterPositionQ32,
        engine_.nominalIntervalQ32(timingChannel),
        navigation_.grooveDraft.length);
    const services::GrooveRecorderView started = grooveRecorder_.view();
    navigation_.grooveRecordState = uiRecordState(started.state);
    navigation_.grooveRecordCountInRemaining = started.countInRemaining;
    navigation_.grooveRecordPlayheadStep = started.playheadStep;
    navigation_.grooveRecordPlayheadPhase256 = started.playheadPhase256;
    navigation_.grooveRecordCapturedMask = started.capturedMask;
    invalidate();
}

void UiController::serviceGrooveRecorder(const engine::EngineSnapshot& snapshot) {
    if (navigation_.screen != Screen::GrooveRecorder) return;
    const bool changed = grooveRecorder_.service(
        snapshot.masterPositionQ32, navigation_.grooveDraft.length);
    const services::GrooveRecorderView view = grooveRecorder_.view();
    navigation_.grooveRecordState = uiRecordState(view.state);
    navigation_.grooveRecordCountInRemaining = view.countInRemaining;
    navigation_.grooveRecordPlayheadStep = view.playheadStep;
    navigation_.grooveRecordPlayheadPhase256 = view.playheadPhase256;
    navigation_.grooveRecordCapturedMask = view.capturedMask;
    navigation_.grooveCursor = view.playheadStep;
    if (changed) invalidate();
}

void UiController::captureGrooveTap(
    const hal::ButtonSample& button,
    const std::uint32_t nowUs) {
    const services::GrooveRecorderView before = grooveRecorder_.view();
    if (before.state != services::GrooveRecordState::Recording) return;

    const std::uint32_t physicalTapUs = button.edgeTimestampUs != 0U
        ? button.edgeTimestampUs
        : nowUs;
    const std::uint64_t tapPositionQ32 = engine_.estimateMasterPositionAtUs(
        physicalTapUs, nowUs);
    if (!grooveRecorder_.capture(tapPositionQ32, navigation_.grooveDraft)) return;

    const services::GrooveRecorderView after = grooveRecorder_.view();
    navigation_.grooveRecordCapturedMask = after.capturedMask;
    navigation_.grooveRecordPlayheadStep = after.playheadStep;
    navigation_.grooveRecordPlayheadPhase256 = after.playheadPhase256;
    navigation_.grooveCursor = after.playheadStep;
    grooveEditorDirty_ = true;
    updateGroovePreview();
    invalidate();
}

void UiController::clearGrooveRecording() {
    navigation_.grooveDraft.offsets256.fill(0);
    grooveRecorder_.clearCaptured();
    navigation_.grooveRecordCapturedMask = 0U;
    navigation_.grooveRecordPlayheadStep = 0U;
    navigation_.grooveRecordPlayheadPhase256 = 0U;
    navigation_.grooveCursor = 0U;
    grooveEditorDirty_ = true;
    updateGroovePreview();
    navigation_.screen = Screen::GrooveRecorder;
    navigation_.editing = false;
    invalidate();
}


}  // namespace clockfw::ui
