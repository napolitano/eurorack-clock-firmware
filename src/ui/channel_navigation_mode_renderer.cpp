/**
 * @file channel_navigation_mode_renderer.cpp
 * @brief Horizontal mode-carousel and mode-confirmation rendering.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/channel_navigation_mode_renderer.h"

#include <cstddef>

#include "ui/mode_functions.h"
#include "ui_text.h"

namespace clockfw::ui {
namespace {

text::TextId modeFunctionTitleId(const ModeFunction modeFunction) {
    switch (modeFunction) {
        case ModeFunction::Off: return text::TextId::ModeOffLong;
        case ModeFunction::Clock: return text::TextId::ModeClockLong;
        case ModeFunction::Euclid: return text::TextId::ModeEuclidLong;
        case ModeFunction::Sequencer: return text::TextId::ModeSequencerLong;
        case ModeFunction::UnifiedClock: return text::TextId::ModeUnifiedLong;
        case ModeFunction::DividerBank: return text::TextId::ModeDividerLong;
        default: return text::TextId::ModeClockLong;
    }
}

text::TextId modeFunctionBandLabelId(const ModeFunction modeFunction) {
    switch (modeFunction) {
        case ModeFunction::UnifiedClock: return text::TextId::ModeUnifiedShort;
        case ModeFunction::DividerBank: return text::TextId::ModeDividerShort;
        case ModeFunction::Clock: return text::TextId::ModeClockLong;
        case ModeFunction::Euclid: return text::TextId::ModeEuclidLong;
        case ModeFunction::Sequencer: return text::TextId::ModeSequencerBand;
        case ModeFunction::Off: return text::TextId::ModeOffLong;
        default: return text::TextId::ModeClockLong;
    }
}

std::size_t wrappedModeIndex(const int index) {
    const int count = static_cast<int>(kModeFunctions.size());
    return static_cast<std::size_t>((index % count + count) % count);
}

}  // namespace

void ChannelNavigationRenderer::renderModeSelect(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    const char* const heading = text::get(text::TextId::Mode);
    const hal::TextBounds headingBounds = display_.measureText(heading, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>(
            (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(headingBounds.width)) / 2),
        0,
        heading);
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);

    // The selected mode is always anchored in the physical center. Turning the
    // encoder changes which catalog entry occupies that anchor; the neighbors
    // move with it, so the UI remains spatially stable as the catalog grows.
    constexpr std::int16_t kCenterX = 64;
    constexpr std::int16_t kSlotPitch = 44;
    constexpr std::int16_t kSlotWidth = 40;
    constexpr std::int16_t kSelectedX = 43;
    constexpr std::int16_t kSelectedY = 14;
    constexpr std::int16_t kSelectedWidth = 42;
    constexpr std::int16_t kSelectedHeight = 40;
    constexpr std::int16_t kIconOriginY = 17;
    constexpr std::int16_t kLabelY = 45;

    const int selectedIndex = navigation.cursor < kModeFunctions.size()
        ? static_cast<int>(navigation.cursor)
        : 0;

    for (int relative = -1; relative <= 1; ++relative) {
        const std::size_t catalogIndex = wrappedModeIndex(selectedIndex + relative);
        const ModeFunction modeFunction = kModeFunctions[catalogIndex];
        const std::int16_t centerX = static_cast<std::int16_t>(
            kCenterX + relative * kSlotPitch);
        const std::int16_t slotX = static_cast<std::int16_t>(centerX - kSlotWidth / 2);
        const bool selected = relative == 0;
        const hal::PixelColor foreground = selected
            ? hal::PixelColor::Black
            : hal::PixelColor::White;

        if (selected) {
            display_.fillRectangle(
                kSelectedX,
                kSelectedY,
                kSelectedWidth,
                kSelectedHeight,
                hal::PixelColor::White);
        }

        drawModeFunctionIcon(modeFunction, slotX, kIconOriginY, foreground);

        const char* const label = text::get(modeFunctionBandLabelId(modeFunction));
        const hal::TextBounds labelBounds = display_.measureText(label, 0, 0);
        display_.setTextColor(foreground);
        display_.drawText(
            static_cast<std::int16_t>(
                centerX - static_cast<std::int16_t>(labelBounds.width / 2U)),
            kLabelY,
            label);
        display_.setTextColor(hal::PixelColor::White);
    }

    display_.present();
}

void ChannelNavigationRenderer::renderModeChangeConfirm(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    const char* const title = text::get(text::TextId::ChangeMode);
    const hal::TextBounds titleBounds = display_.measureText(title, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>((hal::OledDisplay::kWidth - titleBounds.width) / 2),
        4,
        title);

    const char* const modeName = text::get(
        modeFunctionTitleId(navigation.pendingModeFunction));
    const hal::TextBounds modeBounds = display_.measureText(modeName, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>((hal::OledDisplay::kWidth - modeBounds.width) / 2),
        21,
        modeName);

    const char* const choices[2] = {
        text::get(text::TextId::No),
        text::get(text::TextId::Yes)};
    constexpr std::int16_t kChoiceCenterX[2] = {31, 96};
    for (std::uint8_t index = 0U; index < 2U; ++index) {
        const hal::TextBounds bounds = display_.measureText(choices[index], 0, 0);
        const std::int16_t boxWidth = static_cast<std::int16_t>(bounds.width + 10U);
        const std::int16_t boxX = static_cast<std::int16_t>(kChoiceCenterX[index] - boxWidth / 2);
        if (navigation.cursor == index) {
            display_.fillRectangle(boxX, 40, boxWidth, 14);
            display_.setTextColor(hal::PixelColor::Black);
        } else {
            display_.drawRectangle(boxX, 40, boxWidth, 14);
        }
        display_.drawText(
            static_cast<std::int16_t>(kChoiceCenterX[index] - bounds.width / 2),
            43,
            choices[index]);
        display_.setTextColor(hal::PixelColor::White);
    }
    display_.present();
}

void ChannelNavigationRenderer::drawOffModeIcon(
    const std::int16_t x,
    const std::int16_t y,
    const hal::PixelColor color) {
    display_.drawRectangle(x, y, 12, 12, color);
    display_.drawLine(x + 2, y + 9, x + 9, y + 2, color);
}

void ChannelNavigationRenderer::drawClockModeIcon(
    const std::int16_t x,
    const std::int16_t y,
    const hal::PixelColor color) {
    display_.drawLine(x, y + 8, x, y + 3, color);
    display_.drawLine(x, y + 3, x + 7, y + 3, color);
    display_.drawLine(x + 7, y + 3, x + 7, y + 8, color);
    display_.drawLine(x + 7, y + 8, x + 14, y + 8, color);
    display_.drawLine(x + 14, y + 8, x + 14, y + 3, color);
    display_.drawLine(x + 14, y + 3, x + 21, y + 3, color);
}

void ChannelNavigationRenderer::drawEuclidModeIcon(
    const std::int16_t centerX,
    const std::int16_t centerY,
    const hal::PixelColor color) {
    constexpr std::int8_t kXOffsets[8] = {0, 5, 7, 5, 0, -5, -7, -5};
    constexpr std::int8_t kYOffsets[8] = {-7, -5, 0, 5, 7, 5, 0, -5};
    constexpr bool kFilledPoints[8] = {true, false, false, true, false, true, false, false};

    for (std::uint8_t pointIndex = 0U; pointIndex < 8U; ++pointIndex) {
        const std::int16_t pointX = centerX + kXOffsets[pointIndex];
        const std::int16_t pointY = centerY + kYOffsets[pointIndex];
        if (kFilledPoints[pointIndex]) {
            display_.fillRectangle(pointX - 1, pointY - 1, 3, 3, color);
        } else {
            display_.fillRectangle(pointX, pointY, 1, 1, color);
        }
    }
}

void ChannelNavigationRenderer::drawSequencerModeIcon(
    const std::int16_t x,
    const std::int16_t y,
    const hal::PixelColor color) {
    for (std::uint8_t step = 0U; step < 6U; ++step) {
        const std::int16_t stepX = static_cast<std::int16_t>(x + static_cast<int>(step) * 5);
        if (step == 0U || step == 2U || step == 5U) {
            display_.fillRectangle(stepX, y, 4, 7, color);
        } else {
            display_.drawRectangle(stepX, y + 3, 4, 4, color);
        }
    }
}

void ChannelNavigationRenderer::drawUnifiedModeIcon(
    const std::int16_t x,
    const std::int16_t y,
    const hal::PixelColor color) {
    display_.drawVerticalLine(x, y + 3, 8, color);
    display_.drawHorizontalLine(x, y + 6, 8, color);
    display_.drawVerticalLine(x + 8, y + 1, 12, color);
    display_.drawHorizontalLine(x + 8, y + 1, 10, color);
    display_.drawHorizontalLine(x + 8, y + 6, 10, color);
    display_.drawHorizontalLine(x + 8, y + 11, 10, color);
    display_.fillRectangle(x + 18, y, 3, 3, color);
    display_.fillRectangle(x + 18, y + 5, 3, 3, color);
    display_.fillRectangle(x + 18, y + 10, 3, 3, color);
}

void ChannelNavigationRenderer::drawDividerModeIcon(
    const std::int16_t x,
    const std::int16_t y,
    const hal::PixelColor color) {
    display_.drawHorizontalLine(x, y, 24, color);
    for (std::int16_t tick = 0; tick <= 24; tick += 6) {
        display_.drawVerticalLine(static_cast<std::int16_t>(x + tick), y, 3, color);
    }
    display_.drawHorizontalLine(x, y + 6, 24, color);
    for (std::int16_t tick = 0; tick <= 24; tick += 12) {
        display_.drawVerticalLine(static_cast<std::int16_t>(x + tick), y + 6, 3, color);
    }
    display_.drawHorizontalLine(x, y + 12, 24, color);
    display_.drawVerticalLine(x, y + 12, 3, color);
    display_.drawVerticalLine(x + 24, y + 12, 3, color);
}

void ChannelNavigationRenderer::drawModeFunctionIcon(
    const ModeFunction modeFunction,
    const std::int16_t x,
    const std::int16_t y,
    const hal::PixelColor color) {
    switch (modeFunction) {
        case ModeFunction::Off:
            drawOffModeIcon(x + 14, y + 5, color);
            break;
        case ModeFunction::Clock:
            drawClockModeIcon(x + 9, y + 5, color);
            break;
        case ModeFunction::Euclid:
            drawEuclidModeIcon(x + 20, y + 11, color);
            break;
        case ModeFunction::Sequencer:
            drawSequencerModeIcon(x + 5, y + 8, color);
            break;
        case ModeFunction::UnifiedClock:
            drawUnifiedModeIcon(x + 9, y + 4, color);
            break;
        case ModeFunction::DividerBank:
        default:
            drawDividerModeIcon(x + 8, y + 4, color);
            break;
    }
}

}  // namespace clockfw::ui
