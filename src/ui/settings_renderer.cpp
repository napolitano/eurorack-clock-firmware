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

#include "services/template_service.h"
#include "ui/menu_model.h"
#include "ui/preset_name_alphabet.h"
#include "ui_text.h"

namespace clockfw::ui {
namespace {

/** Number of text-setting rows visible at once on the 64-pixel display. */
constexpr std::uint8_t kVisibleSettingsRows = 5U;

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

void SettingsRenderer::renderSettings(
    const ClockState& state,
    const NavigationState& navigation) {
    display_.clear();
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
    const std::uint8_t itemCount = settingsPageItemCount(
        navigation.settingsPage,
        state.channels[navigation.selectedChannel].common.mode);

    for (std::uint8_t visibleRow = 0U; visibleRow < kVisibleSettingsRows; ++visibleRow) {
        const std::uint8_t rowIndex = navigation.scrollOffset + visibleRow;
        if (rowIndex >= itemCount) {
            break;
        }

        const MenuRow row = buildMenuRow(
            navigation.settingsPage,
            rowIndex,
            navigation.selectedChannel,
            state);
        const std::int16_t y = static_cast<std::int16_t>(11 + static_cast<int>(visibleRow) * 10);
        const bool selected = rowIndex == navigation.cursor;

        if (selected) {
            display_.drawCharacter(0, y, '>');
        }
        display_.drawText(8, y, row.label);

        if (row.value[0] == '\0') {
            continue;
        }

        const hal::TextBounds valueBounds = display_.measureText(row.value, 0, y);
        const std::int16_t valueX = 121 - static_cast<std::int16_t>(valueBounds.width);
        if (selected && navigation.editing) {
            display_.fillRectangle(valueX - 1, y - 1, static_cast<std::int16_t>(valueBounds.width + 2U), 9);
            display_.setTextColor(hal::PixelColor::Black);
            display_.drawText(valueX, y, row.value);
            display_.setTextColor(hal::PixelColor::White);
        } else {
            display_.drawText(valueX, y, row.value);
        }
    }

    if (itemCount > kVisibleSettingsRows) {
        constexpr std::int16_t kScrollY = 11;
        constexpr std::int16_t kScrollHeight = 50;
        display_.drawVerticalLine(127, kScrollY, kScrollHeight);

        const std::uint8_t thumbHeight = static_cast<std::uint8_t>(
            (kScrollHeight * kVisibleSettingsRows) / itemCount);
        const std::uint8_t thumbY = static_cast<std::uint8_t>(
            static_cast<unsigned>(kScrollY) +
            ((static_cast<unsigned>(kScrollHeight) - thumbHeight) * navigation.cursor) /
                (itemCount - 1U));
        display_.fillRectangle(125, thumbY, 3, thumbHeight);
    }

    display_.present();
}

void SettingsRenderer::renderTemplates(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    display_.drawText(0, 0, text::get(text::TextId::Templates));
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);

    for (std::uint8_t visibleRow = 0U; visibleRow < 4U; ++visibleRow) {
        const std::uint8_t templateIndex = navigation.scrollOffset + visibleRow;
        if (templateIndex >= services::TemplateService::kTemplateCount) {
            break;
        }

        const std::int16_t y = static_cast<std::int16_t>(13 + static_cast<int>(visibleRow) * 12);
        const bool selected = templateIndex == navigation.cursor;
        if (selected) {
            display_.fillRectangle(0, y - 1, 124, 10);
            display_.setTextColor(hal::PixelColor::Black);
        }
        display_.drawText(2, y, services::TemplateService::name(templateIndex));
        display_.setTextColor(hal::PixelColor::White);
    }

    display_.present();
}

void SettingsRenderer::renderPresetSlots(
    const NavigationState& navigation,
    const services::PersistentStateService& persistentState) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    display_.drawText(
        0,
        0,
        text::get(
            navigation.presetSlotAction == PresetSlotAction::Load
                ? text::TextId::LoadPresetTitle
                : text::TextId::SavePresetTitle));
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);

    constexpr std::uint8_t kVisiblePresetRows = 5U;
    for (std::uint8_t visibleRow = 0U; visibleRow < kVisiblePresetRows; ++visibleRow) {
        const std::uint8_t slotIndex = navigation.scrollOffset + visibleRow;
        if (slotIndex >= services::PersistentStateService::kUserPresetSlotCount) {
            break;
        }

        char name[services::PersistentStateService::kPresetNameLength + 1U]{};
        persistentState.presetName(slotIndex, name, sizeof(name));
        const char* shownName = persistentState.presetExists(slotIndex)
            ? name
            : text::get(text::TextId::Empty);
        const std::int16_t y = static_cast<std::int16_t>(12 + static_cast<int>(visibleRow) * 10);
        const bool selected = navigation.cursor == slotIndex;

        char slotNumber[3]{};
        std::snprintf(slotNumber, sizeof(slotNumber), "%u", static_cast<unsigned>(slotIndex + 1U));
        if (selected) {
            display_.fillRectangle(0, y - 1, 126, 9);
            display_.setTextColor(hal::PixelColor::Black);
        }
        display_.drawText(2, y, slotNumber);
        display_.drawText(16, y, shownName);
        display_.setTextColor(hal::PixelColor::White);
    }

    constexpr std::int16_t kScrollY = 11;
    constexpr std::int16_t kScrollHeight = 50;
    display_.drawVerticalLine(127, kScrollY, kScrollHeight);
    constexpr std::uint8_t kThumbHeight = 31U;
    const std::uint8_t thumbY = static_cast<std::uint8_t>(
        kScrollY + ((kScrollHeight - kThumbHeight) * navigation.cursor) /
            (services::PersistentStateService::kUserPresetSlotCount - 1U));
    display_.fillRectangle(125, thumbY, 3, kThumbHeight);
    display_.present();
}

