/**
 * @file clock_types.h
 * @brief Domain types and persistent configuration model for the clock firmware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace clockfw {

/** Number of independent gate channels provided by the module. */
inline constexpr std::size_t kChannelCount = 8U;

/** @brief Selects the source used to drive the master timeline. */
enum class ClockSource : std::uint8_t { Internal, External, Auto };

/** @brief Selects the top-level routing model used by the eight physical outputs. */
enum class OperatingMode : std::uint8_t { Independent, UnifiedClock, DividerBank };

/** @brief Selects the fixed divisor family used by the divider-bank operating mode. */
enum class DividerBank : std::uint8_t { PowersOfTwo, Integers, Primes };

/** @brief Selects the rhythm generator used by one output channel. */
enum class ChannelMode : std::uint8_t {
    Clock = 0U,
    Euclid = 1U,
    Sequencer = 2U,
    Off = 3U
};

/** @brief Selects the STOP-mode OLED idle animation. Values intentionally match the UI MODE numbers. */
enum class ScreensaverMode : std::uint8_t {
    Fractal = 1U,
    Orbit = 2U,
    None = 3U,
    Plug = 4U,
    Clock = 5U,
    Heartbeat = 6U,
    Acid = 7U,
    Spectrum = 8U,
    Field = 9U,
    Blox = 10U,
    Matrix = 11U,
    CubeCover = 12U
};

/** @brief Represents the master transport state. */
enum class TransportState : std::uint8_t { Stopped, Paused, Playing };

/** @brief Selects whether the integer rate factor multiplies or divides the base rate. */
enum class ClockRatioMode : std::uint8_t { Multiply, Divide };

/** @brief Selects whether a channel follows a global reset or keeps its local phase. */
enum class ResetMode : std::uint8_t { Global, Free };

/** @brief Selects the external-sync edge used for timing capture. */
enum class SyncEdge : std::uint8_t { Rising, Falling };

/** @brief Selects external reset interpretation after the comparator. */
enum class ExternalResetMode : std::uint8_t { Trigger, Gate };

/** @brief Selects transport behavior after the external clock is lost. */
enum class SyncLossMode : std::uint8_t { Stop, Freewheel, Internal };

/** @brief Musical meter expressed as numerator and note-value denominator. */
struct MeterSettings {
    std::uint8_t beats = 4U;
    std::uint8_t unit = 4U;
};

/** @brief Integer and rational rate configuration for one channel. */
struct RateSettings {
    ClockRatioMode mode = ClockRatioMode::Multiply;
    std::uint8_t factor = 1U;
    std::uint8_t numerator = 1U;
    std::uint8_t denominator = 1U;
};

/** @brief Settings shared by CLOCK, EUCLID, and SEQ channel modes. */
struct CommonChannelSettings {
    ChannelMode mode = ChannelMode::Clock;
    RateSettings rate{};
    std::uint8_t swingPercent = 0U;
    std::uint8_t probabilityPercent = 100U;
    std::uint16_t gateLengthMs = 10U;
    std::uint8_t phasePercent = 0U;
    ResetMode resetMode = ResetMode::Global;
    bool muted = false;
};

/** @brief CLOCK-specific channel settings. */
struct ClockChannelSettings {
    MeterSettings meter{4U, 4U};
};

/** @brief Euclidean-pattern settings. */
struct EuclidSettings {
    std::uint8_t steps = 16U;
    std::uint8_t hits = 4U;
    std::uint8_t rotation = 0U;
};

/** @brief Binary gate-sequencer settings. */
struct SequencerSettings {
    std::uint8_t length = 16U;
    std::uint8_t rotation = 0U;
    std::uint64_t pattern = 0x1111ULL;
};


/** @brief Settings for the global one-clock-to-eight-outputs operating mode. */
struct UnifiedClockSettings {
    RateSettings rate{};
    std::uint8_t swingPercent = 0U;
    std::uint16_t gateLengthMs = 10U;
    std::uint8_t phasePercent = 0U;
    /** Maximum per-output timing displacement applied only in ONE CLOCK mode. */
    std::uint16_t humanizeUs = 0U;
};

/** @brief Settings for the fixed eight-output clock-divider operating mode. */
struct DividerBankSettings {
    DividerBank bank = DividerBank::PowersOfTwo;
    std::uint16_t gateLengthMs = 10U;
};

/** @brief Complete configuration for one physical output channel. */
struct ChannelConfig {
    CommonChannelSettings common{};
    ClockChannelSettings clock{};
    EuclidSettings euclid{};
    SequencerSettings sequencer{};
};

/** @brief External synchronization behavior and filtering configuration. */
struct ExternalSyncSettings {
    std::uint8_t pulsesPerQuarterNote = 1U;
    SyncEdge edge = SyncEdge::Rising;
    SyncLossMode lossMode = SyncLossMode::Freewheel;
    ExternalResetMode resetMode = ExternalResetMode::Trigger;
    std::uint16_t glitchFilterUs = 1000U;
    std::uint16_t timeoutMs = 1500U;
};

/** @brief STOP-mode display protection and idle-animation preferences. */
struct DisplayPreferences {
    ScreensaverMode screensaverMode = ScreensaverMode::Clock;
    std::uint8_t screensaverAfterMinutes = 2U;
    std::uint8_t dimAfterMinutes = 5U;
    std::uint8_t offAfterMinutes = 10U;
};

/** @brief User-adjustable limits applied to manual and tap-tempo changes only. */
struct TempoRangeSettings {
    std::uint16_t minimumBpm = 20U;
    std::uint16_t maximumBpm = 999U;
};

/** @brief Complete mutable application state edited by the user interface. */
struct ClockState {
    OperatingMode operatingMode = OperatingMode::UnifiedClock;
    std::uint16_t bpm = 120U;
    TempoRangeSettings tempoRange{};
    MeterSettings masterMeter{4U, 4U};
    TransportState transport = TransportState::Stopped;
    ClockSource source = ClockSource::Internal;
    ExternalSyncSettings externalSync{};
    UnifiedClockSettings unifiedClock{};
    DividerBankSettings dividerBank{};
    DisplayPreferences display{};
    ChannelConfig channels[kChannelCount]{};
};

}  // namespace clockfw
