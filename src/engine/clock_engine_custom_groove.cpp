/**
 * @file clock_engine_custom_groove.cpp
 * @brief Custom Groove library and runtime-preview updates for ClockEngine.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "engine/clock_engine_custom_groove.h"

#include <algorithm>

#include "config.h"
#include "domain/groove_catalog.h"

#include "hal/interrupt_lock.h"

namespace clockfw::engine {

void ClockEngine::updateCustomGrooveSlot(
    const std::uint8_t slotIndex,
    const CustomGroovePattern& pattern,
    const bool rescheduleAffected) {
    if (slotIndex >= kCustomGrooveSlotCount || !isCustomGroovePatternValid(pattern)) {
        return;
    }

    hal::InterruptLock interruptLock;
    customGrooves_[slotIndex] = pattern;
    if (!rescheduleAffected) {
        return;
    }
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        const GrooveSettings& grooveSettings = configuration_.channels[channelIndex].common.groove;
        if (grooveSettings.preset == GroovePreset::Custom &&
            grooveSettings.customSlot == slotIndex) {
            scheduleChannelFromCurrentPosition(channelIndex);
        }
    }
}

void ClockEngine::setCustomGroovePreview(
    const std::size_t channelIndex,
    const CustomGroovePattern& pattern,
    const std::uint8_t amountPercent,
    const std::uint8_t rotation,
    const bool rescheduleChannel) {
    if (channelIndex >= kChannelCount || !isCustomGroovePatternValid(pattern) ||
        amountPercent > 100U || rotation >= pattern.length) {
        return;
    }
    hal::InterruptLock interruptLock;
    customGroovePreviews_[channelIndex] = pattern;
    customGroovePreviewAmount_[channelIndex] = amountPercent;
    customGroovePreviewRotation_[channelIndex] = rotation;
    customGroovePreviewActive_[channelIndex] = true;
    if (rescheduleChannel) {
        scheduleChannelFromCurrentPosition(channelIndex);
    }
}

void ClockEngine::clearCustomGroovePreview(
    const std::size_t channelIndex,
    const bool rescheduleChannel) {
    if (channelIndex >= kChannelCount) {
        return;
    }
    hal::InterruptLock interruptLock;
    customGroovePreviewActive_[channelIndex] = false;
    if (rescheduleChannel) {
        scheduleChannelFromCurrentPosition(channelIndex);
    }
}


std::uint64_t ClockEngine::calculateShortestDeterministicIntervalUs(
    const std::size_t channelIndex) const {
    const ChannelConfig& channel = configuration_.channels[channelIndex];
    const std::uint64_t baseIntervalUs = calculateBaseIntervalUs(channelIndex);
    const std::uint8_t swingPercent = std::min<std::uint8_t>(channel.common.swingPercent, 50U);
    const GrooveSettings& grooveSettings = channel.common.groove;
    const bool previewActive = customGroovePreviewActive_[channelIndex];
    const bool customActive = previewActive ||
        (grooveSettings.preset == GroovePreset::Custom &&
         grooveSettings.customSlot < kCustomGrooveSlotCount);

    if (!customActive) {
        const std::uint64_t swingDelayUs = (baseIntervalUs * swingPercent) / 100ULL;
        const std::uint16_t groovePermille = groove::maximumDelayPermille(
            grooveSettings.preset,
            grooveSettings.amountPercent);
        const std::uint64_t grooveDelayUs = (baseIntervalUs * groovePermille) / 1000ULL;
        const std::uint64_t deterministicDelayUs = std::min<std::uint64_t>(
            swingDelayUs + grooveDelayUs,
            baseIntervalUs > config::kSchedulerTickUs
                ? baseIntervalUs - config::kSchedulerTickUs
                : 0U);
        return baseIntervalUs - deterministicDelayUs;
    }

    const CustomGroovePattern& customPattern = previewActive
        ? customGroovePreviews_[channelIndex]
        : customGrooves_[grooveSettings.customSlot];
    const std::uint8_t customAmount = previewActive
        ? customGroovePreviewAmount_[channelIndex]
        : grooveSettings.amountPercent;
    const std::uint8_t customRotation = previewActive
        ? customGroovePreviewRotation_[channelIndex]
        : grooveSettings.rotation;
    if (!isCustomGroovePatternValid(customPattern)) {
        return baseIntervalUs;
    }

    const std::int64_t symmetricLimitUs = baseIntervalUs > config::kSchedulerTickUs
        ? static_cast<std::int64_t>(
            (baseIntervalUs - config::kSchedulerTickUs) / 2ULL)
        : 0LL;
    auto shapedOffsetUs = [&](const std::uint64_t eventSerial) -> std::int64_t {
        const std::int64_t swingUs = (eventSerial & 1ULL) != 0ULL
            ? static_cast<std::int64_t>((baseIntervalUs * swingPercent) / 100ULL)
            : 0LL;
        const std::int16_t offset256 = customGrooveOffset256(
            customPattern,
            eventSerial,
            customAmount,
            customRotation);
        const std::uint64_t magnitude = static_cast<std::uint64_t>(
            offset256 < 0 ? -static_cast<std::int32_t>(offset256) : offset256);
        const std::int64_t customUs = static_cast<std::int64_t>(
            (baseIntervalUs * magnitude) / 256ULL) * (offset256 < 0 ? -1LL : 1LL);
        return std::clamp<std::int64_t>(
            swingUs + customUs,
            -symmetricLimitUs,
            symmetricLimitUs);
    };

    // Custom Groove has at most 64 steps; including Swing parity gives a period
    // no larger than 128 events. Scan the complete combined pattern so the gate
    // limiter follows the actual adjacent event spacing, not a factory-table proxy.
    const std::uint64_t period = (customPattern.length % 2U) == 0U
        ? customPattern.length
        : static_cast<std::uint64_t>(customPattern.length) * 2ULL;
    std::uint64_t shortest = baseIntervalUs;
    for (std::uint64_t serial = 0ULL; serial < period; ++serial) {
        const std::int64_t current = shapedOffsetUs(serial);
        const std::int64_t next = shapedOffsetUs(serial + 1ULL);
        const std::int64_t interval = static_cast<std::int64_t>(baseIntervalUs) + next - current;
        const std::uint64_t bounded = interval > static_cast<std::int64_t>(config::kSchedulerTickUs)
            ? static_cast<std::uint64_t>(interval)
            : static_cast<std::uint64_t>(config::kSchedulerTickUs);
        shortest = std::min(shortest, bounded);
    }
    return shortest;
}

std::uint64_t ClockEngine::calculateShortestActualIntervalUs(
    const std::size_t channelIndex) const {
    const std::uint64_t shortestShapedIntervalUs =
        calculateShortestDeterministicIntervalUs(channelIndex);
    const std::uint64_t humanizePairAllowanceUs =
        static_cast<std::uint64_t>(effectiveHumanizeUs(channelIndex)) * 2ULL;
    return shortestShapedIntervalUs > humanizePairAllowanceUs
        ? shortestShapedIntervalUs - humanizePairAllowanceUs
        : config::kSchedulerTickUs;
}



std::uint16_t ClockEngine::effectiveHumanizeUs(const std::size_t channelIndex) const {
    if (configuration_.operatingMode != OperatingMode::UnifiedClock ||
        configuration_.unifiedHumanizeUs == 0U || channelIndex >= kChannelCount) {
        return 0U;
    }

    const std::uint64_t shortestShapedIntervalUs =
        calculateShortestDeterministicIntervalUs(channelIndex);
    if (shortestShapedIntervalUs <= config::kSchedulerTickUs) {
        return 0U;
    }

    // Adjacent events may receive opposite signed humanize offsets. Reserve one
    // scheduler quantum even after deterministic Swing + Groove displacement.
    const std::uint64_t maximumSafeUs =
        (shortestShapedIntervalUs - config::kSchedulerTickUs) / 2ULL;
    const std::uint64_t quantizedSafeUs =
        (maximumSafeUs / config::kSchedulerTickUs) * config::kSchedulerTickUs;
    const std::uint64_t configuredUs = std::min<std::uint64_t>(
        configuration_.unifiedHumanizeUs, config::kMaximumHumanizeUs);
    return static_cast<std::uint16_t>(std::min(configuredUs, quantizedSafeUs));
}

}  // namespace clockfw::engine
