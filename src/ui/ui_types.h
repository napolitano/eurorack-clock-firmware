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

#include "domain/custom_groove.h"

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
    HighScoreClearConfirm,
    FactoryResetConfirm,
    NameEntry,
    GrooveEditor,
    GrooveSlots,
    GrooveOverwriteConfirm,
    GrooveDeleteConfirm,
    GrooveDiscardConfirm,
    GrooveNameEntry,
    InformationPopover
};

/** @brief Hierarchical text settings pages. */
enum class SettingsPage : std::uint8_t {
    Root,
    General,
    InputAssignments,
    Hardware,
    Diagnostics,
    DiagnosticsInputs,
    DiagnosticsOutputs,
    Master,
    Sync,
    Preferences,
    Screensaver,
    Info,
    Licenses,
    Updates,
    Channel,
    ChannelTiming,
    ChannelOutput,
    Clock,
    Euclid,
    Sequencer,
    SequencerPattern,
    UnifiedClock,
    UnifiedTiming,
    UnifiedOutput,
    Groove,
    GrooveEditorMenu,
    DividerBank
};

/** @brief One action exposed by the horizontally scrolling mode selector. */
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

/** @brief Operation being performed on Custom Groove slots. */
enum class GrooveSlotAction : std::uint8_t { LoadEditor, Activate, Save, Rename, Delete };

/** @brief Live digital levels shown by the hardware diagnostics pages. */
struct DiagnosticSnapshot {
    bool syncHigh = false;
    bool resetHigh = false;
    std::array<bool, 8U> outputs{};

    bool operator==(const DiagnosticSnapshot& other) const {
        return syncHigh == other.syncHigh &&
               resetHigh == other.resetHigh &&
               outputs == other.outputs;
    }
    bool operator!=(const DiagnosticSnapshot& other) const { return !(*this == other); }
};

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
    /** 0 hides the Performance-screen tap indicator; 1..4 select shrinking animation frames. */
    std::uint8_t tapIndicatorFrame = 0U;
    /** True when the selected ranked Easter egg has created a durable leaderboard record. */
    bool highScoreResetAvailable = false;
    PresetSlotAction presetSlotAction = PresetSlotAction::Load;
    std::uint8_t selectedPresetSlot = 0U;
    std::uint8_t nameCharacterIndex = 0U;
    std::array<char, 17U> presetNameBuffer{};
    GrooveSlotAction grooveSlotAction = GrooveSlotAction::LoadEditor;
    std::uint8_t selectedGrooveSlot = 0U;
    std::uint8_t grooveCursor = 0U;
    /** 0 = FIT; otherwise visible window size in four-step increments. */
    std::uint8_t grooveZoomSteps = 0U;
    CustomGroovePattern grooveDraft{};
    std::array<char, 17U> grooveNameBuffer{};
    /** Title/value copied from one read-only settings row while its detail popover is open. */
    std::array<char, 18U> informationPopoverTitle{};
    std::array<char, 64U> informationPopoverValue{};
};

}  // namespace clockfw::ui
