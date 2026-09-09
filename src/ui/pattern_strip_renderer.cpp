/**
 * @file pattern_strip_renderer.cpp
 * @brief Compact playback pattern visualizations for Euclid and sequencer modes.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/pattern_strip_renderer.h"

#include <algorithm>

#include "clock_core.h"

namespace clockfw::ui {
namespace {

/** First of the two physical OLED rows reserved for current-step/page indication. */
constexpr std::int16_t kBottomIndicatorY = 62;

/** Width and height of one visible sequencer gate cell. */
constexpr std::int16_t kSequencerCellSize = 5;

/** Width and height of one visible Euclidean hit/rest cell. */
constexpr std::int16_t kEuclidCellSize = 5;

}  // namespace

PatternStripRenderer::PatternStripRenderer(hal::OledDisplay& display) : display_(display) {}

void PatternStripRenderer::drawEuclidPattern(
    const EuclidSettings& settings,
    const std::uint8_t currentStep,
    const std::int16_t topY) {
    const std::uint8_t stepCount = settings.steps == 0U ? 1U : settings.steps;
    const std::uint8_t boundedStep = static_cast<std::uint8_t>(currentStep % stepCount);
    const std::uint8_t blockIndex = static_cast<std::uint8_t>(boundedStep / 16U);
    const std::uint8_t blockBase = static_cast<std::uint8_t>(blockIndex * 16U);
    constexpr std::int16_t kSlotWidth = hal::OledDisplay::kWidth / 16;

    // Euclid deliberately uses the same sixteen-cell visual language as SEQ:
    // filled cells are hits, outlined cells are rests, and the active step is
    // underlined. This keeps both rhythm generators immediately comparable.
    for (std::uint8_t slot = 0U; slot < 16U; ++slot) {
        const std::uint8_t absoluteStep = static_cast<std::uint8_t>(blockBase + slot);
        if (absoluteStep >= stepCount) {
            continue;
        }
        const std::int16_t slotX = static_cast<std::int16_t>(slot * kSlotWidth);
        const std::int16_t cellX = static_cast<std::int16_t>(slotX + 1);
        if (core::isEuclideanHit(absoluteStep, settings)) {
            display_.fillRectangle(cellX, topY, kEuclidCellSize, kEuclidCellSize);
        } else {
            display_.drawRectangle(cellX, topY, kEuclidCellSize, kEuclidCellSize);
        }
    }

    drawCurrentStepUnderline(
        static_cast<std::int16_t>((boundedStep % 16U) * kSlotWidth),
        kSlotWidth);
    drawPatternBlockIndicator(stepCount, boundedStep);
}

void PatternStripRenderer::drawSequencerPlaybackBlock(
    const SequencerSettings& settings,
    const std::uint8_t currentStep,
    const std::int16_t topY) {
    const std::uint8_t safeLength = settings.length == 0U ? 1U : settings.length;
    const std::uint8_t boundedStep = static_cast<std::uint8_t>(currentStep % safeLength);
    const std::uint8_t blockIndex = static_cast<std::uint8_t>(boundedStep / 16U);
    const std::uint8_t blockBase = static_cast<std::uint8_t>(blockIndex * 16U);
    constexpr std::int16_t kSlotWidth = hal::OledDisplay::kWidth / 16;

    for (std::uint8_t slot = 0U; slot < 16U; ++slot) {
        const std::uint8_t absoluteStep = static_cast<std::uint8_t>(blockBase + slot);
        const std::int16_t slotX = static_cast<std::int16_t>(slot * kSlotWidth);
        const std::int16_t cellX = static_cast<std::int16_t>(slotX + 1);
        if (absoluteStep >= safeLength) {
            continue;
        }

        if (core::isSequencerHit(absoluteStep, settings)) {
            display_.fillRectangle(cellX, topY, kSequencerCellSize, kSequencerCellSize);
        } else {
            display_.drawRectangle(cellX, topY, kSequencerCellSize, kSequencerCellSize);
        }
    }

    drawCurrentStepUnderline(
        static_cast<std::int16_t>((boundedStep % 16U) * kSlotWidth),
        kSlotWidth);
    drawSequencerBlockIndicator(safeLength, boundedStep);
}

void PatternStripRenderer::drawSequencerBlockIndicator(
    const std::uint8_t sequenceLength,
    const std::uint8_t activeStep) {
    drawPatternBlockIndicator(sequenceLength, activeStep);
}

void PatternStripRenderer::drawPatternBlockIndicator(
    const std::uint8_t sequenceLength,
    const std::uint8_t activeStep) {
    if (sequenceLength <= 16U) {
        return;
    }

    const std::uint8_t blockCount = static_cast<std::uint8_t>((sequenceLength + 15U) / 16U);
    const std::uint8_t activeBlock = static_cast<std::uint8_t>(
        std::min<std::uint8_t>(activeStep / 16U, static_cast<std::uint8_t>(blockCount - 1U)));
    constexpr std::int16_t kGap = 3;
    const std::int16_t totalGapWidth = static_cast<std::int16_t>((blockCount - 1U) * kGap);
    const std::int16_t segmentWidth = static_cast<std::int16_t>(
        (hal::OledDisplay::kWidth - totalGapWidth) / blockCount);

    for (std::uint8_t block = 0U; block < blockCount; ++block) {
        const std::int16_t x = static_cast<std::int16_t>(block * (segmentWidth + kGap));
        if (block == activeBlock) {
            display_.fillRectangle(x, kBottomIndicatorY, segmentWidth, 2);
        } else {
            display_.drawHorizontalLine(x, 63, segmentWidth);
        }
    }
}

void PatternStripRenderer::drawCurrentStepUnderline(
    const std::int16_t slotX,
    const std::int16_t slotWidth) {
    const std::int16_t underlineWidth = std::max<std::int16_t>(1, slotWidth - 1);
    display_.fillRectangle(slotX, 59, underlineWidth, 2);
}

}  // namespace clockfw::ui
