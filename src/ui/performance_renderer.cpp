/**
 * @file performance_renderer.cpp
 * @brief Rendering implementation for the normal clock performance screen.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/performance_renderer.h"

#include <algorithm>
#include <cstdio>

#include "engine/output_mode_resolver.h"
#include "ui/text_formatter.h"
#include "ui_text.h"

namespace clockfw::ui {
namespace {

/** Top pixel row of the normal 18-pixel tempo numerals. */
constexpr std::int16_t kTempoTopY = 14;

/** Top pixel row of the larger CLOCK-mode tempo numerals. */
constexpr std::int16_t kClockTempoTopY = 20;

/** Top pixel row of the divider-mode tempo, leaving room for eight factor slots. */
constexpr std::int16_t kDividerTempoTopY = 13;

/** Vertical position for small context values beside/below the tempo. */
constexpr std::int16_t kTempoContextY = 24;

/** Vertical position for values flanking the larger CLOCK tempo. */
constexpr std::int16_t kClockTempoContextY = 31;

/** Top pixel row used by Euclid/Sequencer pattern marks. */
constexpr std::int16_t kPatternTopY = 52;

/** X position after the role/lock area when no lock icon is present. */
constexpr std::int16_t kChannelStatusMasterX = 15;

/** X position after the role/lock area when the slave lock icon is present. */
constexpr std::int16_t kChannelStatusLockedSlaveX = 21;

/** Formats the compact rate value shown at the right of CLOCK-mode tempo. */
void formatClockPerformanceRate(
    const CommonChannelSettings& settings,
    char* const output,
    const std::size_t outputSize) {
    if (settings.rate.numerator == 1U &&
        settings.rate.denominator == 1U &&
        settings.rate.factor == 1U) {
        output[0] = '\0';
        return;
    }

    if (settings.rate.numerator != 1U || settings.rate.denominator != 1U) {
        std::snprintf(
            output,
            outputSize,
            "%u:%u",
            settings.rate.numerator,
            settings.rate.denominator);
        return;
    }

    std::snprintf(
        output,
        outputSize,
        "%c%u",
        settings.rate.mode == ClockRatioMode::Multiply ? 'x' : '/',
        settings.rate.factor);
}

}  // namespace

PerformanceRenderer::PerformanceRenderer(hal::OledDisplay& display)
    : display_(display), patternStripRenderer_(display) {}

void PerformanceRenderer::render(
    const ClockState& state,
    const NavigationState& navigation,
    const engine::EngineSnapshot& engineSnapshot) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    drawClockRoleStatus(state.source, engineSnapshot.externalLocked);

    const ChannelConfig& configuredChannel = state.channels[navigation.selectedChannel];
    const ChannelConfig selectedChannel = engine::resolvePhysicalOutputConfiguration(
        state, navigation.selectedChannel);
    const bool lockedSlave =
        engineSnapshot.externalLocked &&
        (state.source == ClockSource::External || state.source == ClockSource::Auto);
    drawChannelStatus(
        navigation.selectedChannel,
        state.operatingMode,
        configuredChannel.common.mode,
        lockedSlave ? kChannelStatusLockedSlaveX : kChannelStatusMasterX);
    if (state.operatingMode == OperatingMode::UnifiedClock && state.unifiedClock.humanizeUs > 0U) {
        drawHumanizeIcon(42, 1);
    }

    char meterText[8]{};
    std::snprintf(
        meterText,
        sizeof(meterText),
        text::get(text::TextId::MeterFormat),
        state.masterMeter.beats,
        state.masterMeter.unit);
    const hal::TextBounds meterBounds = display_.measureText(meterText, 0, 1);
    display_.drawText(
        static_cast<std::int16_t>(
            (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(meterBounds.width)) / 2),
        1,
        meterText);

    drawTransport(state.transport);

    if (state.operatingMode == OperatingMode::Independent &&
        configuredChannel.common.mode == ChannelMode::Off) {
        drawTempoText("---", hal::DisplayFont::TempoLarge, kClockTempoTopY);
        display_.present();
        return;
    }

    char tempoText[6]{};
    std::snprintf(tempoText, sizeof(tempoText), "%u", state.bpm);

    const bool dividerView = state.operatingMode == OperatingMode::DividerBank;
    const bool clockOnlyView = state.operatingMode != OperatingMode::Independent ||
        configuredChannel.common.mode == ChannelMode::Clock;
    const hal::DisplayFont tempoFont = dividerView
        ? hal::DisplayFont::Tempo
        : (clockOnlyView ? hal::DisplayFont::TempoLarge : hal::DisplayFont::Tempo);
    const std::int16_t tempoTopY = dividerView
        ? kDividerTempoTopY
        : (clockOnlyView ? kClockTempoTopY : kTempoTopY);
    const std::int16_t contextY = clockOnlyView ? kClockTempoContextY : kTempoContextY;

    display_.setFont(tempoFont);
    const hal::TextBounds tempoBounds = display_.measureText(tempoText, 0, tempoTopY);
    const std::int16_t tempoX = static_cast<std::int16_t>(
        (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(tempoBounds.width)) / 2);
    display_.drawText(tempoX, tempoTopY, tempoText);
    display_.setFont(hal::DisplayFont::Small);

    if (selectedChannel.common.swingPercent > 0U) {
        char swingText[8]{};
        std::snprintf(
            swingText,
            sizeof(swingText),
            text::get(text::TextId::PercentFormat),
            selectedChannel.common.swingPercent);
        // Swing is a secondary left-edge status value; it must never move the centered BPM.
        display_.drawText(1, contextY, swingText);
    }

    if (dividerView) {
        drawDividerBankSlots(state);
        display_.present();
        return;
    }

    if (clockOnlyView) {
        char rateText[8]{};
        formatClockPerformanceRate(selectedChannel.common, rateText, sizeof(rateText));
        if (rateText[0] != '\0') {
            const hal::TextBounds rateBounds = display_.measureText(rateText, 0, contextY);
            // Rate is a secondary right-edge status value; BPM remains mathematically centered.
            const std::int16_t rateX = static_cast<std::int16_t>(
                hal::OledDisplay::kWidth - static_cast<std::int16_t>(rateBounds.width) - 1);
            display_.drawText(rateX, contextY, rateText);
        }
        display_.present();
        return;
    }

    // Compact pattern counts are vertically centered beside the BPM and right-aligned.
    // Detailed rotation remains available in the mode settings, while performance view
    // keeps the pattern strip visually dominant.
    char detailText[12]{};
    formatChannelSummary(configuredChannel, detailText, sizeof(detailText));
    display_.setFont(hal::DisplayFont::Small);
    const hal::TextBounds detailBounds = display_.measureText(detailText, 0, contextY);
    display_.drawText(
        static_cast<std::int16_t>(
            hal::OledDisplay::kWidth - static_cast<std::int16_t>(detailBounds.width) - 1),
        contextY,
        detailText);

    const std::uint8_t currentStep = engineSnapshot.channelStep[navigation.selectedChannel];
    if (configuredChannel.common.mode == ChannelMode::Euclid) {
        patternStripRenderer_.drawEuclidPattern(
            configuredChannel.euclid,
            currentStep,
            kPatternTopY);
    } else if (configuredChannel.common.mode == ChannelMode::Sequencer) {
        patternStripRenderer_.drawSequencerPlaybackBlock(
            configuredChannel.sequencer,
            currentStep,
            kPatternTopY);
    }

    display_.present();
}

