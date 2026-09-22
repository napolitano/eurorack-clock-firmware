/**
 * @file ui_controller.h
 * @brief Navigation, gesture handling, and render invalidation for the OLED user interface.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

#include "domain/clock_types.h"
#include "engine/clock_engine.h"
#include "game/arcade_leaderboard_store.h"
#include "hal/control_panel.h"
#include "hal/external_input_capture.h"
#include "hal/gate_output_driver.h"
#include "services/persistent_state_service.h"
#include "services/custom_groove_store.h"
#include "services/tap_tempo.h"
#include "ui/settings_editor.h"
#include "ui/ui_renderer.h"
#include "ui/ui_types.h"

namespace clockfw::ui {

/**
 * @brief Owns UI navigation, button semantics, screen transitions, and render invalidation.
 *
 * This class contains no GPIO or display-driver calls. Physical inputs arrive as
 * debounced HAL samples, validated value mutation is delegated to SettingsEditor,
 * persistence is delegated to PersistentStateService, and drawing to UiRenderer.
 */
class UiController final {
public:
    /**
     * @brief Constructs the controller around application state and collaborators.
     * @param state Mutable musical/application state.
     * @param engine Real-time timing engine receiving committed configuration changes.
     * @param renderer Stateless screen renderer.
     * @param persistentState Wear-aware persistence service for user transport changes.
     * @param leaderboard Optional ranked Easter-egg leaderboard exposed for user reset.
     */
    UiController(
        ClockState& state,
        engine::ClockEngine& engine,
        UiRenderer& renderer,
        services::PersistentStateService& persistentState,
        game::ArcadeLeaderboardStore* leaderboard = nullptr,
        const hal::ExternalInputCapture* externalInputs = nullptr,
        const hal::GateOutputDriver* gateOutputs = nullptr);

    /**
     * @brief Constructs the controller with Custom Groove persistence enabled.
     * @param state Mutable musical/application state.
     * @param engine Real-time timing engine receiving committed configuration changes.
     * @param renderer Stateless screen renderer.
     * @param persistentState Wear-aware persistence service for user transport changes.
     * @param customGrooveStore Fixed-slot Custom Groove persistence service.
     * @param leaderboard Optional ranked Easter-egg leaderboard exposed for user reset.
     * @param externalInputs Optional external-input diagnostics provider.
     * @param gateOutputs Optional gate-output diagnostics provider.
     */
    UiController(
        ClockState& state,
        engine::ClockEngine& engine,
        UiRenderer& renderer,
        services::PersistentStateService& persistentState,
        services::CustomGrooveStore& customGrooveStore,
        game::ArcadeLeaderboardStore* leaderboard = nullptr,
        const hal::ExternalInputCapture* externalInputs = nullptr,
        const hal::GateOutputDriver* gateOutputs = nullptr);

    /** @brief Marks the current frame as requiring a redraw. */
    void invalidate();

    /**
     * @brief Processes one debounced physical-control sample.
     * @param controls Current HAL control sample.
     * @param nowMs Current monotonic time in milliseconds.
     */
    void processControls(const hal::ControlSample& controls, std::uint32_t nowMs);

    /**
     * @brief Services render invalidation and sends a frame when the refresh limit allows it.
     * @param nowMs Current monotonic time in milliseconds.
     */
    void serviceRendering(std::uint32_t nowMs);

    /** @brief Returns current navigation state for diagnostics and tests. */
    const NavigationState& navigation() const;

    /** @brief Feeds an externally captured TAP edge into the normal Tap Tempo estimator. */
    void registerExternalTapTempo(std::uint32_t timestampMs);

    /** @brief Seeds pseudo-random human-readable default names for this boot session. */
    void seedGeneratedNames(std::uint32_t seed);

private:
    /** @brief Routes one encoder detent according to the active screen and edit state. */
    void handleEncoderDelta(std::int8_t delta, bool tapPressed, bool transportPressed);

    /** @brief Handles encoder push/release events. */
    void handleEncoderButton(const hal::ButtonSample& button, std::uint32_t nowMs);

    /** @brief Handles the transport button on performance and sequencer-editor screens. */
    void handleTransportButton(const hal::ButtonSample& button, std::uint32_t nowMs);

    /** @brief Handles tap tempo, overview TAP-Turn mode selection, and sequencer page-back action. */
    void handleTapButton(const hal::ButtonSample& button, std::uint32_t nowMs);

