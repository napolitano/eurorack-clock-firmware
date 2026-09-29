/**
 * @file storybook_example_stories_tests.cpp
 * @brief Validates and executes the thirteen copy-and-edit CLOCK Storybook teaching examples.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <array>
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>

#include "simulator_runtime.h"
#include "tutorial/story_parser.h"
#include "tutorial/story_runner.h"
#include "tutorial/story_simulator_port.h"
#include "tutorial/story_validator.h"

namespace {
using namespace clockfw;
using namespace clockfw::sim;
using namespace clockfw::sim::tutorial;

constexpr std::array<const char*, 13U> kExamples{{
    "01-power-and-first-clock.yaml",
    "02-transport-basics.yaml",
    "03-encoder-tempo.yaml",
    "04-tap-tempo.yaml",
    "05-topology-and-channel.yaml",
    "06-clock-mode.yaml",
    "07-euclidean-rhythm.yaml",
    "08-sequencer-basics.yaml",
    "09-divider-bank.yaml",
    "10-external-sync.yaml",
    "11-eight-independent-clocks-walkthrough.yaml",
    "12-groove-editor-walkthrough.yaml",
    "13-getting-to-know-clock.yaml",
}};

bool require(const bool condition, const std::string& message) {
    if (!condition) std::cerr << "Storybook example failure: " << message << '\n';
    return condition;
}

}  // namespace

int main() {
    const std::filesystem::path sourceRoot = CLOCK_SOURCE_ROOT;
    const std::filesystem::path tutorialRoot = sourceRoot / "docs" / "tutorials";
    const std::filesystem::path examplesRoot = tutorialRoot / "examples";
    bool ok = true;

    for (const char* const filename : kExamples) {
        const StoryParseResult parsed = parseStoryFile(examplesRoot / filename);
        ok &= require(static_cast<bool>(parsed), std::string(filename) + " must parse");
        if (!parsed) {
            for (const auto& issue : parsed.issues) std::cerr << filename << ':' << issue.sourceLine << ": " << issue.reason << '\n';
            continue;
        }
        const auto validation = validateStory(*parsed.story, tutorialRoot);
        if (!validation.empty()) {
            for (const auto& issue : validation) std::cerr << filename << ':' << issue.sourceLine << ": " << issue.reason << '\n';
        }
        ok &= require(validation.empty(), std::string(filename) + " must validate");
        if (!validation.empty()) continue;

        const auto statePath = std::filesystem::path(".clock-storybook-example-") += parsed.story->id + ".bin";
        std::filesystem::remove(statePath);
        SimulatorRuntime runtime(statePath);
        runtime.begin();
        StorySimulatorPort port(runtime);
        StoryRunner runner(port, tutorialRoot);
        const StoryRunResult result = runner.run(*parsed.story);
        if (!result) {
            for (const auto& issue : result.issues) std::cerr << filename << ':' << issue.sourceLine << ": " << issue.reason << '\n';
        }
        ok &= require(static_cast<bool>(result), std::string(filename) + " must execute");
        if (std::string(filename) == "11-eight-independent-clocks-walkthrough.yaml" && result) {
            const ClockState& state = runtime.state();
            ok &= require(state.operatingMode == OperatingMode::Independent,
                          "Example 11 must end in Independent topology");
            ok &= require(runtime.selectedChannelForPresentation() == 3U,
                          "Example 11 must leave channel 4 selected");
            for (std::size_t channel = 0U; channel < kChannelCount; ++channel) {
                const auto& config = state.channels[channel].common;
                ok &= require(config.mode == ChannelMode::Clock,
                              "Example 11 must keep all eight channels in Clock mode");
                ok &= require(config.rate.factor == (channel == 3U ? 2U : 1U),
                              "Example 11 must edit only channel 4 rate to x2");
                ok &= require(config.gateLengthMs == (channel == 3U ? 20U : 10U),
                              "Example 11 must edit only channel 4 gate to 20 ms");
            }
        }
        if (std::string(filename) == "12-groove-editor-walkthrough.yaml" && result) {
            const ClockState& state = runtime.state();
            ok &= require(state.operatingMode == OperatingMode::Independent,
                          "Example 12 must end in Independent topology");
            ok &= require(runtime.selectedChannelForPresentation() == 3U,
                          "Example 12 must leave channel 4 selected");
            const GrooveSettings& groove = state.channels[3U].common.groove;
            ok &= require(groove.preset == GroovePreset::Custom,
                          "Example 12 must activate the saved Custom Groove");
            ok &= require(groove.customSlot == 0U,
                          "Example 12 must save and activate Custom Groove slot 1");
            ok &= require(groove.amountPercent == 80U,
                          "Example 12 must leave Custom Groove Amount at 80 percent");
            ok &= require(groove.rotation == 1U,
                          "Example 12 must leave Custom Groove Rotate at one step");
            for (std::size_t channel = 0U; channel < kChannelCount; ++channel) {
                if (channel == 3U) continue;
                ok &= require(state.channels[channel].common.groove.preset == GroovePreset::Off,
                              "Example 12 must not assign Groove to the other channels");
            }
            ok &= require(state.transport == TransportState::Stopped,
                          "Example 12 must finish with transport stopped");
        }
        if (std::string(filename) == "13-getting-to-know-clock.yaml" && result) {
            const ClockState& state = runtime.state();
            ok &= require(state.operatingMode == OperatingMode::Independent,
                          "Getting to Know CLOCK must end in Independent topology");
            ok &= require(state.channels[0U].common.mode == ChannelMode::Clock,
                          "Getting to Know CLOCK must keep channel 1 in Clock mode");
            ok &= require(state.channels[1U].common.mode == ChannelMode::Euclid,
                          "Getting to Know CLOCK must leave channel 2 in Euclid mode");
            ok &= require(state.channels[2U].common.mode == ChannelMode::Sequencer,
                          "Getting to Know CLOCK must leave channel 3 in Sequencer mode");
            ok &= require(runtime.selectedChannelForPresentation() == 2U,
                          "Getting to Know CLOCK must leave channel 3 selected");
            ok &= require(state.transport == TransportState::Stopped,
                          "Getting to Know CLOCK must finish with transport stopped");
        }
        runtime.flushPersistence();
        std::filesystem::remove(statePath);
    }

    if (!ok) return EXIT_FAILURE;
    std::cout << "CLOCK Storybook teaching examples: 13/13 PASS\n";
    return EXIT_SUCCESS;
}
