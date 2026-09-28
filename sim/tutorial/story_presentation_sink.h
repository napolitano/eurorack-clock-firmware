/**
 * @file story_presentation_sink.h
 * @brief One-way observation interface from the Story Runner into physical tutorial presentation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "tutorial/story_contract.h"

namespace clockfw::sim::tutorial {

/**
 * @brief Receives deterministic physical-presentation events without being able to mutate CLOCK.
 *
 * StoryRunner invokes these callbacks at the same presentation timestamp at which the corresponding
 * accepted simulator action occurs. Implementations may record or render presentation state, but the
 * interface deliberately exposes no SimulatorRuntime or ClockState mutation path.
 */
class StoryPresentationSink {
public:
    virtual ~StoryPresentationSink() = default;

    /** @brief Reports one accepted physical encoder detent. */
    virtual void onEncoderDetent(std::uint64_t presentationUs, int direction) = 0;

    /** @brief Reports physical press/release state for encoder push or one front-panel button. */
    virtual void onControlState(
        std::uint64_t presentationUs,
        ModuleControl control,
        bool pressed) = 0;

    /** @brief Reports one recorded POWER state change. */
    virtual void onPowerState(std::uint64_t presentationUs, bool poweredOn) = 0;

    /**
     * @brief Starts visible plug insertion/removal at the same timestamp as the real cable operation.
     * @param presentationUs Story presentation timestamp.
     * @param patch SYNC or RST cable.
     * @param targetConnected True for insertion, false for removal.
     * @param durationUs Deterministic visible motion duration.
     */
    virtual void onPatchMotion(
        std::uint64_t presentationUs,
        PatchAction patch,
        bool targetConnected,
        std::uint64_t durationUs) = 0;

    /** @brief Marks one visible patch action as settled after its deterministic motion duration. */
    virtual void onPatchSettled(
        std::uint64_t presentationUs,
        PatchAction patch,
        bool connected) = 0;

    /** @brief Reports optional tutorial-scope visibility state. */
    virtual void onScopeState(std::uint64_t presentationUs, ScopeMode mode) = 0;
};

}  // namespace clockfw::sim::tutorial
