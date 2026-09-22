/**
 * @file groove_name_generator.cpp
 * @brief Human-readable pseudo-random default names for user Custom Grooves.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/groove_name_generator.h"

#include <cstdio>

#include "ui_text.h"

namespace clockfw::ui::groovename {
namespace {

std::uint32_t mix(std::uint32_t value) {
    // Small avalanche mixer: deterministic on every target, with enough diffusion
    // that adjacent boot seeds/counters do not walk through the dictionaries in order.
    value ^= value >> 16U;
    value *= 0x7FEB352DU;
    value ^= value >> 15U;
    value *= 0x846CA68BU;
    value ^= value >> 16U;
    return value;
}

}  // namespace

void generateDefaultName(
    const std::uint32_t seed,
    const std::uint32_t sequence,
    const std::uint32_t salt,
    char* const destination,
    const std::size_t destinationSize) {
    if (destination == nullptr || destinationSize == 0U) {
        return;
    }

    const std::uint32_t first = mix(seed ^ (sequence * 0x9E3779B9U) ^ salt);
    const std::uint32_t second = mix(first ^ 0xA5A5A5A5U ^ (salt << 1U));
    const char* const qualifier = text::kGrooveNameQualifiers[first % text::kGrooveNameQualifiers.size()];
    const char* const rhythm = text::kGrooveNameRhythmWords[second % text::kGrooveNameRhythmWords.size()];
    std::snprintf(destination, destinationSize, "%s %s", qualifier, rhythm);
}

}  // namespace clockfw::ui::groovename
