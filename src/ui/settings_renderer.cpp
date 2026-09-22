/**
 * @file settings_renderer.cpp
 * @brief Rendering implementation for settings and template selection screens.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/settings_renderer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdio>
#include <cstring>

#include "services/template_service.h"
#include "ui/menu_model.h"
#include "ui/menu_model_channel.h"
#include "ui/preset_name_alphabet.h"
#include "ui_text.h"

namespace clockfw::ui {
namespace {

/** Number of text-setting rows visible at once on the 64-pixel display. */
constexpr std::uint8_t kVisibleSettingsRows = 5U;

/** Minimum horizontal breathing room between a settings label and its right-aligned value. */
constexpr std::int16_t kSettingsValueGap = 4;

/**
 * Copies one read-only value, shortening it with three dots only when it would
 * collide with its label. Editable values deliberately do not use this path:
 * an abbreviated editable value would hide the value that the encoder changes.
 */
void fitInformationalValue(
    hal::OledDisplay& display,
    const char* const source,
    const std::int16_t maximumWidth,
    char* const destination,
    const std::size_t destinationSize) {
    if (destination == nullptr || destinationSize == 0U) {
        return;
    }
    destination[0] = '\0';
    if (source == nullptr || maximumWidth <= 0) {
        return;
    }
    std::snprintf(destination, destinationSize, "%s", source);
    if (static_cast<std::int16_t>(display.measureText(destination, 0, 0).width) <= maximumWidth) {
        return;
    }

    constexpr char kEllipsis[] = "...";
    const std::size_t sourceLength = std::strlen(source);
    std::size_t prefixLength = std::min<std::size_t>(sourceLength, destinationSize - sizeof(kEllipsis));
    while (prefixLength > 0U) {
        std::memcpy(destination, source, prefixLength);
        destination[prefixLength] = '\0';
        std::strncat(destination, kEllipsis, destinationSize - std::strlen(destination) - 1U);
        if (static_cast<std::int16_t>(display.measureText(destination, 0, 0).width) <= maximumWidth) {
            return;
        }
        --prefixLength;
    }
    std::snprintf(destination, destinationSize, "%s", kEllipsis);
}

/**
 * Version-2 QR payload for exactly: https://github.com/napolitano
 * Error correction: L. The 25x25 symbol is rendered at 2x inside a white
 * 64x64 field, leaving a 7-pixel quiet margin on every side.
 */
constexpr std::array<std::uint32_t, 25U> kUpdateQrRows{{
    0x01FC5D7FUL, 0x01055741UL, 0x0174245DUL, 0x0174AC5DUL, 0x0174475DUL,
    0x01056141UL, 0x01FD557FUL, 0x0000DD00UL, 0x019F9767UL, 0x01ADBE9DUL,
    0x016389FFUL, 0x002B8220UL, 0x010D1BE3UL, 0x018E9A98UL, 0x0165FD5FUL,
    0x0038D120UL, 0x009F8CE3UL, 0x01115B00UL, 0x01156A7FUL, 0x00918141UL,
    0x001F965DUL, 0x00D2B85DUL, 0x01BB1D5DUL, 0x001DD741UL, 0x0127157FUL}};

/** Draws the scannable update QR as a conventional black-on-white symbol. */
void drawUpdateQr(hal::OledDisplay& display) {
    constexpr std::int16_t kFieldX = 32;
    constexpr std::int16_t kQrX = 39;
    constexpr std::int16_t kQrY = 7;
    constexpr std::int16_t kScale = 2;

    display.fillRectangle(kFieldX, 0, 64, 64, hal::PixelColor::White);
    for (std::size_t y = 0U; y < kUpdateQrRows.size(); ++y) {
        for (std::uint8_t x = 0U; x < 25U; ++x) {
            if ((kUpdateQrRows[y] & (1UL << x)) == 0U) {
                continue;
            }
            display.fillRectangle(
                static_cast<std::int16_t>(kQrX + static_cast<std::int16_t>(x) * kScale),
                static_cast<std::int16_t>(kQrY + static_cast<std::int16_t>(y) * kScale),
                kScale,
                kScale,
                hal::PixelColor::Black);
        }
    }
}

}  // namespace

SettingsRenderer::SettingsRenderer(hal::OledDisplay& display) : display_(display) {}

