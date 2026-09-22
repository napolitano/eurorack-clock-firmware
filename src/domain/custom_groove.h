/**
 * @file custom_groove.h
 * @brief Bounded runtime representation for user-authored deterministic groove patterns.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace clockfw {

/** Maximum number of editable steps in one Custom Groove. */
inline constexpr std::size_t kCustomGrooveMaximumSteps = 64U;

/** Number of durable Custom Groove slots supported by the current 8-KiB image. */
inline constexpr std::uint8_t kCustomGrooveSlotCount = 10U;

/** Signed offset unit: one value step equals 1/256 of the nominal event interval. */
inline constexpr std::int8_t kCustomGrooveMinimumOffset256 = -120;
inline constexpr std::int8_t kCustomGrooveMaximumOffset256 = 120;

/**
 * @brief One deterministic Custom Groove pattern kept outside ClockState.
 *
 * Offsets are signed fractions of the nominal event interval. The deliberately
 * bounded range leaves at least 1/16 of one straight interval between two
 * adjacent maximally opposed markers before Swing composition is applied.
 */
struct CustomGroovePattern final {
    std::uint8_t length = 16U;
    std::array<std::int8_t, kCustomGrooveMaximumSteps> offsets256{};
};

/** @brief Returns true when one pattern obeys the durable/runtime bounds. */
bool isCustomGroovePatternValid(const CustomGroovePattern& pattern);

/** @brief Returns one clamped signed offset for the selected serial/rotation. */
std::int16_t customGrooveOffset256(
    const CustomGroovePattern& pattern,
    std::uint64_t eventSerial,
    std::uint8_t amountPercent,
    std::uint8_t rotation);

}  // namespace clockfw
