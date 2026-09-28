/**
 * @file story_text_renderer.cpp
 * @brief Implements measured Storybook text using builtin bitmap or configured host system fonts.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_text_renderer.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <cstdint>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

#include "hal/display_font.h"

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#endif

namespace clockfw::sim::tutorial {
namespace {

using Glyph = clockfw::hal::font::Glyph;

Glyph storyGlyph(const char character) {
    const Glyph production = clockfw::hal::font::glyphFor(character);
    if (production != clockfw::hal::font::kBlank || character == ' ') return production;
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
    if (font.family == "CLOCK Mono") return {0, 4, 6};
    if (font.family != "CLOCK UI") {
        throw std::runtime_error("requested builtin Storybook font is not available: " + font.family);
    }
    if (character == ' ') return {0, -1, 3};
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
    if (last < first) return {0, -1, 3};
    return {first, last, (last - first + 1) + 1};
}

int builtinScaleFor(const StoryFontStyle& font) {
    if (font.sizePx == 0U || (font.sizePx % 7U) != 0U) {
        throw std::runtime_error("builtin Storybook font size must be a positive multiple of 7 pixels");
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

#ifdef _WIN32

std::wstring utf8ToWide(const std::string& text) {
    if (text.empty()) return {};
    const int required = MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(),
                                              static_cast<int>(text.size()), nullptr, 0);
    if (required <= 0) throw std::runtime_error("Storybook text is not valid UTF-8");
    std::wstring result(static_cast<std::size_t>(required), L'\0');
    if (MultiByteToWideChar(CP_UTF8, MB_ERR_INVALID_CHARS, text.data(), static_cast<int>(text.size()),
                            result.data(), required) != required) {
        throw std::runtime_error("cannot convert Storybook text to UTF-16");
    }
    return result;
}

std::wstring familyToWide(const std::string& family) {
    return utf8ToWide(family);
}

struct WindowsFontContext {
    HDC dc = nullptr;
    HFONT font = nullptr;
    HGDIOBJ oldFont = nullptr;
    TEXTMETRICW metrics{};

    explicit WindowsFontContext(const StoryFontStyle& style) {
        dc = CreateCompatibleDC(nullptr);
        if (dc == nullptr) throw std::runtime_error("cannot create Windows font rendering context");
        const std::wstring family = familyToWide(style.family);
        const int weight = static_cast<int>(std::clamp<std::uint16_t>(style.weight, 100U, 1000U));
        font = CreateFontW(-static_cast<int>(style.sizePx), 0, 0, 0, weight, FALSE, FALSE, FALSE,
                           DEFAULT_CHARSET, OUT_TT_PRECIS, CLIP_DEFAULT_PRECIS, ANTIALIASED_QUALITY,
                           DEFAULT_PITCH | FF_DONTCARE, family.c_str());
        if (font == nullptr) {
            DeleteDC(dc);
            dc = nullptr;
            throw std::runtime_error("cannot create system Storybook font: " + style.family);
        }
        oldFont = SelectObject(dc, font);
        wchar_t resolved[LF_FACESIZE]{};
        if (GetTextFaceW(dc, LF_FACESIZE, resolved) <= 0 || _wcsicmp(resolved, family.c_str()) != 0) {
            if (oldFont != nullptr) SelectObject(dc, oldFont);
            DeleteObject(font);
            DeleteDC(dc);
            dc = nullptr;
            font = nullptr;
            throw std::runtime_error(
                "system Storybook font '" + style.family +
                "' is not installed; install it or choose another family in the Storybook theme");
        }
        if (GetTextMetricsW(dc, &metrics) == FALSE) {
            if (oldFont != nullptr) SelectObject(dc, oldFont);
            DeleteObject(font);
            DeleteDC(dc);
            dc = nullptr;
            font = nullptr;
            throw std::runtime_error("cannot read system Storybook font metrics: " + style.family);
        }
    }

    ~WindowsFontContext() {
        if (dc != nullptr && oldFont != nullptr) SelectObject(dc, oldFont);
        if (font != nullptr) DeleteObject(font);
        if (dc != nullptr) DeleteDC(dc);
    }

    WindowsFontContext(const WindowsFontContext&) = delete;
    WindowsFontContext& operator=(const WindowsFontContext&) = delete;
};

int systemTextWidth(const std::string& text, const StoryFontStyle& font) {
    if (text.empty()) return 0;
    WindowsFontContext context(font);
    const std::wstring wide = utf8ToWide(text);
    SIZE size{};
    if (GetTextExtentPoint32W(context.dc, wide.data(), static_cast<int>(wide.size()), &size) == FALSE) {
        throw std::runtime_error("cannot measure system Storybook text");
    }
    return size.cx;
}

int systemFontHeight(const StoryFontStyle& font) {
    WindowsFontContext context(font);
    return std::max(1, static_cast<int>(context.metrics.tmHeight));
}

int drawSystemTextLine(
    TutorialSurface& surface,
    const std::string& text,
    const StoryFontStyle& font,
    const int x,
    const int y,
    const TutorialColor color) {
    if (text.empty()) return 0;
    WindowsFontContext metricsContext(font);
    const std::wstring wide = utf8ToWide(text);
    SIZE extent{};
    if (GetTextExtentPoint32W(metricsContext.dc, wide.data(), static_cast<int>(wide.size()), &extent) == FALSE) {
        throw std::runtime_error("cannot measure system Storybook text");
    }
    const int width = std::max(1, extent.cx + 4);
    const int height = std::max(1, static_cast<int>(metricsContext.metrics.tmHeight) + 4);

    BITMAPINFO info{};
    info.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    info.bmiHeader.biWidth = width;
    info.bmiHeader.biHeight = -height;
    info.bmiHeader.biPlanes = 1;
    info.bmiHeader.biBitCount = 32;
    info.bmiHeader.biCompression = BI_RGB;
    void* pixels = nullptr;
    HBITMAP bitmap = CreateDIBSection(metricsContext.dc, &info, DIB_RGB_COLORS, &pixels, nullptr, 0);
    if (bitmap == nullptr || pixels == nullptr) throw std::runtime_error("cannot create system Storybook glyph raster");
    HGDIOBJ oldBitmap = SelectObject(metricsContext.dc, bitmap);
    std::fill_n(static_cast<std::uint8_t*>(pixels), static_cast<std::size_t>(width) * static_cast<std::size_t>(height) * 4U, 0U);
    SetBkColor(metricsContext.dc, RGB(0, 0, 0));
    SetBkMode(metricsContext.dc, OPAQUE);
    SetTextColor(metricsContext.dc, RGB(255, 255, 255));
    if (TextOutW(metricsContext.dc, 2, 1, wide.data(), static_cast<int>(wide.size())) == FALSE) {
        SelectObject(metricsContext.dc, oldBitmap);
        DeleteObject(bitmap);
        throw std::runtime_error("cannot render system Storybook text");
    }

    const auto* bytes = static_cast<const std::uint8_t*>(pixels);
    for (int py = 0; py < height; ++py) {
        for (int px = 0; px < width; ++px) {
            const std::size_t offset = (static_cast<std::size_t>(py) * static_cast<std::size_t>(width) +
                                        static_cast<std::size_t>(px)) * 4U;
            const std::uint8_t coverage = std::max({bytes[offset], bytes[offset + 1U], bytes[offset + 2U]});
            if (coverage == 0U) continue;
            const std::uint16_t alphaProduct = static_cast<std::uint16_t>(color.alpha) * coverage;
            const TutorialColor blended{color.red, color.green, color.blue,
                                        static_cast<std::uint8_t>((alphaProduct + 127U) / 255U)};
            surface.blendPixel(x + px - 2, y + py - 1, blended);
        }
    }
    SelectObject(metricsContext.dc, oldBitmap);
    DeleteObject(bitmap);
    return extent.cx;
}

#else

int systemTextWidth(const std::string&, const StoryFontStyle& font) {
    throw std::runtime_error(
        "system Storybook font backend is not available in this host build for '" + font.family +
        "'; use a builtin theme or render on Windows");
}

int systemFontHeight(const StoryFontStyle& font) {
    return systemTextWidth("M", font);
}

int drawSystemTextLine(
    TutorialSurface&,
    const std::string&,
    const StoryFontStyle& font,
    int,
    int,
    TutorialColor) {
    return systemTextWidth("M", font);
}

#endif

int fontPixelHeight(const StoryFontStyle& font) {
    if (font.backend == StoryFontBackend::System) return systemFontHeight(font);
    return static_cast<int>(font.sizePx);
}

}  // namespace

int measureStoryTextWidth(const std::string& text, const StoryFontStyle& font) {
    if (font.backend == StoryFontBackend::System) return systemTextWidth(text, font);
    const int scale = builtinScaleFor(font);
    int units = 0;
    for (const char character : text) units += glyphMetrics(character, font).advanceUnits;
    if (!text.empty()) units -= 1;
    return std::max(0, units * scale);
}

StoryTextLayout layoutStoryText(
    const std::string& text,
    const StoryFontStyle& font,
    const int maxWidthPx,
    const int maxHeightPx,
    const std::uint32_t lineSpacingPx) {
    if (maxWidthPx <= 0 || maxHeightPx <= 0) throw std::runtime_error("text layout box must be positive");
    const int glyphHeight = fontPixelHeight(font);
    const int lineHeight = glyphHeight + static_cast<int>(lineSpacingPx);
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
            if (!current.empty()) result.lines.push_back({current, measureStoryTextWidth(current, font)});
        }
        if (newline == std::string::npos) break;
        start = newline + 1U;
    }

    if (result.lines.empty()) result.lines.push_back({"", 0});
    result.widthPx = 0;
    for (const auto& line : result.lines) result.widthPx = std::max(result.widthPx, line.widthPx);
    result.heightPx = glyphHeight + static_cast<int>(result.lines.size() - 1U) * lineHeight;
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
    if (font.backend == StoryFontBackend::System) return drawSystemTextLine(surface, text, font, x, y, color);
    const int scale = builtinScaleFor(font);
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