    /** @brief Handles stop/reset and generic back navigation. */
    void handleResetButton(const hal::ButtonSample& button, std::uint32_t nowMs);

    /** @brief Executes the context-sensitive action for a short encoder press. */
    void handleShortEncoderPress(std::uint32_t nowMs);

    /** @brief Opens the selected channel/global menu after an encoder long press. */
    void handleLongEncoderPress(std::uint32_t nowMs);

    /** @brief Toggles between PLAY and PAUSE without resetting phase. */
    void toggleTransport(std::uint32_t nowMs);

    /** @brief Stops transport and resets the master/global channel phase. */
    void stopTransport(std::uint32_t nowMs);

    /** @brief Feeds one tap timestamp into the rolling tempo estimator and visual tap sequence. */
    void registerTapTempo(std::uint32_t nowMs);

    /** @brief Advances/clears the non-blocking Performance-screen Tap Tempo animation. */
    void serviceTapTempoFeedback(std::uint32_t nowMs);

    /** @brief Opens a text settings page at a specific row and resets edit/scroll state. */
    void openSettingsPage(SettingsPage page, std::uint8_t initialCursor = 0U);

    /** @brief Opens the graphical horizontal mode selector and remembers its return screen. */
    void openModeSelect(Screen returnScreen);

    /** @brief Requests the highlighted mode function, asking for confirmation when it changes state. */
    void commitSelectedMode(std::uint32_t nowMs);

    /** @brief Applies the already confirmed mode function and routes to its most relevant screen. */
    void applyConfirmedMode(std::uint32_t nowMs);

    /** @brief Routes the UI after a mode selection without adding another navigation layer. */
    void navigateAfterModeSelection(ModeFunction modeFunction);

    /** @brief Returns whether the highlighted function already represents the active mode. */
    bool selectedModeIsCurrent(ModeFunction modeFunction) const;

    /** @brief Moves one level back through the settings hierarchy. */
    void backFromSettings();

    /** @brief Activates the currently selected settings row or toggles edit mode. */
    void activateCurrentSetting();

    /** @brief Toggles value editing for a leaf settings row and schedules a redraw. */
    void toggleCurrentSettingEditing();

    /** @brief Activates one row on the settings root page. */
    void activateRootSetting();

    /** @brief Activates one row on the General settings page. */
    void activateGeneralSetting();

    /** @brief Activates one row on the Info settings page. */
    void activateInfoSetting();

    /** @brief Activates one row on the Preferences settings page. */
    void activatePreferencesSetting();

    /** @brief Activates one row on an Independent-channel root page. */
    void activateChannelSetting();

    /** @brief Activates one row on the One Clock root page. */
    void activateUnifiedClockSetting();

    /** @brief Activates one row on a Timing page, including the Groove child page. */
    void activateTimingSetting();

    /** @brief Activates one row on the Groove settings page. */
    void activateGrooveSetting();

    /** @brief Activates one command or value on the Groove Editor menu. */
    void activateGrooveEditorMenuSetting();

    /** @brief Activates one row on the Divider Bank page. */
    void activateDividerBankSetting();

    /** @brief Activates the Sequencer editor entry or one pattern command. */
    void activateSequencerPatternSetting();

    /** @brief Keeps the selected settings row visible inside the five-row viewport. */
    void normalizeScrollOffset();

    /** @brief Applies the highlighted factory template and returns to performance view. */
    void applySelectedTemplate(std::uint32_t nowMs);

    /** @brief Executes or cancels the guarded Top-100 clear action. */
    void confirmHighScoreClear(std::uint32_t nowMs);

    /** @brief Executes or cancels the guarded full factory reset action. */
    void confirmFactoryReset(std::uint32_t nowMs);

    /** @brief Opens the named-preset slot list for an explicit load or save operation. */
    void openPresetSlots(PresetSlotAction action);

    /** @brief Initializes the 16-character high-score-style preset name editor. */
    void preparePresetName();

    /** @brief Rotates the active preset-name character through the supported alphabet. */
    void adjustPresetNameCharacter(std::int8_t delta);

    /** @brief Saves the edited name and complete current state into the selected preset slot. */
    void saveNamedPreset(std::uint32_t nowMs);

    /** @brief Loads the selected preset without causing an implicit transport transition. */
    void loadSelectedPreset(std::uint32_t nowMs);


    /** @brief Opens the graphical Custom Groove editor with a runtime-only live preview. */
    void openGrooveEditor();

    /** @brief Applies the draft to the runtime-only preview channels without persisting it. */
    void updateGroovePreview();

