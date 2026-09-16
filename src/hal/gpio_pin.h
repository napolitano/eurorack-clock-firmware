/**
 * @file gpio_pin.h
 * @brief Framework-independent STM32 GPIO pin identifiers used by the CLOCK HAL.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once
#include <cstdint>

namespace clockfw::mcu {
using Pin = std::uint32_t;
inline constexpr Pin PA0=0xA00U, PA1=0xA01U, PA2=0xA02U, PA3=0xA03U, PA4=0xA04U, PA5=0xA05U, PA6=0xA06U, PA7=0xA07U, PA8=0xA08U, PA9=0xA09U, PA10=0xA0AU;
inline constexpr Pin PB0=0xB00U, PB1=0xB01U, PB3=0xB03U, PB4=0xB04U, PB5=0xB05U, PB6=0xB06U, PB7=0xB07U, PB8=0xB08U, PB9=0xB09U, PB10=0xB0AU, PB12=0xB0CU, PB13=0xB0DU, PB14=0xB0EU, PB15=0xB0FU;
inline constexpr Pin kUnassigned = UINT32_MAX;
} // namespace clockfw::mcu
