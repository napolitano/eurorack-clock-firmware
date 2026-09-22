/**
 * @file channel_navigation_renderer.cpp
 * @brief Rendering implementation for graphical channel and mode navigation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/channel_navigation_renderer.h"

#include <cstdio>

#include "ui/text_formatter.h"
#include "ui_text.h"

namespace clockfw::ui {

ChannelNavigationRenderer::ChannelNavigationRenderer(hal::OledDisplay& display)
    : display_(display), patternStripRenderer_(display) {}

void ChannelNavigationRenderer::renderChannelQuickSelect(
    const ClockState& state,
    const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    if (state.operatingMode != OperatingMode::Independent) {
        renderGlobalModeOverview(state);
        return;
    }

    const char* const title = text::get(text::TextId::SelectChannel);
    const hal::TextBounds titleBounds = display_.measureText(title, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>(
            (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(titleBounds.width)) / 2),
        1,
        title);

    constexpr std::int16_t kGridLeft = 3;
    constexpr std::int16_t kGridTop = 15;
    constexpr std::int16_t kCellWidth = 29;
    constexpr std::int16_t kCellHeight = 21;
    constexpr std::int16_t kColumnPitch = 31;
    constexpr std::int16_t kRowPitch = 23;

    for (std::uint8_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        const std::int16_t column = static_cast<std::int16_t>(channelIndex % 4U);
        const std::int16_t row = static_cast<std::int16_t>(channelIndex / 4U);
        const std::int16_t cellX = static_cast<std::int16_t>(kGridLeft + column * kColumnPitch);
        const std::int16_t cellY = static_cast<std::int16_t>(kGridTop + row * kRowPitch);
        const bool selected = navigation.cursor == channelIndex;
        const hal::PixelColor foreground = selected
            ? hal::PixelColor::Black
            : hal::PixelColor::White;

        if (selected) {
            display_.fillRectangle(cellX, cellY, kCellWidth, kCellHeight);
            display_.setTextColor(hal::PixelColor::Black);
        } else {
            display_.drawRectangle(cellX, cellY, kCellWidth, kCellHeight);
        }

        char channelLabel[3]{};
        std::snprintf(
            channelLabel,
            sizeof(channelLabel),
            "%u",
            static_cast<unsigned>(channelIndex + 1U));
        display_.drawText(
            static_cast<std::int16_t>(cellX + 5),
            static_cast<std::int16_t>(cellY + 7),
            channelLabel);
        drawOverviewModeIcon(
            state,
            channelIndex,
            static_cast<std::int16_t>(cellX + 17),
            static_cast<std::int16_t>(cellY + 6),
            foreground);
        display_.setTextColor(hal::PixelColor::White);
    }

    display_.present();
}

void ChannelNavigationRenderer::renderGlobalModeOverview(const ClockState& state) {
    const bool unified = state.operatingMode == OperatingMode::UnifiedClock;
    const char* const heading = text::get(text::TextId::GlobalMode);
    const char* const modeName = text::get(
        unified ? text::TextId::ModeUnifiedLong : text::TextId::ModeDividerLong);

    const hal::TextBounds headingBounds = display_.measureText(heading, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>((hal::OledDisplay::kWidth - headingBounds.width) / 2),
        2,
        heading);

    if (unified) {
        drawUnifiedModeIcon(53, 21, hal::PixelColor::White);
    } else {
        drawDividerModeIcon(52, 23, hal::PixelColor::White);
    }

    const hal::TextBounds modeBounds = display_.measureText(modeName, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>((hal::OledDisplay::kWidth - modeBounds.width) / 2),
        49,
        modeName);
    display_.present();
}

void ChannelNavigationRenderer::renderSequencerEditor(
    const ClockState& state,
    const NavigationState& navigation,
    const engine::EngineSnapshot& engineSnapshot) {
    const SequencerSettings& sequencer = state.channels[navigation.selectedChannel].sequencer;
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    char header[20]{};
    std::snprintf(
        header,
        sizeof(header),
        text::get(text::TextId::SequenceHeaderFormat),
        navigation.selectedChannel + 1U,
        navigation.sequencerPage + 1U);
    display_.drawText(0, 0, header);
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);

    const std::uint8_t pageBaseStep = navigation.sequencerPage * 16U;
    for (std::uint8_t pageStep = 0U; pageStep < 16U; ++pageStep) {
        const std::uint8_t absoluteStep = pageBaseStep + pageStep;
        const std::int16_t x = static_cast<std::int16_t>(4 + static_cast<int>(pageStep % 8U) * 15);
        const std::int16_t y = static_cast<std::int16_t>(15 + static_cast<int>(pageStep / 8U) * 18);
        const bool inPattern = absoluteStep < sequencer.length;
        const bool gateOn = inPattern && ((sequencer.pattern >> absoluteStep) & 1ULL) != 0ULL;
        const bool selected = pageStep == navigation.sequencerCursor;

        if (selected) {
            display_.drawRectangle(x - 2, y - 2, 11, 11);
        }
        if (gateOn) {
            display_.fillRectangle(x, y, 7, 7);
        } else if (inPattern) {
            display_.drawRectangle(x, y, 7, 7);
        }
    }

    char footer[20]{};
    std::snprintf(
        footer,
        sizeof(footer),
        text::get(text::TextId::SequenceFooterFormat),
        pageBaseStep + navigation.sequencerCursor + 1U,
        sequencer.length);
    display_.drawText(1, 55, footer);

    char rate[8]{};
    formatRate(state.channels[navigation.selectedChannel].common, rate, sizeof(rate));
    display_.drawText(91, 55, rate);

    patternStripRenderer_.drawSequencerBlockIndicator(
        sequencer.length,
        engineSnapshot.channelStep[navigation.selectedChannel]);
    display_.present();
}

void ChannelNavigationRenderer::drawOverviewModeIcon(
    const ClockState& state,
    const std::uint8_t channelIndex,
    const std::int16_t x,
    const std::int16_t y,
    const hal::PixelColor color) {
    if (state.operatingMode == OperatingMode::UnifiedClock) {
        display_.drawVerticalLine(x, y + 1, 7, color);
        display_.drawHorizontalLine(x, y + 4, 7, color);
        display_.drawVerticalLine(x + 7, y, 9, color);
        return;
    }
    if (state.operatingMode == OperatingMode::DividerBank) {
        display_.drawHorizontalLine(x, y + 1, 8, color);
        display_.drawHorizontalLine(x, y + 4, 6, color);
        display_.drawHorizontalLine(x, y + 7, 4, color);
        return;
    }

    const ChannelMode mode = state.channels[channelIndex].common.mode;
    if (mode == ChannelMode::Off) {
        display_.drawRectangle(x, y, 8, 8, color);
        display_.drawLine(x + 1, y + 6, x + 6, y + 1, color);
    } else if (mode == ChannelMode::Clock) {
        display_.drawLine(x, y + 6, x, y + 2, color);
        display_.drawHorizontalLine(x, y + 2, 4, color);
        display_.drawVerticalLine(x + 4, y + 2, 5, color);
        display_.drawHorizontalLine(x + 4, y + 6, 4, color);
    } else if (mode == ChannelMode::Euclid) {
        display_.fillRectangle(x + 3, y, 2, 2, color);
        display_.fillRectangle(x + 6, y + 3, 2, 2, color);
        display_.fillRectangle(x + 3, y + 6, 2, 2, color);
        display_.fillRectangle(x, y + 3, 2, 2, color);
    } else {
        display_.fillRectangle(x, y + 1, 2, 6, color);
        display_.drawRectangle(x + 3, y + 3, 2, 4, color);
        display_.fillRectangle(x + 6, y + 1, 2, 6, color);
    }
}


}  // namespace clockfw::ui
