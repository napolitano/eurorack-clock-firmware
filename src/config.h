/**
 * @file config.h
 * @brief Global compile-time configuration for firmware behavior and hardware policy.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 *
 * This is the primary file to edit when adapting firmware behavior to a new
 * hardware revision or desired factory configuration. Physical GPIO routing is
 * intentionally kept in pin_map.h so behavioral defaults and wiring remain
 * independently reviewable.
 */

#pragma once

#include <cstdint>

namespace clockfw::config {

/** @brief Display transport supported by the built-in monochrome OLED driver. */
enum class DisplayTransport : std::uint8_t { I2c, Spi };

/** @brief Supported 128x64 Solomon Systech OLED controller families. */
enum class DisplayController : std::uint8_t { Ssd1306, Ssd1315 };

/** @brief UI language identifiers. Additional catalogs can be added without changing UI code. */
enum class UiLanguage : std::uint8_t { EnglishUs, GermanDe };

/** @brief Boot-only hidden game selected at compile time. */
enum class EasterEgg : std::uint8_t { PixelRaid = 1U, Formula1 = 2U, Breakout = 3U, EggJourney = 4U, Beatknecht = 5U };

/** Active UI language. English (US) is the only shipped catalog in this prerelease. */
inline constexpr UiLanguage kUiLanguage = UiLanguage::EnglishUs;

#ifndef CLOCK_EASTER_EGG
#define CLOCK_EASTER_EGG 1
#endif
static_assert(CLOCK_EASTER_EGG >= 1 && CLOCK_EASTER_EGG <= 5, "CLOCK_EASTER_EGG must be 1, 2, 3, 4, or 5");
inline constexpr EasterEgg kEasterEgg = static_cast<EasterEgg>(CLOCK_EASTER_EGG);

/**
 * Build-time display-transport override.
 *
 * The generic/base profile uses I2C so broadly available modules work without
 * extra flags. The shipped/reference PlatformIO environment explicitly selects
 * SPI because it gives the shortest display service time.
 */
#ifndef CLOCK_DISPLAY_USE_SPI
#define CLOCK_DISPLAY_USE_SPI 0
#endif

/** Display transport selected for this firmware build. */
inline constexpr DisplayTransport kDisplayTransport = CLOCK_DISPLAY_USE_SPI
    ? DisplayTransport::Spi
    : DisplayTransport::I2c;

/**
 * OLED controller selector. Use numeric part-family identifiers in build flags
 * so PlatformIO environments remain readable: 1306 or 1315.
 */
#ifndef CLOCK_DISPLAY_CONTROLLER
#define CLOCK_DISPLAY_CONTROLLER 1306
#endif
static_assert(
    CLOCK_DISPLAY_CONTROLLER == 1306 || CLOCK_DISPLAY_CONTROLLER == 1315,
    "CLOCK_DISPLAY_CONTROLLER must be 1306 or 1315");
inline constexpr DisplayController kDisplayController =
    CLOCK_DISPLAY_CONTROLLER == 1315 ? DisplayController::Ssd1315 : DisplayController::Ssd1306;

/** Physical OLED width in pixels. */
inline constexpr std::uint16_t kDisplayWidth = 128U;

/** Physical OLED height in pixels. */
inline constexpr std::uint16_t kDisplayHeight = 64U;

/** I2C address override. Zero enables automatic probing of 0x3C followed by 0x3D. */
#ifndef CLOCK_DISPLAY_I2C_ADDRESS
#define CLOCK_DISPLAY_I2C_ADDRESS 0
#endif
inline constexpr std::uint8_t kDisplayI2cAddress =
    static_cast<std::uint8_t>(CLOCK_DISPLAY_I2C_ADDRESS);

/** I2C bus frequency for display transfers. */
#ifndef CLOCK_DISPLAY_I2C_FREQUENCY_HZ
#define CLOCK_DISPLAY_I2C_FREQUENCY_HZ 400000UL
#endif
inline constexpr std::uint32_t kDisplayI2cFrequencyHz = CLOCK_DISPLAY_I2C_FREQUENCY_HZ;

/** SPI bus frequency for display transfers when SPI transport is selected. */
#ifndef CLOCK_DISPLAY_SPI_FREQUENCY_HZ
#define CLOCK_DISPLAY_SPI_FREQUENCY_HZ 1000000UL
#endif
inline constexpr std::uint32_t kDisplaySpiFrequencyHz = CLOCK_DISPLAY_SPI_FREQUENCY_HZ;

/** Normal display contrast. May be overridden for a particular panel. */
#ifndef CLOCK_DISPLAY_CONTRAST
#define CLOCK_DISPLAY_CONTRAST 0x8F
#endif
inline constexpr std::uint8_t kDisplayContrast = static_cast<std::uint8_t>(CLOCK_DISPLAY_CONTRAST);

/** Reduced display contrast used after prolonged STOP inactivity. */
#ifndef CLOCK_DISPLAY_DIMMED_CONTRAST
#define CLOCK_DISPLAY_DIMMED_CONTRAST 0x10
#endif
inline constexpr std::uint8_t kDisplayDimmedContrast =
    static_cast<std::uint8_t>(CLOCK_DISPLAY_DIMMED_CONTRAST);

/** Maximum editable STOP-idle timeout in minutes. */
inline constexpr std::uint8_t kMaximumScreensaverMinutes = 120U;

/** Frame cadence for STOP-mode idle animations. */
inline constexpr std::uint32_t kScreensaverFrameIntervalMs = 100UL;

/** Duration for which the boot screen remains visible. */
inline constexpr std::uint32_t kBootDurationMs = 1000UL;

/** Minimum time between boot progress framebuffer transfers. */
inline constexpr std::uint32_t kBootRefreshIntervalMs = 25UL;

/** Scheduler service frequency used by the current prerelease timing engine. */
inline constexpr std::uint32_t kSchedulerFrequencyHz = 20000UL;

/** Highest preemption priority reserved for the musical timing scheduler. */
inline constexpr std::uint32_t kSchedulerInterruptPreemptPriority = 0U;

/** Scheduler sub-priority; no peer at the same preemption level is expected. */
inline constexpr std::uint32_t kSchedulerInterruptSubPriority = 0U;

/** Scheduler time quantum in microseconds. */
inline constexpr std::uint32_t kSchedulerTickUs = 1000000UL / kSchedulerFrequencyHz;

/** Minimum interval between ordinary OLED frame transfers. */
inline constexpr std::uint32_t kDisplayRefreshMinimumMs = 30UL;

/** Encoder hold duration that opens the selected channel/global menu. */
inline constexpr std::uint32_t kEncoderLongPressMs = 650UL;


/** Delay before a changed transport state is committed to Flash. */
inline constexpr std::uint32_t kPersistenceCommitDelayMs = 3000UL;

/** Absolute firmware-supported lower master-tempo bound. User limits may be stricter. */
inline constexpr std::uint16_t kSupportedMinimumBpm = 1U;

/** Absolute firmware-supported upper master-tempo bound. User limits may be stricter. */
inline constexpr std::uint16_t kSupportedMaximumBpm = 999U;

/** Maximum user-selectable ONE CLOCK timing humanization in microseconds. */
inline constexpr std::uint16_t kMaximumHumanizeUs = 2000U;

/** Shortest accepted Tap Tempo interval; 60 ms permits the supported 999-BPM ceiling. */
inline constexpr std::uint32_t kTapMinimumIntervalMs = 60UL;

/** Longest accepted Tap Tempo interval; 60 s corresponds to 1 BPM. */
inline constexpr std::uint32_t kTapMaximumIntervalMs = 60000UL;

/** Inactivity interval after which Tap Tempo starts a new measurement sequence. */
inline constexpr std::uint32_t kTapSequenceResetMs = 65000UL;


}  // namespace clockfw::config
