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

const char* modeFunctionHint(const ModeFunction modeFunction) {
    if (modeFunction == ModeFunction::UnifiedClock) {
        return text::get(text::TextId::UnifiedModeHint);
    }
    if (modeFunction == ModeFunction::DividerBank) {
        return text::get(text::TextId::DividerModeHint);
    }
    return "";
}

}  // namespace

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

void ChannelNavigationRenderer::renderModeSelect(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    const std::uint8_t modeIndex = navigation.cursor < kModeFunctions.size()
        ? navigation.cursor
        : 2U;
    const ModeFunction modeFunction = kModeFunctions[modeIndex];
    const char* const title = text::get(modeFunctionTitleId(modeFunction));
    const hal::TextBounds titleBounds = display_.measureText(title, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>(
            (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(titleBounds.width)) / 2),
        0,
        title);

    const char* const hint = modeFunctionHint(modeFunction);
    if (hint[0] != '\0') {
        const hal::TextBounds hintBounds = display_.measureText(hint, 0, 0);
        display_.drawText(
            static_cast<std::int16_t>(
                (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(hintBounds.width)) / 2),
            8,
            hint);
    }

    constexpr std::int16_t kGridLeft = 2;
    constexpr std::int16_t kGridTop = 18;
    constexpr std::int16_t kTileWidth = 40;
    constexpr std::int16_t kTileHeight = 20;
    constexpr std::int16_t kColumnPitch = 42;
    constexpr std::int16_t kRowPitch = 22;

    for (std::uint8_t tileIndex = 0U; tileIndex < 6U; ++tileIndex) {
        const std::int16_t column = static_cast<std::int16_t>(tileIndex % 3U);
        const std::int16_t row = static_cast<std::int16_t>(tileIndex / 3U);
        const std::int16_t tileX = static_cast<std::int16_t>(kGridLeft + column * kColumnPitch);
        const std::int16_t tileY = static_cast<std::int16_t>(kGridTop + row * kRowPitch);
        const bool selected = navigation.cursor == tileIndex;
        const hal::PixelColor foreground = selected
            ? hal::PixelColor::Black
            : hal::PixelColor::White;

        if (selected) {
            display_.fillRectangle(tileX, tileY, kTileWidth, kTileHeight);
        } else {
            display_.drawRectangle(tileX, tileY, kTileWidth, kTileHeight);
        }

        drawModeFunctionIcon(kModeFunctions[tileIndex], tileX, static_cast<std::int16_t>(tileY - 1), foreground);
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

void ChannelNavigationRenderer::drawOffModeIcon(
    const std::int16_t x,
    const std::int16_t y,
    const hal::PixelColor color) {
    display_.drawRectangle(x, y, 12, 12, color);
    display_.drawLine(x + 2, y + 9, x + 9, y + 2, color);
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
