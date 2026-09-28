/**
 * @file storybook_panel_presentation_tests.cpp
 * @brief Verifies Storybook physical front-panel presentation stays synchronized with real simulator actions.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <cmath>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "panel_led_visual.h"
#include "panel_layout.h"
#include "tutorial/panel_dynamic_layer.h"
#include "tutorial/panel_presentation.h"
#include "tutorial/story_parser.h"
#include "tutorial/story_runner.h"
#include "tutorial/story_simulator_port.h"

namespace {

using clockfw::sim::SimulatorRuntime;
using clockfw::sim::layout::PanelLayout;
using clockfw::sim::layout::Point;
using clockfw::sim::layout::makeDefaultPanelLayout;
using clockfw::sim::panelLedVisuallyLit;
using clockfw::sim::tutorial::PanelPresentationTimeline;
using clockfw::sim::tutorial::FocusTarget;
using clockfw::sim::tutorial::PatchMotion;
using clockfw::sim::tutorial::ScopeMode;
using clockfw::sim::tutorial::StoryParseResult;
using clockfw::sim::tutorial::StoryRunResult;
using clockfw::sim::tutorial::StoryRunner;
using clockfw::sim::tutorial::StorySimulatorPort;
using clockfw::sim::tutorial::StoryTraceEvent;
using clockfw::sim::tutorial::StoryTraceKind;
using clockfw::sim::tutorial::makePanelPresentationSnapshot;
using clockfw::sim::tutorial::parseStoryText;
using clockfw::sim::tutorial::renderPanelDynamicLayer;

bool require(const bool condition, const char* const message) {
    if (!condition) {
        std::cerr << "Storybook panel presentation failure: " << message << '\n';
        return false;
    }
    return true;
}

const StoryTraceEvent* findTrace(
    const StoryRunResult& run,
    const StoryTraceKind kind,
    const std::string& name,
    const std::string& value) {
    for (const StoryTraceEvent& event : run.trace) {
        if (event.kind == kind && event.name == name && event.value == value) {
            return &event;
        }
    }
    return nullptr;
}

bool samePoint(const Point& lhs, const Point& rhs) {
    return lhs.x == rhs.x && lhs.y == rhs.y;
}

}  // namespace

int main() {
    const std::filesystem::path sourceRoot = CLOCK_SOURCE_ROOT;
    const std::filesystem::path tutorialRoot = sourceRoot / "docs" / "tutorials";
    const std::filesystem::path statePath = ".clock-storybook-panel-presentation.bin";
    std::filesystem::remove(statePath);

    SimulatorRuntime runtime(statePath);
    runtime.begin();
    StorySimulatorPort port(runtime);
    PanelPresentationTimeline timeline;
    StoryRunner runner(port, tutorialRoot, &timeline);

    const StoryParseResult parsed = parseStoryText(R"YAML(
schema: 1
id: panel-presentation-contract
title: "Panel presentation contract"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_FAST
setup:
  factory_reset: true
  power: on
scenes:
  - tutorial:
      actions:
        - encoder:
            direction: clockwise
            detents: 1
        - encoder_push: {}
        - button:
            name: PLAY
        - button:
            name: TAP
        - button:
            name: STOP_BACK
        - sync_cable:
            state: connect
        - rst_cable:
            state: connect
        - scope:
            state: show
            channel: visible
        - focus:
            target: oled_top_bar
            label: "Top bar"
            duration_ms: 400
        - wait_ms: 100
)YAML");

    bool ok = true;
    ok &= require(static_cast<bool>(parsed), "physical-presentation story must parse");
    if (!parsed) return EXIT_FAILURE;
    const StoryRunResult run = runner.run(*parsed.story);
    ok &= require(static_cast<bool>(run), "physical-presentation story must execute");
    ok &= require(timeline.eventCount() >= 12U, "runner must emit one-way physical presentation events");
    ok &= require(runtime.encoderVisualPosition() == 1, "visible encoder must follow the accepted real simulator detent");
    ok &= require(runtime.syncInputTelemetry().cableConnected, "SYNC cable presentation must accompany a real connected cable");
    ok &= require(runtime.resetInputTelemetry().cableConnected, "RST cable presentation must accompany a real connected cable");

    const StoryTraceEvent* playDown = findTrace(run, StoryTraceKind::ControlState, "PLAY", "down");
    const StoryTraceEvent* playUp = findTrace(run, StoryTraceKind::ControlState, "PLAY", "up");
    ok &= require(playDown != nullptr && playUp != nullptr, "PLAY down/up trace events must exist");
    if (playDown != nullptr && playUp != nullptr) {
        ok &= require(timeline.stateAt(playDown->presentationUs).playPressed,
                      "PLAY must be visibly depressed when the real input is down");
        ok &= require(!timeline.stateAt(playUp->presentationUs).playPressed,
                      "PLAY must be visibly released when the real input is up");
    }

    const StoryTraceEvent* syncPatch = findTrace(run, StoryTraceKind::PatchState, "sync_cable", "connected");
    const StoryTraceEvent* resetPatch = findTrace(run, StoryTraceKind::PatchState, "rst_cable", "connected");
    ok &= require(syncPatch != nullptr && resetPatch != nullptr, "SYNC/RST patch traces must exist");
    if (syncPatch != nullptr) {
        const auto syncStart = timeline.stateAt(syncPatch->presentationUs);
        const auto syncMid = timeline.stateAt(syncPatch->presentationUs + 150000ULL);
        const auto syncEnd = timeline.stateAt(syncPatch->presentationUs + 300000ULL);
        ok &= require(syncStart.syncMotion == PatchMotion::Inserting && syncStart.syncInsertion == 0.0F,
                      "SYNC insertion animation must start with the real connect operation");
        ok &= require(syncMid.syncMotion == PatchMotion::Inserting &&
                      std::fabs(syncMid.syncInsertion - 0.5F) < 0.01F,
                      "SYNC insertion must progress deterministically through the HUMAN_FAST patch duration");
        ok &= require(syncEnd.syncMotion == PatchMotion::None && syncEnd.syncInsertion == 1.0F,
                      "SYNC insertion must settle fully connected");
    }
    if (resetPatch != nullptr) {
        const auto resetMid = timeline.stateAt(resetPatch->presentationUs + 150000ULL);
        ok &= require(resetMid.resetMotion == PatchMotion::Inserting &&
                      std::fabs(resetMid.resetInsertion - 0.5F) < 0.01F,
                      "RST insertion must use the same deterministic physical contract");
    }

    const StoryTraceEvent* focusEvent = findTrace(run, StoryTraceKind::Focus, "focus", "Top bar");
    ok &= require(focusEvent != nullptr, "manual focus trace must exist");
    if (focusEvent != nullptr) {
        const auto focused = timeline.stateAt(focusEvent->presentationUs + 200000ULL);
        ok &= require(focused.explicitFocus == FocusTarget::OledRegion && focused.focusWidth == 128 &&
                      focused.focusHeight == 12 && focused.focusLabel == "Top bar",
                      "manual OLED focus must remain presentation-only for its requested duration");
        ok &= require(timeline.stateAt(focusEvent->presentationUs + 400000ULL).explicitFocus == FocusTarget::None,
                      "manual focus must clear at the end of its presentation interval");
    }
    const StoryTraceEvent* encoderTrace = nullptr;
    for (const StoryTraceEvent& event : run.trace) if (event.kind == StoryTraceKind::EncoderDetent) { encoderTrace = &event; break; }
    if (encoderTrace != nullptr) {
        const auto automatic = timeline.stateAt(encoderTrace->presentationUs);
        ok &= require(automatic.automaticFocus == FocusTarget::Encoder && automatic.automaticFocusAgeUs == 0ULL,
                      "real encoder motion must automatically drive a visible focus target");
    }

    const StoryTraceEvent* scopeVisible = findTrace(run, StoryTraceKind::Scope, "scope", "visible_channel");
    ok &= require(scopeVisible != nullptr, "scope visibility trace must exist");
    if (scopeVisible != nullptr) {
        ok &= require(timeline.stateAt(scopeVisible->presentationUs).scopeMode == ScopeMode::VisibleChannel,
                      "visible-channel scope state must be presentation-only and scriptable");
    }

    PanelLayout layout = makeDefaultPanelLayout();
    layout.encoderCenter = {111.0F, 123.0F};
    layout.playButton.center = {222.0F, 234.0F};
    layout.syncInputCenter = {333.0F, 345.0F};
    layout.resetInputCenter = {444.0F, 456.0F};
    layout.ledCenters[0] = {555.0F, 567.0F};

    const auto finalPhysical = timeline.stateAt(run.presentationDurationUs);
    const auto snapshot = makePanelPresentationSnapshot(layout, runtime, finalPhysical, 1.0);
    ok &= require(samePoint(snapshot.encoder.center, layout.encoderCenter) &&
                  samePoint(snapshot.play.center, layout.playButton.center) &&
                  samePoint(snapshot.sync.center, layout.syncInputCenter) &&
                  samePoint(snapshot.reset.center, layout.resetInputCenter) &&
                  samePoint(snapshot.leds[0].center, layout.ledCenters[0]),
                  "tutorial presentation geometry must come exclusively from PanelLayout");
    ok &= require(snapshot.encoder.visualPosition == runtime.encoderVisualPosition(),
                  "encoder presentation must use real simulator visual-position telemetry");
    ok &= require(snapshot.sync.connected == runtime.syncInputTelemetry().cableConnected &&
                  snapshot.reset.connected == runtime.resetInputTelemetry().cableConnected,
                  "patch presentation must use real simulator cable state");

    for (std::size_t index = 0U; index < snapshot.leds.size(); ++index) {
        ok &= require(snapshot.leds[index].lit ==
                      panelLedVisuallyLit(runtime.telemetry()[index], runtime.nowMicroseconds(), 1.0),
                      "every tutorial LED must derive from the shared real gate-telemetry visual rule");
    }

    if (playDown != nullptr) {
        const auto pressedState = timeline.stateAt(playDown->presentationUs);
        const auto pressedSnapshot = makePanelPresentationSnapshot(layout, runtime, pressedState, 1.0);
        const auto layer = renderPanelDynamicLayer(layout, pressedSnapshot);
        const auto& centerPixel = layer.pixel(
            static_cast<std::size_t>(std::lround(layout.playButton.center.x)),
            static_cast<std::size_t>(std::lround(layout.playButton.center.y)));
        ok &= require(centerPixel.alpha > 0U,
                      "headless physical layer must visibly mark a depressed PLAY button at PanelLayout coordinates");
    }

    if (syncPatch != nullptr) {
        const auto patchState = timeline.stateAt(syncPatch->presentationUs + 150000ULL);
        const auto patchSnapshot = makePanelPresentationSnapshot(layout, runtime, patchState, 1.0);
        const auto layer = renderPanelDynamicLayer(layout, patchSnapshot);
        const auto& plugPixel = layer.pixel(
            static_cast<std::size_t>(std::lround(layout.syncInputCenter.x)),
            static_cast<std::size_t>(std::lround(layout.syncInputCenter.y)));
        ok &= require(plugPixel.alpha == 255U,
                      "headless physical layer must draw a visible SYNC plug during insertion");
    }

    const StoryParseResult powerStory = parseStoryText(R"YAML(
schema: 1
id: panel-power-contract
title: "Panel power contract"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_FAST
scenes:
  - tutorial:
      actions:
        - power:
            state: off
        - assert:
            power: off
)YAML");
    ok &= require(static_cast<bool>(powerStory), "POWER presentation story must parse");
    if (powerStory) {
        timeline.clear();
        const StoryRunResult powerRun = runner.run(*powerStory.story);
        ok &= require(static_cast<bool>(powerRun), "POWER OFF story must execute through the real simulator path");
        const auto powerPhysical = timeline.stateAt(powerRun.presentationDurationUs);
        const auto powerSnapshot = makePanelPresentationSnapshot(layout, runtime, powerPhysical, 1.0);
        ok &= require(!powerPhysical.recordedPowerOn && !powerSnapshot.poweredOn,
                      "recorded POWER OFF presentation must agree with real SimulatorRuntime power state");
    }

    runtime.flushPersistence();
    std::filesystem::remove(statePath);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
