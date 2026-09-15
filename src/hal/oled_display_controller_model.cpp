/**
 * @file oled_display_controller_model.cpp
 * @brief SSD1306/SSD1315 scan-orientation model used only by the native simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/oled_display.h"

#if defined(CLOCK_SIMULATOR)

namespace clockfw::hal {

const std::array<std::uint8_t, OledDisplay::kFramebufferSize>& OledDisplay::panelFramebufferForSimulator() const {
    simulatorPanelFramebuffer_.fill(0U);

    // A1/C8 is CLOCK's canonical 0-degree physical orientation. A0 reverses the
    // segment direction (X), while C0 reverses COM scan direction (Y). Model the
    // two controller axes independently rather than consulting the persisted setting.
    for (std::int16_t y = 0; y < kHeight; ++y) {
        for (std::int16_t x = 0; x < kWidth; ++x) {
            const std::int16_t sourceX = simulatorSegmentRemapA1_
                ? x
                : static_cast<std::int16_t>(kWidth - 1 - x);
            const std::int16_t sourceY = simulatorComScanC8_
                ? y
                : static_cast<std::int16_t>(kHeight - 1 - y);
            const std::size_t sourceIndex = static_cast<std::size_t>(sourceX) +
                static_cast<std::size_t>(sourceY / 8) * static_cast<std::size_t>(kWidth);
            const std::uint8_t sourceMask = static_cast<std::uint8_t>(1U << (sourceY & 7));
            if ((framebuffer_[sourceIndex] & sourceMask) == 0U) {
                continue;
            }
            const std::size_t destinationIndex = static_cast<std::size_t>(x) +
                static_cast<std::size_t>(y / 8) * static_cast<std::size_t>(kWidth);
            simulatorPanelFramebuffer_[destinationIndex] |=
                static_cast<std::uint8_t>(1U << (y & 7));
        }
    }
    return simulatorPanelFramebuffer_;
}

void OledDisplay::observeControllerCommandForSimulator(const std::uint8_t command) {
    if (simulatorPendingCommandParameters_ != 0U) {
        --simulatorPendingCommandParameters_;
        return;
    }

    switch (command) {
        case 0xA0U:
            simulatorSegmentRemapA1_ = false;
            break;
        case 0xA1U:
            simulatorSegmentRemapA1_ = true;
            break;
        case 0xC0U:
            simulatorComScanC8_ = false;
            break;
        case 0xC8U:
            simulatorComScanC8_ = true;
            break;

        // Commands used by CLOCK that consume following parameter bytes. Keeping
        // this state makes values such as contrast 0xA0 remain data rather than
        // being misinterpreted as a segment-remap command by the simulator.
        case 0x21U:  // Column address: start, end.
        case 0x22U:  // Page address: start, end.
            simulatorPendingCommandParameters_ = 2U;
            break;
        case 0x20U:  // Memory addressing mode.
        case 0x81U:  // Contrast.
        case 0x8DU:  // Charge pump.
        case 0xA8U:  // Multiplex ratio.
        case 0xD3U:  // Display offset.
        case 0xD5U:  // Clock divide / oscillator.
        case 0xD9U:  // Precharge period.
        case 0xDAU:  // COM pin configuration.
        case 0xDBU:  // VCOMH deselect level.
            simulatorPendingCommandParameters_ = 1U;
            break;
        default:
            break;
    }
}

}  // namespace clockfw::hal

#endif
