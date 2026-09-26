/**
 * @file sequencer_pattern.h
 * @brief Sequencer 2.0 pattern model and deterministic traversal helpers.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace clockfw {

/** Number of user pattern slots owned by each physical sequencer channel. */
inline constexpr std::uint8_t kSequencerPatternSlotsPerChannel = 8U;

/** Maximum Sequencer 2.0 pattern length; the OLED editor is only a viewport. */
inline constexpr std::uint8_t kSequencerMaximumSteps = 128U;

/** Two 64-bit words store the 128 binary gate states without heap allocation. */
inline constexpr std::size_t kSequencerGateWordCount = 2U;

/** Traversal order used when mapping timeline events onto pattern steps. */
enum class SequencerPlayDirection : std::uint8_t {
    Forward = 0U,
    Reverse = 1U,
    PingPong = 2U,
    Random = 3U,
};

/** End-of-pattern behavior. */
enum class SequencerLoopMode : std::uint8_t {
    Loop = 0U,
    Once = 1U,
};

/**
 * @brief One persistent Sequencer 2.0 gate pattern.
 *
 * Per-step probability/gate/tie/ratchet metadata is intentionally not embedded
 * here. It lives in the separate sparse override area so an untouched 128-step
 * pattern remains compact.
 */
struct SequencerPatternV2 final {
    std::uint8_t length = 16U;
    std::uint8_t rotation = 0U;
    SequencerPlayDirection direction = SequencerPlayDirection::Forward;
    SequencerLoopMode loopMode = SequencerLoopMode::Loop;
    std::array<std::uint64_t, kSequencerGateWordCount> gates{{0x1111ULL, 0ULL}};
};

/** Result of mapping one timeline event onto a pattern step. */
struct SequencerTraversalResult final {
    bool active = false;
    std::uint8_t step = 0U;
};

/** @brief Returns true when every bounded field in the pattern is valid. */
bool isSequencerPatternValid(const SequencerPatternV2& pattern);

/** @brief Returns whether one zero-based step contains a gate; out-of-range steps are false. */
bool sequencerPatternGate(const SequencerPatternV2& pattern, std::uint8_t step);

/** @brief Sets or clears one zero-based step; returns false when the step is outside 0..127. */
bool setSequencerPatternGate(SequencerPatternV2& pattern, std::uint8_t step, bool enabled);

/** @brief Clears any gate bits beyond the current active pattern length. */
void clampSequencerPattern(SequencerPatternV2& pattern);

/**
 * @brief Number of event positions in one traversal cycle.
 *
 * Forward, Reverse, and Random use LENGTH events. PingPong omits duplicate end
 * points and therefore uses 2*LENGTH-2 events for lengths greater than one.
 */
std::uint16_t sequencerTraversalCycleLength(const SequencerPatternV2& pattern);

/**
 * @brief Maps a monotonically increasing event serial onto a pattern step.
 *
 * RANDOM uses a deterministic stateless hash of eventSerial and randomSeed. The
 * caller may choose a new seed for a new transport run while reschedules inside
 * that run remain reproducible because they do not depend on call history.
 * ONCE returns active=false after one complete traversal cycle.
 */
SequencerTraversalResult resolveSequencerTraversal(
    std::uint64_t eventSerial,
    const SequencerPatternV2& pattern,
    std::uint32_t randomSeed);

/** @brief Applies the pattern's circular rotation to a resolved zero-based step. */
std::uint8_t rotateSequencerStep(std::uint8_t step, const SequencerPatternV2& pattern);

/** @brief Resolves traversal + rotation and returns whether the addressed event emits a binary gate. */
bool sequencerPatternHitForEvent(
    std::uint64_t eventSerial,
    const SequencerPatternV2& pattern,
    std::uint32_t randomSeed);

}  // namespace clockfw
