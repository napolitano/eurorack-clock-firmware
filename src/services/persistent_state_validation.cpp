/**
 * @file persistent_state_validation.cpp
 * @brief Semantic validation for persisted ClockState payloads.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_validation.h"

#include "config.h"

namespace clockfw::services {

bool isPersistentStateValid(const ClockState& state) {
    if (state.tempoRange.minimumBpm < config::kSupportedMinimumBpm ||
        state.tempoRange.maximumBpm > config::kSupportedMaximumBpm ||
        state.tempoRange.minimumBpm > state.tempoRange.maximumBpm ||
        state.bpm < state.tempoRange.minimumBpm || state.bpm > state.tempoRange.maximumBpm ||
        state.masterMeter.beats < 1U || state.masterMeter.beats > 16U ||
        (state.masterMeter.unit != 1U && state.masterMeter.unit != 2U &&
         state.masterMeter.unit != 4U && state.masterMeter.unit != 8U &&
         state.masterMeter.unit != 16U) ||
        static_cast<std::uint8_t>(state.transport) > static_cast<std::uint8_t>(TransportState::Playing) ||
        static_cast<std::uint8_t>(state.source) > static_cast<std::uint8_t>(ClockSource::Auto) ||
        static_cast<std::uint8_t>(state.operatingMode) > static_cast<std::uint8_t>(OperatingMode::DividerBank) ||
        state.externalSync.pulsesPerQuarterNote == 0U ||
        static_cast<std::uint8_t>(state.externalSync.edge) > static_cast<std::uint8_t>(SyncEdge::Falling) ||
        static_cast<std::uint8_t>(state.externalSync.lossMode) > static_cast<std::uint8_t>(SyncLossMode::Internal) ||
        static_cast<std::uint8_t>(state.externalSync.resetMode) > static_cast<std::uint8_t>(ExternalResetMode::Gate)) {
        return false;
    }


    if (static_cast<std::uint8_t>(state.unifiedClock.rate.mode) >
            static_cast<std::uint8_t>(ClockRatioMode::Divide) ||
        state.unifiedClock.rate.factor == 0U || state.unifiedClock.rate.factor > 32U ||
        state.unifiedClock.rate.numerator == 0U || state.unifiedClock.rate.numerator > 16U ||
        state.unifiedClock.rate.denominator == 0U || state.unifiedClock.rate.denominator > 16U ||
        state.unifiedClock.swingPercent > 50U ||
        state.unifiedClock.phasePercent > 99U ||
        state.unifiedClock.humanizeUs > config::kMaximumHumanizeUs ||
        static_cast<std::uint8_t>(state.dividerBank.bank) >
            static_cast<std::uint8_t>(DividerBank::Primes) ||
        static_cast<std::uint8_t>(state.display.screensaverMode) <
            static_cast<std::uint8_t>(ScreensaverMode::Fractal) ||
        static_cast<std::uint8_t>(state.display.screensaverMode) >
            static_cast<std::uint8_t>(ScreensaverMode::CubeCover) ||
        state.display.screensaverAfterMinutes < 1U ||
        state.display.screensaverAfterMinutes > state.display.dimAfterMinutes ||
        state.display.dimAfterMinutes > state.display.offAfterMinutes ||
        state.display.offAfterMinutes > config::kMaximumScreensaverMinutes) {
        return false;
    }

    for (const ChannelConfig& channel : state.channels) {
        if (static_cast<std::uint8_t>(channel.common.mode) > static_cast<std::uint8_t>(ChannelMode::Off) ||
            static_cast<std::uint8_t>(channel.common.rate.mode) > static_cast<std::uint8_t>(ClockRatioMode::Divide) ||
            channel.common.rate.factor == 0U || channel.common.rate.factor > 32U ||
            channel.common.rate.numerator == 0U || channel.common.rate.numerator > 16U ||
            channel.common.rate.denominator == 0U || channel.common.rate.denominator > 16U ||
            channel.common.swingPercent > 50U || channel.common.probabilityPercent > 100U ||
            channel.common.phasePercent > 99U ||
            static_cast<std::uint8_t>(channel.common.resetMode) > static_cast<std::uint8_t>(ResetMode::Free) ||
            channel.clock.meter.beats < 1U || channel.clock.meter.beats > 16U ||
            (channel.clock.meter.unit != 1U && channel.clock.meter.unit != 2U &&
             channel.clock.meter.unit != 4U && channel.clock.meter.unit != 8U &&
             channel.clock.meter.unit != 16U) ||
            channel.euclid.steps < 1U || channel.euclid.steps > 64U ||
            channel.euclid.hits > channel.euclid.steps ||
            channel.euclid.rotation >= channel.euclid.steps ||
            channel.sequencer.length < 1U || channel.sequencer.length > 64U ||
            channel.sequencer.rotation >= channel.sequencer.length) {
            return false;
        }
    }
    return true;
}


}  // namespace clockfw::services