void SettingsRenderer::renderOverwriteConfirm(
    const NavigationState& navigation,
    const services::PersistentStateService& persistentState) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    const char* const title = text::get(text::TextId::OverwritePreset);
    const hal::TextBounds titleBounds = display_.measureText(title, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>((hal::OledDisplay::kWidth - titleBounds.width) / 2),
        3,
        title);

    char name[services::PersistentStateService::kPresetNameLength + 1U]{};
    persistentState.presetName(navigation.selectedPresetSlot, name, sizeof(name));
    const hal::TextBounds nameBounds = display_.measureText(name, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>((hal::OledDisplay::kWidth - nameBounds.width) / 2),
        20,
        name);

    const char* const choices[2] = {
        text::get(text::TextId::No),
        text::get(text::TextId::Yes)};
    constexpr std::int16_t kChoiceX[2] = {25, 79};
    for (std::uint8_t index = 0U; index < 2U; ++index) {
        const hal::TextBounds bounds = display_.measureText(choices[index], 0, 0);
        const std::int16_t boxWidth = static_cast<std::int16_t>(bounds.width + 8U);
        const std::int16_t boxX = static_cast<std::int16_t>(
            kChoiceX[index] - boxWidth / 2);
        if (navigation.cursor == index) {
            display_.fillRectangle(boxX, 39, boxWidth, 13);
            display_.setTextColor(hal::PixelColor::Black);
        } else {
            display_.drawRectangle(boxX, 39, boxWidth, 13);
        }
        display_.drawText(
            static_cast<std::int16_t>(kChoiceX[index] - static_cast<std::int16_t>(bounds.width / 2)),
            42,
            choices[index]);
        display_.setTextColor(hal::PixelColor::White);
    }

    display_.present();
}

void SettingsRenderer::renderNameEntry(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    display_.drawText(0, 0, text::get(text::TextId::NamePreset));
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);

    // The full 16-character result remains visible while the character repertoire
    // scrolls beneath it like a classic high-score entry wheel.
    display_.drawText(16, 15, navigation.presetNameBuffer.data());
    const std::int16_t cursorX = static_cast<std::int16_t>(16 + navigation.nameCharacterIndex * 6U);
    display_.drawHorizontalLine(cursorX, 24, 5);

    constexpr int kVisibleBandCharacters = 17;
    constexpr int kBandCenterIndex = kVisibleBandCharacters / 2;
    constexpr std::int16_t kBandStartX = 13;
    constexpr std::int16_t kBandY = 35;
    const char activeCharacter = navigation.presetNameBuffer[navigation.nameCharacterIndex];
    const int activeAlphabetIndex = static_cast<int>(presetname::characterIndex(activeCharacter));

    for (int visibleIndex = 0; visibleIndex < kVisibleBandCharacters; ++visibleIndex) {
        const int alphabetOffset = visibleIndex - kBandCenterIndex;
        const char character = presetname::characterAtWrapped(activeAlphabetIndex + alphabetOffset);
        const std::int16_t x = static_cast<std::int16_t>(kBandStartX + visibleIndex * 6);
        if (visibleIndex == kBandCenterIndex) {
            display_.fillRectangle(static_cast<std::int16_t>(x - 1), kBandY - 1, 7, 9);
            display_.setTextColor(hal::PixelColor::Black);
        }
        display_.drawCharacter(x, kBandY, character);
        display_.setTextColor(hal::PixelColor::White);
    }

    const char* const footer = text::get(text::TextId::PushNext);
    const hal::TextBounds footerBounds = display_.measureText(footer, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>(
            (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(footerBounds.width)) / 2),
        55,
        footer);
    display_.present();
}

}  // namespace clockfw::ui
