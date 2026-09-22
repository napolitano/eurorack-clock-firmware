/**
 * @file settings_renderer_lists.cpp
 * @brief Rendering of read-only information, template, preset, and name-entry screens.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/settings_renderer_lists.h"

#include <algorithm>
#include <cstddef>
#include <cstdio>
#include <cstring>

#include "services/template_service.h"
#include "ui/preset_name_alphabet.h"
#include "ui_text.h"

namespace clockfw::ui {
namespace {

void drawCenteredTextLine(
    hal::OledDisplay& display,
    const std::int16_t y,
    const char* const value) {
    const hal::TextBounds bounds = display.measureText(value, 0, 0);
    display.drawText(
        static_cast<std::int16_t>((static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(bounds.width)) / 2),
        y,
        value);
}

}  // namespace

void SettingsRenderer::renderInformationPopover(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    display_.drawText(0, 0, navigation.informationPopoverTitle.data());
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);

    const char* value = navigation.informationPopoverValue.data();
    if (display_.measureText(value, 0, 0).width <= 124U) {
        drawCenteredTextLine(display_, 27, value);
    } else {
        // Read-only detail values are rare and short. Split at the last space
        // that keeps the first line inside the OLED instead of introducing a
        // scrolling text state for information that fits comfortably in two rows.
        char first[32]{};
        char second[32]{};
        std::size_t split = 0U;
        const std::size_t length = std::strlen(value);
        for (std::size_t index = 1U; index < length && index < sizeof(first); ++index) {
            if (value[index] != ' ') {
                continue;
            }
            char candidate[32]{};
            std::memcpy(candidate, value, index);
            candidate[index] = '\0';
            if (display_.measureText(candidate, 0, 0).width <= 124U) {
                split = index;
            }
        }
        if (split == 0U) {
            split = std::min<std::size_t>(20U, length);
        }
        std::memcpy(first, value, std::min(split, sizeof(first) - 1U));
        first[std::min(split, sizeof(first) - 1U)] = '\0';
        const std::size_t secondStart = split < length && value[split] == ' ' ? split + 1U : split;
        std::snprintf(second, sizeof(second), "%s", value + secondStart);
        drawCenteredTextLine(display_, 22, first);
        drawCenteredTextLine(display_, 34, second);
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

void SettingsRenderer::renderHighScoreClearConfirm(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    const char* const title = text::get(text::TextId::ClearHighScores);
    const hal::TextBounds titleBounds = display_.measureText(title, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>((hal::OledDisplay::kWidth - titleBounds.width) / 2),
        13,
        title);

    const char* const choices[2] = {
        text::get(text::TextId::No),
        text::get(text::TextId::Yes)};
    constexpr std::int16_t kChoiceX[2] = {25, 79};
    for (std::uint8_t index = 0U; index < 2U; ++index) {
        const hal::TextBounds bounds = display_.measureText(choices[index], 0, 0);
        const std::int16_t boxWidth = static_cast<std::int16_t>(bounds.width + 8U);
        const std::int16_t boxX = static_cast<std::int16_t>(kChoiceX[index] - boxWidth / 2);
        if (navigation.cursor == index) {
            display_.fillRectangle(boxX, 36, boxWidth, 13);
            display_.setTextColor(hal::PixelColor::Black);
        } else {
            display_.drawRectangle(boxX, 36, boxWidth, 13);
        }
        display_.drawText(
            static_cast<std::int16_t>(kChoiceX[index] - static_cast<std::int16_t>(bounds.width / 2)),
            39,
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

    const char* const nextText = text::get(text::TextId::PushNext);
    const char* const saveText = text::get(text::TextId::HoldToSave);
    const hal::TextBounds saveBounds = display_.measureText(saveText, 0, 0);
    display_.drawText(0, 55, nextText);
    display_.drawText(
        static_cast<std::int16_t>(hal::OledDisplay::kWidth - saveBounds.width),
        55,
        saveText);
    display_.present();
}

}  // namespace clockfw::ui