    /** @brief Clears all runtime preview overrides used by the Custom Groove editor. */
    void clearGroovePreview();

    /** @brief Moves the selected Custom Groove marker by one fine/coarse offset increment. */
    void adjustGrooveOffset(std::int8_t delta, bool coarse);

    /** @brief Changes the editor zoom in four-step windows; direction >0 zooms in. */
    void adjustGrooveZoom(std::int8_t direction);

    /** @brief Opens the fixed-record Custom Groove slot list. */
    void openGrooveSlots(GrooveSlotAction action);

    /** @brief Loads the selected Custom Groove slot into the draft and live preview. */
    void loadSelectedGroove();

    /** @brief Activates one stored Custom Groove directly from the normal Groove menu. */
    void activateSelectedGroove(std::uint32_t nowMs);

    /** @brief Prepares the 16-character Custom Groove name editor with a generated default. */
    void prepareGrooveName(std::uint32_t nowMs);

    /** @brief Rotates the active Custom Groove name character. */
    void adjustGrooveNameCharacter(std::int8_t delta);

    /** @brief Saves the current draft into the selected fixed-record Custom Groove slot. */
    void saveCustomGroove(std::uint32_t nowMs, bool keepExistingName);

    /** @brief Commits the selected slot as the active Groove and returns to the Groove page. */
    void activateSavedCustomGroove(std::uint32_t nowMs);

    /** @brief Returns from the editor, restoring the pre-editor state on discard. */
    void leaveGrooveEditor(bool discardChanges);

    /** @brief Queues the complete current configuration for wear-coalesced persistence. */
    void persistCurrentState(std::uint32_t nowMs);

    /** @brief Records front-panel activity and immediately restores a sleeping/dimmed OLED. */
    void noteUserActivity(std::uint32_t nowMs);

    /** @brief Services STOP-mode screensaver, dimming, and OLED power-off policy. */
    bool serviceStopModeDisplay(std::uint32_t nowMs);

    /** @brief Clamps a signed integer to an inclusive range. */
    static int clampInt(int value, int minimum, int maximum);

    ClockState& state_;
    engine::ClockEngine& engine_;
    UiRenderer& renderer_;
    services::PersistentStateService& persistentState_;
    services::CustomGrooveStore* customGrooveStore_ = nullptr;
    game::ArcadeLeaderboardStore* leaderboard_ = nullptr;
    const hal::ExternalInputCapture* externalInputs_ = nullptr;
    const hal::GateOutputDriver* gateOutputs_ = nullptr;
    SettingsEditor settingsEditor_;
    services::TapTempo tapTempo_{};
    NavigationState navigation_{};

    bool tapVisualSequenceActive_ = false;
    std::uint32_t lastTapVisualAtMs_ = 0U;
    std::uint32_t tapIndicatorStartedAtMs_ = 0U;

    bool renderDirty_ = true;
    std::uint32_t lastRenderAtMs_ = 0U;
    std::uint8_t lastRenderedChannelStep_ = 0xFFU;
    bool lastRenderedExternalLocked_ = false;
    bool lastRenderedPreCountActive_ = false;
    std::uint8_t lastRenderedPreCountRemaining_ = 0U;
    bool hasRenderedEngineStatus_ = false;
    DiagnosticSnapshot lastDiagnosticSnapshot_{};
    bool hasRenderedDiagnosticSnapshot_ = false;

    bool settingsChordActive_ = false;
    bool encoderPressTracking_ = false;
    bool encoderLongPressHandled_ = false;
    std::uint32_t encoderPressedAtMs_ = 0U;
    bool modeTapTurnActive_ = false;
    bool activityClockInitialized_ = false;
    bool screensaverActive_ = false;
    bool displayDimmed_ = false;
    bool displayPoweredOff_ = false;
    std::uint32_t lastUserActivityAtMs_ = 0U;
    std::uint32_t lastScreensaverFrameAtMs_ = 0U;
    std::uint32_t screensaverFrameIndex_ = 0U;
    std::uint32_t lastExternalInputActivitySequence_ = 0U;

    GrooveSettings grooveEditorOriginalSettings_{};
    bool grooveEditorDirty_ = false;
    bool grooveLoadPendingAfterDiscard_ = false;
    bool transportPressConsumedByGrooveZoom_ = false;
    std::uint32_t generatedNameSeed_ = 0xC10C2026U;
    std::uint32_t generatedNameSequence_ = 0U;
};

}  // namespace clockfw::ui
