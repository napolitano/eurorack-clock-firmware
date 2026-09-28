/**
 * @file story_text_renderer.cpp
 * @brief Implements measured deterministic Storybook text using CLOCK-owned 5x7 glyphs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_text_renderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <sstream>
#include <stdexcept>

#include "hal/display_font.h"

namespace clockfw::sim::tutorial {
namespace {

using Glyph = clockfw::hal::font::Glyph;

Glyph storyGlyph(const char character) {
    const Glyph production = clockfw::hal::font::glyphFor(character);
    if (production != clockfw::hal::font::kBlank || character == ' ') {
        return production;
    }
    switch (character) {
        case ',': return {{0,0,0,0,0,4,8}};
        case ';': return {{0,4,4,0,4,4,8}};
        case '\'': return {{4,4,8,0,0,0,0}};
        case '"': return {{10,10,0,0,0,0,0}};
        case '(': return {{2,4,8,8,8,4,2}};
        case ')': return {{8,4,2,2,2,4,8}};
        case '[': return {{14,8,8,8,8,8,14}};
        case ']': return {{14,2,2,2,2,2,14}};
        case '#': return {{10,31,10,10,31,10,0}};
        case '&': return {{12,18,20,8,21,18,13}};
        case '@': return {{14,17,23,21,23,16,14}};
        default: return clockfw::hal::font::glyphFor('?');
    }
}

struct GlyphMetrics {
    int firstColumn = 0;
    int lastColumn = 4;
    int advanceUnits = 6;
};

GlyphMetrics glyphMetrics(const char character, const StoryFontStyle& font) {
    if (font.family == "CLOCK Mono") {
        return {0, 4, 6};
    }
    if (font.family != "CLOCK UI") {
        throw std::runtime_error("requested Storybook font is not available: " + font.family);
    }
    if (character == ' ') {
        return {0, -1, 3};
    }
    const Glyph glyph = storyGlyph(character);
    int first = 5;
    int last = -1;
    for (int column = 0; column < 5; ++column) {
        const std::uint8_t mask = static_cast<std::uint8_t>(1U << static_cast<unsigned>(4 - column));
        for (const std::uint8_t row : glyph) {
            if ((row & mask) != 0U) {
                first = std::min(first, column);
                last = std::max(last, column);
                break;
            }
        }
    }
    if (last < first) {
        return {0, -1, 3};
    }
    return {first, last, (last - first + 1) + 1};
}

int scaleFor(const StoryFontStyle& font) {
    if (font.sizePx == 0U || (font.sizePx % 7U) != 0U) {
        throw std::runtime_error("Storybook font size must be a positive multiple of 7 pixels");
    }
    return static_cast<int>(font.sizePx / 7U);
}

std::vector<std::string> paragraphWords(const std::string& paragraph) {
    std::vector<std::string> words;
    std::istringstream input(paragraph);
    std::string word;
    while (input >> word) words.push_back(word);
    return words;
}

}  // namespace

int measureStoryTextWidth(const std::string& text, const StoryFontStyle& font) {
    const int scale = scaleFor(font);
    int units = 0;
    for (const char character : text) {
        units += glyphMetrics(character, font).advanceUnits;
    }
    if (!text.empty()) units -= 1;  // omit final inter-glyph spacing
    return std::max(0, units * scale);
}

StoryTextLayout layoutStoryText(
    const std::string& text,
    const StoryFontStyle& font,
    const int maxWidthPx,
    const int maxHeightPx,
    const std::uint32_t lineSpacingPx) {
    if (maxWidthPx <= 0 || maxHeightPx <= 0) {
        throw std::runtime_error("text layout box must be positive");
    }
    const int lineHeight = static_cast<int>(font.sizePx + lineSpacingPx);
    StoryTextLayout result{};
    result.lineHeightPx = lineHeight;

    std::size_t start = 0U;
    while (start <= text.size()) {
        const std::size_t newline = text.find('\n', start);
        const std::string paragraph = text.substr(start, newline == std::string::npos ? std::string::npos : newline - start);
        const auto words = paragraphWords(paragraph);
        if (words.empty()) {
            result.lines.push_back({"", 0});
        } else {
            std::string current;
            for (const std::string& word : words) {
                if (measureStoryTextWidth(word, font) > maxWidthPx) {
                    throw std::runtime_error("text overflow: unbreakable word exceeds available width: " + word);
                }
                const std::string candidate = current.empty() ? word : current + " " + word;
                const int candidateWidth = measureStoryTextWidth(candidate, font);
                if (!current.empty() && candidateWidth > maxWidthPx) {
                    result.lines.push_back({current, measureStoryTextWidth(current, font)});
                    current = word;
                } else {
                    current = candidate;
                }
            }
            if (!current.empty()) {
                result.lines.push_back({current, measureStoryTextWidth(current, font)});
            }
        }
        if (newline == std::string::npos) break;
        start = newline + 1U;
    }

    if (result.lines.empty()) result.lines.push_back({"", 0});
    result.widthPx = 0;
    for (const auto& line : result.lines) result.widthPx = std::max(result.widthPx, line.widthPx);
    result.heightPx = static_cast<int>(font.sizePx) +
        static_cast<int>(result.lines.size() - 1U) * lineHeight;
    if (result.widthPx > maxWidthPx || result.heightPx > maxHeightPx) {
        throw std::runtime_error("text overflow: measured block exceeds available layout box");
    }
    return result;
}

int drawStoryTextLine(
    TutorialSurface& surface,
    const std::string& text,
    const StoryFontStyle& font,
    int x,
    const int y,
    const TutorialColor color) {
    const int scale = scaleFor(font);
    const int startX = x;
    for (const char character : text) {
        const Glyph glyph = storyGlyph(character);
        const GlyphMetrics metrics = glyphMetrics(character, font);
        if (metrics.lastColumn >= metrics.firstColumn) {
            for (int row = 0; row < 7; ++row) {
                for (int column = metrics.firstColumn; column <= metrics.lastColumn; ++column) {
                    const std::uint8_t mask = static_cast<std::uint8_t>(1U << static_cast<unsigned>(4 - column));
                    if ((glyph[static_cast<std::size_t>(row)] & mask) == 0U) continue;
                    const int localColumn = font.family == "CLOCK Mono" ? column : column - metrics.firstColumn;
                    surface.fillRect({x + localColumn * scale, y + row * scale, scale, scale}, color);
                }
            }
        }
        x += metrics.advanceUnits * scale;
    }
    return x - startX;
}

void drawStoryText(
    TutorialSurface& surface,
    const StoryTextLayout& layout,
    const StoryFontStyle& font,
    const int x,
    int y,
    const TutorialColor color) {
    for (const auto& line : layout.lines) {
        (void)drawStoryTextLine(surface, line.text, font, x, y, color);
        y += layout.lineHeightPx;
    }
}

}  // namespace clockfw::sim::tutorial
