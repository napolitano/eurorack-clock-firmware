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

/** Historical V1 logical image size used through 1.1/r44. */
inline constexpr std::size_t kV1ImageBytes = 8192U;

/** Architectural ceiling inside either 16-KiB physical A/B slot. */
inline constexpr std::size_t kMaximumImageBytes = 12U * 1024U;

/** Current post-1.1 logical image size; the new 4 KiB is an extension region. */
inline constexpr std::size_t kCurrentImageBytes = kMaximumImageBytes;

/** Sequencer 2.0 extension begins immediately after the historical 8-KiB image. */
inline constexpr std::size_t kSequencer2RegionOffset = kV1ImageBytes;

/** Clock CURRENT + named-preset records start at byte zero. */
inline constexpr std::size_t kClockStateRegionOffset = 0U;

/** Historical single-score compatibility records start here. */
inline constexpr std::size_t kLegacyScoreRegionOffset = 3072U;

/** Four independent Top-100 leaderboard records start here. */
inline constexpr std::size_t kLeaderboardRegionOffset = 4096U;

static_assert(kClockStateRegionOffset < kLegacyScoreRegionOffset);
static_assert(kLegacyScoreRegionOffset < kLeaderboardRegionOffset);
static_assert(kLeaderboardRegionOffset < kV1ImageBytes);
static_assert(kV1ImageBytes < kCurrentImageBytes);
static_assert(kSequencer2RegionOffset == kV1ImageBytes);
static_assert(kCurrentImageBytes <= kMaximumImageBytes);

}  // namespace clockfw::hal::persistent_layout
