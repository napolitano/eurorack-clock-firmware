/**
 * @file display_font.h
 * @brief Pixel-font data used by the monochrome OLED display HAL.
 *
 * The compact 5x7 UI font is project-authored. Tempo numerals are native-resolution
 * 1-bit raster derivatives of Roboto Condensed Bold, licensed under Apache-2.0;
 * see THIRD_PARTY_NOTICES.md.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstdint>

namespace clockfw::hal::font {

/** Seven 5-bit rows describing one compact 5x7 UI glyph. */
using Glyph = std::array<std::uint8_t, 7U>;

/** Empty compact glyph used for spaces and unsupported characters. */
inline constexpr Glyph kBlank{{0, 0, 0, 0, 0, 0, 0}};

/** Height of the normal dedicated tempo font in physical OLED pixels. */
inline constexpr std::uint8_t kTempoGlyphHeight = 18U;

/** Maximum bitmap width of one normal dedicated tempo glyph. */
inline constexpr std::uint8_t kTempoGlyphMaximumWidth = 12U;

/** Horizontal whitespace inserted between normal tempo glyphs. */
inline constexpr std::uint8_t kTempoGlyphSpacing = 2U;

/** Height of the larger CLOCK-mode tempo font in physical OLED pixels. */
inline constexpr std::uint8_t kLargeTempoGlyphHeight = 28U;

/** Maximum bitmap width of one larger CLOCK-mode tempo glyph. */
inline constexpr std::uint8_t kLargeTempoGlyphMaximumWidth = 19U;

/** Horizontal whitespace inserted between larger tempo glyphs. */
inline constexpr std::uint8_t kLargeTempoGlyphSpacing = 2U;

/** @brief One proportional, unscaled normal tempo glyph stored as eighteen 12-bit rows. */
struct TempoGlyph {
    std::uint8_t width = 0U;
    std::array<std::uint16_t, kTempoGlyphHeight> rows{};
};

/** @brief One native, unscaled large tempo glyph stored as twenty-eight 32-bit rows. */
struct LargeTempoGlyph {
    std::uint8_t width = 0U;
    std::array<std::uint32_t, kLargeTempoGlyphHeight> rows{};
};

/**
 * @brief Returns the project-owned 5x7 bitmap for one supported ASCII character.
 * @param character ASCII character. Lowercase letters are normalized to uppercase.
 * @return Seven-row bitmap; unsupported glyphs render as blanks.
 */
Glyph glyphFor(char character);

/**
 * @brief Returns one native-resolution normal tempo glyph.
 * @param character ASCII digit or '-'.
 * @return Pixel-exact 18-pixel-high glyph; unsupported characters return a blank glyph.
 */
TempoGlyph tempoGlyphFor(char character);

/**
 * @brief Returns one native-resolution large CLOCK-mode tempo glyph.
 * @param character ASCII digit or '-'.
 * @return Pixel-exact 24-pixel-high glyph; unsupported characters return a blank glyph.
 */
LargeTempoGlyph largeTempoGlyphFor(char character);

}  // namespace clockfw::hal::font
