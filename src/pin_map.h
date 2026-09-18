/**
 * @file pin_map.h
 * @brief Human-readable GPIO and peripheral pin mapping for the STM32F401 CLOCK hardware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 *
 * Edit this file when PCB routing or breadboard wiring changes. Names describe
 * the physical front-panel or circuit function rather than the MCU peripheral.
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "domain/clock_types.h"
#include "hal/gpio_pin.h"

namespace clockfw::pinmap {

/** Sentinel used for optional GPIO signals that are not connected. */
inline constexpr std::uint32_t kUnassignedDigitalPin = mcu::kUnassigned;

/** Encoder phase-A / CLK contact. */
inline constexpr std::uint32_t kEncoderPhaseAPin = mcu::PB6;

/** Encoder phase-B / DT contact. */
inline constexpr std::uint32_t kEncoderPhaseBPin = mcu::PB7;

/** Encoder integrated push switch. */
inline constexpr std::uint32_t kEncoderPushButtonPin = mcu::PB14;

/** Dedicated PLAY/PAUSE front-panel button. */
inline constexpr std::uint32_t kPlayPauseButtonPin = mcu::PA1;

/** Dedicated TAP TEMPO front-panel button. */
inline constexpr std::uint32_t kTapTempoButtonPin = mcu::PA3;

/** Dedicated RESET/BACK front-panel button. */
inline constexpr std::uint32_t kResetBackButtonPin = mcu::PA2;

/** Channel 1 logic output feeding activity LED 1 and 74HCT541 input 1. */
inline constexpr std::uint32_t kChannel1GateLedPin = mcu::PB0;

/** Channel 2 logic output feeding activity LED 2 and 74HCT541 input 2. */
inline constexpr std::uint32_t kChannel2GateLedPin = mcu::PB1;

/** Channel 3 logic output feeding activity LED 3 and 74HCT541 input 3. */
inline constexpr std::uint32_t kChannel3GateLedPin = mcu::PB2;

/** Channel 4 logic output feeding activity LED 4 and 74HCT541 input 4. */
inline constexpr std::uint32_t kChannel4GateLedPin = mcu::PB5;

/** Channel 5 logic output feeding activity LED 5 and 74HCT541 input 5. */
inline constexpr std::uint32_t kChannel5GateLedPin = mcu::PB8;

/** Channel 6 logic output feeding activity LED 6 and 74HCT541 input 6. */
inline constexpr std::uint32_t kChannel6GateLedPin = mcu::PB10;

/** Channel 7 logic output feeding activity LED 7 and 74HCT541 input 7. */
inline constexpr std::uint32_t kChannel7GateLedPin = mcu::PB12;

/** Channel 8 logic output feeding activity LED 8 and 74HCT541 input 8. */
inline constexpr std::uint32_t kChannel8GateLedPin = mcu::PB13;

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

/** Optional active-low gate-buffer output-enable input.
 *
 * The final hardware pin map does not route /OE to the MCU. Gate-output safety
 * is therefore enforced by driving all eight source GPIOs LOW whenever the
 * logical output stage is disabled. Set this to a GPIO only on hardware that
 * explicitly routes a shared active-low /OE signal.
 */
inline constexpr std::uint32_t kGateBufferOutputEnablePin = kUnassignedDigitalPin;

/** Logic level that disables the active-low gate output buffer. */
inline constexpr bool kGateBufferDisabledLevel = true;

/** Logic level that enables the active-low gate output buffer. */
inline constexpr bool kGateBufferEnabledLevel = false;

/**
 * Display wiring defaults. Every display signal can be overridden from
 * PlatformIO build_flags without editing this file. framework-independent encoded STM32 pin identifiers are used so the same
 * map is shared by STM32Cube, host tests, and the simulator.
 */
#ifndef CLOCK_DISPLAY_I2C_SDA_PIN
#define CLOCK_DISPLAY_I2C_SDA_PIN clockfw::mcu::kUnassigned
#endif
#ifndef CLOCK_DISPLAY_I2C_SCL_PIN
#define CLOCK_DISPLAY_I2C_SCL_PIN clockfw::mcu::kUnassigned
#endif
#ifndef CLOCK_DISPLAY_SPI_SCK_PIN
#define CLOCK_DISPLAY_SPI_SCK_PIN clockfw::mcu::PA5
#endif
#ifndef CLOCK_DISPLAY_SPI_MOSI_PIN
#define CLOCK_DISPLAY_SPI_MOSI_PIN clockfw::mcu::PA7
#endif
#ifndef CLOCK_DISPLAY_SPI_CS_PIN
#define CLOCK_DISPLAY_SPI_CS_PIN clockfw::mcu::PA4
#endif
#ifndef CLOCK_DISPLAY_SPI_DC_PIN
#define CLOCK_DISPLAY_SPI_DC_PIN clockfw::mcu::PB9
#endif
#ifndef CLOCK_DISPLAY_RESET_PIN
#define CLOCK_DISPLAY_RESET_PIN clockfw::mcu::PB15
#endif

