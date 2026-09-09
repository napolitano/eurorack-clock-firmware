/**
 * @file panel_layout_ini.cpp
 * @brief Internal strict INI parsing helpers for simulator panel geometry.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "panel_layout_ini.h"

#include <algorithm>
#include <cctype>
#include <cmath>
#include <fstream>

namespace clockfw::sim::layout::ini {
namespace {

std::string trim(std::string value) {
    const auto isSpace = [](const unsigned char character) { return std::isspace(character) != 0; };
    value.erase(value.begin(), std::find_if(value.begin(), value.end(), [&](const char character) {
        return !isSpace(static_cast<unsigned char>(character));
    }));
    value.erase(std::find_if(value.rbegin(), value.rend(), [&](const char character) {
        return !isSpace(static_cast<unsigned char>(character));
    }).base(), value.end());
    return value;
}

std::string lowercase(std::string value) {
    std::transform(value.begin(), value.end(), value.begin(), [](const unsigned char character) {
        return static_cast<char>(std::tolower(character));
    });
    return value;
}

float parseFloat(const std::string& value, const std::string& context) {
    std::size_t consumed = 0U;
    const float parsed = std::stof(value, &consumed);
    if (consumed != value.size() || !std::isfinite(parsed)) {
        throw std::runtime_error("invalid number for " + context + ": " + value);
    }
    return parsed;
}

int parseInt(const std::string& value, const std::string& context) {
    std::size_t consumed = 0U;
    const int parsed = std::stoi(value, &consumed);
    if (consumed != value.size()) {
        throw std::runtime_error("invalid integer for " + context + ": " + value);
    }
    return parsed;
}

bool parseBool(const std::string& value, const std::string& context) {
    const std::string normalized = lowercase(trim(value));
    if (normalized == "true" || normalized == "yes" || normalized == "1" || normalized == "on") return true;
    if (normalized == "false" || normalized == "no" || normalized == "0" || normalized == "off") return false;
    throw std::runtime_error("invalid boolean for " + context + ": " + value);
}

std::uint8_t parseHexByte(const std::string& text, const std::size_t offset, const std::string& context) {
    std::size_t consumed = 0U;
    const unsigned long parsed = std::stoul(text.substr(offset, 2U), &consumed, 16);
    if (consumed != 2U || parsed > 255UL) {
        throw std::runtime_error("invalid RGB color for " + context + ": " + text);
    }
    return static_cast<std::uint8_t>(parsed);
}

Color parseColor(const std::string& value, const std::string& context) {
    if (value.size() != 7U || value.front() != '#') {
        throw std::runtime_error("RGB color for " + context + " must use #RRGGBB");
    }
    try {
        return {parseHexByte(value, 1U, context), parseHexByte(value, 3U, context), parseHexByte(value, 5U, context)};
    } catch (const std::exception&) {
        throw std::runtime_error("invalid RGB color for " + context + ": " + value);
    }
}

JackType parseJackType(const std::string& value, const std::string& context) {
    const std::string normalized = lowercase(trim(value));
    if (normalized == "ts" || normalized == "mono") return JackType::Ts;
    if (normalized == "trs" || normalized == "stereo") return JackType::Trs;
    throw std::runtime_error("invalid jack type for " + context + ": " + value + " (expected ts or trs)");
}

}  // namespace

Document parse(const std::filesystem::path& path) {
    std::ifstream input(path);
    if (!input.is_open()) throw std::runtime_error("cannot open panel layout: " + path.string());

    Document document{};
    std::string sectionName{};
    std::string line{};
    std::size_t lineNumber = 0U;
    while (std::getline(input, line)) {
        ++lineNumber;
        line = trim(line);
        if (line.empty() || line.front() == '#' || line.front() == ';') continue;
        if (line.front() == '[' && line.back() == ']') {
            sectionName = lowercase(trim(line.substr(1U, line.size() - 2U)));
            if (sectionName.empty()) throw std::runtime_error("empty layout section at line " + std::to_string(lineNumber));
            continue;
        }
        if (sectionName.empty()) throw std::runtime_error("layout key outside a section at line " + std::to_string(lineNumber));
        const std::size_t separator = line.find('=');
        if (separator == std::string::npos) throw std::runtime_error("invalid layout assignment at line " + std::to_string(lineNumber));
        const std::string key = lowercase(trim(line.substr(0U, separator)));
        const std::string value = trim(line.substr(separator + 1U));
        if (key.empty()) throw std::runtime_error("empty layout key at line " + std::to_string(lineNumber));
        if (!document[sectionName].emplace(key, value).second) {
            throw std::runtime_error("duplicate layout key '" + key + "' in [" + sectionName + "]");
        }
    }
    return document;
}

std::string take(Section& section, const std::string& key, const std::string& fallback) {
    const auto iterator = section.find(key);
    if (iterator == section.end()) return fallback;
    const std::string value = iterator->second;
    section.erase(iterator);
    return value;
}

float takeFloat(Section& section, const std::string& key, const float fallback, const std::string& context) {
    const auto iterator = section.find(key);
    if (iterator == section.end()) return fallback;
    const float value = parseFloat(iterator->second, context + "." + key);
    section.erase(iterator);
    return value;
}

int takeInt(Section& section, const std::string& key, const int fallback, const std::string& context) {
    const auto iterator = section.find(key);
    if (iterator == section.end()) return fallback;
    const int value = parseInt(iterator->second, context + "." + key);
    section.erase(iterator);
    return value;
}

bool takeBool(Section& section, const std::string& key, const bool fallback, const std::string& context) {
    const auto iterator = section.find(key);
    if (iterator == section.end()) return fallback;
    const bool value = parseBool(iterator->second, context + "." + key);
    section.erase(iterator);
    return value;
}

Color takeColor(Section& section, const std::string& key, const Color fallback, const std::string& context) {
    const auto iterator = section.find(key);
    if (iterator == section.end()) return fallback;
    const Color value = parseColor(iterator->second, context + "." + key);
    section.erase(iterator);
    return value;
}

JackType takeJackType(Section& section, const std::string& key, const JackType fallback, const std::string& context) {
    const auto iterator = section.find(key);
    if (iterator == section.end()) return fallback;
    const JackType value = parseJackType(iterator->second, context + "." + key);
    section.erase(iterator);
    return value;
}

}  // namespace clockfw::sim::layout::ini
