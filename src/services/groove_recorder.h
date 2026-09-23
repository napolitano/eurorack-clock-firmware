/**
 * @file groove_recorder.h
 * @brief Deterministic Custom Groove capture state machine driven by the ClockEngine timeline.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "domain/custom_groove.h"

namespace clockfw::services {

enum class GrooveRecordMode : std::uint8_t { OneShot, Endless };
enum class GrooveRecordState : std::uint8_t { Ready, PreCount, Recording };

/** @brief Read-only recorder state used by the UI renderer. */
struct GrooveRecorderView final {
    GrooveRecordMode mode = GrooveRecordMode::OneShot;
    GrooveRecordState state = GrooveRecordState::Ready;
    std::uint8_t countInBeats = 4U;
    std::uint8_t countInRemaining = 0U;
    std::uint8_t playheadStep = 0U;
    std::uint8_t playheadPhase256 = 0U;
    std::uint64_t capturedMask = 0U;
};

/**
 * @brief Captures human TAP timing directly into the existing signed Custom Groove format.
 *
 * The recorder works entirely in ClockEngine Q32 beat-space. Tempo changes therefore
 * change wall-clock speed without changing the pattern phase or capture math.
 */
class GrooveRecorder final {
public:
    /** @brief Returns the recorder to READY with the supplied non-musical capture settings. */
    void reset(GrooveRecordMode mode, std::uint8_t countInBeats, std::uint64_t capturedMask = 0U);

    /** @brief Changes One-Shot/Endless behavior for the next/current recording pass. */
    void setMode(GrooveRecordMode mode);

    /** @brief Changes the recorder-local Count-In length in master beats. */
    void setCountInBeats(std::uint8_t beats);

    /** @brief Arms recording from the supplied engine position and nominal event interval. */
    void start(std::uint64_t masterPositionQ32, std::uint64_t stepIntervalQ32, std::uint8_t length);

    /** @brief Stops recording and returns the playhead to the start without changing the draft. */
    void stop();

    /** @brief Advances Pre-Count/playhead state from the current engine position. */
    bool service(std::uint64_t masterPositionQ32, std::uint8_t length);

    /** @brief Records one TAP into the nearest nominal step of the current cycle. */
    bool capture(std::uint64_t tapPositionQ32, CustomGroovePattern& pattern);

    /** @brief Clears captured-state bookkeeping; the caller owns actual pattern clearing. */
    void clearCaptured();

    /** @brief Returns the compact immutable state consumed by the UI. */
    GrooveRecorderView view() const;

private:
    GrooveRecordMode mode_ = GrooveRecordMode::OneShot;
    GrooveRecordState state_ = GrooveRecordState::Ready;
    std::uint8_t countInBeats_ = 4U;
    std::uint8_t countInRemaining_ = 0U;
    std::uint8_t playheadStep_ = 0U;
    std::uint8_t playheadPhase256_ = 0U;
    std::uint64_t capturedMask_ = 0U;
    std::uint64_t stepIntervalQ32_ = 0U;
    std::uint64_t preCountEpochQ32_ = 0U;
    std::uint64_t recordEpochQ32_ = 0U;
    std::uint64_t recordStartTargetQ32_ = 0U;
};

}  // namespace clockfw::services
