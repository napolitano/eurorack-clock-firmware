/**
 * @file groove_catalog.cpp
 * @brief Read-only Stage-1 factory groove definitions and timing helpers.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "domain/groove_catalog.h"

#include "domain/custom_groove.h"

#include <algorithm>

namespace clockfw::groove {
namespace {

struct GrooveDefinition {
    const char* label;
    std::uint8_t length;
    std::uint16_t delayPermille[16];
};

// Delays are fractions of one nominal event interval. Keeping Stage-1 factory
// values non-negative makes composition with the existing swing layer auditable:
// the scheduler can cap the summed delay to the remaining monotonic interval.
constexpr GrooveDefinition kDefinitions[kPresetCount] = {
    {"OFF", 1U, {0U}},
    {"SWING 54", 2U, {0U, 80U}},
    {"SWING 58", 2U, {0U, 160U}},
    {"SWING 62", 2U, {0U, 240U}},
    {"SWING 66", 2U, {0U, 320U}},
    {"POCKET A", 8U, {0U, 70U, 20U, 120U, 0U, 90U, 30U, 150U}},
    {"POCKET B", 8U, {0U, 120U, 40U, 60U, 10U, 150U, 30U, 90U}},
    {"POCKET C", 16U, {0U, 90U, 20U, 140U, 10U, 60U, 30U, 120U,
                         0U, 130U, 40U, 80U, 20U, 160U, 30U, 100U}},
    {"CUSTOM", 1U, {0U}},
};

std::size_t presetIndex(const GroovePreset preset) {
    const std::size_t index = static_cast<std::size_t>(preset);
    return index < kPresetCount ? index : 0U;
}

}  // namespace

const char* presetLabel(const GroovePreset preset) {
    return kDefinitions[presetIndex(preset)].label;
}

std::uint8_t patternLength(const GroovePreset preset) {
    return kDefinitions[presetIndex(preset)].length;
}

std::uint16_t delayPermille(
    const GroovePreset preset,
    const std::uint64_t eventSerial,
    const std::uint8_t amountPercent,
    const std::uint8_t rotation) {
    const GrooveDefinition& definition = kDefinitions[presetIndex(preset)];
    if (preset == GroovePreset::Off || amountPercent == 0U || definition.length == 0U) {
        return 0U;
    }
    const std::uint8_t boundedAmount = std::min<std::uint8_t>(amountPercent, 100U);
    const std::uint8_t rotated = static_cast<std::uint8_t>(rotation % definition.length);
    const std::uint8_t step = static_cast<std::uint8_t>(
        (eventSerial + rotated) % definition.length);
    return static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(definition.delayPermille[step]) * boundedAmount) / 100U);
}

std::uint16_t maximumDelayPermille(
    const GroovePreset preset,
    const std::uint8_t amountPercent) {
    const GrooveDefinition& definition = kDefinitions[presetIndex(preset)];
    std::uint16_t maximum = 0U;
    for (std::uint8_t index = 0U; index < definition.length; ++index) {
        maximum = std::max(maximum, definition.delayPermille[index]);
    }
    return static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(maximum) * std::min<std::uint8_t>(amountPercent, 100U)) / 100U);
}

}  // namespace clockfw::groove
