/**
 * @file template_service.cpp
 * @brief Factory template catalog and deterministic template application logic.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/template_service.h"

#include "domain/default_configuration.h"
#include "ui_text.h"

namespace clockfw::services {
namespace {

/** Localized names displayed by the template selection screen. */
constexpr text::TextId kTemplateNameIds[TemplateService::kTemplateCount] = {
    text::TextId::TemplateAllMaster,
    text::TextId::TemplateClockTree,
    text::TextId::TemplateDividers,
    text::TextId::TemplatePolyrhythm,
    text::TextId::TemplateEuclidKit,
    text::TextId::TemplateHybrid
};

}  // namespace

const char* TemplateService::name(const std::size_t templateIndex) {
    return templateIndex < kTemplateCount
        ? text::get(kTemplateNameIds[templateIndex])
        : text::get(text::TextId::Off);
}

bool TemplateService::apply(const std::size_t templateIndex, ClockState& state) {
    if (templateIndex >= kTemplateCount) {
        return false;
    }

    const TransportState liveTransport = state.transport;
    initializeFactoryDefaults(state);
    state.transport = liveTransport;

    // ALL MASTER intentionally inherits the shipping ONE CLOCK factory mode.
    // Every other factory template describes per-channel behavior and must
    // therefore opt back into Independent mode explicitly.
    if (templateIndex != 0U) {
        state.operatingMode = OperatingMode::Independent;
    }

    if (templateIndex == 1U) {
        constexpr ClockRatioMode modes[kChannelCount] = {
            ClockRatioMode::Multiply, ClockRatioMode::Divide, ClockRatioMode::Divide,
            ClockRatioMode::Divide, ClockRatioMode::Multiply, ClockRatioMode::Multiply,
            ClockRatioMode::Multiply, ClockRatioMode::Multiply
        };
        constexpr std::uint8_t factors[kChannelCount] = {1U, 2U, 4U, 8U, 2U, 4U, 8U, 16U};
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            state.channels[channelIndex].common.rate.mode = modes[channelIndex];
            state.channels[channelIndex].common.rate.factor = factors[channelIndex];
        }
    } else if (templateIndex == 2U) {
        constexpr std::uint8_t divisors[kChannelCount] = {1U, 2U, 3U, 4U, 6U, 8U, 12U, 16U};
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            state.channels[channelIndex].common.rate.mode =
                channelIndex == 0U ? ClockRatioMode::Multiply : ClockRatioMode::Divide;
            state.channels[channelIndex].common.rate.factor = divisors[channelIndex];
        }
    } else if (templateIndex == 3U) {
        constexpr std::uint8_t numerators[kChannelCount] = {1U, 2U, 3U, 3U, 4U, 4U, 5U, 5U};
        constexpr std::uint8_t denominators[kChannelCount] = {1U, 3U, 2U, 4U, 3U, 5U, 4U, 8U};
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            state.channels[channelIndex].common.rate.numerator = numerators[channelIndex];
            state.channels[channelIndex].common.rate.denominator = denominators[channelIndex];
        }
    } else if (templateIndex == 4U) {
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            ChannelConfig& channel = state.channels[channelIndex];
            channel.common.mode = ChannelMode::Euclid;
            channel.euclid.steps = 16U;
            channel.euclid.hits = static_cast<std::uint8_t>(3U + channelIndex % 6U);
            channel.euclid.rotation = static_cast<std::uint8_t>(channelIndex);
        }
    } else if (templateIndex == 5U) {
        for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
            state.channels[channelIndex].common.mode = channelIndex < 3U
                ? ChannelMode::Clock
                : (channelIndex < 6U ? ChannelMode::Euclid : ChannelMode::Sequencer);
        }
        state.channels[1U].common.rate = {ClockRatioMode::Divide, 2U, 1U, 1U};
        state.channels[2U].common.rate = {ClockRatioMode::Multiply, 1U, 3U, 2U};
    }

    return true;
}

}  // namespace clockfw::services
