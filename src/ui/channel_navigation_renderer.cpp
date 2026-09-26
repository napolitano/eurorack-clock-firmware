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
#include "ui/sequencer_editor_primitives.h"
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
    const engine::EngineSnapshot& engineSnapshot,
    const SequencerPatternV2* const activePattern,
    const std::uint8_t activeSlot,
    const std::array<SequencerStepMetadataWord, kSequencerMaximumSteps>* const metadata) {
    SequencerPatternV2 fallback{};
    const SequencerSettings& legacy = state.channels[navigation.selectedChannel].sequencer;
    fallback.length = legacy.length == 0U ? 1U : legacy.length;
    fallback.rotation = legacy.rotation < fallback.length ? legacy.rotation : 0U;
    fallback.gates = {{legacy.pattern, 0ULL}};
    const SequencerPatternV2& pattern = activePattern != nullptr ? *activePattern : fallback;

    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    const std::uint8_t pageBaseStep = static_cast<std::uint8_t>(navigation.sequencerPage * 8U);
    const std::uint8_t pageLastStep = static_cast<std::uint8_t>(
        std::min<std::uint16_t>(
            static_cast<std::uint16_t>(pageBaseStep) + 8U,
            pattern.length) - 1U);
    const std::uint8_t selectedStep = static_cast<std::uint8_t>(
        std::min<std::uint16_t>(
            static_cast<std::uint16_t>(pageBaseStep) + navigation.sequencerCursor,
            static_cast<std::uint16_t>(pattern.length - 1U)));
    char header[24]{};
    std::snprintf(
        header, sizeof(header), text::get(text::TextId::SequencerEditorHeaderFormat),
        navigation.selectedChannel + 1U, activeSlot + 1U,
        pageBaseStep + 1U, pageLastStep + 1U);
    display_.drawText(0, 0, header);
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);
    sequencer_editor::drawTimingGrid(display_, pageBaseStep);

    for (std::uint8_t pageStep = 0U; pageStep < 8U; ++pageStep) {
        const std::uint8_t absoluteStep = static_cast<std::uint8_t>(pageBaseStep + pageStep);
        const std::int16_t columnX = static_cast<std::int16_t>(
            sequencer_editor::kGridLeft + static_cast<int>(pageStep) * sequencer_editor::kStepPitch);
        const std::int16_t gateX = static_cast<std::int16_t>(columnX + 4);
        const std::int16_t centerX = static_cast<std::int16_t>(columnX + 8);
        const bool inPattern = absoluteStep < pattern.length;
        const bool selected = pageStep == navigation.sequencerCursor;

        if (selected) {
            display_.drawRectangle(static_cast<std::int16_t>(columnX + 2), 20, 13, 13);
        }
        if (inPattern && sequencerPatternGate(pattern, absoluteStep)) {
            display_.fillRectangle(gateX, sequencer_editor::kGateY, 9, 9);
        } else if (inPattern) {
            display_.drawRectangle(gateX, sequencer_editor::kGateY, 9, 9);
        }
        if (inPattern && metadata != nullptr) {
            const SequencerStepMetadata stepMetadata =
                unpackSequencerStepMetadata((*metadata)[absoluteStep]);
            if (stepMetadata.tie && pageStep < 7U && absoluteStep + 1U < pattern.length) {
                display_.drawHorizontalLine(
                    static_cast<std::int16_t>(gateX + 8),
                    static_cast<std::int16_t>(sequencer_editor::kGateY + 4),
                    8);
            }
            sequencer_editor::drawRatchetSymbol(display_, centerX, stepMetadata.ratchetCount);
            if (stepMetadata.probabilityPercent != 0U) {
                if (selected) {
                    sequencer_editor::drawProbabilityValue(
                        display_, centerX, 41, stepMetadata.probabilityPercent);
                } else {
                    sequencer_editor::drawProbabilitySymbol(display_, centerX, 41);
                }
            }
        }
    }

    const std::uint8_t playbackStep = engineSnapshot.channelStep[navigation.selectedChannel];
    if (engineSnapshot.playing && playbackStep >= pageBaseStep &&
        playbackStep < static_cast<std::uint16_t>(pageBaseStep) + 8U &&
        playbackStep < pattern.length) {
        const std::uint8_t pagePlaybackStep = static_cast<std::uint8_t>(playbackStep - pageBaseStep);
        const std::int16_t playheadCenterX = static_cast<std::int16_t>(
            sequencer_editor::kGridLeft + static_cast<int>(pagePlaybackStep) * sequencer_editor::kStepPitch + 8);
        sequencer_editor::drawPlayheadTriangle(display_, playheadCenterX);
    }

    char footer[20]{};
    std::snprintf(
        footer, sizeof(footer), text::get(text::TextId::SequencerEditorFooterFormat),
        selectedStep + 1U, pattern.length);
    display_.drawText(1, 55, footer);

    sequencer_editor::drawDirectionIcon(display_, pattern.direction, 60, 54);

    char rate[8]{};
    formatRate(state.channels[navigation.selectedChannel].common, rate, sizeof(rate));
    const hal::TextBounds rateBounds = display_.measureText(rate, 0, 55);
    display_.drawText(
        static_cast<std::int16_t>(hal::OledDisplay::kWidth - rateBounds.width - 1),
        55, rate);

    patternStripRenderer_.drawSequencerEditorPageIndicator(pattern.length, pageBaseStep);
    display_.present();
}

