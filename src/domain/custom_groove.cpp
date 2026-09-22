/**
 * @file custom_groove.cpp
 * @brief Validation and deterministic lookup for user-authored groove patterns.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "domain/custom_groove.h"

#include <algorithm>

namespace clockfw {

bool isCustomGroovePatternValid(const CustomGroovePattern& pattern) {
    if (pattern.length == 0U || pattern.length > kCustomGrooveMaximumSteps) {
        return false;
    }
    for (std::size_t index = 0U; index < pattern.length; ++index) {
        if (pattern.offsets256[index] < kCustomGrooveMinimumOffset256 ||
            pattern.offsets256[index] > kCustomGrooveMaximumOffset256) {
            return false;
        }
    }
    return true;
}

std::int16_t customGrooveOffset256(
    const CustomGroovePattern& pattern,
    const std::uint64_t eventSerial,
    const std::uint8_t amountPercent,
    const std::uint8_t rotation) {
    if (!isCustomGroovePatternValid(pattern) || amountPercent == 0U) {
        return 0;
    }
    const std::uint8_t boundedAmount = std::min<std::uint8_t>(amountPercent, 100U);
    const std::uint8_t rotated = static_cast<std::uint8_t>(rotation % pattern.length);
    const std::uint8_t step = static_cast<std::uint8_t>((eventSerial + rotated) % pattern.length);
    return static_cast<std::int16_t>(
        (static_cast<std::int32_t>(pattern.offsets256[step]) * boundedAmount) / 100);
}

}  // namespace clockfw
