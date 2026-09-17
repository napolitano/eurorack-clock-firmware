/**
 * @file settings_renderer_factory_reset.cpp
 * @brief Guarded factory-reset confirmation rendering.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/settings_renderer_factory_reset.h"

#include <cstdint>

#include "ui_text.h"

namespace clockfw::ui {
void SettingsRenderer::renderFactoryResetConfirm(const NavigationState& navigation) {
    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    const char* const title = text::get(text::TextId::FactoryResetConfirm);
    const hal::TextBounds titleBounds = display_.measureText(title, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>((hal::OledDisplay::kWidth - titleBounds.width) / 2),
        7,
        title);

    const char* const warning = text::get(text::TextId::EraseAllData);
    const hal::TextBounds warningBounds = display_.measureText(warning, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>((hal::OledDisplay::kWidth - warningBounds.width) / 2),
        21,
        warning);

    const char* const choices[2] = {
        text::get(text::TextId::No),
        text::get(text::TextId::Yes)};
    constexpr std::int16_t kChoiceX[2] = {35, 93};
    for (std::uint8_t index = 0U; index < 2U; ++index) {
        const hal::TextBounds bounds = display_.measureText(choices[index], 0, 0);
        const std::int16_t boxWidth = static_cast<std::int16_t>(bounds.width + 12U);
        const std::int16_t boxX = static_cast<std::int16_t>(kChoiceX[index] - boxWidth / 2);
        if (navigation.cursor == index) {
            display_.fillRectangle(boxX, 39, boxWidth, 15);
            display_.setTextColor(hal::PixelColor::Black);
        } else {
            display_.drawRectangle(boxX, 39, boxWidth, 15);
        }
        display_.drawText(
            static_cast<std::int16_t>(kChoiceX[index] - static_cast<std::int16_t>(bounds.width / 2)),
            43,
            choices[index]);
        display_.setTextColor(hal::PixelColor::White);
    }

    display_.present();
}

}  // namespace clockfw::ui
