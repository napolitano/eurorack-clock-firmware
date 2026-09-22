/**
 * @file groove_name_generator.h
 * @brief Human-readable pseudo-random default names for user Custom Grooves.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstddef>
#include <cstdint>

namespace clockfw::ui::groovename {

/**
 * @brief Builds one compact two-word musical name from a session seed.
 * @param seed Per-boot/session seed supplied by the application.
 * @param sequence Monotonic generation counter within the current session.
 * @param salt Additional interaction-time salt so human timing changes the result.
 * @param destination Output buffer receiving a NUL-terminated name.
 * @param destinationSize Size of destination in bytes.
 *
 * This generator is intentionally pseudo-random rather than cryptographic. Its
 * job is to avoid repetitive placeholder names while keeping every suggestion
 * readable, editable and within the 16-character persistent-name budget.
 */
void generateDefaultName(
    std::uint32_t seed,
    std::uint32_t sequence,
    std::uint32_t salt,
    char* destination,
    std::size_t destinationSize);

}  // namespace clockfw::ui::groovename
