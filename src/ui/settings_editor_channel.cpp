/**
 * @file settings_editor_channel.cpp
 * @brief Validated mutation helpers for selected-channel timing, mode, and output groups.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/settings_editor.h"

#include <cstddef>

#include "domain/clock_options.h"
#include "ui/menu_model.h"

namespace clockfw::ui {

void SettingsEditor::adjustCommonChannel(
    const std::uint8_t channelIndex,
    const std::uint8_t rowIndex,
    const std::int8_t delta) {
    ChannelConfig& channel = state_.channels[channelIndex];

    if (rowIndex == 3U) {
        channel.common.swingPercent = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.common.swingPercent) + delta, 0, 50));
    } else if (rowIndex == 4U) {
        channel.common.probabilityPercent = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.common.probabilityPercent) + delta, 0, 100));
    } else if (rowIndex == 5U) {
        const std::size_t currentIndex = findGateLengthOptionIndex(channel.common.gateLengthMs);
        const std::size_t newIndex = static_cast<std::size_t>(clampInt(
            static_cast<int>(currentIndex) + delta,
            0,
            static_cast<int>(kGateLengthOptionsMs.size() - 1U)));
        channel.common.gateLengthMs = kGateLengthOptionsMs[newIndex];
    } else if (rowIndex == 6U) {
        channel.common.phasePercent = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.common.phasePercent) + delta, 0, 99));
    } else if (rowIndex == 7U) {
        channel.common.resetMode = delta > 0 ? ResetMode::Free : ResetMode::Global;
    } else if (rowIndex == 8U) {
        channel.common.muted = !channel.common.muted;
    } else {
        return;
    }

    engine_.updateChannel(channelIndex, channel, true);
}

void SettingsEditor::adjustRate(
    const std::uint8_t channelIndex,
    const std::uint8_t rowIndex,
    const std::int8_t delta) {
    ChannelConfig& channel = state_.channels[channelIndex];

    if (rowIndex == 0U) {
        const std::size_t currentIndex = findRateOptionIndex(channel.common);
        const std::size_t newIndex = static_cast<std::size_t>(clampInt(
            static_cast<int>(currentIndex) + delta,
            0,
            static_cast<int>(kRateOptions.size() - 1U)));
        channel.common.rate.mode = kRateOptions[newIndex].mode;
        channel.common.rate.factor = kRateOptions[newIndex].factor;
    } else if (rowIndex == 1U) {
        channel.common.rate.numerator = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.common.rate.numerator) + delta, 1, 16));
    } else if (rowIndex == 2U) {
        channel.common.rate.denominator = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.common.rate.denominator) + delta, 1, 16));
    }

    engine_.updateChannel(channelIndex, channel, true);
}

void SettingsEditor::adjustClock(
    const std::uint8_t channelIndex,
    const std::uint8_t rowIndex,
    const std::int8_t delta) {
    ChannelConfig& channel = state_.channels[channelIndex];

    if (rowIndex == 0U) {
        channel.clock.meter.beats = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.clock.meter.beats) + delta, 1, 16));
    } else if (rowIndex == 1U) {
        const std::size_t currentIndex = findBeatUnitOptionIndex(channel.clock.meter.unit);
        const std::size_t newIndex = static_cast<std::size_t>(clampInt(
            static_cast<int>(currentIndex) + delta,
            0,
            static_cast<int>(kBeatUnitOptions.size() - 1U)));
        channel.clock.meter.unit = kBeatUnitOptions[newIndex];
    }

    engine_.updateChannel(channelIndex, channel, true);
}

void SettingsEditor::adjustEuclid(
    const std::uint8_t channelIndex,
    const std::uint8_t rowIndex,
    const std::int8_t delta) {
    ChannelConfig& channel = state_.channels[channelIndex];

    if (rowIndex == 0U) {
        channel.euclid.steps = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.euclid.steps) + delta, 1, 64));
        if (channel.euclid.hits > channel.euclid.steps) {
            channel.euclid.hits = channel.euclid.steps;
        }
        if (channel.euclid.rotation >= channel.euclid.steps) {
            channel.euclid.rotation = 0U;
        }
    } else if (rowIndex == 1U) {
        channel.euclid.hits = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.euclid.hits) + delta, 0, channel.euclid.steps));
    } else if (rowIndex == 2U) {
        channel.euclid.rotation = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.euclid.rotation) + delta,
            0,
            channel.euclid.steps - 1));
    }

    engine_.updateChannel(channelIndex, channel, false);
}

void SettingsEditor::adjustSequencer(
    const std::uint8_t channelIndex,
    const std::uint8_t rowIndex,
    const std::int8_t delta) {
    ChannelConfig& channel = state_.channels[channelIndex];

    if (rowIndex == 0U) {
        channel.sequencer.length = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.sequencer.length) + delta, 1, 64));
        if (channel.sequencer.rotation >= channel.sequencer.length) {
            channel.sequencer.rotation = 0U;
        }
    } else if (rowIndex == 1U) {
        channel.sequencer.rotation = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.sequencer.rotation) + delta,
            0,
            channel.sequencer.length - 1));
    }

    engine_.updateChannel(channelIndex, channel, false);
}

}  // namespace clockfw::ui
