/**
 * @file settings_editor.cpp
 * @brief Validated mutation service for user-editable clock and channel settings.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/settings_editor.h"

#include <algorithm>
#include <array>
#include <cstddef>

#include "clock_core.h"
#include "config.h"
#include "domain/clock_options.h"
#include "ui/menu_model.h"

namespace clockfw::ui {

SettingsEditor::SettingsEditor(ClockState& state, engine::ClockEngine& engine)
    : state_(state), engine_(engine) {}

void SettingsEditor::changeMasterTempo(const std::int8_t delta) {
    const std::uint16_t previousBpm = state_.bpm;
    state_.bpm = static_cast<std::uint16_t>(clampInt(
        static_cast<int>(state_.bpm) + delta,
        state_.tempoRange.minimumBpm,
        state_.tempoRange.maximumBpm));
    if (state_.bpm != previousBpm) {
        engine_.updateMasterTempo(state_.bpm);
    }
}

void SettingsEditor::adjust(
    const SettingsPage page,
    const std::uint8_t rowIndex,
    const std::uint8_t channelIndex,
    const std::int8_t delta) {
    if (isChannelSettingsPage(page) && channelIndex >= kChannelCount) {
        return;
    }

    switch (page) {
        case SettingsPage::Master:
            adjustMaster(rowIndex, delta);
            break;
        case SettingsPage::Sync:
            adjustSync(rowIndex, delta);
            break;
        case SettingsPage::Screensaver:
            adjustScreensaver(rowIndex, delta);
            break;
        case SettingsPage::Channel:
            adjustCommonChannel(channelIndex, rowIndex, delta);
            break;
        case SettingsPage::Rate:
            adjustRate(channelIndex, rowIndex, delta);
            break;
        case SettingsPage::Clock:
            adjustClock(channelIndex, rowIndex, delta);
            break;
        case SettingsPage::Euclid:
            adjustEuclid(channelIndex, rowIndex, delta);
            break;
        case SettingsPage::Sequencer:
            adjustSequencer(channelIndex, rowIndex, delta);
            break;
        case SettingsPage::UnifiedClock:
            adjustUnifiedClock(rowIndex, delta);
            break;
        case SettingsPage::DividerBank:
            adjustDividerBank(rowIndex, delta);
            break;
        case SettingsPage::Root:
        case SettingsPage::General:
        case SettingsPage::Preferences:
        case SettingsPage::Info:
        case SettingsPage::Licenses:
        case SettingsPage::Updates:
        default:
            break;
    }
}
void SettingsEditor::toggleSequencerStep(
    const std::uint8_t channelIndex,
    const std::uint8_t absoluteStep) {
    if (channelIndex >= kChannelCount) {
        return;
    }

    ChannelConfig& channel = state_.channels[channelIndex];
    if (absoluteStep >= channel.sequencer.length) {
        return;
    }

    channel.sequencer.pattern ^= 1ULL << absoluteStep;
    engine_.updateChannel(channelIndex, channel, false);
}

bool SettingsEditor::executeSequencerCommand(
    const std::uint8_t channelIndex,
    const std::uint8_t rowIndex) {
    if (channelIndex >= kChannelCount) {
        return false;
    }

    ChannelConfig& channel = state_.channels[channelIndex];

    if (rowIndex == 3U) {
        channel.sequencer.pattern = core::invertPattern(
            channel.sequencer.pattern,
            channel.sequencer.length);
    } else if (rowIndex == 4U) {
        channel.sequencer.pattern = 0U;
    } else if (rowIndex == 5U) {
        channel.sequencer.pattern = core::alternatingPattern(channel.sequencer.length, true);
    } else if (rowIndex == 6U) {
        sequencerClipboard_ = channel.sequencer.pattern;
        sequencerClipboardValid_ = true;
        return true;
    } else if (rowIndex == 7U && sequencerClipboardValid_) {
        channel.sequencer.pattern = core::clampPattern(
            sequencerClipboard_,
            channel.sequencer.length);
    } else {
        return false;
    }

    engine_.updateChannel(channelIndex, channel, false);
    return true;
}

void SettingsEditor::adjustMaster(const std::uint8_t rowIndex, const std::int8_t delta) {
    if (rowIndex == 0U) {
        changeMasterTempo(delta);
    } else if (rowIndex == 1U) {
        state_.tempoRange.minimumBpm = static_cast<std::uint16_t>(clampInt(
            static_cast<int>(state_.tempoRange.minimumBpm) + delta,
            config::kSupportedMinimumBpm,
            static_cast<int>(state_.tempoRange.maximumBpm)));
        if (state_.bpm < state_.tempoRange.minimumBpm) {
            state_.bpm = state_.tempoRange.minimumBpm;
            engine_.updateMasterTempo(state_.bpm);
        }
    } else if (rowIndex == 2U) {
        state_.tempoRange.maximumBpm = static_cast<std::uint16_t>(clampInt(
            static_cast<int>(state_.tempoRange.maximumBpm) + delta,
            static_cast<int>(state_.tempoRange.minimumBpm),
            config::kSupportedMaximumBpm));
        if (state_.bpm > state_.tempoRange.maximumBpm) {
            state_.bpm = state_.tempoRange.maximumBpm;
            engine_.updateMasterTempo(state_.bpm);
        }
    } else if (rowIndex == 3U) {
        state_.masterMeter.beats = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(state_.masterMeter.beats) + delta, 1, 16));
        engine_.updateConfiguration(state_, false);
    } else if (rowIndex == 4U) {
        const std::size_t currentIndex = findBeatUnitOptionIndex(state_.masterMeter.unit);
        const std::size_t newIndex = static_cast<std::size_t>(clampInt(
            static_cast<int>(currentIndex) + delta,
            0,
            static_cast<int>(kBeatUnitOptions.size() - 1U)));
        state_.masterMeter.unit = kBeatUnitOptions[newIndex];
        engine_.updateConfiguration(state_, true);
    }
}

void SettingsEditor::adjustScreensaver(
    const std::uint8_t rowIndex,
    const std::int8_t delta) {
    DisplayPreferences& display = state_.display;

    if (rowIndex == 0U) {
        // Explicit UI order preserves persisted enum values while keeping OFF first.
        constexpr std::array<ScreensaverMode, 12U> kOrder{{
            ScreensaverMode::None, ScreensaverMode::Clock, ScreensaverMode::Plug,
            ScreensaverMode::Heartbeat, ScreensaverMode::Acid, ScreensaverMode::Spectrum,
            ScreensaverMode::Field, ScreensaverMode::Blox, ScreensaverMode::Matrix,
            ScreensaverMode::CubeCover, ScreensaverMode::Fractal, ScreensaverMode::Orbit}};
        std::size_t current = 0U;
        for (std::size_t index = 0U; index < kOrder.size(); ++index) {
            if (kOrder[index] == display.screensaverMode) { current = index; break; }
        }
        const int next = clampInt(static_cast<int>(current) + delta, 0, static_cast<int>(kOrder.size() - 1U));
        display.screensaverMode = kOrder[static_cast<std::size_t>(next)];
    } else if (rowIndex == 1U) {
        display.screensaverAfterMinutes = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(display.screensaverAfterMinutes) + delta,
            1,
            static_cast<int>(display.dimAfterMinutes)));
    } else if (rowIndex == 2U) {
        display.dimAfterMinutes = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(display.dimAfterMinutes) + delta,
            static_cast<int>(display.screensaverAfterMinutes),
            static_cast<int>(display.offAfterMinutes)));
    } else if (rowIndex == 3U) {
        display.offAfterMinutes = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(display.offAfterMinutes) + delta,
            static_cast<int>(display.dimAfterMinutes),
            static_cast<int>(config::kMaximumScreensaverMinutes)));
    }
}

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

    if (rowIndex == 1U) {
        channel.sequencer.length = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.sequencer.length) + delta, 1, 64));
        if (channel.sequencer.rotation >= channel.sequencer.length) {
            channel.sequencer.rotation = 0U;
        }
    } else if (rowIndex == 2U) {
        channel.sequencer.rotation = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(channel.sequencer.rotation) + delta,
            0,
            channel.sequencer.length - 1));
    }

    engine_.updateChannel(channelIndex, channel, false);
}


void SettingsEditor::adjustUnifiedClock(
    const std::uint8_t rowIndex,
    const std::int8_t delta) {
    UnifiedClockSettings& settings = state_.unifiedClock;
    if (rowIndex == 1U) {
        std::size_t currentIndex = 9U;
        for (std::size_t index = 0U; index < kRateOptions.size(); ++index) {
            if (kRateOptions[index].mode == settings.rate.mode &&
                kRateOptions[index].factor == settings.rate.factor) {
                currentIndex = index;
                break;
            }
        }
        const std::size_t newIndex = static_cast<std::size_t>(clampInt(
            static_cast<int>(currentIndex) + delta,
            0,
            static_cast<int>(kRateOptions.size() - 1U)));
        settings.rate.mode = kRateOptions[newIndex].mode;
        settings.rate.factor = kRateOptions[newIndex].factor;
    } else if (rowIndex == 2U) {
        settings.rate.numerator = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(settings.rate.numerator) + delta, 1, 16));
    } else if (rowIndex == 3U) {
        settings.rate.denominator = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(settings.rate.denominator) + delta, 1, 16));
    } else if (rowIndex == 4U) {
        settings.swingPercent = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(settings.swingPercent) + delta, 0, 50));
    } else if (rowIndex == 5U) {
        const std::size_t currentIndex = findGateLengthOptionIndex(settings.gateLengthMs);
        const std::size_t newIndex = static_cast<std::size_t>(clampInt(
            static_cast<int>(currentIndex) + delta,
            0,
            static_cast<int>(kGateLengthOptionsMs.size() - 1U)));
        settings.gateLengthMs = kGateLengthOptionsMs[newIndex];
    } else if (rowIndex == 6U) {
        settings.phasePercent = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(settings.phasePercent) + delta, 0, 99));
    } else if (rowIndex == 7U) {
        std::size_t currentIndex = 0U;
        for (std::size_t index = 0U; index < kHumanizeOptionsUs.size(); ++index) {
            if (kHumanizeOptionsUs[index] == settings.humanizeUs) {
                currentIndex = index;
                break;
            }
        }
        const std::size_t newIndex = static_cast<std::size_t>(clampInt(
            static_cast<int>(currentIndex) + delta,
            0,
            static_cast<int>(kHumanizeOptionsUs.size() - 1U)));
        settings.humanizeUs = kHumanizeOptionsUs[newIndex];
    } else {
        return;
    }
    engine_.updateConfiguration(state_, true);
}

void SettingsEditor::adjustDividerBank(
    const std::uint8_t rowIndex,
    const std::int8_t delta) {
    if (rowIndex == 1U) {
        state_.dividerBank.bank = static_cast<DividerBank>(clampInt(
            static_cast<int>(state_.dividerBank.bank) + delta, 0, 2));
    } else if (rowIndex == 2U) {
        const std::size_t currentIndex = findGateLengthOptionIndex(state_.dividerBank.gateLengthMs);
        const std::size_t newIndex = static_cast<std::size_t>(clampInt(
            static_cast<int>(currentIndex) + delta,
            0,
            static_cast<int>(kGateLengthOptionsMs.size() - 1U)));
        state_.dividerBank.gateLengthMs = kGateLengthOptionsMs[newIndex];
    } else {
        return;
    }
    engine_.updateConfiguration(state_, true);
}
int SettingsEditor::clampInt(const int value, const int minimum, const int maximum) {
    return std::max(minimum, std::min(value, maximum));
}

}  // namespace clockfw::ui
