/**
 * @file mode_functions.cpp
 * @brief Mapping implementation for the graphical six-function mode palette.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/mode_functions.h"

#include <cstddef>

namespace clockfw::ui {
namespace {

/** Maps a per-channel generator mode onto its palette action. */
ModeFunction channelModeFunction(const ChannelMode mode) {
    switch (mode) {
        case ChannelMode::Off: return ModeFunction::Off;
        case ChannelMode::Euclid: return ModeFunction::Euclid;
        case ChannelMode::Sequencer: return ModeFunction::Sequencer;
        case ChannelMode::Clock:
        default: return ModeFunction::Clock;
    }
}

}  // namespace

std::uint8_t modeFunctionIndexForState(
    const ClockState& state,
    const std::uint8_t selectedChannel) {
    ModeFunction activeFunction = ModeFunction::Clock;
    if (state.operatingMode == OperatingMode::UnifiedClock) {
        activeFunction = ModeFunction::UnifiedClock;
    } else if (state.operatingMode == OperatingMode::DividerBank) {
        activeFunction = ModeFunction::DividerBank;
    } else if (selectedChannel < kChannelCount) {
        activeFunction = channelModeFunction(state.channels[selectedChannel].common.mode);
    }

    for (std::size_t index = 0U; index < kModeFunctions.size(); ++index) {
        if (kModeFunctions[index] == activeFunction) {
            return static_cast<std::uint8_t>(index);
        }
    }
    return 2U;
}

void applyModeFunction(
    ClockState& state,
    const std::uint8_t selectedChannel,
    const ModeFunction modeFunction) {
    if (modeFunction == ModeFunction::UnifiedClock) {
        state.operatingMode = OperatingMode::UnifiedClock;
        return;
    }
    if (modeFunction == ModeFunction::DividerBank) {
        state.operatingMode = OperatingMode::DividerBank;
        return;
    }

    state.operatingMode = OperatingMode::Independent;
    if (selectedChannel >= kChannelCount) {
        return;
    }

    ChannelMode channelMode = ChannelMode::Clock;
    switch (modeFunction) {
        case ModeFunction::Off: channelMode = ChannelMode::Off; break;
        case ModeFunction::Euclid: channelMode = ChannelMode::Euclid; break;
        case ModeFunction::Sequencer: channelMode = ChannelMode::Sequencer; break;
        case ModeFunction::Clock:
        case ModeFunction::UnifiedClock:
        case ModeFunction::DividerBank:
        default: channelMode = ChannelMode::Clock; break;
    }
    state.channels[selectedChannel].common.mode = channelMode;
}

}  // namespace clockfw::ui