bool SettingsRenderer::informationValueOverflows(
    const char* const label,
    const char* const value) {
    if (label == nullptr || value == nullptr || value[0] == '\0') {
        return false;
    }
    display_.setFont(hal::DisplayFont::Small);
    const hal::TextBounds labelBounds = display_.measureText(label, 0, 0);
    const hal::TextBounds valueBounds = display_.measureText(value, 0, 0);
    const std::int16_t labelRight = static_cast<std::int16_t>(2 + labelBounds.width);
    const std::int16_t maximumValueWidth = static_cast<std::int16_t>(
        121 - labelRight - kSettingsValueGap);
    return static_cast<std::int16_t>(valueBounds.width) > maximumValueWidth;
}

namespace {

void drawDiagnosticBox(
    hal::OledDisplay& display,
    const std::int16_t x,
    const std::int16_t y,
    const std::int16_t width,
    const std::int16_t height,
    const char* const label,
    const bool active) {
    if (active) {
        display.fillRectangle(x, y, width, height, hal::PixelColor::White);
        display.setTextColor(hal::PixelColor::Black);
    } else {
        display.drawRectangle(x, y, width, height, hal::PixelColor::White);
        display.setTextColor(hal::PixelColor::White);
    }
    const hal::TextBounds bounds = display.measureText(label, 0, 0);
    const std::int16_t textX = static_cast<std::int16_t>(x + (width - static_cast<std::int16_t>(bounds.width)) / 2);
    const std::int16_t textY = static_cast<std::int16_t>(y + (height - 7) / 2);
    display.drawText(textX, textY, label);
    display.setTextColor(hal::PixelColor::White);
}

void renderDiagnosticHeader(hal::OledDisplay& display, const char* const title) {
    display.clear();
    display.setFont(hal::DisplayFont::Small);
    display.setTextColor(hal::PixelColor::White);
    const hal::TextBounds bounds = display.measureText(title, 0, 0);
    display.drawText(
        static_cast<std::int16_t>((static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(bounds.width)) / 2),
        0,
        title);
    display.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);
}

}  // namespace