void PerformanceRenderer::drawDividerBankSlots(const ClockState& state) {
    constexpr std::int16_t kGridLeft = 1;
    constexpr std::int16_t kGridTop = 39;
    constexpr std::int16_t kCellWidth = 30;
    constexpr std::int16_t kCellHeight = 11;
    constexpr std::int16_t kColumnPitch = 32;
    constexpr std::int16_t kRowPitch = 12;

    for (std::size_t outputIndex = 0U; outputIndex < kChannelCount; ++outputIndex) {
        const std::int16_t column = static_cast<std::int16_t>(outputIndex % 4U);
        const std::int16_t row = static_cast<std::int16_t>(outputIndex / 4U);
        const std::int16_t x = static_cast<std::int16_t>(kGridLeft + column * kColumnPitch);
        const std::int16_t y = static_cast<std::int16_t>(kGridTop + row * kRowPitch);
        display_.drawRectangle(x, y, kCellWidth, kCellHeight);
        const ChannelConfig output = engine::resolvePhysicalOutputConfiguration(state, outputIndex);
        drawDividerRateSlot(x, y, output.common.rate);
    }
}

void PerformanceRenderer::drawDividerRateSlot(
    const std::int16_t x,
    const std::int16_t y,
    const RateSettings& rate) {
    const bool multiply = rate.mode == ClockRatioMode::Multiply;
    const std::int16_t symbolX = static_cast<std::int16_t>(x + 4);
    const std::int16_t symbolY = static_cast<std::int16_t>(y + 3);

    if (multiply) {
        display_.drawLine(symbolX, symbolY, symbolX + 4, symbolY + 4);
        display_.drawLine(symbolX + 4, symbolY, symbolX, symbolY + 4);
    } else {
        display_.fillRectangle(symbolX + 2, symbolY - 1, 1, 1);
        display_.drawHorizontalLine(symbolX, symbolY + 2, 5);
        display_.fillRectangle(symbolX + 2, symbolY + 5, 1, 1);
    }

    char factorText[4]{};
    std::snprintf(factorText, sizeof(factorText), "%u", static_cast<unsigned>(rate.factor));
    display_.drawText(static_cast<std::int16_t>(x + 11), static_cast<std::int16_t>(y + 2), factorText);
}

