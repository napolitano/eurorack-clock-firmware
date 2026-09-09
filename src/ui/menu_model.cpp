/**
 * @file menu_model.cpp
 * @brief Static settings-page metadata and localized row formatting.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/menu_model.h"

#include <cstdio>

#include "config.h"
#include "domain/clock_labels.h"
#include "domain/clock_options.h"
#include "ui/text_formatter.h"
#include "ui/menu_model_sync.h"
#include "ui_text.h"
#include "version.h"

namespace clockfw::ui {
namespace {

/** Copies one localized static string into a bounded menu field. */
void copyText(char* const destination, const std::size_t size, const text::TextId id) {
    std::snprintf(destination, size, "%s", text::get(id));
}

}  // namespace

const char* settingsPageTitle(const SettingsPage page) {
    switch (page) {
        case SettingsPage::General: return text::get(text::TextId::GeneralSettings);
        case SettingsPage::Master: return text::get(text::TextId::ModeClockLong);
        case SettingsPage::Sync: return text::get(text::TextId::Sync);
        case SettingsPage::Preferences: return text::get(text::TextId::Presets);
        case SettingsPage::Screensaver: return text::get(text::TextId::Screensaver);
        case SettingsPage::Info: return text::get(text::TextId::Info);
        case SettingsPage::Licenses: return text::get(text::TextId::Licenses);
        case SettingsPage::Updates: return text::get(text::TextId::Updates);
        case SettingsPage::Channel: return text::get(text::TextId::Channel);
        case SettingsPage::Rate: return text::get(text::TextId::Rate);
        case SettingsPage::Clock: return text::get(text::TextId::ModeClockLong);
        case SettingsPage::Euclid: return text::get(text::TextId::ModeEuclidLong);
        case SettingsPage::Sequencer: return text::get(text::TextId::ModeSequencerLong);
        case SettingsPage::UnifiedClock: return text::get(text::TextId::ModeUnifiedLong);
        case SettingsPage::DividerBank: return text::get(text::TextId::ModeDividerLong);
        case SettingsPage::Root:
        default: return text::get(text::TextId::RootSettings);
    }
}

std::uint8_t settingsPageItemCount(
    const SettingsPage page,
    const ChannelMode mode) {
    switch (page) {
        case SettingsPage::Root: return 5U;
        case SettingsPage::General: return 3U;
        case SettingsPage::Master: return 5U;
        case SettingsPage::Sync: return 7U;
        case SettingsPage::Preferences: return 4U;
        case SettingsPage::Screensaver: return 4U;
        case SettingsPage::Info: return 5U;
        case SettingsPage::Licenses: return 7U;
        case SettingsPage::Updates: return 1U;
        case SettingsPage::Channel: return mode == ChannelMode::Off ? 1U : 9U;
        case SettingsPage::Rate: return 3U;
        case SettingsPage::Clock: return 2U;
        case SettingsPage::Euclid: return 3U;
        case SettingsPage::Sequencer: return 8U;
        case SettingsPage::UnifiedClock: return 8U;
        case SettingsPage::DividerBank: return 3U;
        default: return 0U;
    }
}

bool isChannelSettingsPage(const SettingsPage page) {
    return page == SettingsPage::Channel ||
           page == SettingsPage::Rate ||
           page == SettingsPage::Clock ||
           page == SettingsPage::Euclid ||
           page == SettingsPage::Sequencer;
}

