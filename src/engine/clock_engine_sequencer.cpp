/**
 * @file clock_engine_sequencer.cpp
 * @brief Sequencer 2.0 snapshot integration for the deterministic clock engine.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "engine/clock_engine.h"

#include "hal/interrupt_lock.h"

namespace clockfw::engine {

void ClockEngine::updateSequencerPattern(
    const std::size_t channelIndex,
    const SequencerPatternV2& pattern,
    const bool rescheduleChannel) {
    if (channelIndex >= kChannelCount || !isSequencerPatternValid(pattern)) {
        return;
    }
    hal::InterruptLock interruptLock;
    sequencerPatterns_[channelIndex] = pattern;
    clampSequencerPattern(sequencerPatterns_[channelIndex]);
    sequencerPatternV2Active_[channelIndex] = true;
    if (rescheduleChannel) {
        scheduleChannelFromCurrentPosition(channelIndex);
    }
    synchronizeChannelStepPhase(channelIndex);
}

}  // namespace clockfw::engine
