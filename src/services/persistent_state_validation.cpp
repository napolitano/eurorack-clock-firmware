/**
 * @file persistent_state_validation.cpp
 * @brief Semantic validation for persisted ClockState payloads.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_validation.h"

#include "config.h"
#include "domain/custom_groove.h"
#include "domain/groove_catalog.h"

namespace clockfw::services {
namespace {

/** @brief Returns whether an enum value is inside its contiguous persisted range. */
template <typename Enum>
bool enumAtMost(const Enum value, const Enum maximum) {
    return static_cast<std::uint8_t>(value) <= static_cast<std::uint8_t>(maximum);
}

/** @brief Validates a musical meter against the supported numerator and note units. */
bool isMeterValid(const MeterSettings& meter) {
    const bool supportedUnit = meter.unit == 1U || meter.unit == 2U || meter.unit == 4U ||
        meter.unit == 8U || meter.unit == 16U;
    return meter.beats >= 1U && meter.beats <= 16U && supportedUnit;
}

/** @brief Validates integer/rational clock-rate settings without normalizing them. */
bool isRateValid(const RateSettings& rate) {
    return enumAtMost(rate.mode, ClockRatioMode::Divide) &&
        rate.factor >= 1U && rate.factor <= 32U &&
        rate.numerator >= 1U && rate.numerator <= 16U &&
        rate.denominator >= 1U && rate.denominator <= 16U;
}

/** @brief Validates one factory/custom groove reference and its rotation domain. */
bool isGrooveValid(const GrooveSettings& grooveSettings) {
    if (!enumAtMost(grooveSettings.preset, GroovePreset::Custom) ||
        grooveSettings.amountPercent > 100U ||
        grooveSettings.customSlot >= kCustomGrooveSlotCount) {
        return false;
    }

    const std::uint8_t patternLength = grooveSettings.preset == GroovePreset::Custom
        ? kCustomGrooveMaximumSteps
        : groove::patternLength(grooveSettings.preset);
    return grooveSettings.rotation < patternLength;
}

/** @brief Validates the two-input role model, including the reserved Fill role boundary. */
bool areInputAssignmentsValid(const ExternalInputAssignments& assignments) {
    // Fill exists in the enum for a later feature but is intentionally not a
    // persistent/selectable 1.1 input role yet. Tap is therefore the current maximum.
    if (!enumAtMost(assignments.input1, InputFunction::Tap) ||
        !enumAtMost(assignments.input2, InputFunction::Tap)) {
        return false;
    }
    return assignments.input1 == InputFunction::Off || assignments.input1 != assignments.input2;
}

/** @brief Validates external clock capture, loss, reset, and smoothing configuration. */
bool isExternalSyncValid(const ExternalSyncSettings& settings) {
    return settings.pulsesPerQuarterNote != 0U &&
        enumAtMost(settings.edge, SyncEdge::Falling) &&
        enumAtMost(settings.lossMode, SyncLossMode::Internal) &&
        enumAtMost(settings.resetMode, ExternalResetMode::Gate) &&
        enumAtMost(settings.smoothing, SyncSmoothing::Full);
}

/** @brief Validates ordered STOP-mode screen-protection timeouts and screensaver selection. */
bool isDisplayPreferencesValid(const DisplayPreferences& display) {
    const std::uint8_t mode = static_cast<std::uint8_t>(display.screensaverMode);
    return mode >= static_cast<std::uint8_t>(ScreensaverMode::Fractal) &&
        mode <= static_cast<std::uint8_t>(ScreensaverMode::Fireworks) &&
        display.screensaverAfterMinutes >= 1U &&
        display.screensaverAfterMinutes <= display.dimAfterMinutes &&
        display.dimAfterMinutes <= display.offAfterMinutes &&
        display.offAfterMinutes <= config::kMaximumScreensaverMinutes;
}

/** @brief Validates all settings owned by one physical output channel. */
bool isChannelValid(const ChannelConfig& channel) {
    const CommonChannelSettings& common = channel.common;
    return enumAtMost(common.mode, ChannelMode::Off) &&
        isRateValid(common.rate) &&
        common.swingPercent <= 50U &&
        isGrooveValid(common.groove) &&
        common.probabilityPercent <= 100U &&
        common.phasePercent <= 99U &&
        enumAtMost(common.resetMode, ResetMode::Free) &&
        isMeterValid(channel.clock.meter) &&
        channel.euclid.steps >= 1U && channel.euclid.steps <= 64U &&
        channel.euclid.hits <= channel.euclid.steps &&
        channel.euclid.rotation < channel.euclid.steps &&
        channel.sequencer.length >= 1U && channel.sequencer.length <= 64U &&
        channel.sequencer.rotation < channel.sequencer.length;
}

}  // namespace

bool isPersistentStateValid(const ClockState& state) {
    const bool tempoRangeValid =
        state.tempoRange.minimumBpm >= config::kSupportedMinimumBpm &&
        state.tempoRange.maximumBpm <= config::kSupportedMaximumBpm &&
        state.tempoRange.minimumBpm <= state.tempoRange.maximumBpm &&
        state.bpm >= state.tempoRange.minimumBpm &&
        state.bpm <= state.tempoRange.maximumBpm;

    if (!tempoRangeValid ||
        !isMeterValid(state.masterMeter) ||
        state.preCountSteps > 64U ||
        !enumAtMost(state.transport, TransportState::Playing) ||
        !enumAtMost(state.source, ClockSource::Auto) ||
        !enumAtMost(state.operatingMode, OperatingMode::DividerBank) ||
        !isExternalSyncValid(state.externalSync) ||
        !areInputAssignmentsValid(state.inputs)) {
        return false;
    }

    if (!isRateValid(state.unifiedClock.rate) ||
        state.unifiedClock.swingPercent > 50U ||
        !isGrooveValid(state.unifiedClock.groove) ||
        state.unifiedClock.phasePercent > 99U ||
        state.unifiedClock.humanizeUs > config::kMaximumHumanizeUs ||
        !enumAtMost(state.dividerBank.bank, DividerBank::Primes) ||
        !isDisplayPreferencesValid(state.display)) {
        return false;
    }

    for (const ChannelConfig& channel : state.channels) {
        if (!isChannelValid(channel)) {
            return false;
        }
    }
    return true;
}

}  // namespace clockfw::services