MenuRow buildMenuRow(
    const SettingsPage page,
    const std::uint8_t rowIndex,
    const std::uint8_t selectedChannel,
    const ClockState& state) {
    MenuRow row{};
    const ChannelConfig& channel = state.channels[selectedChannel];

    if (page == SettingsPage::Root) {
        constexpr text::TextId kLabels[] = {
            text::TextId::GeneralSettings,
            text::TextId::ChannelSettings,
            text::TextId::Presets,
            text::TextId::Info,
            text::TextId::Reset};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex < 4U) {
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
        }
    } else if (page == SettingsPage::General) {
        constexpr text::TextId kLabels[] = {
            text::TextId::ModeClockLong,
            text::TextId::Sync,
            text::TextId::Screensaver};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        copyText(row.value, sizeof(row.value), text::TextId::Arrow);
    } else if (page == SettingsPage::Master) {
        if (rowIndex == 0U) {
            copyText(row.label, sizeof(row.label), text::TextId::Tempo);
            std::snprintf(row.value, sizeof(row.value), "%u", state.bpm);
        } else if (rowIndex == 1U) {
            copyText(row.label, sizeof(row.label), text::TextId::MinimumTempo);
            std::snprintf(row.value, sizeof(row.value), "%u", state.tempoRange.minimumBpm);
        } else if (rowIndex == 2U) {
            copyText(row.label, sizeof(row.label), text::TextId::MaximumTempo);
            std::snprintf(row.value, sizeof(row.value), "%u", state.tempoRange.maximumBpm);
        } else if (rowIndex == 3U) {
            copyText(row.label, sizeof(row.label), text::TextId::MeterBeats);
            std::snprintf(row.value, sizeof(row.value), "%u", state.masterMeter.beats);
        } else {
            copyText(row.label, sizeof(row.label), text::TextId::MeterUnit);
            std::snprintf(row.value, sizeof(row.value), "%u", state.masterMeter.unit);
        }
    } else if (page == SettingsPage::Sync) {
        row = buildSyncMenuRow(rowIndex, state);
    } else if (page == SettingsPage::Preferences) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Current,
            text::TextId::LoadPreset,
            text::TextId::SavePreset,
            text::TextId::Templates};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            copyText(row.value, sizeof(row.value), text::TextId::AutoSave);
        } else {
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
        }
    } else if (page == SettingsPage::Screensaver) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Mode,
            text::TextId::StartAfter,
            text::TextId::DimAfter,
            text::TextId::OffAfter};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            text::TextId modeText = text::TextId::ScreensaverNone;
            switch (state.display.screensaverMode) {
                case ScreensaverMode::Fractal:
                    modeText = text::TextId::ScreensaverFractal;
                    break;
                case ScreensaverMode::Orbit:
                    modeText = text::TextId::ScreensaverOrbit;
                    break;
                case ScreensaverMode::Plug:
                    modeText = text::TextId::ScreensaverPlug;
                    break;
                case ScreensaverMode::Clock:
                    modeText = text::TextId::ScreensaverClock;
                    break;
                case ScreensaverMode::Heartbeat:
                    modeText = text::TextId::ScreensaverHeartbeat;
                    break;
                case ScreensaverMode::Acid:
                    modeText = text::TextId::ScreensaverAcid;
                    break;
                case ScreensaverMode::Spectrum:
                    modeText = text::TextId::ScreensaverSpectrum;
                    break;
                case ScreensaverMode::Field:
                    modeText = text::TextId::ScreensaverField;
                    break;
                case ScreensaverMode::Blox:
                    modeText = text::TextId::ScreensaverBlox;
                    break;
                case ScreensaverMode::Matrix:
                    modeText = text::TextId::ScreensaverMatrix;
                    break;
                case ScreensaverMode::CubeCover:
                    modeText = text::TextId::ScreensaverCubeCover;
                    break;
                case ScreensaverMode::None:
                default:
                    break;
            }
            copyText(row.value, sizeof(row.value), modeText);
        } else {
            const std::uint8_t minutes = rowIndex == 1U
                ? state.display.screensaverAfterMinutes
                : (rowIndex == 2U ? state.display.dimAfterMinutes : state.display.offAfterMinutes);
            std::snprintf(
                row.value,
                sizeof(row.value),
                text::get(text::TextId::MinutesFormat),
                static_cast<unsigned>(minutes));
        }
    } else if (page == SettingsPage::Info) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Product,
            text::TextId::Firmware,
            text::TextId::Author,
            text::TextId::Licenses,
            text::TextId::Updates};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            copyText(row.value, sizeof(row.value), text::TextId::FirmwareName);
        } else if (rowIndex == 1U) {
            std::snprintf(row.value, sizeof(row.value), "%s", CLOCK_FIRMWARE_VERSION);
        } else if (rowIndex == 2U) {
            copyText(row.value, sizeof(row.value), text::TextId::AuthorName);
        } else {
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
        }
    } else if (page == SettingsPage::Licenses) {
        constexpr text::TextId kLabels[] = {
            text::TextId::License,
            text::TextId::Framework,
            text::TextId::CoreLicense,
            text::TextId::HalLicense,
            text::TextId::CmsisLicense,
            text::TextId::Font,
            text::TextId::FontLicense};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            copyText(row.value, sizeof(row.value), text::TextId::FirmwareLicenseShort);
        } else if (rowIndex == 1U) {
            copyText(row.value, sizeof(row.value), text::TextId::FrameworkName);
        } else if (rowIndex == 2U) {
            copyText(row.value, sizeof(row.value), text::TextId::LgplShort);
        } else if (rowIndex == 3U) {
            copyText(row.value, sizeof(row.value), text::TextId::BsdShort);
        } else if (rowIndex == 4U) {
            copyText(row.value, sizeof(row.value), text::TextId::ApacheShort);
        } else if (rowIndex == 5U) {
            copyText(row.value, sizeof(row.value), text::TextId::FontName);
        } else {
            copyText(row.value, sizeof(row.value), text::TextId::ApacheShort);
        }
    } else if (page == SettingsPage::Updates) {
        copyText(row.label, sizeof(row.label), text::TextId::Updates);
    } else if (page == SettingsPage::Channel) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Mode,
            text::TextId::Rate,
            text::TextId::ModeConfig,
            text::TextId::Swing,
            text::TextId::Probability,
            text::TextId::Gate,
            text::TextId::Phase,
            text::TextId::Reset,
            text::TextId::Mute};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            std::snprintf(row.value, sizeof(row.value), "%s", channelModeLongLabel(channel.common.mode));
        } else if (rowIndex == 1U) {
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
        } else if (rowIndex == 2U) {
            std::snprintf(row.label, sizeof(row.label), "%s", channelModeLongLabel(channel.common.mode));
            if (channel.common.mode != ChannelMode::Off) {
                copyText(row.value, sizeof(row.value), text::TextId::Arrow);
            }
        } else if (rowIndex == 3U) {
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::PercentFormat), channel.common.swingPercent);
        } else if (rowIndex == 4U) {
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::PercentFormat), channel.common.probabilityPercent);
        } else if (rowIndex == 5U) {
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::MillisecondsFormat), channel.common.gateLengthMs);
        } else if (rowIndex == 6U) {
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::PercentFormat), channel.common.phasePercent);
        } else if (rowIndex == 7U) {
            copyText(
                row.value,
                sizeof(row.value),
                channel.common.resetMode == ResetMode::Global ? text::TextId::Global : text::TextId::SyncFree);
        } else {
            copyText(row.value, sizeof(row.value), channel.common.muted ? text::TextId::On : text::TextId::Off);
        }
    } else if (page == SettingsPage::Rate) {
        if (rowIndex == 0U) {
            copyText(row.label, sizeof(row.label), text::TextId::DivideMultiply);
            formatRate(channel.common, row.value, sizeof(row.value));
        } else if (rowIndex == 1U) {
            copyText(row.label, sizeof(row.label), text::TextId::PolyNumerator);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.common.rate.numerator);
        } else {
            copyText(row.label, sizeof(row.label), text::TextId::PolyDenominator);
            std::snprintf(row.value, sizeof(row.value), "%u", channel.common.rate.denominator);
        }
    } else if (page == SettingsPage::Clock) {
        copyText(
            row.label,
            sizeof(row.label),
            rowIndex == 0U ? text::TextId::MeterBeats : text::TextId::MeterUnit);
        std::snprintf(
            row.value,
            sizeof(row.value),
            "%u",
            rowIndex == 0U ? channel.clock.meter.beats : channel.clock.meter.unit);
    } else if (page == SettingsPage::Euclid) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Steps,
            text::TextId::Hits,
            text::TextId::Rotate};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        const std::uint8_t value = rowIndex == 0U
            ? channel.euclid.steps
            : (rowIndex == 1U ? channel.euclid.hits : channel.euclid.rotation);
        std::snprintf(row.value, sizeof(row.value), "%u", value);
    } else if (page == SettingsPage::Sequencer) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Editor,
            text::TextId::Length,
            text::TextId::Rotate,
            text::TextId::Invert,
            text::TextId::Clear,
            text::TextId::FillAlternate,
            text::TextId::Copy,
            text::TextId::Paste};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
        } else if (rowIndex == 1U) {
            std::snprintf(row.value, sizeof(row.value), "%u", channel.sequencer.length);
        } else if (rowIndex == 2U) {
            std::snprintf(row.value, sizeof(row.value), "%u", channel.sequencer.rotation);
        }
    } else if (page == SettingsPage::UnifiedClock) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Mode,
            text::TextId::DivideMultiply,
            text::TextId::PolyNumerator,
            text::TextId::PolyDenominator,
            text::TextId::Swing,
            text::TextId::Gate,
            text::TextId::Phase,
            text::TextId::Humanize};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            copyText(row.value, sizeof(row.value), text::TextId::ModeUnifiedLong);
        } else if (rowIndex == 1U) {
            CommonChannelSettings temporary{};
            temporary.rate = state.unifiedClock.rate;
            formatRate(temporary, row.value, sizeof(row.value));
        } else if (rowIndex == 2U) {
            std::snprintf(row.value, sizeof(row.value), "%u", state.unifiedClock.rate.numerator);
        } else if (rowIndex == 3U) {
            std::snprintf(row.value, sizeof(row.value), "%u", state.unifiedClock.rate.denominator);
        } else if (rowIndex == 4U) {
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::PercentFormat), state.unifiedClock.swingPercent);
        } else if (rowIndex == 5U) {
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::MillisecondsFormat), state.unifiedClock.gateLengthMs);
        } else if (rowIndex == 6U) {
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::PercentFormat), state.unifiedClock.phasePercent);
        } else if (state.unifiedClock.humanizeUs == 0U) {
            copyText(row.value, sizeof(row.value), text::TextId::Off);
        } else {
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::MicrosecondsFormat), state.unifiedClock.humanizeUs);
        }
    } else if (page == SettingsPage::DividerBank) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Mode,
            text::TextId::Bank,
            text::TextId::Gate};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            copyText(row.value, sizeof(row.value), text::TextId::ModeDividerLong);
        } else if (rowIndex == 1U) {
            const text::TextId bankText = state.dividerBank.bank == DividerBank::PowersOfTwo
                ? text::TextId::BankPowers
                : (state.dividerBank.bank == DividerBank::Integers
                    ? text::TextId::BankIntegers
                    : text::TextId::BankPrimes);
            copyText(row.value, sizeof(row.value), bankText);
        } else {
            std::snprintf(row.value, sizeof(row.value), text::get(text::TextId::MillisecondsFormat), state.dividerBank.gateLengthMs);
        }
    }

    return row;
}

std::size_t findRateOptionIndex(const CommonChannelSettings& settings) {
    for (std::size_t index = 0U; index < kRateOptions.size(); ++index) {
        if (kRateOptions[index].mode == settings.rate.mode &&
            kRateOptions[index].factor == settings.rate.factor) {
            return index;
        }
    }
    return 9U;
}

std::size_t findGateLengthOptionIndex(const std::uint16_t gateLengthMs) {
    for (std::size_t index = 0U; index < kGateLengthOptionsMs.size(); ++index) {
        if (kGateLengthOptionsMs[index] == gateLengthMs) {
            return index;
        }
    }
    return 3U;
}

std::size_t findBeatUnitOptionIndex(const std::uint8_t beatUnit) {
    for (std::size_t index = 0U; index < kBeatUnitOptions.size(); ++index) {
        if (kBeatUnitOptions[index] == beatUnit) {
            return index;
        }
    }
    return 1U;
}

}  // namespace clockfw::ui
