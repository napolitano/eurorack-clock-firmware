/**
 * @file platform_io.h
 * @brief Framework-independent GPIO, interrupt, time, and MCU initialization boundary.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once
#include <cstdint>
#include "hal/gpio_pin.h"

namespace clockfw::hal::platform {
using InterruptCallback = void (*)();
enum class InterruptEdge : std::uint8_t { Rising, Falling, Change };
/** @brief Initializes HAL, the 84 MHz system clock, and the 1 MHz TIM5 timebase. */
void initializeMcu();
/** @brief Configures one GPIO as an input with its internal pull-up enabled. */
void configureInputPullup(mcu::Pin pin);
/** @brief Configures one GPIO as a push-pull output. */
void configureOutput(mcu::Pin pin);
/** @brief Reads one GPIO logic level. */
bool read(mcu::Pin pin);
/** @brief Writes one GPIO logic level. */
void write(mcu::Pin pin, bool high);
/** @brief Registers an EXTI callback for one encoded GPIO pin. */
void attachInterrupt(mcu::Pin pin, InterruptCallback callback, InterruptEdge edge);
/**
 * @brief Starts the dedicated quadrature-encoder backend for the supplied A/B pins.
 *
 * Production uses TIM4 encoder mode on PB6/PB7 so contact transitions continue to
 * be counted even while foreground code or interrupts are temporarily delayed.
 * Host/simulator builds provide an equivalent software counter for deterministic tests.
 *
 * @return true when the encoder backend was configured successfully.
 */
bool beginQuadratureEncoder(mcu::Pin phaseA, mcu::Pin phaseB);
/** @brief Returns the wrapping signed-position counter in raw quadrature transitions. */
std::uint32_t quadratureEncoderCount();
/** @brief Returns the current A/B electrical phase as bit1=A and bit0=B. */
std::uint8_t quadratureEncoderState();
/** @brief Returns milliseconds elapsed since MCU initialization. */
std::uint32_t milliseconds();
/** @brief Returns the wrapping 32-bit TIM5 microsecond counter. */
std::uint32_t microseconds();
/** @brief Blocks foreground execution for the requested milliseconds. */
void delayMilliseconds(std::uint32_t durationMs);
/** @brief Disables interrupts and returns the previous PRIMASK state. */
std::uint32_t enterCritical();
/** @brief Restores the PRIMASK state captured by enterCritical(). */
void exitCritical(std::uint32_t previousPrimask);
} // namespace clockfw::hal::platform
