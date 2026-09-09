/**
 * @file preset_name_alphabet.h
 * @brief Shared character repertoire for named user presets.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>

namespace clockfw::ui::presetname {

/** Complete cyclic character band available to the preset-name editor. */
inline constexpr char kAlphabet[] = " ABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789-_";

/** @brief Number of selectable characters, excluding the terminating null byte. */
inline constexpr std::size_t kAlphabetLength = sizeof(kAlphabet) - 1U;

/** @brief Returns the alphabet position of a stored character, falling back to space. */
std::size_t characterIndex(char character);

/** @brief Returns one alphabet character using wrap-around indexing. */
char characterAtWrapped(int index);

}  // namespace clockfw::ui::presetname