void SettingsRenderer::renderSettings(
    const ClockState& state,
    const NavigationState& navigation,
    const DiagnosticSnapshot& diagnostics) {
    display_.clear();
    if (navigation.settingsPage == SettingsPage::DiagnosticsInputs) {
        renderDiagnosticHeader(display_, text::get(text::TextId::Inputs));
        constexpr std::int16_t kWidth = 44;
        constexpr std::int16_t kHeight = 22;
        constexpr std::int16_t kGap = 8;
        constexpr std::int16_t kStartX = (128 - (kWidth * 2 + kGap)) / 2;
        drawDiagnosticBox(display_, kStartX, 25, kWidth, kHeight, text::get(text::TextId::Input1), diagnostics.syncHigh);
        drawDiagnosticBox(display_, kStartX + kWidth + kGap, 25, kWidth, kHeight, text::get(text::TextId::Input2), diagnostics.resetHigh);
        display_.present();
        return;
    }
    if (navigation.settingsPage == SettingsPage::DiagnosticsOutputs) {
        renderDiagnosticHeader(display_, text::get(text::TextId::Outputs));
        constexpr std::int16_t kWidth = 25;
        constexpr std::int16_t kHeight = 18;
        constexpr std::int16_t kGapX = 6;
        constexpr std::int16_t kGapY = 5;
        constexpr std::int16_t kStartX = (128 - (kWidth * 4 + kGapX * 3)) / 2;
        constexpr std::int16_t kStartY = 15;
        for (std::uint8_t index = 0U; index < 8U; ++index) {
            char label[2]{static_cast<char>('1' + index), '\0'};
            const std::int16_t column = static_cast<std::int16_t>(index % 4U);
            const std::int16_t row = static_cast<std::int16_t>(index / 4U);
            drawDiagnosticBox(
                display_,
                static_cast<std::int16_t>(kStartX + column * (kWidth + kGapX)),
                static_cast<std::int16_t>(kStartY + row * (kHeight + kGapY)),
                kWidth,
                kHeight,
                label,
                diagnostics.outputs[index]);
        }
        display_.present();
        return;
    }
    if (navigation.settingsPage == SettingsPage::Updates) {
        drawUpdateQr(display_);
        display_.present();
        return;
    }
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    display_.drawText(0, 0, settingsPageTitle(navigation.settingsPage));

    if (isChannelSettingsPage(navigation.settingsPage)) {
        char channelText[6]{};
        std::snprintf(
            channelText,
            sizeof(channelText),
            text::get(text::TextId::ChannelFormat),
            navigation.selectedChannel + 1U);
        const hal::TextBounds bounds = display_.measureText(channelText, 0, 0);
        const std::int16_t channelTextX = static_cast<std::int16_t>(
            static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(bounds.width) - 2);
        display_.drawText(channelTextX, 0, channelText);
    }

    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);
    const ChannelMode channelMode = state.channels[navigation.selectedChannel].common.mode;
    const std::uint8_t itemCount = settingsPageItemCount(
        navigation.settingsPage, channelMode, navigation.highScoreResetAvailable);
    const std::uint8_t visibleRows = kVisibleSettingsRows;
    std::int16_t y = 11;

    for (std::uint8_t visibleRow = 0U; visibleRow < visibleRows; ++visibleRow) {
        const std::uint8_t rowIndex = navigation.scrollOffset + visibleRow;
        if (rowIndex >= itemCount) {
            break;
        }
        MenuRow row = buildMenuRow(
            navigation.settingsPage,
            rowIndex,
            navigation.selectedChannel,
            state,
            navigation.highScoreResetAvailable);
        if (navigation.settingsPage == SettingsPage::GrooveEditorMenu) {
            if (rowIndex == 2U) {
                if (navigation.grooveZoomSteps == 0U) {
                    std::snprintf(row.value, sizeof(row.value), "%s", text::get(text::TextId::Fit));
                } else {
                    std::snprintf(row.value, sizeof(row.value), "%u", navigation.grooveZoomSteps);
                }
            } else if (rowIndex == 3U) {
                std::snprintf(row.value, sizeof(row.value), "%u", navigation.grooveDraft.length);
            }
        }
        const bool selected = rowIndex == navigation.cursor;
        if (selected) {
            display_.fillRectangle(0, y - 1, 124, 9, hal::PixelColor::White);
            display_.setTextColor(hal::PixelColor::Black);
        }
        display_.drawText(2, y, row.label);

        if (row.value[0] != '\0') {
            char renderedValue[sizeof(row.value)]{};
            const char* valueText = row.value;
            if (row.expandableInformation) {
                const hal::TextBounds labelBounds = display_.measureText(row.label, 0, y);
                const std::int16_t labelRight = static_cast<std::int16_t>(2 + labelBounds.width);
                const std::int16_t maximumValueWidth = static_cast<std::int16_t>(
                    121 - labelRight - kSettingsValueGap);
                fitInformationalValue(
                    display_, row.value, maximumValueWidth, renderedValue, sizeof(renderedValue));
                valueText = renderedValue;
            }
            const hal::TextBounds valueBounds = display_.measureText(valueText, 0, y);
            const std::int16_t valueX = 121 - static_cast<std::int16_t>(valueBounds.width);
            if (selected && navigation.editing) {
                // The selected row is inverted. Re-invert only the active value
                // so edit mode remains visually distinct without a left cursor.
                display_.fillRectangle(
                    valueX - 1,
                    y - 1,
                    static_cast<std::int16_t>(valueBounds.width + 2U),
                    9,
                    hal::PixelColor::Black);
                display_.setTextColor(hal::PixelColor::White);
                display_.drawText(valueX, y, valueText);
            } else {
                display_.drawText(valueX, y, valueText);
            }
        }
        display_.setTextColor(hal::PixelColor::White);
        y = static_cast<std::int16_t>(y + 10);
    }
    if (itemCount > visibleRows) {
        constexpr std::int16_t kScrollY = 11;
        constexpr std::int16_t kScrollHeight = 50;
        display_.drawVerticalLine(127, kScrollY, kScrollHeight);

        const std::uint8_t thumbHeight = static_cast<std::uint8_t>(
            (kScrollHeight * visibleRows) / itemCount);
        const std::uint8_t thumbY = static_cast<std::uint8_t>(
            static_cast<unsigned>(kScrollY) +
            ((static_cast<unsigned>(kScrollHeight) - thumbHeight) * navigation.cursor) /
                (itemCount - 1U));
        display_.fillRectangle(125, thumbY, 3, thumbHeight);
    }

    display_.present();
}

}  // namespace clockfw::ui
