/**
 * @file ui_types.h
 * @brief User-interface navigation state and screen identifiers.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstdint>

namespace clockfw::ui {

/** @brief Top-level OLED working screens. */
enum class Screen : std::uint8_t {
    Performance,
    ChannelQuickSelect,
    ModeSelect,
    ModeChangeConfirm,
    Settings,
    SequencerEditor,
    Templates,
    PresetSlots,
    OverwriteConfirm,
    NameEntry
};

/** @brief Hierarchical text settings pages. */
enum class SettingsPage : std::uint8_t {
    Root,
    General,
    Master,
    Sync,
    Preferences,
    Screensaver,
    Info,
    Licenses,
    Updates,
    Channel,
    Rate,
    Clock,
    Euclid,
    Sequencer,
    UnifiedClock,
    DividerBank
};

/** @brief One action exposed by the six-tile graphical mode palette. */
enum class ModeFunction : std::uint8_t {
    Off,
    Clock,
    Euclid,
    Sequencer,
    UnifiedClock,
    DividerBank
};

/** @brief Operation being performed on the explicit user preset slots. */
enum class PresetSlotAction : std::uint8_t { Load, Save };

/** @brief Complete mutable UI navigation state, deliberately separate from musical state. */
struct NavigationState {
    Screen screen = Screen::Performance;
    SettingsPage settingsPage = SettingsPage::Root;
    std::uint8_t selectedChannel = 0U;
    std::uint8_t cursor = 0U;
    Screen modeSelectReturnScreen = Screen::Settings;
    ModeFunction pendingModeFunction = ModeFunction::Clock;
    Screen settingsExitScreen = Screen::Performance;
    std::uint8_t scrollOffset = 0U;
    bool editing = false;
    std::uint8_t sequencerCursor = 0U;
    std::uint8_t sequencerPage = 0U;
    PresetSlotAction presetSlotAction = PresetSlotAction::Load;
    std::uint8_t selectedPresetSlot = 0U;
    std::uint8_t nameCharacterIndex = 0U;
    std::array<char, 17U> presetNameBuffer{};
};

}  // namespace clockfw::ui
