/**
 * @file storybook_example_stories_tests.cpp
 * @brief Validates and executes the eleven copy-and-edit CLOCK Storybook teaching examples.
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
using namespace clockfw::sim;
using namespace clockfw::sim::tutorial;

constexpr std::array<const char*, 11U> kExamples{{
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
        runtime.flushPersistence();
        std::filesystem::remove(statePath);
    }

    if (!ok) return EXIT_FAILURE;
    std::cout << "CLOCK Storybook teaching examples: 11/11 PASS\n";
    return EXIT_SUCCESS;
}
