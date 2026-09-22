/**
 * @file menu_model.cpp
 * @brief Static settings-page metadata and localized row formatting.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/menu_model.h"
#include "ui/menu_model_channel.h"

#include <cstdio>

#include "config.h"
#include "domain/clock_labels.h"
#include "domain/clock_options.h"
#include "ui/text_formatter.h"
#include "ui/menu_model_sync.h"
#include "ui/menu_model_groove.h"
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
        case SettingsPage::InputAssignments: return text::get(text::TextId::Inputs);
        case SettingsPage::Hardware: return text::get(text::TextId::Hardware);
        case SettingsPage::Diagnostics: return text::get(text::TextId::Diagnostics);
        case SettingsPage::DiagnosticsInputs: return text::get(text::TextId::Inputs);
        case SettingsPage::DiagnosticsOutputs: return text::get(text::TextId::Outputs);
        case SettingsPage::Master: return text::get(text::TextId::ModeClockLong);
        case SettingsPage::Sync: return text::get(text::TextId::InputConfig);
        case SettingsPage::Preferences: return text::get(text::TextId::Presets);
        case SettingsPage::Screensaver: return text::get(text::TextId::Screensaver);
        case SettingsPage::Info: return text::get(text::TextId::Info);
        case SettingsPage::Licenses: return text::get(text::TextId::Licenses);
        case SettingsPage::Updates: return text::get(text::TextId::Updates);
        case SettingsPage::Channel: return text::get(text::TextId::Channel);
        case SettingsPage::ChannelTiming:
        case SettingsPage::UnifiedTiming: return text::get(text::TextId::Timing);
        case SettingsPage::ChannelOutput:
        case SettingsPage::UnifiedOutput: return text::get(text::TextId::Output);
        case SettingsPage::Clock: return text::get(text::TextId::ModeClockLong);
        case SettingsPage::Euclid: return text::get(text::TextId::ModeEuclidLong);
        case SettingsPage::Sequencer: return text::get(text::TextId::ModeSequencerLong);
        case SettingsPage::SequencerPattern: return text::get(text::TextId::Pattern);
        case SettingsPage::UnifiedClock: return text::get(text::TextId::ModeUnifiedLong);
        case SettingsPage::Groove: return text::get(text::TextId::Groove);
        case SettingsPage::GrooveEditorMenu: return text::get(text::TextId::GrooveMenu);
        case SettingsPage::DividerBank: return text::get(text::TextId::ModeDividerLong);
        case SettingsPage::Root:
        default: return text::get(text::TextId::RootSettings);
    }
}

std::uint8_t settingsPageItemCount(
    const SettingsPage page,
    const ChannelMode mode,
    const bool highScoreResetAvailable) {
    switch (page) {
        case SettingsPage::Root: return highScoreResetAvailable ? 6U : 5U;
        case SettingsPage::General: return 5U;
        case SettingsPage::InputAssignments: return 3U;
        case SettingsPage::Hardware: return 2U;
        case SettingsPage::Diagnostics: return 2U;
        case SettingsPage::DiagnosticsInputs:
        case SettingsPage::DiagnosticsOutputs: return 0U;
        case SettingsPage::Master: return 6U;
        case SettingsPage::Sync: return 8U;
        case SettingsPage::Preferences: return 4U;
        case SettingsPage::Screensaver: return 4U;
        case SettingsPage::Info: return 6U;
        case SettingsPage::Licenses: return 7U;
        case SettingsPage::Updates: return 1U;
        case SettingsPage::Channel: return channelMenuItemCount(mode);
        case SettingsPage::ChannelTiming: return 5U;
        case SettingsPage::ChannelOutput: return 5U;
        case SettingsPage::Clock: return 2U;
        case SettingsPage::Euclid: return 3U;
        case SettingsPage::Sequencer: return 3U;
        case SettingsPage::SequencerPattern: return 6U;
        case SettingsPage::UnifiedClock: return 3U;
        case SettingsPage::UnifiedTiming: return 6U;
        case SettingsPage::UnifiedOutput: return 2U;
        case SettingsPage::Groove: return 7U;
        case SettingsPage::GrooveEditorMenu: return 4U;
        case SettingsPage::DividerBank: return 3U;
        default: return 0U;
    }
}

bool isChannelSettingsPage(const SettingsPage page) {
    return page == SettingsPage::Channel ||
           page == SettingsPage::ChannelTiming ||
           page == SettingsPage::ChannelOutput ||
           page == SettingsPage::Clock ||
           page == SettingsPage::Euclid ||
           page == SettingsPage::Sequencer ||
           page == SettingsPage::SequencerPattern;
}

MenuRow buildMenuRow(
    const SettingsPage page,
    const std::uint8_t rowIndex,
    const std::uint8_t selectedChannel,
    const ClockState& state,
    const bool highScoreResetAvailable) {
    MenuRow row{};
    const ChannelConfig& channel = state.channels[selectedChannel];

    if (page == SettingsPage::Root) {
        if (rowIndex < 4U) {
            constexpr text::TextId kLabels[] = {
                text::TextId::GeneralSettings,
                text::TextId::ChannelSettings,
                text::TextId::Presets,
                text::TextId::Info};
            copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
        } else if (highScoreResetAvailable && rowIndex == 4U) {
            copyText(row.label, sizeof(row.label), text::TextId::HighScores);
            copyText(row.value, sizeof(row.value), text::TextId::Clear);
        } else {
            copyText(row.label, sizeof(row.label), text::TextId::PhaseReset);
        }
    } else if (page == SettingsPage::General) {
        constexpr text::TextId kLabels[] = {
            text::TextId::ModeClockLong,
            text::TextId::Inputs,
            text::TextId::Screensaver,
            text::TextId::Diagnostics,
            text::TextId::Hardware};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        copyText(row.value, sizeof(row.value), text::TextId::Arrow);
    } else if (page == SettingsPage::InputAssignments) {
        if (rowIndex == 0U) {
            copyText(row.label, sizeof(row.label), text::TextId::Input1);
            std::snprintf(row.value, sizeof(row.value), "%s", inputFunctionLabel(state.inputs.input1));
        } else if (rowIndex == 1U) {
            copyText(row.label, sizeof(row.label), text::TextId::Input2);
            std::snprintf(row.value, sizeof(row.value), "%s", inputFunctionLabel(state.inputs.input2));
        } else {
            copyText(row.label, sizeof(row.label), text::TextId::InputConfig);
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
        }
    } else if (page == SettingsPage::Hardware) {
        if (rowIndex == 0U) {
            copyText(row.label, sizeof(row.label), text::TextId::EncoderDirection);
            copyText(
                row.value,
                sizeof(row.value),
                state.device.encoderDirectionReversed ? text::TextId::Reversed : text::TextId::Normal);
        } else {
            copyText(row.label, sizeof(row.label), text::TextId::Orientation);
            copyText(
                row.value,
                sizeof(row.value),
                state.device.displayRotated180 ? text::TextId::Degrees180 : text::TextId::Degrees0);
        }
    } else if (page == SettingsPage::Diagnostics) {
        constexpr text::TextId kLabels[] = {text::TextId::Inputs, text::TextId::Outputs};
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
        } else if (rowIndex == 4U) {
            copyText(row.label, sizeof(row.label), text::TextId::MeterUnit);
            std::snprintf(row.value, sizeof(row.value), "%u", state.masterMeter.unit);
        } else {
            copyText(row.label, sizeof(row.label), text::TextId::PreCount);
            if (state.preCountSteps == 0U) {
                copyText(row.value, sizeof(row.value), text::TextId::Off);
            } else {
                std::snprintf(row.value, sizeof(row.value), "%u", state.preCountSteps);
            }
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
    } else if (page == SettingsPage::GrooveEditorMenu) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Save, text::TextId::Load, text::TextId::Zoom, text::TextId::Length};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex < 2U) {
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
                case ScreensaverMode::Fractal: modeText = text::TextId::ScreensaverFractal; break;
                case ScreensaverMode::Orbit: modeText = text::TextId::ScreensaverOrbit; break;
                case ScreensaverMode::Plug: modeText = text::TextId::ScreensaverPlug; break;
                case ScreensaverMode::Clock: modeText = text::TextId::ScreensaverClock; break;
                case ScreensaverMode::Heartbeat: modeText = text::TextId::ScreensaverHeartbeat; break;
                case ScreensaverMode::Acid: modeText = text::TextId::ScreensaverAcid; break;
                case ScreensaverMode::Spectrum: modeText = text::TextId::ScreensaverSpectrum; break;
                case ScreensaverMode::Field: modeText = text::TextId::ScreensaverField; break;
                case ScreensaverMode::Blox: modeText = text::TextId::ScreensaverBlox; break;
                case ScreensaverMode::Matrix: modeText = text::TextId::ScreensaverMatrix; break;
                case ScreensaverMode::CubeCover: modeText = text::TextId::ScreensaverCubeCover; break;
                case ScreensaverMode::MakeMusic: modeText = text::TextId::ScreensaverMakeMusic; break;
                case ScreensaverMode::Labyrinth: modeText = text::TextId::ScreensaverLabyrinth; break;
                case ScreensaverMode::Starfield: modeText = text::TextId::ScreensaverStarfield; break;
                case ScreensaverMode::Fireworks: modeText = text::TextId::ScreensaverFireworks; break;
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
            text::TextId::Updates,
            text::TextId::FactoryReset};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            copyText(row.value, sizeof(row.value), text::TextId::FirmwareName);
            row.expandableInformation = true;
        } else if (rowIndex == 1U) {
            std::snprintf(row.value, sizeof(row.value), "%s", CLOCK_FIRMWARE_VERSION);
            row.expandableInformation = true;
        } else if (rowIndex == 2U) {
            copyText(row.value, sizeof(row.value), text::TextId::AuthorName);
            row.expandableInformation = true;
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
            copyText(row.value, sizeof(row.value), text::TextId::CubeLicenseShort);
        } else if (rowIndex == 3U) {
            copyText(row.value, sizeof(row.value), text::TextId::BsdShort);
        } else if (rowIndex == 4U) {
            copyText(row.value, sizeof(row.value), text::TextId::ApacheShort);
        } else if (rowIndex == 5U) {
            copyText(row.value, sizeof(row.value), text::TextId::FontName);
        } else {
            copyText(row.value, sizeof(row.value), text::TextId::ApacheShort);
        }
        row.expandableInformation = true;
    } else if (page == SettingsPage::Updates) {
        copyText(row.label, sizeof(row.label), text::TextId::Updates);
    } else if (page == SettingsPage::Channel) {
        row = buildChannelMenuRow(rowIndex, channel);
    } else if (page == SettingsPage::ChannelTiming) {
        row = buildChannelTimingMenuRow(rowIndex, channel);
    } else if (page == SettingsPage::ChannelOutput) {
        row = buildChannelOutputMenuRow(rowIndex, channel);
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
            text::TextId::Length,
            text::TextId::Rotate,
            text::TextId::Pattern};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            std::snprintf(row.value, sizeof(row.value), "%u", channel.sequencer.length);
        } else if (rowIndex == 1U) {
            std::snprintf(row.value, sizeof(row.value), "%u", channel.sequencer.rotation);
        } else {
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
        }
    } else if (page == SettingsPage::SequencerPattern) {
        constexpr text::TextId kLabels[] = {
            text::TextId::Editor,
            text::TextId::Invert,
            text::TextId::Clear,
            text::TextId::FillAlternate,
            text::TextId::Copy,
            text::TextId::Paste};
        copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
        if (rowIndex == 0U) {
            copyText(row.value, sizeof(row.value), text::TextId::Arrow);
        }
    } else if (page == SettingsPage::UnifiedClock) {
        row = buildUnifiedClockMenuRow(rowIndex);
    } else if (page == SettingsPage::UnifiedTiming) {
        row = buildUnifiedTimingMenuRow(rowIndex, state.unifiedClock);
    } else if (page == SettingsPage::UnifiedOutput) {
        row = buildUnifiedOutputMenuRow(rowIndex, state.unifiedClock);
    } else if (page == SettingsPage::Groove) {
        row = buildGrooveMenuRow(rowIndex, selectedChannel, state);
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
