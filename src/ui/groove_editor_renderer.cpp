/**
 * @file groove_editor_renderer.cpp
 * @brief Compact 128x64 Custom Groove editor and library rendering.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/groove_editor_renderer.h"

#include <algorithm>
#include <cstdio>

#include "ui/preset_name_alphabet.h"
#include "ui_text.h"

namespace clockfw::ui {
namespace {
constexpr std::int16_t kHeaderRuleY = 9;
constexpr std::int16_t kBeatLabelY = 11;
constexpr std::int16_t kGridTop = 20;
constexpr std::int16_t kGridBottom = 48;
constexpr std::int16_t kMarkerY = 35;
constexpr std::int16_t kLeft = 4;
constexpr std::int16_t kRight = 123;

std::uint8_t visibleSteps(const NavigationState& navigation) {
    const std::uint8_t length = navigation.grooveDraft.length;
    if (navigation.grooveZoomSteps == 0U || navigation.grooveZoomSteps >= length) {
        return length;
    }
    return std::max<std::uint8_t>(4U, navigation.grooveZoomSteps);
}

std::uint8_t windowStart(const NavigationState& navigation, const std::uint8_t visible) {
    if (visible >= navigation.grooveDraft.length) {
        return 0U;
    }
    const int selected = navigation.grooveCursor;
    int start = selected - static_cast<int>(visible / 2U);
    start = (start / 4) * 4;
    const int maximum = static_cast<int>(navigation.grooveDraft.length - visible);
    start = std::max(0, std::min(start, maximum));
    return static_cast<std::uint8_t>(start);
}

std::int16_t nominalX(const std::uint8_t localStep, const std::uint8_t visible) {
    if (visible <= 1U) {
        return (kLeft + kRight) / 2;
    }
    return static_cast<std::int16_t>(
        kLeft + (static_cast<int>(kRight - kLeft) * localStep) / (visible - 1U));
}

void clearDiamondBackground(hal::OledDisplay& display, const std::int16_t x) {
    // A one-pixel black quiet zone separates every marker from the timing grid.
    // Clearing only the geometric diamond interior still leaves guide pixels
    // visually touching its diagonal edges, which reads as a line through the
    // marker on a 1-bit OLED. The 9x9 backdrop makes the marker unambiguous.
    constexpr std::int16_t kBackdropRadius = 4;
    display.fillRectangle(
        static_cast<std::int16_t>(x - kBackdropRadius),
        static_cast<std::int16_t>(kMarkerY - kBackdropRadius),
        static_cast<std::int16_t>(kBackdropRadius * 2 + 1),
        static_cast<std::int16_t>(kBackdropRadius * 2 + 1),
        hal::PixelColor::Black);
}

void drawDiamond(hal::OledDisplay& display, const std::int16_t x, const bool selected) {
    constexpr std::int16_t r = 3;
    clearDiamondBackground(display, x);
    display.drawLine(x, kMarkerY - r, x + r, kMarkerY);
    display.drawLine(x + r, kMarkerY, x, kMarkerY + r);
    display.drawLine(x, kMarkerY + r, x - r, kMarkerY);
    display.drawLine(x - r, kMarkerY, x, kMarkerY - r);
    if (selected) {
        display.drawHorizontalLine(x - 2, kMarkerY, 5);
        display.drawHorizontalLine(x - 1, kMarkerY - 1, 3);
        display.drawHorizontalLine(x - 1, kMarkerY + 1, 3);
    }
}

void drawChoices(hal::OledDisplay& display, const NavigationState& navigation) {
    const char* const choices[2] = {text::get(text::TextId::No), text::get(text::TextId::Yes)};
    constexpr std::int16_t centers[2] = {35, 91};
    for (std::uint8_t index = 0U; index < 2U; ++index) {
        const hal::TextBounds bounds = display.measureText(choices[index], 0, 0);
        const std::int16_t width = static_cast<std::int16_t>(bounds.width + 10U);
        const std::int16_t x = static_cast<std::int16_t>(centers[index] - width / 2);
        if (navigation.cursor == index) {
            display.fillRectangle(x, 39, width, 13);
            display.setTextColor(hal::PixelColor::Black);
        } else {
            display.drawRectangle(x, 39, width, 13);
        }
        display.drawText(
            static_cast<std::int16_t>(centers[index] - static_cast<std::int16_t>(bounds.width / 2U)),
            42,
            choices[index]);
        display.setTextColor(hal::PixelColor::White);
    }
}
}  // namespace

GrooveEditorRenderer::GrooveEditorRenderer(hal::OledDisplay& display) : display_(display) {}

void GrooveEditorRenderer::renderEditor(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    display_.drawText(0, 0, text::get(text::TextId::GrooveEditorTitle));
    display_.drawHorizontalLine(0, kHeaderRuleY, hal::OledDisplay::kWidth);

    const std::uint8_t visible = visibleSteps(navigation);
    const std::uint8_t start = windowStart(navigation, visible);

    for (std::uint8_t local = 0U; local < visible; ++local) {
        const std::uint8_t step = static_cast<std::uint8_t>(start + local);
        const std::int16_t x = nominalX(local, visible);
        if ((step % 4U) == 0U) {
            display_.drawVerticalLine(x, kGridTop, kGridBottom - kGridTop + 1);
            char beat[4]{};
            std::snprintf(beat, sizeof(beat), "%u", static_cast<unsigned>(step / 4U + 1U));
            const hal::TextBounds bounds = display_.measureText(beat, 0, 0);
            display_.drawText(
                static_cast<std::int16_t>(x - static_cast<std::int16_t>(bounds.width / 2U)),
                kBeatLabelY,
                beat);
        } else {
            for (std::int16_t y = kGridTop; y <= kGridBottom; y += 2) {
                display_.setPixel(x, y);
            }
        }
    }

    const int spacing = visible > 1U ? (kRight - kLeft) / static_cast<int>(visible - 1U) : 0;
    for (std::uint8_t local = 0U; local < visible; ++local) {
        const std::uint8_t step = static_cast<std::uint8_t>(start + local);
        const std::int16_t baseX = nominalX(local, visible);
        const int shifted = static_cast<int>(baseX) +
            (spacing * static_cast<int>(navigation.grooveDraft.offsets256[step])) / 256;
        const std::int16_t x = static_cast<std::int16_t>(std::clamp(shifted, 2, 125));
        drawDiamond(display_, x, step == navigation.grooveCursor);
    }

    char status[24]{};
    const std::int8_t offset = navigation.grooveDraft.offsets256[navigation.grooveCursor];
    std::snprintf(
        status,
        sizeof(status),
        text::get(text::TextId::CustomGrooveStatusFormat),
        static_cast<unsigned>(navigation.grooveCursor + 1U),
        static_cast<int>(offset),
        static_cast<unsigned>(navigation.grooveDraft.length));
    display_.drawText(0, 55, status);
    if (navigation.grooveZoomSteps == 0U) {
        display_.drawText(102, 55, text::get(text::TextId::Fit));
    } else {
        char zoom[5]{};
        std::snprintf(zoom, sizeof(zoom), text::get(text::TextId::ZoomFormat), static_cast<unsigned>(navigation.grooveZoomSteps));
        display_.drawText(102, 55, zoom);
    }
    display_.present();
}

void GrooveEditorRenderer::renderSlots(
    const NavigationState& navigation,
    const services::CustomGrooveStore& store) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    text::TextId title = text::TextId::LoadGroove;
    if (navigation.grooveSlotAction == GrooveSlotAction::Save) {
        title = text::TextId::SaveGroove;
    } else if (navigation.grooveSlotAction == GrooveSlotAction::Rename) {
        title = text::TextId::RenameGroove;
    } else if (navigation.grooveSlotAction == GrooveSlotAction::Delete) {
        title = text::TextId::DeleteGroove;
    }
    display_.drawText(0, 0, text::get(title));
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);
    for (std::uint8_t row = 0U; row < 5U; ++row) {
        const std::uint8_t slot = static_cast<std::uint8_t>(navigation.scrollOffset + row);
        if (slot >= kCustomGrooveSlotCount) break;
        const std::int16_t y = static_cast<std::int16_t>(12 + row * 10U);
        char name[services::CustomGrooveStore::kNameLength + 1U]{};
        store.name(slot, name, sizeof(name));
        if (!store.exists(slot)) std::snprintf(name, sizeof(name), "%s", text::get(text::TextId::EmptyBracketed));
        if (slot == navigation.cursor) {
            display_.fillRectangle(0, y - 1, 126, 9);
            display_.setTextColor(hal::PixelColor::Black);
        }
        char number[4]{};
        std::snprintf(number, sizeof(number), "%u", static_cast<unsigned>(slot + 1U));
        display_.drawText(2, y, number);
        display_.drawText(16, y, name);
        display_.setTextColor(hal::PixelColor::White);
    }
    display_.present();
}

void GrooveEditorRenderer::renderConfirm(const char* const title, const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    const hal::TextBounds bounds = display_.measureText(title, 0, 0);
    display_.drawText(static_cast<std::int16_t>((hal::OledDisplay::kWidth - bounds.width) / 2), 16, title);
    drawChoices(display_, navigation);
    display_.present();
}

void GrooveEditorRenderer::renderNameEntry(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);
    display_.drawText(
        0,
        0,
        text::get(
            navigation.grooveSlotAction == GrooveSlotAction::Rename
                ? text::TextId::RenameGroove
                : text::TextId::GrooveName));
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);
    display_.drawText(16, 15, navigation.grooveNameBuffer.data());
    const std::int16_t cursorX = static_cast<std::int16_t>(16 + navigation.nameCharacterIndex * 6U);
    display_.drawHorizontalLine(cursorX, 24, 5);
    constexpr int visibleCharacters = 17;
    constexpr int center = visibleCharacters / 2;
    constexpr std::int16_t startX = 13;
    const char active = navigation.grooveNameBuffer[navigation.nameCharacterIndex];
    const int activeIndex = static_cast<int>(presetname::characterIndex(active));
    for (int index = 0; index < visibleCharacters; ++index) {
        const char character = presetname::characterAtWrapped(activeIndex + index - center);
        const std::int16_t x = static_cast<std::int16_t>(startX + index * 6);
        if (index == center) {
            display_.fillRectangle(static_cast<std::int16_t>(x - 1), 34, 7, 9);
            display_.setTextColor(hal::PixelColor::Black);
        }
        display_.drawCharacter(x, 35, character);
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
