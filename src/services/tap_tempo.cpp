/**
 * @file tap_tempo.cpp
 * @brief Stateful tap-tempo estimator used by the performance UI.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/tap_tempo.h"

#include <algorithm>

#include "config.h"

namespace clockfw::services {


void TapTempo::reset() {
    previousTapAtMs_ = 0U;
    intervalsMs_.fill(0U);
    intervalCount_ = 0U;
}

std::uint16_t TapTempo::registerTap(
    const std::uint32_t nowMs,
    const std::uint16_t minimumBpm,
    const std::uint16_t maximumBpm) {
    if (previousTapAtMs_ == 0U || nowMs - previousTapAtMs_ > config::kTapSequenceResetMs) {
        intervalCount_ = 0U;
        previousTapAtMs_ = nowMs;
        return 0U;
    }

    const std::uint32_t intervalMs = nowMs - previousTapAtMs_;
    previousTapAtMs_ = nowMs;

    // Reject implausible taps before they can contaminate the rolling average.
    if (intervalMs < config::kTapMinimumIntervalMs || intervalMs > config::kTapMaximumIntervalMs) {
        intervalCount_ = 0U;
        return 0U;
    }

    if (intervalCount_ < intervalsMs_.size()) {
        intervalsMs_[intervalCount_++] = intervalMs;
    } else {
        for (std::size_t index = 0U; index + 1U < intervalsMs_.size(); ++index) {
            intervalsMs_[index] = intervalsMs_[index + 1U];
        }
        intervalsMs_.back() = intervalMs;
    }

    std::uint32_t sumMs = 0U;
    for (std::uint8_t index = 0U; index < intervalCount_; ++index) {
        sumMs += intervalsMs_[index];
    }

    const std::uint64_t roundedBpm =
        (60000ULL * intervalCount_ + sumMs / 2U) / sumMs;
    const std::uint64_t clampedBpm = std::clamp<std::uint64_t>(
        roundedBpm, minimumBpm, maximumBpm);
    return static_cast<std::uint16_t>(clampedBpm);
}

}  // namespace clockfw::services
