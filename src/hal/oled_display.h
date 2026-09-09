/**
 * @file oled_display.h
 * @brief Dependency-free framebuffer and SSD1306/SSD1315 transport abstraction.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstdint>

#include "config.h"

namespace clockfw::hal {

/** @brief Font roles supported by the display abstraction. */
enum class DisplayFont : std::uint8_t { Small, Tempo, TempoLarge };

/** @brief Monochrome pixel color used by drawing operations. */
enum class PixelColor : std::uint8_t { Black, White };

/** @brief Bounding rectangle returned by text measurement. */
struct TextBounds {
    std::int16_t x = 0;
    std::int16_t y = 0;
    std::uint16_t width = 0U;
    std::uint16_t height = 0U;
};

/**
 * @brief Owns the 1 KiB framebuffer and the minimal SSD1306/SSD1315 protocol implementation.
 *
 * The class deliberately exposes only primitives required by the firmware UI.
 * It has no dependency on Adafruit GFX or Adafruit SSD1306. Bus selection is a
 * compile-time configuration choice in config.h and is invisible to callers.
 */
class OledDisplay final {
public:
    /** @brief Physical display width in pixels. */
    static constexpr std::int16_t kWidth = static_cast<std::int16_t>(config::kDisplayWidth);

    /** @brief Physical display height in pixels. */
    static constexpr std::int16_t kHeight = static_cast<std::int16_t>(config::kDisplayHeight);

    /** @brief Number of bytes in the page-oriented monochrome framebuffer. */
    static constexpr std::size_t kFramebufferSize =
        static_cast<std::size_t>(config::kDisplayWidth) * config::kDisplayHeight / 8U;

    /** @brief Initializes the configured bus and OLED controller. */
    bool begin();

    /** @brief Clears the in-memory framebuffer. */
    void clear();

    /** @brief Publishes the current framebuffer for display; SPI transfers immediately, I2C is deferred. */
    void present();

    /**
     * @brief Services at most one deferred I2C bus transaction.
     * @return true when one I2C transaction was attempted, false when no deferred work exists.
     *
     * SPI builds keep their immediate dirty-page transfer path, so this method
     * becomes a cheap no-op. Callers may therefore service it unconditionally.
     */
    bool service();

    /** @brief Selects the font role used by subsequent text operations. */
    void setFont(DisplayFont font);

    /** @brief Selects the monochrome text color. */
    void setTextColor(PixelColor color);

    /** @brief Measures a null-terminated text string using the selected font. */
    TextBounds measureText(const char* text, std::int16_t cursorX, std::int16_t cursorY) const;

    /** @brief Draws text at the specified cursor position using the selected font. */
    void drawText(std::int16_t x, std::int16_t y, const char* text);

    /** @brief Draws one ASCII character at the specified cursor position. */
    void drawCharacter(std::int16_t x, std::int16_t y, char character);

    /** @brief Draws a one-pixel horizontal line. */
    void drawHorizontalLine(std::int16_t x, std::int16_t y, std::int16_t width, PixelColor color = PixelColor::White);

    /** @brief Draws a one-pixel vertical line. */
    void drawVerticalLine(std::int16_t x, std::int16_t y, std::int16_t height, PixelColor color = PixelColor::White);

    /** @brief Draws a line between two pixel coordinates. */
    void drawLine(std::int16_t x0, std::int16_t y0, std::int16_t x1, std::int16_t y1, PixelColor color = PixelColor::White);

    /** @brief Draws a rectangle outline. */
    void drawRectangle(std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height, PixelColor color = PixelColor::White);

    /** @brief Draws a filled rectangle. */
    void fillRectangle(std::int16_t x, std::int16_t y, std::int16_t width, std::int16_t height, PixelColor color = PixelColor::White);

    /** @brief Sets one framebuffer pixel with clipping. */
    void setPixel(std::int16_t x, std::int16_t y, PixelColor color = PixelColor::White);

