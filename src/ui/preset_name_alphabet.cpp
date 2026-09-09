/**
 * @file preset_name_alphabet.cpp
 * @brief Character lookup helpers for the cyclic preset-name editor band.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/preset_name_alphabet.h"

namespace clockfw::ui::presetname {

std::size_t characterIndex(const char character) {
    for (std::size_t index = 0U; index < kAlphabetLength; ++index) {
        if (kAlphabet[index] == character) {
            return index;
        }
    }
    return 0U;
}

char characterAtWrapped(const int index) {
    const int length = static_cast<int>(kAlphabetLength);
    const int wrapped = ((index % length) + length) % length;
    return kAlphabet[static_cast<std::size_t>(wrapped)];
}

}  // namespace clockfw::ui::presetname
