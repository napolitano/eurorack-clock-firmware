/**
 * @file defaults.h
 * @brief Editable factory defaults for all user-facing clock settings.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 *
 * This file intentionally contains defaults only. Validation ranges and runtime
 * policy belong in config.h; physical wiring belongs in pin_map.h.
 */

#pragma once

#include <cstdint>

#include "domain/clock_types.h"

namespace clockfw::defaults {

/** Factory top-level output routing mode. */
inline constexpr OperatingMode kOperatingMode = OperatingMode::UnifiedClock;

/** Factory master tempo. */
inline constexpr std::uint16_t kMasterBpm = 120U;

/** Factory lower limit for manual and tap-tempo changes. */
inline constexpr std::uint16_t kMinimumBpm = 20U;

/** Factory upper limit for manual and tap-tempo changes. */
inline constexpr std::uint16_t kMaximumBpm = 999U;

/** Factory master meter. */
inline constexpr MeterSettings kMasterMeter{4U, 4U};

/** Factory transport state after boot. */
inline constexpr TransportState kTransportState = TransportState::Stopped;

/** Factory master clock source. */
inline constexpr ClockSource kClockSource = ClockSource::Internal;

/** Factory shared-clock rate used in One Clock mode. */
inline constexpr RateSettings kUnifiedClockRate{ClockRatioMode::Multiply, 1U, 1U, 1U};

/** Factory shared-clock swing percentage. */
inline constexpr std::uint8_t kUnifiedClockSwingPercent = 0U;

/** Factory shared-clock gate length. */
inline constexpr std::uint16_t kUnifiedClockGateLengthMs = 10U;

/** Factory shared-clock phase. */
inline constexpr std::uint8_t kUnifiedClockPhasePercent = 0U;

/** Factory ONE CLOCK per-output timing humanization; zero keeps exact timing. */
inline constexpr std::uint16_t kUnifiedClockHumanizeUs = 0U;

/** Factory divider-bank family. */
inline constexpr DividerBank kDividerBank = DividerBank::PowersOfTwo;

/** Factory divider-bank gate length. */
inline constexpr std::uint16_t kDividerGateLengthMs = 10U;


/** Factory STOP-mode screensaver effect. */
inline constexpr ScreensaverMode kScreensaverMode = ScreensaverMode::Clock;

/** Minutes of STOP inactivity before the idle animation starts. */
inline constexpr std::uint8_t kScreensaverAfterMinutes = 2U;

/** Minutes of STOP inactivity before OLED contrast is reduced. */
inline constexpr std::uint8_t kScreensaverDimAfterMinutes = 5U;

/** Minutes of STOP inactivity before the OLED panel is switched off. */
inline constexpr std::uint8_t kScreensaverOffAfterMinutes = 10U;

/** Factory per-channel generator mode. */
inline constexpr ChannelMode kChannelMode = ChannelMode::Clock;

/** Factory integer/rational rate. */
inline constexpr RateSettings kChannelRate{ClockRatioMode::Multiply, 1U, 1U, 1U};

/** Factory channel swing percentage. */
inline constexpr std::uint8_t kSwingPercent = 0U;

/** Factory trigger probability. */
inline constexpr std::uint8_t kProbabilityPercent = 100U;

/** Factory gate length in milliseconds. */
inline constexpr std::uint16_t kGateLengthMs = 10U;

/** Factory channel phase offset in percent. */
inline constexpr std::uint8_t kPhasePercent = 0U;

/** Factory channel reset behavior. */
inline constexpr ResetMode kResetMode = ResetMode::Global;

/** Factory channel mute state. */
inline constexpr bool kMuted = false;

/** Factory local CLOCK meter. */
inline constexpr MeterSettings kClockMeter{4U, 4U};

/** Factory Euclidean sequence length. */
inline constexpr std::uint8_t kEuclidSteps = 16U;

/** Base Euclidean hit count; channel defaults add channelIndex modulo four. */
inline constexpr std::uint8_t kEuclidBaseHits = 4U;

/** Factory Euclidean rotation. */
inline constexpr std::uint8_t kEuclidRotation = 0U;

/** Factory sequencer length. */
inline constexpr std::uint8_t kSequencerLength = 16U;

/** Factory sequencer rotation. */
inline constexpr std::uint8_t kSequencerRotation = 0U;

/** Factory sequence used by even zero-based channels. */
inline constexpr std::uint64_t kSequencerPatternA = 0x1111ULL;

/** Factory sequence used by odd zero-based channels. */
inline constexpr std::uint64_t kSequencerPatternB = 0x5555ULL;

/** Factory external-sync PPQN. */
inline constexpr std::uint8_t kExternalSyncPpqn = 1U;

/** Factory external-sync edge. */
inline constexpr SyncEdge kExternalSyncEdge = SyncEdge::Rising;

/** Factory behavior when an external clock is lost. */
inline constexpr SyncLossMode kExternalSyncLossMode = SyncLossMode::Freewheel;

/** Factory interpretation of the external RST comparator level. */
inline constexpr ExternalResetMode kExternalResetMode = ExternalResetMode::Trigger;

/** Factory external-sync glitch-rejection interval in microseconds. */
inline constexpr std::uint16_t kExternalSyncGlitchFilterUs = 1000U;

/** Factory external-sync timeout in milliseconds. */
inline constexpr std::uint16_t kExternalSyncTimeoutMs = 1500U;

}  // namespace clockfw::defaults
