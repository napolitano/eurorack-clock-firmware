/**
 * @file clock_application.h
 * @brief Top-level composition root and lifecycle coordinator for the firmware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include "domain/clock_types.h"
#include "engine/clock_engine.h"
#include "game/breakout_game.h"
#include "game/arcade_leaderboard_store.h"
#include "game/formula1_game.h"
#include "game/egg_journey_game.h"
#include "game/pixel_raid_game.h"
#include "game/beatknecht.h"
#include "hal/control_panel.h"
#include "hal/gate_output_driver.h"
#include "hal/external_input_capture.h"
#include "hal/oled_display.h"
#include "hal/periodic_timer.h"
#include "hal/persistent_storage.h"
#include "services/persistent_state_service.h"
#include "services/custom_groove_store.h"
#include "services/external_sync_controller.h"
#include "ui/ui_controller.h"
#include "ui/ui_renderer.h"

namespace clockfw::app {

/**
 * @brief Wires together HAL, real-time engine, persistence, domain state, and UI components.
 *
 * ClockApplication intentionally contains lifecycle orchestration only. Hardware
 * details remain in HAL classes, timing behavior in ClockEngine, persistence in
 * PersistentStateService, and interaction behavior in UiController.
 */
class ClockApplication final {
public:
    /** @brief Constructs all firmware components in dependency order. */
    ClockApplication();

    /** @brief Initializes factory defaults, persisted metadata, hardware, boot UI, and scheduler safely. */
    void begin();

    /**
     * @brief Initializes the application from an explicit state snapshot.
     * @param initialState State to activate after safe hardware initialization.
     *
     * The supplied transport value is deliberately ignored for startup execution:
     * every boot enters STOP, even when the persisted or supplied state was PLAY.
     */
    void begin(const ClockState& initialState);

    /** @brief Executes one non-real-time application iteration. */
    void runOnce();

#ifdef CLOCK_SIMULATOR
    /** @brief Returns read-only current application state for simulator instrumentation. */
    const ClockState& stateForSimulator() const;

    /** @brief Returns read-only OLED HAL instance exposing the real firmware framebuffer. */
    const hal::OledDisplay& displayForSimulator() const;

    /** @brief Returns current timing-engine snapshot for simulator diagnostics. */
    engine::EngineSnapshot engineSnapshotForSimulator() const;

    /** @brief Injects one already-conditioned SYNC comparator level transition. */
    void injectExternalSyncLevelForSimulator(bool high);

    /** @brief Clears the simulator-provided external-sync lock after a cable/signal loss. */
    void clearExternalSyncForSimulator();

    /** @brief Injects one already-conditioned RST comparator level transition. */
    void injectExternalResetLevelForSimulator(bool high);

    /** @brief Returns true once boot/game handling has completed and the scheduler is active. */
    bool runningForSimulator() const;

    /** @brief Returns true while the configured boot Easter egg owns display/controls. */
    bool easterEggActiveForSimulator() const;

    /** @brief Returns the logical gate-output enable state used by the simulator. */
    bool gateOutputEnabledForSimulator() const;

    /** @brief Places the simulated module into its electrically safe powered-off state. */
    void powerOffForSimulator();

    /** @brief Opens the production General Settings page from a desktop host shell. */
    bool openGeneralSettingsForSimulator();

    /** @brief Returns current UI navigation state for host integration tests. */
    const ui::NavigationState& navigationForSimulator() const;
#endif

private:
    /** @brief Timer ISR thunk used by the HAL periodic-timer callback boundary. */
    static void schedulerInterruptThunk();

    /** @brief Renders the boot screen and returns true only when encoder push stayed held throughout. */
    bool runBootSequence();

    /** @brief Runs the compile-time selected boot Easter egg synchronously. */
    void runSelectedEasterEgg();

#ifdef CLOCK_SIMULATOR
    /** @brief Starts the compile-time selected boot Easter egg in non-blocking simulator mode. */
    void beginSelectedEasterEggForSimulator();

    /** @brief Services the active simulator Easter egg; false means it has exited. */
    bool serviceSelectedEasterEggForSimulator(std::uint32_t nowMs);
#endif

    /** @brief Loads the fixed-record Custom Groove library into the real-time engine cache. */
    void loadCustomGrooveLibrary();

    /** @brief Applies persistent device-local control/display preferences to the HAL only when they change. */
    void synchronizeDevicePreferences(bool force = false);

    /** @brief Enters a safe permanent failure state when the display cannot be initialized. */
    [[noreturn]] void haltSafely();

#ifdef CLOCK_SIMULATOR
    /** @brief Services the non-blocking boot/game lifecycle used only by the native simulator. */
    void serviceSimulatorStartup(std::uint32_t nowMs);

    /** @brief Completes simulator startup and enables the real scheduler/output stage. */
    void finishSimulatorStartup();
#endif

    /** Active composition-root instance used only by the static timer ISR thunk. */
    static ClockApplication* activeInstance_;

    ClockState state_{};
    hal::GateOutputDriver gateOutputs_{};
    hal::ControlPanel controlPanel_{};
    hal::ExternalInputCapture externalInputs_{};
    hal::OledDisplay display_{};
    hal::PeriodicTimer schedulerTimer_{};
    hal::PersistentStorage persistentStorage_{};
    game::ArcadeLeaderboardStore pixelRaidLeaderboard_;
    game::ArcadeLeaderboardStore formula1Leaderboard_;
    game::ArcadeLeaderboardStore breakoutLeaderboard_;
    game::ArcadeLeaderboardStore eggJourneyLeaderboard_;
    game::ArcadeLeaderboardStore* selectedLeaderboard_ = nullptr;
    game::PixelRaidGame pixelRaidGame_;
    game::Formula1Game formula1Game_;
    game::BreakoutGame breakoutGame_;
    game::EggJourneyGame eggJourneyGame_;
    game::Beatknecht beatknecht_;
    services::PersistentStateService persistentState_;
    services::CustomGrooveStore customGrooveStore_;
    engine::ClockEngine engine_;
    services::ExternalSyncController externalSyncController_;
    ui::UiRenderer renderer_;
    ui::UiController uiController_;
    bool appliedEncoderDirectionReversed_ = false;
    bool appliedDisplayRotated180_ = false;
    bool devicePreferencesApplied_ = false;
#ifdef CLOCK_SIMULATOR
    enum class SimulatorLifecycle : std::uint8_t { Booting, EasterEgg, Running, PoweredOff };
    SimulatorLifecycle simulatorLifecycle_ = SimulatorLifecycle::PoweredOff;
    std::uint32_t simulatorBootStartedAtMs_ = 0U;
    std::uint32_t simulatorLastBootRenderAtMs_ = UINT32_MAX;
    bool simulatorEncoderHeldForEntireBoot_ = false;
#endif
};

}  // namespace clockfw::app
