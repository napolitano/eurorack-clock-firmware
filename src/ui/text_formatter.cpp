/**
 * @file text_formatter.cpp
 * @brief Compact string formatting helpers for the 128x64 OLED UI.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/text_formatter.h"

#include <cstdio>

#include "ui_text.h"

namespace clockfw::ui {

void formatRate(
    const CommonChannelSettings& settings,
    char* const output,
    const std::size_t outputSize) {
    char integerRate[8]{};
    const char ratePrefix = settings.rate.mode == ClockRatioMode::Multiply ? 'x' : '/';
    std::snprintf(integerRate, sizeof(integerRate), "%c%u", ratePrefix, settings.rate.factor);

    if (settings.rate.numerator == 1U && settings.rate.denominator == 1U) {
        std::snprintf(output, outputSize, "%s", integerRate);
    } else if (settings.rate.factor == 1U && settings.rate.mode == ClockRatioMode::Multiply) {
        std::snprintf(
            output,
            outputSize,
            "%u:%u",
            settings.rate.numerator,
            settings.rate.denominator);
    } else {
        std::snprintf(
            output,
            outputSize,
            "%s %u:%u",
            integerRate,
            settings.rate.numerator,
            settings.rate.denominator);
    }
}

void formatChannelDetail(
    const ChannelConfig& channel,
    char* const output,
    const std::size_t outputSize) {
    if (channel.common.mode == ChannelMode::Off) {
        if (outputSize > 0U) {
            output[0] = '\0';
        }
    } else if (channel.common.mode == ChannelMode::Clock) {
        formatRate(channel.common, output, outputSize);
    } else if (channel.common.mode == ChannelMode::Euclid) {
        std::snprintf(
            output,
            outputSize,
            text::get(text::TextId::EuclidDetailFormat),
            channel.euclid.hits,
            channel.euclid.steps,
            channel.euclid.rotation);
    } else {
        std::snprintf(
            output,
            outputSize,
            text::get(text::TextId::SequencerDetailFormat),
            channel.sequencer.length);
    }
}

void formatChannelSummary(
    const ChannelConfig& channel,
    char* const output,
    const std::size_t outputSize) {
    if (channel.common.mode == ChannelMode::Off) {
        std::snprintf(output, outputSize, "%s", text::get(text::TextId::Off));
    } else if (channel.common.mode == ChannelMode::Clock) {
        formatRate(channel.common, output, outputSize);
    } else if (channel.common.mode == ChannelMode::Euclid) {
        std::snprintf(
            output,
            outputSize,
            text::get(text::TextId::EuclidSummaryFormat),
            channel.euclid.hits,
            channel.euclid.steps);
    } else {
        std::snprintf(output, outputSize, "%u", channel.sequencer.length);
    }
}

}  // namespace clockfw::ui
