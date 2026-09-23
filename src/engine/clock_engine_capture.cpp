/**
 * @file clock_engine_capture.cpp
 * @brief Read-only timing helpers used by high-resolution Groove Record capture.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "engine/clock_engine.h"

#include "clock_core.h"
#include "config.h"
#include "hal/interrupt_lock.h"

namespace clockfw::engine {

std::uint64_t ClockEngine::nominalIntervalQ32(const std::size_t channelIndex) const {
    if (channelIndex >= kChannelCount) {
        return 0U;
    }
    hal::InterruptLock interruptLock;
    return calculateNominalBaseIntervalQ32(channelIndex);
}

std::uint64_t ClockEngine::estimateMasterPositionAtUs(
    const std::uint32_t eventTimestampUs,
    const std::uint32_t nowUs) const {
    hal::InterruptLock interruptLock;
    const std::uint32_t elapsedUs = nowUs - eventTimestampUs;
    // A debounced front-panel edge should be only about 25 ms old. Bounding the
    // compensation avoids treating an invalid/stale timestamp as a wrap-sized
    // rewind while still covering unusually slow foreground service.
    const std::uint32_t boundedElapsedUs = elapsedUs > 100'000U ? 100'000U : elapsedUs;
    const std::uint64_t elapsedTicks = boundedElapsedUs / config::kSchedulerTickUs;
    volatile std::uint64_t remainder = 0U;
    const std::uint64_t incrementQ32 = core::calculateMasterIncrementMilliBpmQ32(
        effectiveBpmMilli(),
        configuration_.beatUnit,
        config::kSchedulerFrequencyHz,
        remainder);
    const std::uint64_t rewindQ32 = incrementQ32 != 0U &&
            elapsedTicks > UINT64_MAX / incrementQ32
        ? UINT64_MAX
        : incrementQ32 * elapsedTicks;
    return rewindQ32 >= masterPositionQ32_ ? 0U : masterPositionQ32_ - rewindQ32;
}


}  // namespace clockfw::engine
