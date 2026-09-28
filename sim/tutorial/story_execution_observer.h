/**
 * @file story_execution_observer.h
 * @brief Read-only deterministic sampling boundary for Storybook frame generation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

#include "simulator_runtime.h"
#include "tutorial/story_model.h"

namespace clockfw::sim::tutorial {

/** @brief One read-only presentation sample from the currently executing Story Runner. */
struct StoryExecutionSample {
    std::uint64_t presentationUs = 0ULL;
    std::uint64_t firmwareUs = 0ULL;
    std::size_t sceneIndex = 0U;
    const StoryScene* scene = nullptr;
    std::string_view activeSubtitle{};
    SimulatorRuntime* runtime = nullptr;
};

/**
 * @brief Observer used by host-only output tooling to sample one Story Runner execution.
 *
 * The observer can request presentation timestamps but cannot drive CLOCK or mutate the runner.
 * Samples are delivered while the real simulator is already at the matching firmware timestamp.
 */
class StoryExecutionObserver {
public:
    virtual ~StoryExecutionObserver() = default;

    /** @brief Returns the next absolute presentation timestamp to sample, or nullopt when finished. */
    virtual std::optional<std::uint64_t> nextPresentationSampleUs() const = 0;

    /** @brief Consumes one read-only sample from the single live story execution. */
    virtual void onPresentationSample(const StoryExecutionSample& sample) = 0;
};

}  // namespace clockfw::sim::tutorial
