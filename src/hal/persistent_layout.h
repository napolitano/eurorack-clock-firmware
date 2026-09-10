/**
 * @file persistent_layout.h
 * @brief Stable logical layout boundaries for CLOCK's internal Flash image.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>

namespace clockfw::hal::persistent_layout {

/** V1 logical image size used by the 0.19/1.0 persistence family. */
inline constexpr std::size_t kV1ImageBytes = 8192U;

/** Architectural ceiling inside either 16-KiB physical A/B slot. */
inline constexpr std::size_t kMaximumImageBytes = 12U * 1024U;

/** Clock CURRENT + named-preset records start at byte zero. */
inline constexpr std::size_t kClockStateRegionOffset = 0U;

/** Historical single-score compatibility records start here. */
inline constexpr std::size_t kLegacyScoreRegionOffset = 3072U;

/** Four independent Top-100 leaderboard records start here. */
inline constexpr std::size_t kLeaderboardRegionOffset = 4096U;

static_assert(kClockStateRegionOffset < kLegacyScoreRegionOffset);
static_assert(kLegacyScoreRegionOffset < kLeaderboardRegionOffset);
static_assert(kLeaderboardRegionOffset < kV1ImageBytes);
static_assert(kV1ImageBytes <= kMaximumImageBytes);

}  // namespace clockfw::hal::persistent_layout
