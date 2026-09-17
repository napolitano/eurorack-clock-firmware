"""Regression tests for the final CLOCK OLED hardware contract.

These tests intentionally lock the proven STM32 SPI configuration as well as
its physical pins. The display is transmit-only at board level, but SPI1 stays
in normal 2-line master mode; PA6/MISO must simply remain unconfigured.

Author: Axel Napolitano
License: PolyForm-Noncommercial-1.0.0
"""
from __future__ import annotations

import re
import unittest
from pathlib import Path

ROOT = Path(__file__).resolve().parents[2]
TRANSPORT = ROOT / "src" / "hal" / "oled_display_transport.cpp"
PIN_MAP = ROOT / "src" / "pin_map.h"


class DisplayHardwareContractTests(unittest.TestCase):
    def test_spi_keeps_proven_two_line_master_mode(self) -> None:
        source = TRANSPORT.read_text(encoding="utf-8")
        self.assertIn(
            "gDisplaySpi.Init.Direction = SPI_DIRECTION_2LINES;",
            source,
            "OLED SPI direction changed: final hardware must keep the proven 2-line STM32 master mode",
        )
        self.assertNotIn(
            "SPI_DIRECTION_1LINE",
            source,
            "OLED regression: 1-line SPI previously left the real display dark",
        )

    def test_miso_pa6_is_not_configured_by_oled_transport(self) -> None:
        source = TRANSPORT.read_text(encoding="utf-8")
        forbidden = ("kDisplaySpiMisoPin", "GPIO_PIN_6", "mcu::PA6", "PA6/MISO remains completely")
        for token in forbidden:
            with self.subTest(token=token):
                self.assertNotIn(token, source)
        self.assertEqual(source.count("HAL_GPIO_Init(sckPort, &gpio);"), 1)
        self.assertEqual(source.count("HAL_GPIO_Init(mosiPort, &gpio);"), 1)

    def test_final_display_pin_defaults_are_exact(self) -> None:
        source = PIN_MAP.read_text(encoding="utf-8")
        expected = {
            "CLOCK_DISPLAY_SPI_CS_PIN": "clockfw::mcu::PA4",
            "CLOCK_DISPLAY_SPI_SCK_PIN": "clockfw::mcu::PA5",
            "CLOCK_DISPLAY_SPI_MOSI_PIN": "clockfw::mcu::PA7",
            "CLOCK_DISPLAY_SPI_DC_PIN": "clockfw::mcu::PB9",
            "CLOCK_DISPLAY_RESET_PIN": "clockfw::mcu::PB15",
        }
        for macro, pin in expected.items():
            with self.subTest(signal=macro):
                pattern = rf"#define\s+{re.escape(macro)}\s+{re.escape(pin)}\b"
                self.assertRegex(source, pattern)

    def test_final_pin_map_has_compile_time_display_guards(self) -> None:
        source = PIN_MAP.read_text(encoding="utf-8")
        guards = {
            "kDisplaySpiChipSelectPin": "mcu::PA4",
            "kDisplaySpiClockPin": "mcu::PA5",
            "kDisplaySpiDataPin": "mcu::PA7",
            "kDisplaySpiDataCommandPin": "mcu::PB9",
            "kDisplayResetPin": "mcu::PB15",
        }
        for signal, pin in guards.items():
            with self.subTest(signal=signal):
                self.assertIn(f"static_assert({signal} == {pin},", source)


    def test_spi_mode_and_gpio_alternate_function_contract(self) -> None:
        source = TRANSPORT.read_text(encoding="utf-8")
        required = (
            "gDisplaySpi.Init.Mode = SPI_MODE_MASTER;",
            "gDisplaySpi.Init.DataSize = SPI_DATASIZE_8BIT;",
            "gDisplaySpi.Init.CLKPolarity = SPI_POLARITY_LOW;",
            "gDisplaySpi.Init.CLKPhase = SPI_PHASE_1EDGE;",
            "gDisplaySpi.Init.NSS = SPI_NSS_SOFT;",
            "gDisplaySpi.Init.FirstBit = SPI_FIRSTBIT_MSB;",
            "gDisplaySpi.Init.TIMode = SPI_TIMODE_DISABLE;",
            "gDisplaySpi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;",
            "gpio.Mode = GPIO_MODE_AF_PP;",
            "gpio.Pull = GPIO_NOPULL;",
            "gpio.Alternate = GPIO_AF5_SPI1;",
        )
        for token in required:
            with self.subTest(token=token):
                self.assertIn(token, source)

    def test_controller_reset_sequence_keeps_proven_pulse_timing(self) -> None:
        source = TRANSPORT.read_text(encoding="utf-8")
        start = source.index("void OledDisplay::resetController()")
        end = source.index("void OledDisplay::sendCommand", start)
        reset = source[start:end]
        expected_order = (
            "platform::write(pinmap::kDisplayResetPin, true);",
            "platform::delayMilliseconds(20U);",
            "platform::write(pinmap::kDisplayResetPin, false);",
            "platform::delayMilliseconds(10U);",
            "platform::write(pinmap::kDisplayResetPin, true);",
            "platform::delayMilliseconds(20U);",
        )
        cursor = 0
        for token in expected_order:
            with self.subTest(token=token):
                found = reset.find(token, cursor)
                self.assertGreaterEqual(found, 0)
                cursor = found + len(token)

    def test_transmit_path_uses_hal_spi_transmit_without_receive(self) -> None:
        source = TRANSPORT.read_text(encoding="utf-8")
        self.assertIn("HAL_SPI_Transmit(&gDisplaySpi", source)
        self.assertNotIn("HAL_SPI_TransmitReceive(&gDisplaySpi", source)
        self.assertNotIn("HAL_SPI_Receive(&gDisplaySpi", source)


if __name__ == "__main__":
    unittest.main()
