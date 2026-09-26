/**
 * @file sequencer_editor_primitives.h
 * @brief Compact drawing primitives shared by the Sequencer 2.0 editor.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

#include "domain/sequencer_pattern.h"
#include "hal/oled_display.h"

namespace clockfw::ui::sequencer_editor {

inline constexpr std::int16_t kGridLeft = 4;
inline constexpr std::int16_t kStepPitch = 15;
inline constexpr std::int16_t kGateY = 22;

/** @brief Draws the solid beat and dotted intermediate-step timing grid. */
void drawTimingGrid(hal::OledDisplay& display, std::uint8_t pageBaseStep);
/** @brief Draws the compact percent marker used for a non-selected Probability override. */
void drawProbabilitySymbol(hal::OledDisplay& display, std::int16_t centerX, std::int16_t topY);
/** @brief Draws the selected Probability value as compact 3x5 digits in place of the percent marker. */
void drawProbabilityValue(
    hal::OledDisplay& display, std::int16_t centerX, std::int16_t topY, std::uint8_t probabilityPercent);
/** @brief Draws the one- or two-row Ratchet dot symbol for counts 2..8. */
void drawRatchetSymbol(hal::OledDisplay& display, std::int16_t centerX, std::uint8_t ratchetCount);
/** @brief Draws the independent live-playback triangle below one visible editor step. */
void drawPlayheadTriangle(hal::OledDisplay& display, std::int16_t centerX);
/** @brief Draws the pattern traversal direction pictogram. */
void drawDirectionIcon(
    hal::OledDisplay& display,
    SequencerPlayDirection direction,
    std::int16_t x,
    std::int16_t y);

}  // namespace clockfw::ui::sequencer_editor