/** I2C SDA peripheral pin for the selected OLED wiring profile. */
inline constexpr std::uint32_t kDisplayI2cDataPin = CLOCK_DISPLAY_I2C_SDA_PIN;

/** I2C SCL peripheral pin for the selected OLED wiring profile. */
inline constexpr std::uint32_t kDisplayI2cClockPin = CLOCK_DISPLAY_I2C_SCL_PIN;

/** SPI SCK peripheral pin for the selected OLED wiring profile. */
inline constexpr std::uint32_t kDisplaySpiClockPin = CLOCK_DISPLAY_SPI_SCK_PIN;

/** SPI MOSI/DIN peripheral pin for the selected OLED wiring profile. */
inline constexpr std::uint32_t kDisplaySpiDataPin = CLOCK_DISPLAY_SPI_MOSI_PIN;

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
#define CLOCK_EXTERNAL_SYNC_PIN clockfw::mcu::PA8
#endif
#ifndef CLOCK_EXTERNAL_SYNC_JACK_DETECT_PIN
#define CLOCK_EXTERNAL_SYNC_JACK_DETECT_PIN 0xFFFFFFFFUL
#endif
#ifndef CLOCK_EXTERNAL_RESET_PIN
#define CLOCK_EXTERNAL_RESET_PIN clockfw::mcu::PA9
#endif
#ifndef CLOCK_EXTERNAL_RESET_JACK_DETECT_PIN
#define CLOCK_EXTERNAL_RESET_JACK_DETECT_PIN 0xFFFFFFFFUL
#endif

/**
 * External SYNC comparator output on the final hardware pin map.
 * PA8 is sampled through GPIO EXTI on both edges.
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

constexpr bool distinctAssignedPins(const std::array<std::uint32_t, 21U>& pins) {
    for (std::size_t i = 0U; i < pins.size(); ++i) {
        if (pins[i] == kUnassignedDigitalPin) {
            continue;
        }
        for (std::size_t j = i + 1U; j < pins.size(); ++j) {
            if (pins[j] != kUnassignedDigitalPin && pins[i] == pins[j]) {
                return false;
            }
        }
    }
    return true;
}

inline constexpr std::array<std::uint32_t, 21U> kFinalHardwareDigitalPins{{
    kPlayPauseButtonPin, kResetBackButtonPin, kTapTempoButtonPin,
    kDisplaySpiChipSelectPin, kDisplaySpiClockPin, kDisplaySpiDataPin, kDisplaySpiDataCommandPin,
    kChannel1GateLedPin, kChannel2GateLedPin, kChannel3GateLedPin,
    kChannel4GateLedPin, kEncoderPhaseAPin, kEncoderPhaseBPin,
    kChannel5GateLedPin, kChannel6GateLedPin, kChannel7GateLedPin,
    kChannel8GateLedPin, kEncoderPushButtonPin, kDisplayResetPin,
    kExternalSyncSignalPin, kExternalResetSignalPin
}};

#ifndef CLOCK_ENFORCE_FINAL_PIN_MAP
#define CLOCK_ENFORCE_FINAL_PIN_MAP 1
#endif

#if CLOCK_ENFORCE_FINAL_PIN_MAP
static_assert(distinctAssignedPins(kFinalHardwareDigitalPins),
              "Final hardware GPIO map contains a duplicate assignment");
static_assert(kDisplaySpiChipSelectPin == mcu::PA4,
              "Final hardware requires OLED CS on PA4");
static_assert(kDisplaySpiClockPin == mcu::PA5,
              "Final hardware requires OLED CLK/SCK on PA5");
static_assert(kDisplaySpiDataPin == mcu::PA7,
              "Final hardware requires OLED DIN/MOSI on PA7");
static_assert(kDisplaySpiDataCommandPin == mcu::PB9,
              "Final hardware requires OLED D/C on PB9");
static_assert(kDisplayResetPin == mcu::PB15,
              "Final hardware requires OLED RES on PB15");
#endif

}  // namespace clockfw::pinmap
