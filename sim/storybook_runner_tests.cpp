/**
 * @file storybook_runner_tests.cpp
 * @brief Exercises deterministic Storybook execution timing against the real simulator runtime.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "domain/clock_types.h"
#include "tutorial/interaction_profile_loader.h"
#include "tutorial/story_parser.h"
#include "tutorial/story_runner.h"
#include "tutorial/story_simulator_port.h"
#include "tutorial/story_validator.h"

namespace {

using clockfw::ClockSource;
using clockfw::TransportState;
using clockfw::sim::SimulatorRuntime;
using clockfw::sim::tutorial::InteractionProfile;
using clockfw::sim::tutorial::Story;
using clockfw::sim::tutorial::StoryParseResult;
using clockfw::sim::tutorial::StoryRunResult;
using clockfw::sim::tutorial::StoryRunner;
using clockfw::sim::tutorial::StorySimulatorPort;
using clockfw::sim::tutorial::StoryTraceEvent;
using clockfw::sim::tutorial::StoryTraceKind;
using clockfw::sim::tutorial::loadInteractionTiming;
using clockfw::sim::tutorial::parseStoryFile;
using clockfw::sim::tutorial::parseStoryText;
using clockfw::sim::tutorial::validateStory;

bool require(const bool condition, const char* const message) {
    if (!condition) {
        std::cerr << "Storybook runner failure: " << message << '\n';
        return false;
    }
    return true;
}

bool containsReason(const StoryRunResult& result, const std::string& needle) {
    for (const auto& issue : result.issues) {
        if (issue.reason.find(needle) != std::string::npos) {
            return true;
        }
    }
    return false;
}

bool sameTrace(const std::vector<StoryTraceEvent>& lhs, const std::vector<StoryTraceEvent>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (std::size_t index = 0U; index < lhs.size(); ++index) {
        const StoryTraceEvent& a = lhs[index];
        const StoryTraceEvent& b = rhs[index];
        if (a.kind != b.kind || a.presentationUs != b.presentationUs || a.firmwareUs != b.firmwareUs ||
            a.sceneIndex != b.sceneIndex || a.actionIndex != b.actionIndex ||
            a.name != b.name || a.value != b.value) {
            return false;
        }
    }
    return true;
}

const StoryTraceEvent* findEvent(
    const std::vector<StoryTraceEvent>& trace,
    const StoryTraceKind kind,
    const std::size_t sceneIndex,
    const bool last) {
    const StoryTraceEvent* found = nullptr;
    for (const StoryTraceEvent& event : trace) {
        if (event.kind == kind && event.sceneIndex && *event.sceneIndex == sceneIndex) {
            found = &event;
            if (!last) return found;
        }
    }
    return found;
}

struct ExecutedStory final {
    StoryRunResult result;
    ClockSource source = ClockSource::Internal;
    TransportState transport = TransportState::Stopped;
    bool syncConnected = false;
    bool syncRunning = false;
    bool syncLocked = false;
};

ExecutedStory executeReference(
    const Story& story,
    const std::filesystem::path& tutorialRoot,
    const std::filesystem::path& statePath) {
    std::filesystem::remove(statePath);
    SimulatorRuntime runtime(statePath);
    runtime.begin();
    StorySimulatorPort port(runtime);
    StoryRunner runner(port, tutorialRoot);

    ExecutedStory execution;
    execution.result = runner.run(story);
    execution.source = port.clockSource();
    execution.transport = port.transportState();
    const auto sync = port.syncTelemetry();
    execution.syncConnected = sync.cableConnected;
    execution.syncRunning = sync.generatorRunning;
    execution.syncLocked = sync.locked;
    runtime.flushPersistence();
    std::filesystem::remove(statePath);
    return execution;
}

}  // namespace

int main() {
    const std::filesystem::path sourceRoot = CLOCK_SOURCE_ROOT;
    const std::filesystem::path tutorialRoot = sourceRoot / "docs" / "tutorials";
    bool ok = true;

    for (const InteractionProfile profile : {
             InteractionProfile::HumanSlow,
             InteractionProfile::HumanNormal,
             InteractionProfile::HumanFast}) {
        const auto timing = loadInteractionTiming(tutorialRoot, profile);
        ok &= require(static_cast<bool>(timing), "all three deterministic interaction profiles must resolve");
        if (timing) {
            ok &= require(timing.timing->encoderDetentMs > 0U && timing.timing->buttonDownMs > 0U,
                          "resolved interaction timings must be positive");
        }
    }

    const StoryParseResult reference = parseStoryFile(tutorialRoot / "stories" / "external-sync.yaml");
    ok &= require(static_cast<bool>(reference), "External-SYNC reference story must parse");
    if (!reference) return EXIT_FAILURE;
    ok &= require(validateStory(*reference.story, tutorialRoot).empty(),
                  "External-SYNC reference story must validate");

    const ExecutedStory first = executeReference(
        *reference.story, tutorialRoot, ".clock-storybook-runner-a.bin");
    const ExecutedStory second = executeReference(
        *reference.story, tutorialRoot, ".clock-storybook-runner-b.bin");
    ok &= require(static_cast<bool>(first.result), "External-SYNC story must complete successfully");
    ok &= require(static_cast<bool>(second.result), "repeated External-SYNC story must complete successfully");
    ok &= require(sameTrace(first.result.trace, second.result.trace),
                  "identical story runs must emit identical logical timelines");
    ok &= require(first.result.presentationDurationUs == second.result.presentationDurationUs,
                  "identical story runs must have identical presentation duration");
    ok &= require(first.source == ClockSource::Auto,
                  "reference story must leave production CLOCK SOURCE at AUTO");
    ok &= require(first.transport == TransportState::Stopped,
                  "reference story must end in explicit manual STOP");
    ok &= require(first.syncConnected && first.syncRunning && first.syncLocked,
                  "incoming SYNC must remain connected/running/locked after manual STOP");

    const StoryTraceEvent* chapterBegin = findEvent(first.result.trace, StoryTraceKind::SceneBegin, 0U, false);
    const StoryTraceEvent* chapterEnd = findEvent(first.result.trace, StoryTraceKind::SceneEnd, 0U, true);
    ok &= require(chapterBegin != nullptr && chapterEnd != nullptr,
                  "chapter trace boundaries must be emitted");
    if (chapterBegin != nullptr && chapterEnd != nullptr) {
        ok &= require(chapterEnd->presentationUs - chapterBegin->presentationUs == 3000000ULL,
                      "chapter duration must advance presentation time exactly");
        ok &= require(chapterEnd->firmwareUs == chapterBegin->firmwareUs,
                      "pure chapter presentation must freeze firmware time in Phase 1");
    }

    const StoryParseResult actionMatrix = parseStoryText(R"YAML(
schema: 1
id: runner-action-matrix
title: "Runner action matrix"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_FAST
setup:
  factory_reset: true
  power: on
scenes:
  - tutorial:
      subtitle: "Exercise physical and environment actions."
      actions:
        - wait_ms: 1100
        - encoder:
            direction: clockwise
            detents: 2
        - encoder:
            direction: counter_clockwise
            detents: 1
        - button:
            name: TAP
            state: down
        - encoder_push: {}
        - button:
            name: TAP
            state: up
        - button:
            name: TAP
        - sync_source:
            bpm: 100
            ppqn: 4
            waveform: triangle
        - sync_cable:
            state: connect
        - sync_generator:
            state: hold
        - sync_generator:
            state: run
        - rst_cable:
            state: connect
        - rst_generator:
            state: hold
        - rst_pulse: {}
        - scope:
            state: show
            channel: visible
        - subtitle:
            text: "Power cycle CLOCK."
        - scope:
            state: hide
        - power:
            state: off
        - assert:
            power: off
        - power:
            state: on
        - wait_ms: 1100
        - assert:
            power: on
)YAML");
    ok &= require(static_cast<bool>(actionMatrix), "runner action-matrix story must parse");
    if (actionMatrix) {
        const ExecutedStory matrix = executeReference(
            *actionMatrix.story, tutorialRoot, ".clock-storybook-runner-matrix.bin");
        ok &= require(static_cast<bool>(matrix.result), "runner must execute all Phase-1 action classes used by the action matrix");
    }

    const StoryParseResult timeoutStory = parseStoryText(R"YAML(
schema: 1
id: runner-timeout
title: "Runner timeout"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - wait_until:
            power: off
            timeout_ms: 5
)YAML");
    ok &= require(static_cast<bool>(timeoutStory), "timeout story must parse");
    if (timeoutStory) {
        const ExecutedStory timeout = executeReference(
            *timeoutStory.story, tutorialRoot, ".clock-storybook-runner-timeout.bin");
        ok &= require(!timeout.result && containsReason(timeout.result, "wait_until timeout"),
                      "wait_until timeout must fail explicitly");
        ok &= require(!timeout.result.issues.empty() && timeout.result.issues.front().sceneIndex == 0U &&
                      timeout.result.issues.front().actionIndex == 0U,
                      "runner failure must preserve scene/action context");
    }

    const StoryParseResult assertionStory = parseStoryText(R"YAML(
schema: 1
id: runner-assertion
title: "Runner assertion"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - assert:
            power: off
)YAML");
    ok &= require(static_cast<bool>(assertionStory), "assertion failure story must parse");
    if (assertionStory) {
        const ExecutedStory assertion = executeReference(
            *assertionStory.story, tutorialRoot, ".clock-storybook-runner-assert.bin");
        ok &= require(!assertion.result && containsReason(assertion.result, "assertion failed"),
                      "failed assertions must fail story execution explicitly");
    }

    if (!ok) return EXIT_FAILURE;
    std::cout << "CLOCK Storybook deterministic runner: PASS\n";
    return EXIT_SUCCESS;
}
