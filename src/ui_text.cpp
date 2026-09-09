/**
 * @file ui_text.cpp
 * @brief Runtime lookup implementation for centralized localized UI text.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui_text.h"

namespace clockfw::text {

const TextCatalog& catalogFor(const config::UiLanguage language) {
    switch (language) {
        case config::UiLanguage::EnglishUs:
            return kEnglishUs;
        case config::UiLanguage::GermanDe:
            // A partial translation is intentionally not shipped. Until a
            // complete German catalog exists, fall back atomically to en-US.
            return kEnglishUs;
        default:
            return kEnglishUs;
    }
}

const char* get(const TextId id, const config::UiLanguage language) {
    return catalogFor(language)[static_cast<std::size_t>(id)];
}

}  // namespace clockfw::text
