/**
 * @file clock_labels.cpp
 * @brief Localized labels for domain enum values.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "domain/clock_labels.h"

#include "ui_text.h"

namespace clockfw {

const char* channelModeShortLabel(const ChannelMode mode) {
    switch (mode) {
        case ChannelMode::Off: return text::get(text::TextId::ModeOffShort);
        case ChannelMode::Euclid: return text::get(text::TextId::ModeEuclidShort);
        case ChannelMode::Sequencer: return text::get(text::TextId::ModeSequencerShort);
        case ChannelMode::Clock:
        default: return text::get(text::TextId::ModeClockShort);
    }
}

const char* channelModeLongLabel(const ChannelMode mode) {
    switch (mode) {
        case ChannelMode::Off: return text::get(text::TextId::ModeOffLong);
        case ChannelMode::Euclid: return text::get(text::TextId::ModeEuclidLong);
        case ChannelMode::Sequencer: return text::get(text::TextId::ModeSequencerLong);
        case ChannelMode::Clock:
        default: return text::get(text::TextId::ModeClockLong);
    }
}

const char* clockSourceLabel(const ClockSource source) {
    switch (source) {
        case ClockSource::External: return text::get(text::TextId::SourceExternal);
        case ClockSource::Auto: return text::get(text::TextId::SourceAuto);
        case ClockSource::Internal:
        default: return text::get(text::TextId::SourceInternal);
    }
}

}  // namespace clockfw
