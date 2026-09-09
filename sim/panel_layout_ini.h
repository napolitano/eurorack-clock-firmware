/**
 * @file panel_layout_ini.h
 * @brief Internal strict INI parsing helpers for simulator panel geometry.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <filesystem>
#include <stdexcept>
#include <string>
#include <unordered_map>

#include "panel_layout.h"

namespace clockfw::sim::layout::ini {

using Section = std::unordered_map<std::string, std::string>;
using Document = std::unordered_map<std::string, Section>;

/**
 * @brief Parses one strict simulator panel-layout INI document.
 * @param path Input file path.
 * @return Parsed section/key document.
 */
Document parse(const std::filesystem::path& path);
/** @brief Removes one string value from a section or returns the supplied fallback. */
std::string take(Section& section, const std::string& key, const std::string& fallback);
/** @brief Removes and validates one floating-point layout value. */
float takeFloat(Section& section, const std::string& key, float fallback, const std::string& context);
/** @brief Removes and validates one integer layout value. */
int takeInt(Section& section, const std::string& key, int fallback, const std::string& context);
/** @brief Removes and validates one Boolean layout value. */
bool takeBool(Section& section, const std::string& key, bool fallback, const std::string& context);
/** @brief Removes and parses one configured RGB color value. */
Color takeColor(Section& section, const std::string& key, Color fallback, const std::string& context);
/** @brief Removes and parses one simulator jack-geometry profile identifier. */
JackType takeJackType(Section& section, const std::string& key, JackType fallback, const std::string& context);

/**
 * @brief Removes one named section, invokes a consumer, and rejects unconsumed keys.
 * @tparam Callback Callable accepting a mutable Section reference.
 * @param document Parsed document to consume.
 * @param sectionName Section name.
 * @param callback Consumer that removes all recognized keys.
 */
template <typename Callback>
void consumeSection(Document& document, const std::string& sectionName, Callback callback) {
    const auto sectionIterator = document.find(sectionName);
    if (sectionIterator == document.end()) return;
    Section section = std::move(sectionIterator->second);
    document.erase(sectionIterator);
    callback(section);
    if (!section.empty()) {
        throw std::runtime_error("unknown key '" + section.begin()->first + "' in [" + sectionName + "]");
    }
}

}  // namespace clockfw::sim::layout::ini
