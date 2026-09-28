/**
 * @file panel_presentation.cpp
 * @brief Implements deterministic front-panel physical presentation without duplicating CLOCK behaviour.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/panel_presentation.h"

#include <algorithm>

#include "panel_led_visual.h"

namespace clockfw::sim::tutorial {
namespace {

float clampUnit(const double value) {
    return static_cast<float>(std::clamp(value, 0.0, 1.0));
}

void applyControlState(
    PhysicalPresentationState& state,
    const ModuleControl control,
    const bool pressed) {
    switch (control) {
        case ModuleControl::EncoderPush:
            state.encoderPressed = pressed;
            return;
        case ModuleControl::Play:
            state.playPressed = pressed;
            return;
        case ModuleControl::Tap:
            state.tapPressed = pressed;
            return;
        case ModuleControl::StopBack:
            state.stopPressed = pressed;
            return;
        case ModuleControl::Encoder:
        case ModuleControl::Power:
            return;
    }
}

}  // namespace

void PanelPresentationTimeline::clear() {
    events_.clear();
}

std::size_t PanelPresentationTimeline::eventCount() const {
    return events_.size();
}

PhysicalPresentationState PanelPresentationTimeline::stateAt(const std::uint64_t presentationUs) const {
    PhysicalPresentationState state{};
    struct ActivePatch final {
        bool active = false;
        bool targetConnected = false;
        std::uint64_t startUs = 0ULL;
        std::uint64_t durationUs = 0ULL;
        bool settledConnected = false;
    } syncPatch, resetPatch;

    for (const Event& event : events_) {
        if (event.presentationUs > presentationUs) {
            break;
        }
        switch (event.kind) {
            case EventKind::Encoder:
                state.encoderDetentDelta += static_cast<std::int64_t>(event.direction);
                break;
            case EventKind::Control:
                applyControlState(state, event.control, event.state);
                break;
            case EventKind::Power:
                state.recordedPowerOn = event.state;
                break;
            case EventKind::PatchMotion: {
                ActivePatch& patch = event.patch == PatchAction::SyncCable ? syncPatch : resetPatch;
                patch.active = true;
                patch.targetConnected = event.state;
                patch.startUs = event.presentationUs;
                patch.durationUs = event.durationUs;
                break;
            }
            case EventKind::PatchSettled: {
                ActivePatch& patch = event.patch == PatchAction::SyncCable ? syncPatch : resetPatch;
                patch.active = false;
                patch.settledConnected = event.state;
                break;
            }
            case EventKind::Scope:
                state.scopeMode = event.scope;
                break;
        }
    }

    const auto resolvePatch = [presentationUs](
        const ActivePatch& patch,
        PatchMotion& motion,
        float& insertion) {
        if (!patch.active) {
            motion = PatchMotion::None;
            insertion = patch.settledConnected ? 1.0F : 0.0F;
            return;
        }
        const double progress = patch.durationUs == 0ULL
            ? 1.0
            : static_cast<double>(presentationUs - patch.startUs) /
                static_cast<double>(patch.durationUs);
        const float clamped = clampUnit(progress);
        if (patch.targetConnected) {
            motion = PatchMotion::Inserting;
            insertion = clamped;
        } else {
            motion = PatchMotion::Removing;
            insertion = 1.0F - clamped;
        }
    };
    resolvePatch(syncPatch, state.syncMotion, state.syncInsertion);
    resolvePatch(resetPatch, state.resetMotion, state.resetInsertion);
    return state;
}

void PanelPresentationTimeline::onEncoderDetent(
    const std::uint64_t presentationUs,
    const int direction) {
    events_.push_back({EventKind::Encoder, presentationUs, ModuleControl::Encoder,
                       PatchAction::SyncCable, ScopeMode::Hidden, direction, false, 0ULL});
}

void PanelPresentationTimeline::onControlState(
    const std::uint64_t presentationUs,
    const ModuleControl control,
    const bool pressed) {
    events_.push_back({EventKind::Control, presentationUs, control,
                       PatchAction::SyncCable, ScopeMode::Hidden, 0, pressed, 0ULL});
}

void PanelPresentationTimeline::onPowerState(
    const std::uint64_t presentationUs,
    const bool poweredOn) {
    events_.push_back({EventKind::Power, presentationUs, ModuleControl::Power,
                       PatchAction::SyncCable, ScopeMode::Hidden, 0, poweredOn, 0ULL});
}

void PanelPresentationTimeline::onPatchMotion(
    const std::uint64_t presentationUs,
    const PatchAction patch,
    const bool targetConnected,
    const std::uint64_t durationUs) {
    events_.push_back({EventKind::PatchMotion, presentationUs, ModuleControl::Encoder,
                       patch, ScopeMode::Hidden, 0, targetConnected, durationUs});
}

void PanelPresentationTimeline::onPatchSettled(
    const std::uint64_t presentationUs,
    const PatchAction patch,
    const bool connected) {
    events_.push_back({EventKind::PatchSettled, presentationUs, ModuleControl::Encoder,
                       patch, ScopeMode::Hidden, 0, connected, 0ULL});
}

void PanelPresentationTimeline::onScopeState(
    const std::uint64_t presentationUs,
    const ScopeMode mode) {
    events_.push_back({EventKind::Scope, presentationUs, ModuleControl::Encoder,
                       PatchAction::SyncCable, mode, 0, false, 0ULL});
}

PanelPresentationSnapshot makePanelPresentationSnapshot(
    const layout::PanelLayout& panelLayout,
    const SimulatorRuntime& runtime,
    const PhysicalPresentationState& physicalState,
    const double speedMultiplier) {
    PanelPresentationSnapshot snapshot{};
    snapshot.poweredOn = runtime.poweredOn();
    snapshot.encoder = {
        panelLayout.encoderCenter,
        panelLayout.encoderRadius,
        runtime.encoderVisualPosition(),
        physicalState.encoderPressed};
    snapshot.play = {panelLayout.playButton.center, panelLayout.playButton.radius, physicalState.playPressed};
    snapshot.tap = {panelLayout.tapButton.center, panelLayout.tapButton.radius, physicalState.tapPressed};
    snapshot.stop = {panelLayout.stopButton.center, panelLayout.stopButton.radius, physicalState.stopPressed};

    const SyncInputTelemetry sync = runtime.syncInputTelemetry();
    snapshot.sync = {
        panelLayout.syncInputCenter,
        panelLayout.jackGeometry(panelLayout.syncJackType),
        sync.cableConnected,
        sync.signalHigh,
        physicalState.syncMotion,
        physicalState.syncMotion == PatchMotion::None
            ? (sync.cableConnected ? 1.0F : 0.0F)
            : physicalState.syncInsertion};

    const ResetInputTelemetry reset = runtime.resetInputTelemetry();
    snapshot.reset = {
        panelLayout.resetInputCenter,
        panelLayout.jackGeometry(panelLayout.resetJackType),
        reset.cableConnected,
        reset.signalHigh,
        physicalState.resetMotion,
        physicalState.resetMotion == PatchMotion::None
            ? (reset.cableConnected ? 1.0F : 0.0F)
            : physicalState.resetInsertion};

    const auto& channels = runtime.telemetry();
    const std::uint64_t nowUs = runtime.nowMicroseconds();
    for (std::size_t index = 0U; index < snapshot.leds.size(); ++index) {
        const bool lit = panelLedVisuallyLit(channels[index], nowUs, speedMultiplier);
        snapshot.leds[index] = {
            panelLayout.ledCenters[index],
            panelLayout.ledRadius,
            lit ? panelLayout.ledOnColor : panelLayout.ledOffColor,
            lit};
    }
    snapshot.scopeMode = physicalState.scopeMode;
    return snapshot;
}

}  // namespace clockfw::sim::tutorial
