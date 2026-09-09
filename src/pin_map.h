/**
 * @file pin_map.h
 * @brief Human-readable GPIO and peripheral pin mapping for the STM32F401 prototype.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 *
 * Edit this file when PCB routing or breadboard wiring changes. Names describe
 * the physical front-panel or circuit function rather than the MCU peripheral.
 */

#pragma once

#include <Arduino.h>

#include <array>
#include <cstdint>

#include "domain/clock_types.h"

namespace clockfw::pinmap {

/** Sentinel used for optional GPIO signals that are not connected. */
inline constexpr std::uint32_t kUnassignedDigitalPin = UINT32_MAX;

/** Encoder phase-A / CLK contact. */
inline constexpr std::uint32_t kEncoderPhaseAPin = PA0;

/** Encoder phase-B / DT contact. */
inline constexpr std::uint32_t kEncoderPhaseBPin = PA1;

/** Encoder integrated push switch. */
inline constexpr std::uint32_t kEncoderPushButtonPin = PB10;

/** Dedicated PLAY/PAUSE front-panel button. */
inline constexpr std::uint32_t kPlayPauseButtonPin = PB12;

/** Dedicated TAP TEMPO front-panel button. */
inline constexpr std::uint32_t kTapTempoButtonPin = PB13;

/** Dedicated RESET/BACK front-panel button. */
inline constexpr std::uint32_t kResetBackButtonPin = PB14;

/** Channel 1 logic output feeding activity LED 1 and 74HCT244 input 1. */
inline constexpr std::uint32_t kChannel1GateLedPin = PA2;

/** Channel 2 logic output feeding activity LED 2 and 74HCT244 input 2. */
inline constexpr std::uint32_t kChannel2GateLedPin = PA3;

/** Channel 3 logic output feeding activity LED 3 and 74HCT244 input 3. */
inline constexpr std::uint32_t kChannel3GateLedPin = PA8;

/** Channel 4 logic output feeding activity LED 4 and 74HCT244 input 4. */
inline constexpr std::uint32_t kChannel4GateLedPin = PA9;

/** Channel 5 logic output feeding activity LED 5 and 74HCT244 input 5. */
inline constexpr std::uint32_t kChannel5GateLedPin = PA10;

/** Channel 6 logic output feeding activity LED 6 and 74HCT244 input 6. */
inline constexpr std::uint32_t kChannel6GateLedPin = PB0;

/** Channel 7 logic output feeding activity LED 7 and 74HCT244 input 7. */
inline constexpr std::uint32_t kChannel7GateLedPin = PB1;

/** Channel 8 logic output feeding activity LED 8 and 74HCT244 input 8. */
inline constexpr std::uint32_t kChannel8GateLedPin = PB5;

/** Ordered channel-pin table used by the gate-output driver. */
inline constexpr std::array<std::uint32_t, kChannelCount> kGateChannelPins{{
    kChannel1GateLedPin,
    kChannel2GateLedPin,
    kChannel3GateLedPin,
    kChannel4GateLedPin,
    kChannel5GateLedPin,
    kChannel6GateLedPin,
    kChannel7GateLedPin,
    kChannel8GateLedPin
}};

/** Active-low output-enable input shared by both 74HCT244 buffer banks. */
inline constexpr std::uint32_t kGateBufferOutputEnablePin = PB8;

/** Logic level that disables the active-low gate output buffer. */
inline constexpr std::uint8_t kGateBufferDisabledLevel = HIGH;

/** Logic level that enables the active-low gate output buffer. */
inline constexpr std::uint8_t kGateBufferEnabledLevel = LOW;

/**
 * Display wiring defaults. Every display signal can be overridden from
 * PlatformIO build_flags without editing this file. Arduino digital-pin names
 * (PA7, PB9, ...) are used deliberately: SPIClass's constructor accepts
 * digital pin numbers and performs the PinName conversion internally.
 */
#ifndef CLOCK_DISPLAY_I2C_SDA_PIN
#define CLOCK_DISPLAY_I2C_SDA_PIN PB7
#endif
#ifndef CLOCK_DISPLAY_I2C_SCL_PIN
#define CLOCK_DISPLAY_I2C_SCL_PIN PB6
#endif
#ifndef CLOCK_DISPLAY_SPI_SCK_PIN
#define CLOCK_DISPLAY_SPI_SCK_PIN PA5
#endif
#ifndef CLOCK_DISPLAY_SPI_MOSI_PIN
#define CLOCK_DISPLAY_SPI_MOSI_PIN PA7
#endif
#ifndef CLOCK_DISPLAY_SPI_MISO_PIN
#define CLOCK_DISPLAY_SPI_MISO_PIN PA6
#endif
#ifndef CLOCK_DISPLAY_SPI_CS_PIN
#define CLOCK_DISPLAY_SPI_CS_PIN PA4
#endif
#ifndef CLOCK_DISPLAY_SPI_DC_PIN
#define CLOCK_DISPLAY_SPI_DC_PIN PB9
#endif
#ifndef CLOCK_DISPLAY_RESET_PIN
#define CLOCK_DISPLAY_RESET_PIN PB15
#endif

/** I2C SDA peripheral pin for the selected OLED wiring profile. */
inline constexpr std::uint32_t kDisplayI2cDataPin = CLOCK_DISPLAY_I2C_SDA_PIN;

/** I2C SCL peripheral pin for the selected OLED wiring profile. */
inline constexpr std::uint32_t kDisplayI2cClockPin = CLOCK_DISPLAY_I2C_SCL_PIN;

/** SPI SCK peripheral pin for the selected OLED wiring profile. */
inline constexpr std::uint32_t kDisplaySpiClockPin = CLOCK_DISPLAY_SPI_SCK_PIN;

/** SPI MOSI/DIN peripheral pin for the selected OLED wiring profile. */
inline constexpr std::uint32_t kDisplaySpiDataPin = CLOCK_DISPLAY_SPI_MOSI_PIN;

/** SPI MISO pin required by STM32duino; it is electrically unused by the write-only OLED. */
inline constexpr std::uint32_t kDisplaySpiMisoPin = CLOCK_DISPLAY_SPI_MISO_PIN;

/** SPI OLED chip-select GPIO. */
inline constexpr std::uint32_t kDisplaySpiChipSelectPin = CLOCK_DISPLAY_SPI_CS_PIN;

/** SPI OLED data/command GPIO. */
inline constexpr std::uint32_t kDisplaySpiDataCommandPin = CLOCK_DISPLAY_SPI_DC_PIN;

/** OLED reset GPIO. Active-low at the panel. */
inline constexpr std::uint32_t kDisplayResetPin = CLOCK_DISPLAY_RESET_PIN;

static_assert(kDisplaySpiChipSelectPin != kDisplaySpiDataCommandPin,
              "OLED CS and D/C must use different GPIOs");
static_assert(kDisplaySpiChipSelectPin != kDisplayResetPin,
              "OLED CS and RESET must use different GPIOs");
static_assert(kDisplaySpiDataCommandPin != kDisplayResetPin,
              "OLED D/C and RESET must use different GPIOs");

#ifndef CLOCK_EXTERNAL_SYNC_PIN
#define CLOCK_EXTERNAL_SYNC_PIN 0xFFFFFFFFUL
#endif
#ifndef CLOCK_EXTERNAL_SYNC_JACK_DETECT_PIN
#define CLOCK_EXTERNAL_SYNC_JACK_DETECT_PIN 0xFFFFFFFFUL
#endif
#ifndef CLOCK_EXTERNAL_RESET_PIN
#define CLOCK_EXTERNAL_RESET_PIN 0xFFFFFFFFUL
#endif
#ifndef CLOCK_EXTERNAL_RESET_JACK_DETECT_PIN
#define CLOCK_EXTERNAL_RESET_JACK_DETECT_PIN 0xFFFFFFFFUL
#endif

/**
 * External SYNC comparator output. The shipping default intentionally remains
 * unassigned until PCB routing is frozen. The final STM32 pin should expose a
 * hardware timer input-capture channel; GPIO CHANGE IRQ remains the fallback.
 */
inline constexpr std::uint32_t kExternalSyncSignalPin = CLOCK_EXTERNAL_SYNC_PIN;

/** Thonkiconn switch contact used for external-sync jack detection. */
inline constexpr std::uint32_t kExternalSyncJackDetectPin = CLOCK_EXTERNAL_SYNC_JACK_DETECT_PIN;

/** External RST comparator output. */
inline constexpr std::uint32_t kExternalResetSignalPin = CLOCK_EXTERNAL_RESET_PIN;

/** Thonkiconn switch contact used for external-reset jack detection. */
inline constexpr std::uint32_t kExternalResetJackDetectPin = CLOCK_EXTERNAL_RESET_JACK_DETECT_PIN;

static_assert(
    kExternalSyncSignalPin == kUnassignedDigitalPin ||
    kExternalResetSignalPin == kUnassignedDigitalPin ||
    kExternalSyncSignalPin != kExternalResetSignalPin,
    "External SYNC and RST comparator outputs must not share a GPIO");

}  // namespace clockfw::pinmap
