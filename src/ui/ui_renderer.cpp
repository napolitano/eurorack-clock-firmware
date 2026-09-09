/**
 * @file ui_renderer.cpp
 * @brief Top-level UI rendering dispatcher and boot-screen implementation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/ui_renderer.h"

#include <cstdio>

#include "config.h"
#include "ui/clock_logo_bitmap.h"
#include "ui_text.h"
#include "version.h"

namespace clockfw::ui {
namespace {

constexpr std::uint16_t kBootVersionBaselineY = 46U;

void drawBootLogoBitmap(hal::OledDisplay& display) {
    for (std::uint16_t row = 0U; row < bootlogo::kHeight; ++row) {
        for (std::uint16_t byteIndex = 0U; byteIndex < bootlogo::kBytesPerRow; ++byteIndex) {
            const std::uint8_t packed = bootlogo::kBitmap[
                static_cast<std::size_t>(row) * bootlogo::kBytesPerRow + byteIndex];
            if (packed == 0U) {
                continue;
            }
            for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
                const std::uint8_t mask = static_cast<std::uint8_t>(0x80U >> bit);
                if ((packed & mask) == 0U) {
                    continue;
                }
                display.setPixel(
                    static_cast<std::int16_t>(bootlogo::kX +
                        static_cast<std::int16_t>(byteIndex * 8U + bit)),
                    static_cast<std::int16_t>(bootlogo::kY + static_cast<std::int16_t>(row)),
                    hal::PixelColor::White);
            }
        }
    }
}

}  // namespace

UiRenderer::UiRenderer(
    hal::OledDisplay& display,
    const services::PersistentStateService& persistentState)
    : display_(display),
      persistentState_(persistentState),
      performanceRenderer_(display),
      channelNavigationRenderer_(display),
      settingsRenderer_(display),
      screensaverRenderer_(display) {}

void UiRenderer::render(
    const ClockState& state,
    const NavigationState& navigation,
    const engine::EngineSnapshot& engineSnapshot) {
    switch (navigation.screen) {
        case Screen::Performance:
            performanceRenderer_.render(state, navigation, engineSnapshot);
            break;
        case Screen::ChannelQuickSelect:
            channelNavigationRenderer_.renderChannelQuickSelect(state, navigation);
            break;
        case Screen::ModeSelect:
            channelNavigationRenderer_.renderModeSelect(navigation);
            break;
        case Screen::ModeChangeConfirm:
            channelNavigationRenderer_.renderModeChangeConfirm(navigation);
            break;
        case Screen::Settings:
            settingsRenderer_.renderSettings(state, navigation);
            break;
        case Screen::SequencerEditor:
            channelNavigationRenderer_.renderSequencerEditor(state, navigation, engineSnapshot);
            break;
        case Screen::Templates:
            settingsRenderer_.renderTemplates(navigation);
            break;
        case Screen::PresetSlots:
            settingsRenderer_.renderPresetSlots(navigation, persistentState_);
            break;
        case Screen::OverwriteConfirm:
            settingsRenderer_.renderOverwriteConfirm(navigation, persistentState_);
            break;
        case Screen::NameEntry:
            settingsRenderer_.renderNameEntry(navigation);
            break;
    }
}

void UiRenderer::renderBootScreen(std::uint32_t elapsedMs) {
    if (elapsedMs > config::kBootDurationMs) {
        elapsedMs = config::kBootDurationMs;
    }

    display_.clear();
    display_.setFont(hal::DisplayFont::Small);
    display_.setTextColor(hal::PixelColor::White);

    drawBootLogoBitmap(display_);

    char versionText[24]{};
    std::snprintf(
        versionText,
        sizeof(versionText),
        text::get(text::TextId::FirmwareVersionFormat),
        CLOCK_FIRMWARE_VERSION);
    const hal::TextBounds versionBounds = display_.measureText(versionText, 0, 0);
    display_.drawText(
        static_cast<std::int16_t>(
            (static_cast<int>(hal::OledDisplay::kWidth) - static_cast<int>(versionBounds.width)) / 2),
        static_cast<std::int16_t>(kBootVersionBaselineY),
        versionText);

    const std::uint16_t progressWidth = static_cast<std::uint16_t>(
        (static_cast<std::uint32_t>(hal::OledDisplay::kWidth) * elapsedMs) / config::kBootDurationMs);
    if (progressWidth > 0U) {
        display_.fillRectangle(
            0,
            hal::OledDisplay::kHeight - 2,
            static_cast<std::int16_t>(progressWidth),
            2,
            hal::PixelColor::White);
    }

    display_.present();
}

void UiRenderer::renderScreensaver(
    const ScreensaverMode mode,
    const std::uint32_t frameIndex) {
    screensaverRenderer_.render(mode, frameIndex);
}

void UiRenderer::setDisplayDimmed(const bool dimmed) {
    display_.setContrast(dimmed ? config::kDisplayDimmedContrast : config::kDisplayContrast);
}

void UiRenderer::setDisplayPower(const bool enabled) {
    display_.setPower(enabled);
}

}  // namespace clockfw::ui
