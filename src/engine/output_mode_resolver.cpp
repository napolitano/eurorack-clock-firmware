/**
 * @file output_mode_resolver.cpp
 * @brief Operating-mode to physical-output configuration resolver.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "engine/output_mode_resolver.h"

#include <cstdint>

namespace clockfw::engine {
namespace {

/** Fixed divisors exposed by the powers-of-two divider family. */
constexpr std::uint8_t kPowerOfTwoDivisors[kChannelCount] = {
    1U, 2U, 4U, 8U, 16U, 32U, 64U, 128U};

/** Fixed divisors exposed by the consecutive-integer divider family. */
constexpr std::uint8_t kIntegerDivisors[kChannelCount] = {
    1U, 2U, 3U, 4U, 5U, 6U, 7U, 8U};

/** Fixed divisors exposed by the prime-number divider family. */
constexpr std::uint8_t kPrimeDivisors[kChannelCount] = {
    1U, 2U, 3U, 5U, 7U, 11U, 13U, 17U};

/** Returns the fixed divisor assigned to one physical output. */
std::uint8_t dividerForOutput(
    const DividerBank bank,
    const std::size_t channelIndex) {
    const std::uint8_t* divisors = kPowerOfTwoDivisors;
    if (bank == DividerBank::Integers) {
        divisors = kIntegerDivisors;
    } else if (bank == DividerBank::Primes) {
        divisors = kPrimeDivisors;
    }
    return divisors[channelIndex];
}

/** Builds the deterministic shared-clock configuration. */
ChannelConfig makeUnifiedClockChannel(const ClockState& state) {
    ChannelConfig channel{};
    channel.common.mode = ChannelMode::Clock;
    channel.common.rate = state.unifiedClock.rate;
    channel.common.swingPercent = state.unifiedClock.swingPercent;
    channel.common.probabilityPercent = 100U;
    channel.common.gateLengthMs = state.unifiedClock.gateLengthMs;
    channel.common.phasePercent = state.unifiedClock.phasePercent;
    channel.common.resetMode = ResetMode::Global;
    channel.common.muted = false;
    channel.clock.meter = state.masterMeter;
    return channel;
}

/** Builds one deterministic divider-bank output. */
ChannelConfig makeDividerChannel(
    const ClockState& state,
    const std::size_t channelIndex) {
    ChannelConfig channel{};
    channel.common.mode = ChannelMode::Clock;
    const std::uint8_t divisor = dividerForOutput(state.dividerBank.bank, channelIndex);
    channel.common.rate.mode = divisor == 1U
        ? ClockRatioMode::Multiply
        : ClockRatioMode::Divide;
    channel.common.rate.factor = divisor;
    channel.common.rate.numerator = 1U;
    channel.common.rate.denominator = 1U;
    channel.common.swingPercent = 0U;
    channel.common.probabilityPercent = 100U;
    channel.common.gateLengthMs = state.dividerBank.gateLengthMs;
    channel.common.phasePercent = 0U;
    channel.common.resetMode = ResetMode::Global;
    channel.common.muted = false;
    channel.clock.meter = state.masterMeter;
    return channel;
}

}  // namespace

ChannelConfig resolvePhysicalOutputConfiguration(
    const ClockState& state,
    const std::size_t channelIndex) {
    if (channelIndex >= kChannelCount) {
        return ChannelConfig{};
    }
    if (state.operatingMode == OperatingMode::UnifiedClock) {
        return makeUnifiedClockChannel(state);
    }
    if (state.operatingMode == OperatingMode::DividerBank) {
        return makeDividerChannel(state, channelIndex);
    }
    return state.channels[channelIndex];
}

}  // namespace clockfw::engine
