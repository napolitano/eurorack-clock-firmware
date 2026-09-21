/**
 * @file menu_model_channel.cpp
 * @brief Flat priority-ordered channel settings model and visual section metadata.
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
    switch (mode) {
        case ChannelMode::Off: return 1U;
        case ChannelMode::Clock: return 13U;
        case ChannelMode::Euclid:
        case ChannelMode::Sequencer: return 14U;
        default: return 1U;
    }
}

ChannelMenuAction channelMenuAction(const ChannelMode mode, const std::uint8_t rowIndex) {
    if (rowIndex == 0U || mode == ChannelMode::Off) {
        return ChannelMenuAction::Mode;
    }

    if (mode == ChannelMode::Clock) {
        constexpr ChannelMenuAction kActions[] = {
            ChannelMenuAction::Mode,
            ChannelMenuAction::Rate,
            ChannelMenuAction::PolyNumerator,
            ChannelMenuAction::PolyDenominator,
            ChannelMenuAction::Swing,
            ChannelMenuAction::Groove,
            ChannelMenuAction::MeterBeats,
            ChannelMenuAction::MeterUnit,
            ChannelMenuAction::Probability,
            ChannelMenuAction::Gate,
            ChannelMenuAction::Phase,
            ChannelMenuAction::Reset,
            ChannelMenuAction::Mute};
        return kActions[rowIndex < 13U ? rowIndex : 0U];
    }

    if (mode == ChannelMode::Euclid) {
        constexpr ChannelMenuAction kActions[] = {
            ChannelMenuAction::Mode,
            ChannelMenuAction::EuclidSteps,
            ChannelMenuAction::EuclidHits,
            ChannelMenuAction::EuclidRotate,
            ChannelMenuAction::Rate,
            ChannelMenuAction::PolyNumerator,
            ChannelMenuAction::PolyDenominator,
            ChannelMenuAction::Swing,
            ChannelMenuAction::Groove,
            ChannelMenuAction::Probability,
            ChannelMenuAction::Gate,
            ChannelMenuAction::Phase,
            ChannelMenuAction::Reset,
            ChannelMenuAction::Mute};
        return kActions[rowIndex < 14U ? rowIndex : 0U];
    }

    constexpr ChannelMenuAction kActions[] = {
        ChannelMenuAction::Mode,
        ChannelMenuAction::SequencerLength,
        ChannelMenuAction::SequencerRotate,
        ChannelMenuAction::SequencerPattern,
        ChannelMenuAction::Rate,
        ChannelMenuAction::PolyNumerator,
        ChannelMenuAction::PolyDenominator,
        ChannelMenuAction::Swing,
        ChannelMenuAction::Groove,
        ChannelMenuAction::Probability,
        ChannelMenuAction::Gate,
        ChannelMenuAction::Phase,
        ChannelMenuAction::Reset,
        ChannelMenuAction::Mute};
    return kActions[rowIndex < 14U ? rowIndex : 0U];
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

MenuRow buildFlatChannelMenuRow(
    const std::uint8_t rowIndex,
    const ChannelConfig& channel) {
    MenuRow row{};
    const ChannelMenuAction action = channelMenuAction(channel.common.mode, rowIndex);

    switch (action) {
        case ChannelMenuAction::Mode:
            copyText(row.label, sizeof(row.label), text::TextId::Mode);
            std::snprintf(row.value, sizeof(row.value), "%s", channelModeLongLabel(channel.common.mode));
            break;
        case ChannelMenuAction::Rate:
            copyText(row.label, sizeof(row.label), text::TextId::DivideMultiply);
            formatRate(channel.common, row.value, sizeof(row.value));
            break;
        case ChannelMenuAction::PolyNumerator:
            copyText(row.label, sizeof(row.label), text::TextId::PolyNumerator);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.common.rate.numerator);
            break;
        case ChannelMenuAction::PolyDenominator:
            copyText(row.label, sizeof(row.label), text::TextId::PolyDenominator);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.common.rate.denominator);
            break;
        case ChannelMenuAction::Swing:
            copyText(row.label, sizeof(row.label), text::TextId::Swing);
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::PercentFormat), channel.common.swingPercent);
            break;
        case ChannelMenuAction::Groove:
            copyText(row.label, sizeof(row.label), text::TextId::Groove);
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
            break;
        case ChannelMenuAction::MeterBeats:
            copyText(row.label, sizeof(row.label), text::TextId::MeterBeats);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.clock.meter.beats);
            break;
        case ChannelMenuAction::MeterUnit:
            copyText(row.label, sizeof(row.label), text::TextId::MeterUnit);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.clock.meter.unit);
            break;
        case ChannelMenuAction::EuclidSteps:
            copyText(row.label, sizeof(row.label), text::TextId::Steps);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.euclid.steps);
            break;
        case ChannelMenuAction::EuclidHits:
            copyText(row.label, sizeof(row.label), text::TextId::Hits);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.euclid.hits);
            break;
        case ChannelMenuAction::EuclidRotate:
            copyText(row.label, sizeof(row.label), text::TextId::Rotate);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.euclid.rotation);
            break;
        case ChannelMenuAction::SequencerLength:
            copyText(row.label, sizeof(row.label), text::TextId::Length);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.sequencer.length);
            break;
        case ChannelMenuAction::SequencerRotate:
            copyText(row.label, sizeof(row.label), text::TextId::Rotate);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.sequencer.rotation);
            break;
        case ChannelMenuAction::SequencerPattern:
            copyText(row.label, sizeof(row.label), text::TextId::Pattern);
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
            break;
        case ChannelMenuAction::Probability:
            copyText(row.label, sizeof(row.label), text::TextId::Probability);
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::PercentFormat), channel.common.probabilityPercent);
            break;
        case ChannelMenuAction::Gate:
            copyText(row.label, sizeof(row.label), text::TextId::Gate);
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::MillisecondsFormat), channel.common.gateLengthMs);
            break;
        case ChannelMenuAction::Phase:
            copyText(row.label, sizeof(row.label), text::TextId::Phase);
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::PercentFormat), channel.common.phasePercent);
            break;
        case ChannelMenuAction::Reset:
            copyText(row.label, sizeof(row.label), text::TextId::Reset);
            copyText(row.value, sizeof(row.value), channel.common.resetMode == ResetMode::Global ? text::TextId::Global : text::TextId::SyncFree);
            break;
        case ChannelMenuAction::Mute:
            copyText(row.label, sizeof(row.label), text::TextId::Mute);
            copyText(row.value, sizeof(row.value), channel.common.muted ? text::TextId::On : text::TextId::Off);
            break;
    }
    return row;
}

SettingsSection settingsRowSection(
    const SettingsPage page,
    const ChannelMode mode,
    const std::uint8_t rowIndex) {
    if (page == SettingsPage::UnifiedClock) {
        if (rowIndex >= 1U && rowIndex <= 6U) return SettingsSection::Timing;
        if (rowIndex >= 7U) return SettingsSection::Output;
        return SettingsSection::None;
    }
    if (page != SettingsPage::Channel || rowIndex == 0U || mode == ChannelMode::Off) {
        return SettingsSection::None;
    }

    if (mode == ChannelMode::Clock) {
        if (rowIndex <= 5U) return SettingsSection::Timing;
        if (rowIndex <= 7U) return SettingsSection::Clock;
        return SettingsSection::Output;
    }
    if (mode == ChannelMode::Euclid) {
        if (rowIndex <= 3U) return SettingsSection::Euclid;
        if (rowIndex <= 8U) return SettingsSection::Timing;
        return SettingsSection::Output;
    }
    if (mode == ChannelMode::Sequencer) {
        if (rowIndex <= 3U) return SettingsSection::Sequencer;
        if (rowIndex <= 8U) return SettingsSection::Timing;
        return SettingsSection::Output;
    }
    return SettingsSection::None;
}

std::uint8_t groupedSettingsVisibleItemCount(
    const SettingsPage page,
    const ChannelMode mode,
    const std::uint8_t startRow,
    const std::uint8_t itemCount) {
    constexpr std::int16_t kContentTopY = 11;
    constexpr std::int16_t kLastTextTopY = 57;
    constexpr std::int16_t kTextHeight = 7;
    constexpr std::int16_t kLineAdvance = 9;
    constexpr std::int16_t kSectionMarginTop = 4;
    constexpr std::int16_t kNormalLineGap = kLineAdvance - kTextHeight;
    constexpr std::int16_t kSectionExtraGap = kSectionMarginTop - kNormalLineGap;
    static_assert(kSectionExtraGap >= 0);

    std::int16_t y = kContentTopY;
    std::uint8_t visibleItems = 0U;
    SettingsSection renderedSection = SettingsSection::None;
    bool hasRenderedContent = false;

    for (std::uint8_t rowIndex = startRow; rowIndex < itemCount; ++rowIndex) {
        const SettingsSection section = settingsRowSection(page, mode, rowIndex);
        const bool needsHeading = section != SettingsSection::None && section != renderedSection;
        if (needsHeading) {
            // A heading at the top of the viewport is sticky and therefore has no
            // top margin. Headings appearing below existing content keep a real
            // four-pixel blank margin above their seven-pixel glyph box.
            if (hasRenderedContent) {
                y = static_cast<std::int16_t>(y + kSectionExtraGap);
            }
            if (y > kLastTextTopY) {
                break;
            }
            y = static_cast<std::int16_t>(y + kLineAdvance);
            renderedSection = section;
            hasRenderedContent = true;
        }

        if (y > kLastTextTopY) {
            break;
        }
        ++visibleItems;
        y = static_cast<std::int16_t>(y + kLineAdvance);
        hasRenderedContent = true;
    }
    return visibleItems;
}

const char* settingsSectionLabel(const SettingsSection section) {
    switch (section) {
        case SettingsSection::Timing: return text::get(text::TextId::Timing);
        case SettingsSection::Clock: return text::get(text::TextId::ModeClockLong);
        case SettingsSection::Euclid: return text::get(text::TextId::ModeEuclidLong);
        case SettingsSection::Sequencer: return text::get(text::TextId::ModeSequencerLong);
        case SettingsSection::Output: return text::get(text::TextId::Output);
        case SettingsSection::None:
        default: return "";
    }
}

}  // namespace clockfw::ui
