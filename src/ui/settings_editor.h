/**
 * @file settings_editor.h
 * @brief Validated mutation service for user-editable clock and channel settings.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>

#include "domain/clock_types.h"
#include "engine/clock_engine.h"
#include "ui/ui_types.h"

namespace clockfw::ui {

/**
 * @brief Applies validated settings mutations and synchronizes committed changes to the engine.
 *
 * Navigation is intentionally not handled here. The editor knows what a value means,
 * while UiController decides which screen/row is currently active.
 */
class SettingsEditor final {
public:
    /**
     * @brief Constructs the editor around mutable application state and the real-time engine.
     * @param state Mutable musical/application state.
     * @param engine Real-time engine receiving committed changes.
     */
    SettingsEditor(ClockState& state, engine::ClockEngine& engine);

    /**
     * @brief Changes the master tempo by one or more encoder detents.
     * @param delta Signed encoder detent count.
     */
    void changeMasterTempo(std::int8_t delta);

    /**
     * @brief Adjusts one settings value according to page and row semantics.
     * @param page Active settings page.
     * @param rowIndex Selected row on the page.
     * @param channelIndex Selected channel for channel-scoped pages.
     * @param delta Signed encoder detent count.
     */
    void adjust(
        SettingsPage page,
        std::uint8_t rowIndex,
        std::uint8_t channelIndex,
        std::int8_t delta);

    /**
     * @brief Toggles one gate bit in a sequencer pattern.
     * @param channelIndex Selected channel.
     * @param absoluteStep Zero-based absolute step in the 64-step pattern.
     */
    void toggleSequencerStep(std::uint8_t channelIndex, std::uint8_t absoluteStep);

    /**
     * @brief Executes a non-numeric sequencer command.
     * @param channelIndex Selected channel.
     * @param rowIndex Sequencer-settings row containing the command.
     * @return True when a command was recognized and executed.
     */
    bool executeSequencerCommand(std::uint8_t channelIndex, std::uint8_t rowIndex);

private:
    /** @brief Adjusts one master-settings row. */
    void adjustMaster(std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Adjusts one external-sync settings row. */
    void adjustSync(std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Adjusts STOP-mode screensaver and OLED timeout preferences. */
    void adjustScreensaver(std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Adjusts one common channel-settings row. */
    void adjustCommonChannel(std::uint8_t channelIndex, std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Adjusts one rate-settings row. */
    void adjustRate(std::uint8_t channelIndex, std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Adjusts one CLOCK-specific settings row. */
    void adjustClock(std::uint8_t channelIndex, std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Adjusts one EUCLID-specific settings row. */
    void adjustEuclid(std::uint8_t channelIndex, std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Adjusts one SEQ-specific settings row. */
    void adjustSequencer(std::uint8_t channelIndex, std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Adjusts one shared eight-output clock setting. */
    void adjustUnifiedClock(std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Adjusts one fixed divider-bank setting. */
    void adjustDividerBank(std::uint8_t rowIndex, std::int8_t delta);

    /** @brief Clamps a signed integer to an inclusive range. */
    static int clampInt(int value, int minimum, int maximum);

    ClockState& state_;
    engine::ClockEngine& engine_;
    std::uint64_t sequencerClipboard_ = 0U;
    bool sequencerClipboardValid_ = false;
};

}  // namespace clockfw::ui