    /** @brief Changes the OLED panel contrast without modifying framebuffer contents. */
    void setContrast(std::uint8_t contrast);

    /** @brief Switches the OLED panel on or off while preserving display RAM. */
    void setPower(bool enabled);

#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
    /** @brief Returns the raw drawing framebuffer for deterministic host screenshots and native simulation. */
    const std::array<std::uint8_t, kFramebufferSize>& framebufferForTest() const;

    /** @brief Returns the bytes last confirmed as transferred to the physical-display model. */
    const std::array<std::uint8_t, kFramebufferSize>& presentedFramebufferForTest() const;
#endif

private:

    /** @brief Configures the I2C transport and resolves the OLED address. */
    bool beginI2c();

    /** @brief Configures the SPI transport GPIO and peripheral. */
    bool beginSpi();

    /** @brief Pulses the optional active-low hardware reset line when one is configured. */
    void resetController();

    /** @brief Sends one controller command byte over the selected transport. */
    void sendCommand(std::uint8_t command);

    /** @brief Sends a contiguous controller command sequence in one bus transaction. */
    void sendCommands(const std::uint8_t* commands, std::size_t size);

    /** @brief Sends a contiguous block of framebuffer data over the selected transport. */
    void sendData(const std::uint8_t* data, std::size_t size);

    /** @brief Runs the existing immediate dirty-page transfer used by SPI builds. */
    void presentImmediate();

    /** @brief Queues the newest complete framebuffer as the current I2C refresh target. */
    void queueI2cPresent();

    /** @brief Services one I2C address-window or data transaction. */
    bool serviceI2cRefresh();

    /** @brief Sends one bounded I2C data transaction and reports ACK success. */
    bool sendI2cDataChunk(const std::uint8_t* data, std::size_t size);

    /** @brief Sends one bounded I2C command transaction and reports ACK success. */
    bool sendI2cCommandChunk(const std::uint8_t* commands, std::size_t size);

    /** @brief Probes one I2C address for an ACK response. */
    bool isI2cDevicePresent(std::uint8_t address);

    /** @brief Writes one framebuffer pixel with clipping. */
    void drawPixel(std::int16_t x, std::int16_t y, PixelColor color);

    /** @brief Draws one project-owned compact 5x7 glyph at its native resolution. */
    void drawSmallGlyph(std::int16_t x, std::int16_t y, char character);

    /** @brief Draws one dedicated proportional tempo numeral at its native resolution. */
    void drawTempoGlyph(std::int16_t x, std::int16_t y, char character);

    /** @brief Draws one larger CLOCK-mode tempo numeral at its native resolution. */
    void drawLargeTempoGlyph(std::int16_t x, std::int16_t y, char character);

    /** @brief Measures a complete string rendered with the dedicated tempo numeral font. */
    std::uint16_t measureTempoTextWidth(const char* text) const;

    /** @brief Measures a complete string rendered with the larger CLOCK-mode tempo font. */
    std::uint16_t measureLargeTempoTextWidth(const char* text) const;

    enum class I2cRefreshPhase : std::uint8_t { Idle, SetPageWindow, SendPageData };

    std::array<std::uint8_t, kFramebufferSize> framebuffer_{};
    std::array<std::uint8_t, kFramebufferSize> presentedFramebuffer_{};
    std::array<std::uint8_t, kFramebufferSize> queuedI2cFramebuffer_{};
    DisplayFont font_ = DisplayFont::Small;
    PixelColor textColor_ = PixelColor::White;
    std::uint8_t i2cAddress_ = 0U;
    std::uint8_t i2cRefreshPage_ = 0U;
    std::uint8_t i2cRefreshOffset_ = 0U;
    I2cRefreshPhase i2cRefreshPhase_ = I2cRefreshPhase::Idle;
    bool i2cFrameQueued_ = false;
    bool hasPresentedFramebuffer_ = false;
};

}  // namespace clockfw::hal
