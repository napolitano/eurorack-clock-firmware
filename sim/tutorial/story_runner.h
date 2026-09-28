/**
 * @file story_runner.h
 * @brief Deterministic CLOCK Storybook runner over the strict simulator boundary.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

#include "tutorial/story_model.h"
#include "tutorial/story_presentation_sink.h"
#include "tutorial/story_simulator_port.h"

namespace clockfw::sim::tutorial {

/** @brief Stable logical trace event classes emitted by the deterministic Story Runner. */
enum class StoryTraceKind : std::uint8_t {
    Setup,
    SceneBegin,
    SceneEnd,
    ActionBegin,
    ActionEnd,
    EncoderDetent,
    ControlState,
    PowerState,
    PatchState,
    GeneratorState,
    SourceConfigured,
    ResetPulse,
    Subtitle,
    Scope,
    WaitSatisfied,
    AssertionPassed,
};

/** @brief One deterministic runner event suitable for later renderer synchronization/regression checks. */
struct StoryTraceEvent {
    StoryTraceKind kind = StoryTraceKind::Setup;
    std::uint64_t presentationUs = 0ULL;
    std::uint64_t firmwareUs = 0ULL;
    std::optional<std::size_t> sceneIndex;
    std::optional<std::size_t> actionIndex;
    std::string name;
    std::string value;
};

/** @brief Complete result of one Story Runner execution. */
struct StoryRunResult {
    std::vector<StoryTraceEvent> trace;
    std::vector<StoryIssue> issues;
    std::uint64_t presentationDurationUs = 0ULL;

    /** @brief Returns true only when the story completed with no execution issue. */
    explicit operator bool() const;
};

/**
 * @brief Executes one validated story through real simulator boundaries while owning only orchestration timing.
 *
 * Presentation-only chapter/text/callout time never advances firmware time in Phase 1. Tutorial interaction
 * pacing, explicit waits and wait_until polling advance both presentation and simulator/firmware time.
 */
class StoryRunner final {
public:
    /** @brief Binds one strict simulator port and the canonical docs/tutorials resource root. */
    StoryRunner(
        StorySimulatorPort& port,
        std::filesystem::path tutorialRoot,
        StoryPresentationSink* presentationSink = nullptr);

    /** @brief Validates and executes one complete story deterministically. */
    StoryRunResult run(const Story& story);

private:
    StorySimulatorPort& port_;
    std::filesystem::path tutorialRoot_;
    StoryPresentationSink* presentationSink_ = nullptr;
};

}  // namespace clockfw::sim::tutorial