void PerformanceRenderer::drawTransport(const TransportState transport) {
    const char* label = transport == TransportState::Playing
        ? text::get(text::TextId::TransportPlay)
        : (transport == TransportState::Paused
            ? text::get(text::TextId::TransportPause)
            : text::get(text::TextId::TransportStop));
    const hal::TextBounds bounds = display_.measureText(label, 0, 1);

    if (transport == TransportState::Playing || transport == TransportState::Paused) {
        const std::int16_t badgeWidth = static_cast<std::int16_t>(bounds.width + 4U);
        const std::int16_t badgeX = static_cast<std::int16_t>(hal::OledDisplay::kWidth - badgeWidth);
        if (transport == TransportState::Playing) {
            display_.fillRectangle(badgeX, 0, badgeWidth, 9);
            display_.setTextColor(hal::PixelColor::Black);
        } else {
            display_.drawRectangle(badgeX, 0, badgeWidth, 9);
        }
        display_.drawText(static_cast<std::int16_t>(badgeX + 2), 1, label);
        display_.setTextColor(hal::PixelColor::White);
        return;
    }

    display_.drawText(
        static_cast<std::int16_t>(
            hal::OledDisplay::kWidth - static_cast<std::int16_t>(bounds.width) - 1),
        1,
        label);
}

void PerformanceRenderer::drawTempoText(
    const char* const text,
    const hal::DisplayFont font,
    const std::int16_t topY) {
    display_.setFont(font);
    const hal::TextBounds bounds = display_.measureText(text, 0, topY);
    display_.drawText(
        static_cast<std::int16_t>(
            (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(bounds.width)) / 2),
        topY,
        text);
    display_.setFont(hal::DisplayFont::Small);
}

void PerformanceRenderer::drawChannelStatus(
    const std::uint8_t channelIndex,
    const OperatingMode operatingMode,
    const ChannelMode mode,
    const std::int16_t leftEdgeX) {
    if (operatingMode == OperatingMode::UnifiedClock) {
        display_.drawText(leftEdgeX, 1, text::get(text::TextId::ModeUnifiedShort));
        return;
    }
    if (operatingMode == OperatingMode::DividerBank) {
        display_.drawText(leftEdgeX, 1, text::get(text::TextId::ModeDividerShort));
        return;
    }

    char status[8]{};
    std::snprintf(
        status,
        sizeof(status),
        text::get(text::TextId::ChannelStatusFormat),
        static_cast<unsigned>(channelIndex + 1U),
        statusModeCharacter(mode));
    display_.drawText(leftEdgeX, 1, status);
}

void PerformanceRenderer::drawClockRoleStatus(
    const ClockSource source,
    const bool externalLocked) {
    const bool slave = source == ClockSource::External ||
        (source == ClockSource::Auto && externalLocked);
    constexpr std::int16_t kBadgeX = 0;
    constexpr std::int16_t kBadgeY = 0;
    constexpr std::int16_t kBadgeSize = 9;

    if (slave) {
        display_.drawRectangle(kBadgeX, kBadgeY, kBadgeSize, kBadgeSize);
        display_.drawCharacter(kBadgeX + 2, 1, 'S');
        if (externalLocked) {
            drawLockIcon(11, 1);
        }
        return;
    }

    display_.fillRectangle(kBadgeX, kBadgeY, kBadgeSize, kBadgeSize);
    display_.setTextColor(hal::PixelColor::Black);
    display_.drawCharacter(kBadgeX + 2, 1, 'M');
    display_.setTextColor(hal::PixelColor::White);
}

void PerformanceRenderer::drawLockIcon(const std::int16_t x, const std::int16_t y) {
    display_.drawRectangle(x, static_cast<std::int16_t>(y + 3), 5, 4);
    display_.drawHorizontalLine(static_cast<std::int16_t>(x + 1), y, 3);
    display_.drawVerticalLine(x, static_cast<std::int16_t>(y + 1), 3);
    display_.drawVerticalLine(static_cast<std::int16_t>(x + 4), static_cast<std::int16_t>(y + 1), 3);
}

void PerformanceRenderer::drawHumanizeIcon(const std::int16_t x, const std::int16_t y) {
    // Compact stick figure made only from public OLED primitives.
    display_.drawRectangle(static_cast<std::int16_t>(x + 2), y, 3, 3);
    display_.drawVerticalLine(static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 3), 3);
    display_.drawHorizontalLine(x, static_cast<std::int16_t>(y + 4), 7);
    display_.drawLine(
        static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 5),
        static_cast<std::int16_t>(x + 1), static_cast<std::int16_t>(y + 7));
    display_.drawLine(
        static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 5),
        static_cast<std::int16_t>(x + 5), static_cast<std::int16_t>(y + 7));
}

char PerformanceRenderer::statusModeCharacter(const ChannelMode mode) {
    switch (mode) {
        case ChannelMode::Off: return 'O';
        case ChannelMode::Euclid: return 'E';
        case ChannelMode::Sequencer: return 'S';
        case ChannelMode::Clock:
        default: return 'C';
    }
}

}  // namespace clockfw::ui
