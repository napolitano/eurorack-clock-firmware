/**
 * @file settings_editor_channel.cpp
 * @brief Mutation routing for the flat priority-ordered Independent channel menu.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/settings_editor.h"

#include "ui/menu_model_channel.h"

namespace clockfw::ui {

void SettingsEditor::adjustFlatChannel(
    const std::uint8_t channelIndex,
    const std::uint8_t rowIndex,
    const std::int8_t delta) {
    const ChannelMode mode = state_.channels[channelIndex].common.mode;
    switch (channelMenuAction(mode, rowIndex)) {
        case ChannelMenuAction::Rate:
            adjustRate(channelIndex, 0U, delta);
            break;
        case ChannelMenuAction::PolyNumerator:
            adjustRate(channelIndex, 1U, delta);
            break;
        case ChannelMenuAction::PolyDenominator:
            adjustRate(channelIndex, 2U, delta);
            break;
        case ChannelMenuAction::Swing:
            adjustCommonChannel(channelIndex, 3U, delta);
            break;
        case ChannelMenuAction::MeterBeats:
            adjustClock(channelIndex, 0U, delta);
            break;
        case ChannelMenuAction::MeterUnit:
            adjustClock(channelIndex, 1U, delta);
            break;
        case ChannelMenuAction::EuclidSteps:
            adjustEuclid(channelIndex, 0U, delta);
            break;
        case ChannelMenuAction::EuclidHits:
            adjustEuclid(channelIndex, 1U, delta);
            break;
        case ChannelMenuAction::EuclidRotate:
            adjustEuclid(channelIndex, 2U, delta);
            break;
        case ChannelMenuAction::SequencerLength:
            adjustSequencer(channelIndex, 1U, delta);
            break;
        case ChannelMenuAction::SequencerRotate:
            adjustSequencer(channelIndex, 2U, delta);
            break;
        case ChannelMenuAction::Probability:
            adjustCommonChannel(channelIndex, 4U, delta);
            break;
        case ChannelMenuAction::Gate:
            adjustCommonChannel(channelIndex, 5U, delta);
            break;
        case ChannelMenuAction::Phase:
            adjustCommonChannel(channelIndex, 6U, delta);
            break;
        case ChannelMenuAction::Reset:
            adjustCommonChannel(channelIndex, 7U, delta);
            break;
        case ChannelMenuAction::Mute:
            adjustCommonChannel(channelIndex, 8U, delta);
            break;
        case ChannelMenuAction::Mode:
        case ChannelMenuAction::Groove:
        case ChannelMenuAction::SequencerPattern:
        default:
            break;
    }
}

}  // namespace clockfw::ui
