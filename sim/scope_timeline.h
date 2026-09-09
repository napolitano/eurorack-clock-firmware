/**
 * @file scope_timeline.h
 * @brief Time-axis and musical reference-grid model for the native simulator oscilloscope.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "domain/clock_types.h"

namespace clockfw::sim::scope {

/** Fixed oscilloscope spans exposed by the interactive simulator. */
inline constexpr std::array<std::uint64_t, 7U> kWindowOptionsUs{{
    500000ULL,
    1000000ULL,
    2000000ULL,
    4000000ULL,
    8000000ULL,
    16000000ULL,
    32000000ULL,
}};

/** Four seconds preserves the pre-zoom simulator view as the default. */
inline constexpr std::size_t kDefaultWindowIndex = 3U;

/** @brief Maximum transition history required by any supported zoom stage. */
inline constexpr std::uint64_t kMaximumWindowUs = kWindowOptionsUs.back();

/** Keep a readable number of minor reference lines at very high clock rates. */
inline constexpr std::uint32_t kMaximumVisibleMinorReferences = 56U;

/**
 * @brief Musical reference ruler used by the developer oscilloscope.
 *
 * intervalUs is the unswung/unhumanized reference-event period. minorEvery and
 * majorEvery are integer serial strides on that same musical lattice. This is
 * deliberately not a fixed millisecond ruler: the references must remain phase
 * coherent with the clock when BPM or rate differs from a convenient round value.
 */
struct MusicalGridSpec {
    double intervalUs = 500000.0;
    std::uint32_t minorEvery = 1U;
    std::uint32_t majorEvery = 4U;
    const char* basis = "BEAT";
};

/** @brief Returns the next shorter fixed window index, clamped at maximum zoom-in. */
std::size_t zoomInIndex(std::size_t index);

/** @brief Returns the next longer fixed window index, clamped at maximum zoom-out. */
std::size_t zoomOutIndex(std::size_t index);

/**
 * @brief Resolves the tempo actually used by the engine for the scope reference grid.
 * Manual MIN/MAX limits intentionally do not affect external sync.
 */
std::uint32_t effectiveReferenceBpmMilli(
    const ClockState& state,
    bool externalLocked,
    std::uint32_t externalBpmMilli);

/**
 * @brief Builds an unswung musical reference grid for the current operating mode.
 *
 * ONE CLOCK follows its shared rate exactly. DIVIDER uses the undivided output-1
 * beat. INDEPENDENT uses the neutral 1/16 pattern lattice, on which CLOCK note
 * values and x1 EUCLID/SEQ steps are integer-aligned.
 */
MusicalGridSpec musicalGridSpec(
    const ClockState& state,
    std::uint32_t effectiveBpmMilli,
    std::uint64_t windowUs);

/** @brief Returns the first non-negative minor-reference serial visible at/after startUs. */
std::uint64_t firstMinorReferenceSerialAtOrAfter(
    std::int64_t startUs,
    const MusicalGridSpec& grid);

/** @brief Returns the transport-relative timestamp for one exact reference serial. */
double referenceTimeUs(std::uint64_t serial, const MusicalGridSpec& grid);

/** @brief Maps one timestamp onto the visible [0, 1] scope axis. */
double normalizedPosition(double timestampUs, double startUs, std::uint64_t windowUs);

/** @brief Returns true when one reference serial is a major reference. */
bool isMajorReference(std::uint64_t serial, const MusicalGridSpec& grid);

}  // namespace clockfw::sim::scope
