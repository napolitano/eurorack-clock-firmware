/**
 * @file sequencer_editor_primitives.cpp
 * @brief Compact drawing primitives shared by the Sequencer 2.0 editor.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/sequencer_editor_primitives.h"

#include <algorithm>
#include <array>
#include <cstdio>

namespace clockfw::ui::sequencer_editor {
namespace {

constexpr std::int16_t kGridLineTop = 15;
constexpr std::int16_t kGridLineBottom = 50;
constexpr std::int16_t kBeatLabelY = 11;

constexpr std::array<std::uint16_t, 10U> kTinyDigitRows{{
    0b111101101101111U,  // 0
    0b010110010010111U,  // 1
    0b111001111100111U,  // 2
    0b111001111001111U,  // 3
    0b101101111001001U,  // 4
    0b111100111001111U,  // 5
    0b111100111101111U,  // 6
    0b111001001001001U,  // 7
    0b111101111101111U,  // 8
    0b111101111001111U,  // 9
}};

void drawTinyDigit(
    hal::OledDisplay& display,
    const std::int16_t x,
    const std::int16_t y,
    const std::uint8_t digit) {
    if (digit > 9U) {
        return;
    }
    const std::uint16_t bitmap = kTinyDigitRows[digit];
    for (std::uint8_t row = 0U; row < 5U; ++row) {
        for (std::uint8_t column = 0U; column < 3U; ++column) {
            const std::uint8_t bitIndex = static_cast<std::uint8_t>(14U - (row * 3U + column));
            if ((bitmap & static_cast<std::uint16_t>(1U << bitIndex)) != 0U) {
                display.setPixel(
                    static_cast<std::int16_t>(x + column),
                    static_cast<std::int16_t>(y + row));
            }
        }
    }
}

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

void drawTimingGrid(hal::OledDisplay& display, const std::uint8_t pageBaseStep) {
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

    // The ruler labels each visible four-step beat at its leading edge.  The
    // viewport always starts on an eight-step boundary, so both labels are
    // stable while the edit cursor moves within the page.
    for (std::uint8_t visibleBeat = 0U; visibleBeat < 2U; ++visibleBeat) {
        const std::uint16_t beatNumber = static_cast<std::uint16_t>(
            pageBaseStep / 4U + visibleBeat + 1U);
        char label[4]{};
        std::snprintf(label, sizeof(label), "%u", static_cast<unsigned>(beatNumber));
        const std::int16_t beatX = static_cast<std::int16_t>(
            kGridLeft + static_cast<int>(visibleBeat) * 4 * kStepPitch + 2);
        display.drawText(beatX, kBeatLabelY, label);
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

void drawProbabilityValue(
    hal::OledDisplay& display,
    const std::int16_t centerX,
    const std::int16_t topY,
    const std::uint8_t probabilityPercent) {
    const std::uint8_t bounded = std::min<std::uint8_t>(probabilityPercent, 100U);
    std::array<std::uint8_t, 3U> digits{};
    std::uint8_t digitCount = 0U;
    if (bounded >= 100U) {
        digits[0] = 1U;
        digits[1] = 0U;
        digits[2] = 0U;
        digitCount = 3U;
    } else if (bounded >= 10U) {
        digits[0] = static_cast<std::uint8_t>(bounded / 10U);
        digits[1] = static_cast<std::uint8_t>(bounded % 10U);
        digitCount = 2U;
    } else {
        digits[0] = bounded;
        digitCount = 1U;
    }

    const std::int16_t width = static_cast<std::int16_t>(digitCount * 3U + (digitCount - 1U));
    const std::int16_t startX = static_cast<std::int16_t>(centerX - width / 2);
    for (std::uint8_t index = 0U; index < digitCount; ++index) {
        drawTinyDigit(
            display,
            static_cast<std::int16_t>(startX + static_cast<std::int16_t>(static_cast<int>(index) * 4)),
            topY,
            digits[index]);
    }
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
    drawCenteredDots(display, centerX, 34, firstRow);
    drawCenteredDots(display, centerX, 37, secondRow);
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


}  // namespace clockfw::ui::sequencer_editor
