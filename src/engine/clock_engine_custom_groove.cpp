/**
 * @file clock_engine_custom_groove.cpp
 * @brief Custom Groove library and runtime-preview updates for ClockEngine.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "engine/clock_engine_custom_groove.h"

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


}  // namespace clockfw::engine
