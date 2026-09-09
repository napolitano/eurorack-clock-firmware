/**
 * @file default_configuration.cpp
 * @brief Factory-default state construction from the editable defaults catalog.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "domain/default_configuration.h"

#include "defaults.h"

namespace clockfw {

void initializeFactoryDefaults(ClockState& state) {
    state.operatingMode = defaults::kOperatingMode;
    state.bpm = defaults::kMasterBpm;
    state.tempoRange = {defaults::kMinimumBpm, defaults::kMaximumBpm};
    state.masterMeter = defaults::kMasterMeter;
    state.transport = defaults::kTransportState;
    state.source = defaults::kClockSource;
    state.externalSync = {
        defaults::kExternalSyncPpqn,
        defaults::kExternalSyncEdge,
        defaults::kExternalSyncLossMode,
        defaults::kExternalResetMode,
        defaults::kExternalSyncGlitchFilterUs,
        defaults::kExternalSyncTimeoutMs};
    state.unifiedClock = {
        defaults::kUnifiedClockRate,
        defaults::kUnifiedClockSwingPercent,
        defaults::kUnifiedClockGateLengthMs,
        defaults::kUnifiedClockPhasePercent};
    state.dividerBank = {
        defaults::kDividerBank,
        defaults::kDividerGateLengthMs};
    state.display = {
        defaults::kScreensaverMode,
        defaults::kScreensaverAfterMinutes,
        defaults::kScreensaverDimAfterMinutes,
        defaults::kScreensaverOffAfterMinutes};

    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        ChannelConfig& channel = state.channels[channelIndex];
        channel.common.mode = defaults::kChannelMode;
        channel.common.rate = defaults::kChannelRate;
        channel.common.swingPercent = defaults::kSwingPercent;
        channel.common.probabilityPercent = defaults::kProbabilityPercent;
        channel.common.gateLengthMs = defaults::kGateLengthMs;
        channel.common.phasePercent = defaults::kPhasePercent;
        channel.common.resetMode = defaults::kResetMode;
        channel.common.muted = defaults::kMuted;
        channel.clock.meter = defaults::kClockMeter;
        channel.euclid = {
            defaults::kEuclidSteps,
            static_cast<std::uint8_t>(defaults::kEuclidBaseHits + (channelIndex % 4U)),
            defaults::kEuclidRotation};
        channel.sequencer.length = defaults::kSequencerLength;
        channel.sequencer.rotation = defaults::kSequencerRotation;
        channel.sequencer.pattern = (channelIndex & 1U) != 0U
            ? defaults::kSequencerPatternB
            : defaults::kSequencerPatternA;
    }
}

}  // namespace clockfw