void ChannelNavigationRenderer::renderSequencerStepEditor(
    const ClockState& state,
    const NavigationState& navigation,
    const SequencerPatternV2& activePattern,
    const std::uint8_t activeSlot,
    const SequencerStepMetadata& metadata) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    const std::uint8_t absoluteStep = static_cast<std::uint8_t>(
        navigation.sequencerPage * 8U + navigation.sequencerCursor);
    char header[24]{};
    std::snprintf(
        header, sizeof(header), text::get(text::TextId::SequencerStepEditorTitleFormat),
        absoluteStep + 1U, navigation.selectedChannel + 1U, activeSlot + 1U);
    display_.drawText(0, 0, header);
    display_.drawHorizontalLine(0, 9, hal::OledDisplay::kWidth);

    const char* labels[5] = {
        text::get(text::TextId::Gate),
        text::get(text::TextId::Probability),
        text::get(text::TextId::Length),
        text::get(text::TextId::Ratchet),
        text::get(text::TextId::Tie),
    };
    char values[5][12]{};
    std::snprintf(
        values[0], sizeof(values[0]), "%s",
        text::get(sequencerPatternGate(activePattern, absoluteStep)
            ? text::TextId::On : text::TextId::Off));
    if (metadata.probabilityPercent == 0U) {
        std::snprintf(values[1], sizeof(values[1]), "%s", text::get(text::TextId::DefaultValue));
    } else {
        std::snprintf(
            values[1], sizeof(values[1]), text::get(text::TextId::PercentFormat), metadata.probabilityPercent);
    }
    if (metadata.gateProfile == SequencerGateProfile::Default) {
        std::snprintf(
            values[2], sizeof(values[2]), "%s", text::get(text::TextId::DefaultValue));
    } else {
        const std::uint8_t duty = sequencerGateProfileDutyPercent(metadata.gateProfile);
        if (duty != 0U) {
            std::snprintf(
                values[2], sizeof(values[2]), text::get(text::TextId::PercentFormat), duty);
        } else {
            const std::uint16_t milliseconds =
                sequencerGateProfileMilliseconds(metadata.gateProfile, 0U);
            std::snprintf(
                values[2], sizeof(values[2]), text::get(text::TextId::MillisecondsFormat),
                milliseconds);
        }
    }
    std::snprintf(values[3], sizeof(values[3]), "%u", metadata.ratchetCount);
    if (activePattern.direction == SequencerPlayDirection::Random) {
        std::snprintf(values[4], sizeof(values[4]), "%s", text::get(text::TextId::NotAvailable));
    } else {
        std::snprintf(
            values[4], sizeof(values[4]), "%s",
            text::get(metadata.tie ? text::TextId::On : text::TextId::Off));
    }

    for (std::uint8_t row = 0U; row < 5U; ++row) {
        const std::int16_t y = static_cast<std::int16_t>(12 + row * 9U);
        const bool selected = navigation.cursor == row;
        if (selected) {
            display_.drawText(0, y, text::get(text::TextId::Arrow));
        }
        display_.drawText(8, y, labels[row]);
        const hal::TextBounds bounds = display_.measureText(values[row], 0, y);
        const std::int16_t valueX = static_cast<std::int16_t>(127 - bounds.width);
        if (selected && navigation.editing && row != 0U) {
            display_.fillRectangle(
                valueX - 1, y - 1,
                static_cast<std::int16_t>(bounds.width + 2U), 9,
                hal::PixelColor::White);
            display_.setTextColor(hal::PixelColor::Black);
            display_.drawText(valueX, y, values[row]);
            display_.setTextColor(hal::PixelColor::White);
        } else {
            display_.drawText(valueX, y, values[row]);
        }
    }
    (void)state;
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
