/**
 * @file oled_display_controller_model.cpp
 * @brief Native-simulator model of the physical OLED transfer orientation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/oled_display.h"

#if defined(CLOCK_SIMULATOR)

namespace clockfw::hal {

const std::array<std::uint8_t, OledDisplay::kFramebufferSize>& OledDisplay::panelFramebufferForSimulator() const {
    // Production keeps the SSD1306/SSD1315 controller in A1/C8 and rotates the
    // bytes sent to display RAM when 180-degree mounting is selected. Apply the
    // same transfer transform here so simulator and hardware share one contract.
    prepareTransportFramebuffer(framebuffer_, simulatorPanelFramebuffer_);
    return simulatorPanelFramebuffer_;
}

}  // namespace clockfw::hal

#endif
