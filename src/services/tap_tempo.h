/**
 * @file tap_tempo.h
 * @brief Stateful tap-tempo estimator used by the performance UI.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstdint>

namespace clockfw::services {

/** @brief Estimates BPM from up to four recent valid tap intervals. */
class TapTempo final {
public:
    /** @brief Clears all stored taps and interval history. */
    void reset();

    /**
     * @brief Processes one tap timestamp.
     * @param nowMs Monotonic tap time in milliseconds.
     * @param minimumBpm Lower BPM clamp.
     * @param maximumBpm Upper BPM clamp.
     * @return New BPM when enough valid timing data exists, otherwise 0.
     */
    std::uint16_t registerTap(
        std::uint32_t nowMs,
        std::uint16_t minimumBpm,
        std::uint16_t maximumBpm);

private:
    std::uint32_t previousTapAtMs_ = 0U;
    bool havePreviousTap_ = false;
    std::array<std::uint32_t, 4U> intervalsMs_{};
    std::uint8_t intervalCount_ = 0U;
};

}  // namespace clockfw::services
