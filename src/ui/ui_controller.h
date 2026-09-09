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
#include "hal/control_panel.h"
#include "services/persistent_state_service.h"
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
     */
    UiController(
        ClockState& state,
        engine::ClockEngine& engine,
        UiRenderer& renderer,
        services::PersistentStateService& persistentState);

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

private:
    /** @brief Routes one encoder detent according to the active screen and edit state. */
    void handleEncoderDelta(std::int8_t delta, bool tapPressed);

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

    /** @brief Feeds one tap timestamp into the rolling tempo estimator. */
    void registerTapTempo(std::uint32_t nowMs);

    /** @brief Opens a text settings page at a specific row and resets edit/scroll state. */
    void openSettingsPage(SettingsPage page, std::uint8_t initialCursor = 0U);

    /** @brief Opens the graphical six-function mode palette and remembers its return screen. */
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

    /** @brief Keeps the selected settings row visible inside the five-row viewport. */
    void normalizeScrollOffset();

    /** @brief Applies the highlighted factory template and returns to performance view. */
    void applySelectedTemplate(std::uint32_t nowMs);

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
    SettingsEditor settingsEditor_;
    services::TapTempo tapTempo_{};
    NavigationState navigation_{};

    bool renderDirty_ = true;
    std::uint32_t lastRenderAtMs_ = 0U;
    std::uint8_t lastRenderedChannelStep_ = 0xFFU;
    bool lastRenderedExternalLocked_ = false;
    bool hasRenderedEngineStatus_ = false;

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
};

}  // namespace clockfw::ui
