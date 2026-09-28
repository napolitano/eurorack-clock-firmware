/**
 * @file storybook_reference_stories_tests.cpp
 * @brief Validates and executes every canonical Phase-1 CLOCK Storybook reference story.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "domain/clock_types.h"
#include "simulator_runtime.h"
#include "tutorial/story_parser.h"
#include "tutorial/story_runner.h"
#include "tutorial/story_simulator_port.h"
#include "tutorial/story_validator.h"

namespace {

using namespace clockfw;
using namespace clockfw::sim;
using namespace clockfw::sim::tutorial;

constexpr std::array<const char*, 13U> kReferenceFiles{{
    "getting-started.yaml",
    "play-stop.yaml",
    "changing-tempo.yaml",
    "tap-tempo.yaml",
    "selecting-operating-topology.yaml",
    "selecting-independent-channel.yaml",
    "clock-mode.yaml",
    "euclidean-mode.yaml",
    "sequencer-mode.yaml",
    "divider-bank.yaml",
    "saving-loading-preset.yaml",
    "external-sync.yaml",
    "external-rst.yaml",
}};

bool require(const bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Storybook reference failure: " << message << '\n';
        return false;
    }
    return true;
}

bool sameTrace(const std::vector<StoryTraceEvent>& lhs, const std::vector<StoryTraceEvent>& rhs) {
    if (lhs.size() != rhs.size()) return false;
    for (std::size_t index = 0U; index < lhs.size(); ++index) {
        const auto& a = lhs[index];
        const auto& b = rhs[index];
        if (a.kind != b.kind || a.presentationUs != b.presentationUs || a.firmwareUs != b.firmwareUs ||
            a.sceneIndex != b.sceneIndex || a.actionIndex != b.actionIndex ||
            a.name != b.name || a.value != b.value) {
            return false;
        }
    }
    return true;
}

struct Snapshot final {
    StoryRunResult result;
    ClockState state{};
    std::uint8_t selectedChannel = 0U;
    SyncInputTelemetry sync{};
    ResetInputTelemetry reset{};
};

Snapshot execute(
    const Story& story,
    const std::filesystem::path& tutorialRoot,
    const std::filesystem::path& statePath) {
    std::filesystem::remove(statePath);
    SimulatorRuntime runtime(statePath);
    runtime.begin();
    StorySimulatorPort port(runtime);
    StoryRunner runner(port, tutorialRoot);
    Snapshot snapshot;
    snapshot.result = runner.run(story);
    snapshot.state = runtime.state();
    snapshot.selectedChannel = runtime.selectedChannelForPresentation();
    snapshot.sync = runtime.syncInputTelemetry();
    snapshot.reset = runtime.resetInputTelemetry();
    runtime.flushPersistence();
    std::filesystem::remove(statePath);
    return snapshot;
}

bool checkExpected(const Story& story, const Snapshot& snapshot) {
    const ClockState& state = snapshot.state;
    bool ok = true;
    if (story.id == "getting-started") {
        ok &= require(state.transport == TransportState::Stopped, "getting-started must end in STOP");
        ok &= require(state.bpm == 124U, "getting-started must demonstrate a four-BPM encoder increase");
    } else if (story.id == "play-stop") {
        ok &= require(state.transport == TransportState::Stopped, "play-stop must end in STOP");
    } else if (story.id == "changing-tempo") {
        ok &= require(state.bpm == 125U, "changing-tempo must end at 125 BPM");
    } else if (story.id == "tap-tempo") {
        ok &= require(state.bpm == 100U, "tap-tempo must resolve the demonstrated 600-ms taps to 100 BPM");
        ok &= require(state.source == ClockSource::Auto, "tap-tempo must not change AUTO clock source");
    } else if (story.id == "selecting-operating-topology") {
        ok &= require(state.operatingMode == OperatingMode::DividerBank,
                      "operating-topology story must select Divider Bank through the real carousel");
    } else if (story.id == "selecting-independent-channel") {
        ok &= require(state.operatingMode == OperatingMode::Independent,
                      "independent-channel story must enter Independent topology");
        ok &= require(snapshot.selectedChannel == 2U, "independent-channel story must select physical channel 3");
    } else if (story.id == "clock-mode") {
        ok &= require(state.operatingMode == OperatingMode::Independent &&
                          state.channels[0].common.mode == ChannelMode::Clock,
                      "clock-mode story must select Independent Clock");
        ok &= require(state.transport == TransportState::Stopped, "clock-mode must stop after demonstration");
    } else if (story.id == "euclidean-mode") {
        ok &= require(state.operatingMode == OperatingMode::Independent &&
                          state.channels[0].common.mode == ChannelMode::Euclid,
                      "euclidean-mode story must select Euclid through the real carousel");
        ok &= require(state.channels[0].euclid.steps == 12U && state.channels[0].euclid.hits == 5U,
                      "euclidean-mode must edit 12 steps / 5 hits through Settings");
        ok &= require(state.transport == TransportState::Stopped, "euclidean-mode must end in STOP");
    } else if (story.id == "sequencer-mode") {
        ok &= require(state.operatingMode == OperatingMode::Independent &&
                          state.channels[0].common.mode == ChannelMode::Sequencer,
                      "sequencer-mode story must select Sequencer through the real carousel");
        ok &= require(state.transport == TransportState::Playing,
                      "sequencer-mode must leave transport playing while returning to Performance");
    } else if (story.id == "divider-bank") {
        ok &= require(state.operatingMode == OperatingMode::DividerBank,
                      "divider-bank story must select Divider Bank");
        ok &= require(state.dividerBank.bank == DividerBank::Integers,
                      "divider-bank story must change bank to INTEGERS through Settings");
        ok &= require(state.transport == TransportState::Stopped, "divider-bank story must end in STOP");
    } else if (story.id == "saving-loading-preset") {
        ok &= require(state.bpm == 120U, "preset load must restore the saved 120-BPM state after a tempo change");
    } else if (story.id == "external-sync") {
        ok &= require(state.source == ClockSource::Auto, "external-sync must leave SOURCE at AUTO");
        ok &= require(state.transport == TransportState::Stopped, "external-sync must end in manual STOP");
        ok &= require(snapshot.sync.cableConnected && snapshot.sync.generatorRunning && snapshot.sync.locked,
                      "external-sync must leave incoming SYNC connected/running/locked after STOP");
    } else if (story.id == "external-rst") {
        ok &= require(state.transport == TransportState::Stopped, "external-rst must end in STOP");
        ok &= require(snapshot.reset.cableConnected && !snapshot.reset.generatorRunning,
                      "external-rst must leave the RST cable connected with continuous generator held");
        ok &= require(snapshot.reset.resetCount >= 1ULL, "external-rst must inject at least one real reset pulse");
    }
    return ok;
}

}  // namespace

int main() {
    const std::filesystem::path sourceRoot = CLOCK_SOURCE_ROOT;
    const std::filesystem::path tutorialRoot = sourceRoot / "docs" / "tutorials";
    const std::filesystem::path storiesRoot = tutorialRoot / "stories";
    bool ok = true;
    std::vector<Story> stories;
    stories.reserve(kReferenceFiles.size());

    for (const char* const filename : kReferenceFiles) {
        const StoryParseResult parsed = parseStoryFile(storiesRoot / filename);
        ok &= require(static_cast<bool>(parsed), std::string(filename) + " must parse");
        if (!parsed) {
            for (const auto& issue : parsed.issues) {
                std::cerr << filename << ':' << issue.sourceLine << ": " << issue.reason << '\n';
            }
            continue;
        }
        const auto issues = validateStory(*parsed.story, tutorialRoot);
        if (!issues.empty()) {
            for (const auto& issue : issues) {
                std::cerr << filename << ':' << issue.sourceLine << ": " << issue.reason << '\n';
            }
        }
        ok &= require(issues.empty(), std::string(filename) + " must validate");
        stories.push_back(*parsed.story);
    }

    ok &= require(stories.size() == kReferenceFiles.size(), "all 13 reference stories must load");
    ok &= require(validateUniqueStoryIds(stories).empty(), "reference story IDs must be unique");

    for (const Story& story : stories) {
        const Snapshot first = execute(
            story, tutorialRoot, ".clock-storybook-reference-" + story.id + "-a.bin");
        const Snapshot second = execute(
            story, tutorialRoot, ".clock-storybook-reference-" + story.id + "-b.bin");
        if (!first.result) {
            for (const auto& issue : first.result.issues) {
                std::cerr << story.id << ':' << issue.sourceLine << ": " << issue.reason << '\n';
            }
        }
        ok &= require(static_cast<bool>(first.result), story.id + " must execute successfully");
        ok &= require(static_cast<bool>(second.result), story.id + " must repeat successfully");
        ok &= require(sameTrace(first.result.trace, second.result.trace),
                      story.id + " must emit an identical deterministic trace on repeat");
        ok &= require(first.result.presentationDurationUs == second.result.presentationDurationUs,
                      story.id + " must keep deterministic presentation duration");
        if (first.result) ok &= checkExpected(story, first);
    }

    if (!ok) return EXIT_FAILURE;
    std::cout << "CLOCK Storybook canonical reference stories: 13/13 PASS\n";
    return EXIT_SUCCESS;
}
