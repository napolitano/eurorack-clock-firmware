/**
 * @file sequencer_editor_primitives.cpp
 * @brief Compact drawing primitives shared by the Sequencer 2.0 editor.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/sequencer_editor_primitives.h"

#include <algorithm>

namespace clockfw::ui::sequencer_editor {
namespace {

constexpr std::int16_t kGridLineTop = 15;
constexpr std::int16_t kGridLineBottom = 50;

void drawDottedVerticalLine(
    hal::OledDisplay& display,
    const std::int16_t x,
    const std::int16_t top,
    const std::int16_t bottom) {
    for (std::int16_t y = top; y <= bottom; y = static_cast<std::int16_t>(y + 2)) {
        display.setPixel(x, y);
    }
}

void drawCenteredDots(
    hal::OledDisplay& display,
    const std::int16_t centerX,
    const std::int16_t y,
    const std::uint8_t count) {
    if (count == 0U) {
        return;
    }
    const std::int16_t startX = static_cast<std::int16_t>(
        centerX - static_cast<std::int16_t>(count - 1U));
    for (std::uint8_t dot = 0U; dot < count; ++dot) {
        display.setPixel(
            static_cast<std::int16_t>(startX + static_cast<std::int16_t>(dot * 2U)), y);
    }
}

}  // namespace

void drawTimingGrid(hal::OledDisplay& display) {
    for (std::uint8_t boundary = 0U; boundary <= 8U; ++boundary) {
        const std::int16_t x = static_cast<std::int16_t>(
            kGridLeft + static_cast<int>(boundary) * kStepPitch);
        const bool beatBoundary = (boundary % 4U) == 0U;
        if (beatBoundary) {
            display.drawVerticalLine(
                x, 11,
                static_cast<std::int16_t>(kGridLineBottom - 11 + 1));
        } else {
            display.drawVerticalLine(x, 13, 2);
            drawDottedVerticalLine(display, x, kGridLineTop, kGridLineBottom);
        }
    }
}

void drawProbabilitySymbol(
    hal::OledDisplay& display,
    const std::int16_t centerX,
    const std::int16_t topY) {
    display.setPixel(static_cast<std::int16_t>(centerX - 2), topY);
    display.setPixel(static_cast<std::int16_t>(centerX - 1), topY);
    display.drawLine(
        static_cast<std::int16_t>(centerX + 2), topY,
        static_cast<std::int16_t>(centerX - 2),
        static_cast<std::int16_t>(topY + 4));
    display.setPixel(
        static_cast<std::int16_t>(centerX + 1),
        static_cast<std::int16_t>(topY + 4));
    display.setPixel(
        static_cast<std::int16_t>(centerX + 2),
        static_cast<std::int16_t>(topY + 4));
}

void drawRatchetSymbol(
    hal::OledDisplay& display,
    const std::int16_t centerX,
    const std::uint8_t ratchetCount) {
    if (ratchetCount <= 1U) {
        return;
    }
    const std::uint8_t firstRow = std::min<std::uint8_t>(ratchetCount, 4U);
    const std::uint8_t secondRow = ratchetCount > 4U
        ? static_cast<std::uint8_t>(ratchetCount - 4U)
        : 0U;
    drawCenteredDots(display, centerX, 43, firstRow);
    drawCenteredDots(display, centerX, 46, secondRow);
}

void drawPlayheadTriangle(hal::OledDisplay& display, const std::int16_t centerX) {
    display.setPixel(centerX, 48);
    display.drawHorizontalLine(static_cast<std::int16_t>(centerX - 1), 49, 3);
    display.drawHorizontalLine(static_cast<std::int16_t>(centerX - 2), 50, 5);
}

void drawDirectionIcon(
    hal::OledDisplay& display,
    const SequencerPlayDirection direction,
    const std::int16_t x,
    const std::int16_t y) {
    switch (direction) {
        case SequencerPlayDirection::Forward:
            display.drawHorizontalLine(x, static_cast<std::int16_t>(y + 3), 7);
            display.drawLine(
                static_cast<std::int16_t>(x + 4), y,
                static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 3));
            display.drawLine(
                static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 3),
                static_cast<std::int16_t>(x + 4), static_cast<std::int16_t>(y + 6));
            break;
        case SequencerPlayDirection::Reverse:
            display.drawHorizontalLine(x, static_cast<std::int16_t>(y + 3), 7);
            display.drawLine(
                static_cast<std::int16_t>(x + 3), y,
                x, static_cast<std::int16_t>(y + 3));
            display.drawLine(
                x, static_cast<std::int16_t>(y + 3),
                static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 6));
            break;
        case SequencerPlayDirection::PingPong:
            display.drawHorizontalLine(x, static_cast<std::int16_t>(y + 1), 7);
            display.drawLine(
                static_cast<std::int16_t>(x + 4), y,
                static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 1));
            display.drawHorizontalLine(x, static_cast<std::int16_t>(y + 5), 7);
            display.drawLine(
                static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 4),
                x, static_cast<std::int16_t>(y + 5));
            break;
        case SequencerPlayDirection::Random:
            display.drawLine(x, y, static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 6));
            display.drawLine(x, static_cast<std::int16_t>(y + 6), static_cast<std::int16_t>(x + 7), y);
            display.drawLine(static_cast<std::int16_t>(x + 5), y, static_cast<std::int16_t>(x + 7), y);
            display.drawLine(static_cast<std::int16_t>(x + 7), y, static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 2));
            display.drawLine(static_cast<std::int16_t>(x + 5), static_cast<std::int16_t>(y + 6), static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 6));
            display.drawLine(static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 4), static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 6));
            break;
    }
}

void drawLoopModeIcon(
    hal::OledDisplay& display,
    const SequencerLoopMode loopMode,
    const std::int16_t x,
    const std::int16_t y) {
    if (loopMode == SequencerLoopMode::Once) {
        display.drawHorizontalLine(x, static_cast<std::int16_t>(y + 3), 6);
        display.drawLine(
            static_cast<std::int16_t>(x + 3), y,
            static_cast<std::int16_t>(x + 6), static_cast<std::int16_t>(y + 3));
        display.drawLine(
            static_cast<std::int16_t>(x + 6), static_cast<std::int16_t>(y + 3),
            static_cast<std::int16_t>(x + 3), static_cast<std::int16_t>(y + 6));
        display.drawVerticalLine(static_cast<std::int16_t>(x + 8), y, 7);
        return;
    }
    display.drawHorizontalLine(static_cast<std::int16_t>(x + 1), y, 6);
    display.drawVerticalLine(x, static_cast<std::int16_t>(y + 1), 5);
    display.drawHorizontalLine(x, static_cast<std::int16_t>(y + 6), 6);
    display.drawVerticalLine(static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 1), 4);
    display.drawLine(
        static_cast<std::int16_t>(x + 5), static_cast<std::int16_t>(y + 3),
        static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 5));
    display.drawLine(
        static_cast<std::int16_t>(x + 7), static_cast<std::int16_t>(y + 5),
        static_cast<std::int16_t>(x + 5), static_cast<std::int16_t>(y + 7));
}

}  // namespace clockfw::ui::sequencer_editor
