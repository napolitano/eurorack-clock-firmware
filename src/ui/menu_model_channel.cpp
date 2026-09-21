/**
 * @file menu_model_channel.cpp
 * @brief Compact mode-aware channel settings hierarchy.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/menu_model_channel.h"

#include <cstdio>

#include "domain/clock_labels.h"
#include "ui/menu_model.h"
#include "ui/text_formatter.h"
#include "ui_text.h"

namespace clockfw::ui {
namespace {

void copyText(char* const destination, const std::size_t size, const text::TextId id) {
    std::snprintf(destination, size, "%s", text::get(id));
}

}  // namespace

std::uint8_t channelMenuItemCount(const ChannelMode mode) {
    return mode == ChannelMode::Off ? 1U : 4U;
}

ChannelMenuAction channelMenuAction(const ChannelMode mode, const std::uint8_t rowIndex) {
    if (rowIndex == 0U || mode == ChannelMode::Off) {
        return ChannelMenuAction::Mode;
    }

    if (mode == ChannelMode::Clock) {
        constexpr ChannelMenuAction kActions[] = {
            ChannelMenuAction::Mode,
            ChannelMenuAction::Timing,
            ChannelMenuAction::Clock,
            ChannelMenuAction::Output};
        return kActions[rowIndex < 4U ? rowIndex : 0U];
    }

    if (mode == ChannelMode::Euclid) {
        constexpr ChannelMenuAction kActions[] = {
            ChannelMenuAction::Mode,
            ChannelMenuAction::Euclid,
            ChannelMenuAction::Timing,
            ChannelMenuAction::Output};
        return kActions[rowIndex < 4U ? rowIndex : 0U];
    }

    constexpr ChannelMenuAction kActions[] = {
        ChannelMenuAction::Mode,
        ChannelMenuAction::Sequencer,
        ChannelMenuAction::Timing,
        ChannelMenuAction::Output};
    return kActions[rowIndex < 4U ? rowIndex : 0U];
}

std::uint8_t channelMenuIndexForAction(
    const ChannelMode mode,
    const ChannelMenuAction action) {
    const std::uint8_t count = channelMenuItemCount(mode);
    for (std::uint8_t index = 0U; index < count; ++index) {
        if (channelMenuAction(mode, index) == action) {
            return index;
        }
    }
    return 0U;
}

MenuRow buildChannelMenuRow(
    const std::uint8_t rowIndex,
    const ChannelConfig& channel) {
    MenuRow row{};
    const ChannelMenuAction action = channelMenuAction(channel.common.mode, rowIndex);

    switch (action) {
        case ChannelMenuAction::Mode:
            copyText(row.label, sizeof(row.label), text::TextId::Mode);
            std::snprintf(
                row.value,
                sizeof(row.value),
                "%s",
                channelModeLongLabel(channel.common.mode));
            break;
        case ChannelMenuAction::Timing:
            copyText(row.label, sizeof(row.label), text::TextId::Timing);
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
            break;
        case ChannelMenuAction::Clock:
            copyText(row.label, sizeof(row.label), text::TextId::ModeClockLong);
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
            break;
        case ChannelMenuAction::Euclid:
            copyText(row.label, sizeof(row.label), text::TextId::ModeEuclidLong);
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
            break;
        case ChannelMenuAction::Sequencer:
            copyText(row.label, sizeof(row.label), text::TextId::ModeSequencerLong);
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
            break;
        case ChannelMenuAction::Output:
            copyText(row.label, sizeof(row.label), text::TextId::Output);
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
            break;
    }
    return row;
}


MenuRow buildChannelTimingMenuRow(
    const std::uint8_t rowIndex,
    const ChannelConfig& channel) {
    MenuRow row{};
    constexpr text::TextId kLabels[] = {
        text::TextId::DivideMultiply,
        text::TextId::PolyNumerator,
        text::TextId::PolyDenominator,
        text::TextId::Swing,
        text::TextId::Groove};
    copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
    if (rowIndex == 0U) {
        formatRate(channel.common, row.value, sizeof(row.value));
    } else if (rowIndex == 1U) {
        std::snprintf(row.value, sizeof(row.value), "%u", channel.common.rate.numerator);
    } else if (rowIndex == 2U) {
        std::snprintf(row.value, sizeof(row.value), "%u", channel.common.rate.denominator);
    } else if (rowIndex == 3U) {
        std::snprintf(
            row.value,
            sizeof(row.value),
            text::get(text::TextId::PercentFormat),
            channel.common.swingPercent);
    } else {
        copyText(row.value, sizeof(row.value), text::TextId::Arrow);
    }
    return row;
}

MenuRow buildChannelOutputMenuRow(
    const std::uint8_t rowIndex,
    const ChannelConfig& channel) {
    MenuRow row{};
    constexpr text::TextId kLabels[] = {
        text::TextId::Probability,
        text::TextId::Gate,
        text::TextId::Phase,
        text::TextId::Reset,
        text::TextId::Mute};
    copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
    if (rowIndex == 0U) {
        std::snprintf(
            row.value,
            sizeof(row.value),
            text::get(text::TextId::PercentFormat),
            channel.common.probabilityPercent);
    } else if (rowIndex == 1U) {
        std::snprintf(
            row.value,
            sizeof(row.value),
            text::get(text::TextId::MillisecondsFormat),
            channel.common.gateLengthMs);
    } else if (rowIndex == 2U) {
        std::snprintf(
            row.value,
            sizeof(row.value),
            text::get(text::TextId::PercentFormat),
            channel.common.phasePercent);
    } else if (rowIndex == 3U) {
        copyText(
            row.value,
            sizeof(row.value),
            channel.common.resetMode == ResetMode::Global
                ? text::TextId::Global
                : text::TextId::SyncFree);
    } else {
        copyText(
            row.value,
            sizeof(row.value),
            channel.common.muted ? text::TextId::On : text::TextId::Off);
    }
    return row;
}

MenuRow buildUnifiedClockMenuRow(const std::uint8_t rowIndex) {
    MenuRow row{};
    constexpr text::TextId kLabels[] = {
        text::TextId::Mode,
        text::TextId::Timing,
        text::TextId::Output};
    copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
    copyText(
        row.value,
        sizeof(row.value),
        rowIndex == 0U ? text::TextId::ModeUnifiedLong : text::TextId::Arrow);
    return row;
}

MenuRow buildUnifiedTimingMenuRow(
    const std::uint8_t rowIndex,
    const UnifiedClockSettings& settings) {
    MenuRow row{};
    constexpr text::TextId kLabels[] = {
        text::TextId::DivideMultiply,
        text::TextId::PolyNumerator,
        text::TextId::PolyDenominator,
        text::TextId::Swing,
        text::TextId::Groove,
        text::TextId::Humanize};
    copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
    if (rowIndex == 0U) {
        CommonChannelSettings temporary{};
        temporary.rate = settings.rate;
        formatRate(temporary, row.value, sizeof(row.value));
    } else if (rowIndex == 1U) {
        std::snprintf(row.value, sizeof(row.value), "%u", settings.rate.numerator);
    } else if (rowIndex == 2U) {
        std::snprintf(row.value, sizeof(row.value), "%u", settings.rate.denominator);
    } else if (rowIndex == 3U) {
        std::snprintf(
            row.value,
            sizeof(row.value),
            text::get(text::TextId::PercentFormat),
            settings.swingPercent);
    } else if (rowIndex == 4U) {
        copyText(row.value, sizeof(row.value), text::TextId::Arrow);
    } else if (settings.humanizeUs == 0U) {
        copyText(row.value, sizeof(row.value), text::TextId::Off);
    } else {
        std::snprintf(
            row.value,
            sizeof(row.value),
            text::get(text::TextId::MicrosecondsFormat),
            settings.humanizeUs);
    }
    return row;
}

MenuRow buildUnifiedOutputMenuRow(
    const std::uint8_t rowIndex,
    const UnifiedClockSettings& settings) {
    MenuRow row{};
    copyText(
        row.label,
        sizeof(row.label),
        rowIndex == 0U ? text::TextId::Gate : text::TextId::Phase);
    if (rowIndex == 0U) {
        std::snprintf(
            row.value,
            sizeof(row.value),
            text::get(text::TextId::MillisecondsFormat),
            settings.gateLengthMs);
    } else {
        std::snprintf(
            row.value,
            sizeof(row.value),
            text::get(text::TextId::PercentFormat),
            settings.phasePercent);
    }
    return row;
}

}  // namespace clockfw::ui
