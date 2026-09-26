/**
 * @file menu_model_sequencer.cpp
 * @brief Sequencer 2.0 settings-row formatting.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/menu_model.h"

#include <cstdio>

#include "ui_text.h"

namespace clockfw::ui {
namespace {

/** Copies one localized static string into a bounded menu field. */
void copyText(char* const destination, const std::size_t size, const text::TextId id) {
    std::snprintf(destination, size, "%s", text::get(id));
}

}  // namespace

MenuRow buildSequencerV2MenuRow(
    const std::uint8_t rowIndex,
    const std::uint8_t activeSlot,
    const SequencerPatternV2& pattern) {
    MenuRow row{};
    constexpr text::TextId kLabels[] = {
        text::TextId::Pattern,
        text::TextId::Length,
        text::TextId::Rotate,
        text::TextId::Mode,
        text::TextId::Loop,
        text::TextId::Editor};
    if (rowIndex >= 6U) {
        return row;
    }
    copyText(row.label, sizeof(row.label), kLabels[rowIndex]);
    if (rowIndex == 0U) {
        std::snprintf(
            row.value, sizeof(row.value),
            text::get(text::TextId::PatternSlotFormat), activeSlot + 1U);
    } else if (rowIndex == 1U) {
        std::snprintf(row.value, sizeof(row.value), "%u", pattern.length);
    } else if (rowIndex == 2U) {
        std::snprintf(row.value, sizeof(row.value), "%u", pattern.rotation);
    } else if (rowIndex == 3U) {
        text::TextId id = text::TextId::Forward;
        switch (pattern.direction) {
            case SequencerPlayDirection::Forward: id = text::TextId::Forward; break;
            case SequencerPlayDirection::Reverse: id = text::TextId::Reverse; break;
            case SequencerPlayDirection::PingPong: id = text::TextId::PingPong; break;
            case SequencerPlayDirection::Random: id = text::TextId::Random; break;
        }
        copyText(row.value, sizeof(row.value), id);
    } else if (rowIndex == 4U) {
        copyText(
            row.value, sizeof(row.value),
            pattern.loopMode == SequencerLoopMode::Once ? text::TextId::Once : text::TextId::Loop);
    } else {
        copyText(row.value, sizeof(row.value), text::TextId::Arrow);
    }
    return row;
}

}  // namespace clockfw::ui
