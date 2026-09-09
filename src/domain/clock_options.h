/**
 * @file clock_options.h
 * @brief Shared UI option tables for musical rate and gate parameters.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstdint>

#include "domain/clock_types.h"

namespace clockfw {

/** @brief One selectable integer divide/multiply option. */
struct RateOption {
    ClockRatioMode mode;
    std::uint8_t factor;
};

/** Ordered rate options presented by the user interface. */
inline constexpr std::array<RateOption, 19U> kRateOptions{{
    {ClockRatioMode::Divide, 32U}, {ClockRatioMode::Divide, 24U},
    {ClockRatioMode::Divide, 16U}, {ClockRatioMode::Divide, 12U},
    {ClockRatioMode::Divide, 8U},  {ClockRatioMode::Divide, 6U},
    {ClockRatioMode::Divide, 4U},  {ClockRatioMode::Divide, 3U},
    {ClockRatioMode::Divide, 2U},  {ClockRatioMode::Multiply, 1U},
    {ClockRatioMode::Multiply, 2U},{ClockRatioMode::Multiply, 3U},
    {ClockRatioMode::Multiply, 4U},{ClockRatioMode::Multiply, 6U},
    {ClockRatioMode::Multiply, 8U},{ClockRatioMode::Multiply, 12U},
    {ClockRatioMode::Multiply, 16U},{ClockRatioMode::Multiply, 24U},
    {ClockRatioMode::Multiply, 32U}
}};

/** Supported gate lengths in milliseconds. */
inline constexpr std::array<std::uint16_t, 7U> kGateLengthOptionsMs{{1U, 2U, 5U, 10U, 20U, 50U, 100U}};

/** Supported meter note-value denominators. */
inline constexpr std::array<std::uint8_t, 4U> kBeatUnitOptions{{2U, 4U, 8U, 16U}};

/** Supported external synchronization pulse resolutions. */
inline constexpr std::array<std::uint8_t, 4U> kExternalPpqnOptions{{1U, 2U, 4U, 24U}};

/** Curated ONE CLOCK timing-humanization amplitudes in microseconds. */
inline constexpr std::array<std::uint16_t, 5U> kHumanizeOptionsUs{{0U, 250U, 500U, 1000U, 2000U}};

}  // namespace clockfw
