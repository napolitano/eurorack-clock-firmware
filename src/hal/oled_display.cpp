/**
 * @file oled_display.cpp
 * @brief Minimal framebuffer and drawing primitives for SSD1306/SSD1315 OLEDs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/oled_display.h"

#include <algorithm>
#include <cstdlib>
#include <cstring>

#include "hal/display_font.h"

namespace clockfw::hal {

bool OledDisplay::begin() {
    static_assert(config::kDisplayWidth == 128U && config::kDisplayHeight == 64U,
                  "The built-in SSD1306/SSD1315 profiles target a 128x64 panel.");

    bool transportReady = false;
    if (config::kDisplayTransport == config::DisplayTransport::Spi) {
        // SPI control lines must be in a defined idle state before RESET is
        // released. In particular, keep CS high while the panel resets.
        transportReady = beginSpi();
        if (!transportReady) {
            return false;
        }
        resetController();
    } else {
        resetController();
        transportReady = beginI2c();
        if (!transportReady) {
            return false;
        }
    }

    // SSD1306 and SSD1315 expose the same command set used by this 128x64
    // profile. Waveshare likewise drives its SSD1306 0.96-inch A/B and SSD1315
    // 0.96-inch C/D/E modules through the same OLED_0in96 driver. Keep the
    // explicit controller selection in config so profiles can diverge later
    // without leaking controller knowledge into UI or transport code.
    switch (config::kDisplayController) {
        case config::DisplayController::Ssd1306:
        case config::DisplayController::Ssd1315:
            break;
    }
    sendCommand(0xAEU);  // Display off.
    sendCommand(0xD5U); sendCommand(0x80U);  // Clock divide / oscillator.
    sendCommand(0xA8U); sendCommand(0x3FU);  // Multiplex ratio 1/64.
    sendCommand(0xD3U); sendCommand(0x00U);  // Display offset.
    sendCommand(0x40U);                      // Start line 0.
    sendCommand(0x8DU); sendCommand(0x14U);  // Internal charge pump.
    sendCommand(0x20U); sendCommand(0x00U);  // Horizontal addressing mode.
    sendCommand(0xA1U);                      // Segment remap.
    sendCommand(0xC8U);                      // COM scan decrement.
    sendCommand(0xDAU); sendCommand(0x12U);  // COM pin configuration.
    sendCommand(0x81U); sendCommand(config::kDisplayContrast);
    sendCommand(0xD9U); sendCommand(0xF1U);  // Precharge period.
    sendCommand(0xDBU); sendCommand(0x40U);  // VCOMH deselect level.
    sendCommand(0xA4U);                      // Resume RAM display.
    sendCommand(0xA6U);                      // Normal, non-inverted pixels.
    sendCommand(0xAFU);                      // Display on.

    clear();
    present();
    // Startup occurs before the scheduler/output stage is enabled, so it is safe
    // to finish the initial blanking frame synchronously. Runtime I2C refreshes
    // are serviced incrementally instead.
    while (service()) {
    }
    return true;
}

void OledDisplay::clear() {
    framebuffer_.fill(0U);
}

void OledDisplay::setContrast(const std::uint8_t contrast) {
    sendCommand(0x81U);
    sendCommand(contrast);
}

void OledDisplay::setPower(const bool enabled) {
    sendCommand(enabled ? 0xAFU : 0xAEU);
}



#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
const std::array<std::uint8_t, OledDisplay::kFramebufferSize>& OledDisplay::framebufferForTest() const {
    return framebuffer_;
}

const std::array<std::uint8_t, OledDisplay::kFramebufferSize>& OledDisplay::presentedFramebufferForTest() const {
    return presentedFramebuffer_;
}
#endif

void OledDisplay::setFont(const DisplayFont font) {
    font_ = font;
}

void OledDisplay::setTextColor(const PixelColor color) {
    textColor_ = color;
}

TextBounds OledDisplay::measureText(
    const char* const text,
    const std::int16_t cursorX,
    const std::int16_t cursorY) const {
    TextBounds bounds{};
    bounds.x = cursorX;
    bounds.y = cursorY;

    if (font_ == DisplayFont::Tempo) {
        bounds.width = measureTempoTextWidth(text);
        bounds.height = font::kTempoGlyphHeight;
        return bounds;
    }
    if (font_ == DisplayFont::TempoLarge) {
        bounds.width = measureLargeTempoTextWidth(text);
        bounds.height = font::kLargeTempoGlyphHeight;
        return bounds;
    }

    const std::size_t length = text != nullptr ? std::strlen(text) : 0U;
    bounds.width = length == 0U
        ? 0U
        : static_cast<std::uint16_t>(length * 6U - 1U);
    bounds.height = 7U;
    return bounds;
}

void OledDisplay::drawText(
    const std::int16_t x,
    const std::int16_t y,
    const char* const text) {
    if (text == nullptr) {
        return;
    }

    std::int16_t cursorX = x;
    if (font_ == DisplayFont::Tempo) {
        for (const char* character = text; *character != '\0'; ++character) {
            const font::TempoGlyph glyph = font::tempoGlyphFor(*character);
            drawTempoGlyph(cursorX, y, *character);
            cursorX = static_cast<std::int16_t>(
                cursorX + glyph.width + font::kTempoGlyphSpacing);
        }
        return;
    }
    if (font_ == DisplayFont::TempoLarge) {
        for (const char* character = text; *character != '\0'; ++character) {
            const font::LargeTempoGlyph glyph = font::largeTempoGlyphFor(*character);
            drawLargeTempoGlyph(cursorX, y, *character);
            cursorX = static_cast<std::int16_t>(
                cursorX + glyph.width + font::kLargeTempoGlyphSpacing);
        }
        return;
    }

    for (const char* character = text; *character != '\0'; ++character) {
        drawSmallGlyph(cursorX, y, *character);
        cursorX = static_cast<std::int16_t>(cursorX + 6);
    }
}

void OledDisplay::drawCharacter(
    const std::int16_t x,
    const std::int16_t y,
    const char character) {
    if (font_ == DisplayFont::Tempo) {
        drawTempoGlyph(x, y, character);
    } else if (font_ == DisplayFont::TempoLarge) {
        drawLargeTempoGlyph(x, y, character);
    } else {
        drawSmallGlyph(x, y, character);
    }
}

void OledDisplay::drawHorizontalLine(
    const std::int16_t x,
    const std::int16_t y,
    const std::int16_t width,
    const PixelColor color) {
    for (std::int16_t offset = 0; offset < width; ++offset) {
        drawPixel(static_cast<std::int16_t>(x + offset), y, color);
    }
}

void OledDisplay::drawVerticalLine(
    const std::int16_t x,
    const std::int16_t y,
    const std::int16_t height,
    const PixelColor color) {
    for (std::int16_t offset = 0; offset < height; ++offset) {
        drawPixel(x, static_cast<std::int16_t>(y + offset), color);
    }
}

void OledDisplay::drawLine(
    std::int16_t x0,
    std::int16_t y0,
    const std::int16_t x1,
    const std::int16_t y1,
    const PixelColor color) {
    const std::int16_t dx = static_cast<std::int16_t>(std::abs(x1 - x0));
    const std::int16_t sx = x0 < x1 ? 1 : -1;
    const std::int16_t dy = static_cast<std::int16_t>(-std::abs(y1 - y0));
    const std::int16_t sy = y0 < y1 ? 1 : -1;
    std::int16_t error = static_cast<std::int16_t>(dx + dy);

    while (true) {
        drawPixel(x0, y0, color);
        if (x0 == x1 && y0 == y1) {
            break;
        }
        const std::int16_t doubledError = static_cast<std::int16_t>(2 * error);
        if (doubledError >= dy) {
            error = static_cast<std::int16_t>(error + dy);
            x0 = static_cast<std::int16_t>(x0 + sx);
        }
        if (doubledError <= dx) {
            error = static_cast<std::int16_t>(error + dx);
            y0 = static_cast<std::int16_t>(y0 + sy);
        }
    }
}

void OledDisplay::drawRectangle(
    const std::int16_t x,
    const std::int16_t y,
    const std::int16_t width,
    const std::int16_t height,
    const PixelColor color) {
    if (width <= 0 || height <= 0) {
        return;
    }
    drawHorizontalLine(x, y, width, color);
    drawHorizontalLine(x, static_cast<std::int16_t>(y + height - 1), width, color);
    drawVerticalLine(x, y, height, color);
    drawVerticalLine(static_cast<std::int16_t>(x + width - 1), y, height, color);
}

void OledDisplay::fillRectangle(
    const std::int16_t x,
    const std::int16_t y,
    const std::int16_t width,
    const std::int16_t height,
    const PixelColor color) {
    for (std::int16_t row = 0; row < height; ++row) {
        drawHorizontalLine(x, static_cast<std::int16_t>(y + row), width, color);
    }
}

void OledDisplay::setPixel(
    const std::int16_t x,
    const std::int16_t y,
    const PixelColor color) {
    drawPixel(x, y, color);
}

void OledDisplay::drawPixel(
    const std::int16_t x,
    const std::int16_t y,
    const PixelColor color) {
    if (x < 0 || y < 0 || x >= kWidth || y >= kHeight) {
        return;
    }
    const std::size_t byteIndex =
        static_cast<std::size_t>(x) + static_cast<std::size_t>(y / 8) * config::kDisplayWidth;
    const std::uint8_t bitMask = static_cast<std::uint8_t>(1U << (y & 7));
    if (color == PixelColor::White) {
        framebuffer_[byteIndex] |= bitMask;
    } else {
        framebuffer_[byteIndex] &= static_cast<std::uint8_t>(~bitMask);
    }
}

void OledDisplay::drawSmallGlyph(
    const std::int16_t x,
    const std::int16_t y,
    const char character) {
    const font::Glyph glyph = font::glyphFor(character);
    for (std::uint8_t row = 0U; row < 7U; ++row) {
        for (std::uint8_t column = 0U; column < 5U; ++column) {
            const bool set = (glyph[row] & static_cast<std::uint8_t>(1U << (4U - column))) != 0U;
            if (set) {
                drawPixel(
                    static_cast<std::int16_t>(x + column),
                    static_cast<std::int16_t>(y + row),
                    textColor_);
            }
        }
    }
}

void OledDisplay::drawTempoGlyph(
    const std::int16_t x,
    const std::int16_t y,
    const char character) {
    const font::TempoGlyph glyph = font::tempoGlyphFor(character);
    for (std::uint8_t row = 0U; row < font::kTempoGlyphHeight; ++row) {
        for (std::uint8_t column = 0U; column < glyph.width; ++column) {
            const std::uint8_t bitIndex = static_cast<std::uint8_t>(glyph.width - 1U - column);
            const bool set = (glyph.rows[row] & static_cast<std::uint16_t>(1U << bitIndex)) != 0U;
            if (set) {
                drawPixel(
                    static_cast<std::int16_t>(x + column),
                    static_cast<std::int16_t>(y + row),
                    textColor_);
            }
        }
    }
}

void OledDisplay::drawLargeTempoGlyph(
    const std::int16_t x,
    const std::int16_t y,
    const char character) {
    const font::LargeTempoGlyph glyph = font::largeTempoGlyphFor(character);
    for (std::uint8_t row = 0U; row < font::kLargeTempoGlyphHeight; ++row) {
        for (std::uint8_t column = 0U; column < glyph.width; ++column) {
            const std::uint8_t bitIndex = static_cast<std::uint8_t>(glyph.width - 1U - column);
            const bool set = (glyph.rows[row] & static_cast<std::uint32_t>(1UL << bitIndex)) != 0U;
            if (set) {
                drawPixel(
                    static_cast<std::int16_t>(x + column),
                    static_cast<std::int16_t>(y + row),
                    textColor_);
            }
        }
    }
}

std::uint16_t OledDisplay::measureTempoTextWidth(const char* const text) const {
    if (text == nullptr || *text == '\0') {
        return 0U;
    }

    std::uint16_t width = 0U;
    std::size_t glyphCount = 0U;
    for (const char* character = text; *character != '\0'; ++character) {
        width = static_cast<std::uint16_t>(width + font::tempoGlyphFor(*character).width);
        ++glyphCount;
    }
    if (glyphCount > 1U) {
        width = static_cast<std::uint16_t>(
            width + (glyphCount - 1U) * font::kTempoGlyphSpacing);
    }
    return width;
}

std::uint16_t OledDisplay::measureLargeTempoTextWidth(const char* const text) const {
    if (text == nullptr || *text == '\0') {
        return 0U;
    }

    std::uint16_t width = 0U;
    std::size_t glyphCount = 0U;
    for (const char* character = text; *character != '\0'; ++character) {
        width = static_cast<std::uint16_t>(width + font::largeTempoGlyphFor(*character).width);
        ++glyphCount;
    }
    if (glyphCount > 1U) {
        width = static_cast<std::uint16_t>(
            width + (glyphCount - 1U) * font::kLargeTempoGlyphSpacing);
    }
    return width;
}

}  // namespace clockfw::hal
