/**
 * @file clock_application.cpp
 * @brief Top-level composition root and lifecycle coordinator for the firmware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "app/clock_application.h"

#include <cstdint>

#include "config.h"
#include "domain/default_configuration.h"
#include "hal/system_clock.h"

namespace clockfw::app {

namespace {

game::ArcadeLeaderboardStore* selectedLeaderboardFor(
    const config::EasterEgg selected,
    game::ArcadeLeaderboardStore& pixelRaid,
    game::ArcadeLeaderboardStore& formula1,
    game::ArcadeLeaderboardStore& breakout,
    game::ArcadeLeaderboardStore& eggJourney) {
    switch (selected) {
        case config::EasterEgg::PixelRaid: return &pixelRaid;
        case config::EasterEgg::Formula1: return &formula1;
        case config::EasterEgg::Breakout: return &breakout;
        case config::EasterEgg::EggJourney: return &eggJourney;
        case config::EasterEgg::Beatknecht:
        default: return nullptr;
    }
}

}  // namespace


ClockApplication* ClockApplication::activeInstance_ = nullptr;

ClockApplication::ClockApplication()
    : pixelRaidLeaderboard_(persistentStorage_, game::ArcadeGameId::PixelRaid),
      formula1Leaderboard_(persistentStorage_, game::ArcadeGameId::Formula1),
      breakoutLeaderboard_(persistentStorage_, game::ArcadeGameId::Breakout),
      eggJourneyLeaderboard_(persistentStorage_, game::ArcadeGameId::EggJourney),
      selectedLeaderboard_(selectedLeaderboardFor(
          config::kEasterEgg,
          pixelRaidLeaderboard_,
          formula1Leaderboard_,
          breakoutLeaderboard_,
          eggJourneyLeaderboard_)),
      pixelRaidGame_(display_, controlPanel_, gateOutputs_, pixelRaidLeaderboard_),
      formula1Game_(display_, controlPanel_, gateOutputs_, formula1Leaderboard_),
      breakoutGame_(display_, controlPanel_, gateOutputs_, breakoutLeaderboard_),
      eggJourneyGame_(display_, controlPanel_, gateOutputs_, eggJourneyLeaderboard_),
      beatknecht_(display_, controlPanel_, gateOutputs_),
      persistentState_(persistentStorage_),
      customGrooveStore_(persistentStorage_),
      engine_(gateOutputs_),
      externalSyncController_(externalInputs_, engine_),
      renderer_(display_, persistentState_, customGrooveStore_),
      uiController_(state_, engine_, renderer_, persistentState_, customGrooveStore_, selectedLeaderboard_, &externalInputs_, &gateOutputs_) {}


void ClockApplication::loadCustomGrooveLibrary() {
    for (std::uint8_t slotIndex = 0U; slotIndex < kCustomGrooveSlotCount; ++slotIndex) {
        CustomGroovePattern pattern{};
        if (customGrooveStore_.load(slotIndex, pattern)) {
            engine_.updateCustomGrooveSlot(slotIndex, pattern, false);
        }
    }
}

void ClockApplication::begin() {
    ClockState initialState{};
    initializeFactoryDefaults(initialState);
    begin(initialState);
}

void ClockApplication::begin(const ClockState& initialState) {
    state_ = initialState;

    // Restore the complete durable configuration before hardware starts. A valid
    // CURRENT record supersedes factory/default input values, but transport safety
    // is intentionally stronger than persistence: every power-up enters STOP and
    // requires an explicit user action before the clock can produce gates.
    persistentState_.begin();
    (void)persistentState_.restoreCurrentState(state_);
    loadCustomGrooveLibrary();
    state_.transport = TransportState::Stopped;
    activeInstance_ = this;

    // The external gate buffer is disabled before any channel GPIO is configured.
    gateOutputs_.beginDisabled();
    controlPanel_.begin();
    externalInputs_.begin();

    if (!display_.begin()) {
        haltSafely();
    }

    // CLOCK has no hardware RNG. Use boot-time jitter as a per-session seed and
    // mix in stable state so generated Custom Groove names vary naturally without
    // pretending to provide cryptographic randomness. Human save timing is mixed
    // again when each name is generated.
    const std::uint32_t nameSeed = static_cast<std::uint32_t>(hal::SystemClock::microseconds()) ^
        (static_cast<std::uint32_t>(state_.bpm) << 16U) ^ 0x53474E4DU;
    uiController_.seedGeneratedNames(nameSeed);
    synchronizeDevicePreferences(true);

#ifdef CLOCK_SIMULATOR
    // Desktop simulation must not block inside boot delay loops; SDL still needs
    // to process input/rendering so the real boot screen and boot chord are testable.
    simulatorLifecycle_ = SimulatorLifecycle::Booting;
    simulatorBootStartedAtMs_ = hal::SystemClock::milliseconds();
    simulatorLastBootRenderAtMs_ = UINT32_MAX;
    simulatorEncoderHeldForEntireBoot_ =
        controlPanel_.sample(simulatorBootStartedAtMs_).encoderButton.pressed;
    renderer_.renderBootScreen(0U);
#else
    const bool launchEasterEgg = runBootSequence();
    if (launchEasterEgg) {
        // Ranked games create their durable leaderboard record on first launch.
        // This marker later exposes the guarded HI-SCORES reset in Settings.
        if (selectedLeaderboard_ != nullptr) {
            (void)selectedLeaderboard_->markStarted();
        }
        // Boot Easter eggs run before the scheduler. Arcade games keep the logical
        // gate-output stage muted; BEATKNECHT enables it only while deliberately
        // producing its eight gate patterns.
        runSelectedEasterEgg();
    }

    // ClockEngine::begin() receives the already-forced STOP state. Starting the
    // scheduler therefore cannot produce gates even after the logical gate output is enabled.
    engine_.begin(state_);
    externalSyncController_.begin(state_);
    schedulerTimer_.start(config::kSchedulerFrequencyHz, schedulerInterruptThunk);

    gateOutputs_.setAllChannelsLow();
    gateOutputs_.enableOutputStage();
    uiController_.invalidate();
#endif
}

void ClockApplication::runOnce() {
    const std::uint32_t nowMs = hal::SystemClock::milliseconds();
#ifdef CLOCK_SIMULATOR
    if (simulatorLifecycle_ != SimulatorLifecycle::Running) {
        serviceSimulatorStartup(nowMs);
        (void)display_.service();
        return;
    }
#endif
    TransportState externalTransport{};
    if (externalSyncController_.consumeTransportTransition(externalTransport)) {
        state_.transport = externalTransport;
        uiController_.invalidate();
    }

    std::uint32_t externalTapTimestampUs = 0U;
    if (externalSyncController_.consumeTapRequest(externalTapTimestampUs)) {
        uiController_.registerExternalTapTempo(externalTapTimestampUs / 1000U);
    }

    const hal::ControlSample controls = controlPanel_.sample(nowMs);
    const TransportState transportBeforeControls = state_.transport;
    uiController_.processControls(controls, nowMs);
    if (state_.transport != transportBeforeControls) {
        externalSyncController_.notifyManualTransportState(state_.transport);
    }
    synchronizeDevicePreferences();
    externalSyncController_.updateConfiguration(state_);
    // Render visible state changes before any timing-safe Flash flush. This keeps
    // PAUSE/STOP feedback immediate even when the subsequent sector commit takes
    // perceptible time on the F401. PLAY never reaches the Flash write path.
    uiController_.serviceRendering(nowMs);
    // Runtime I2C transfers are intentionally bounded to one short transaction
    // per foreground pass. SPI builds return immediately here because present()
    // already uses the faster synchronous dirty-page path.
    (void)display_.service();
    // STM32F4 stalls instruction/data fetches from Flash while a sector is being
    // erased or programmed. Never start a persistence commit while PLAYING;
    // queued CURRENT/preset writes are flushed once transport is PAUSED/STOPPED.
    persistentState_.service(nowMs, state_.transport != TransportState::Playing);
}

#ifdef CLOCK_SIMULATOR
const ClockState& ClockApplication::stateForSimulator() const {
    return state_;
}

const hal::OledDisplay& ClockApplication::displayForSimulator() const {
    return display_;
}

engine::EngineSnapshot ClockApplication::engineSnapshotForSimulator() const {
    return engine_.snapshot();
}

void ClockApplication::injectExternalSyncLevelForSimulator(const bool high) {
    externalInputs_.injectSyncEdgeForTest(hal::SystemClock::microseconds(), high);
}

void ClockApplication::clearExternalSyncForSimulator() {
    externalSyncController_.clearExternalLock();
}

void ClockApplication::injectExternalResetLevelForSimulator(const bool high) {
    externalInputs_.injectResetEdgeForTest(hal::SystemClock::microseconds(), high);
}

bool ClockApplication::runningForSimulator() const {
    return simulatorLifecycle_ == SimulatorLifecycle::Running;
}

bool ClockApplication::easterEggActiveForSimulator() const {
    return simulatorLifecycle_ == SimulatorLifecycle::EasterEgg;
}

bool ClockApplication::gateOutputEnabledForSimulator() const {
    return gateOutputs_.outputStageEnabled();
}

void ClockApplication::powerOffForSimulator() {
    schedulerTimer_.stop();
    gateOutputs_.setAllChannelsLow();
    gateOutputs_.disableOutputStage();
    display_.clear();
    display_.present();
    simulatorLifecycle_ = SimulatorLifecycle::PoweredOff;
    activeInstance_ = nullptr;
}
#endif

#ifdef CLOCK_SIMULATOR
void ClockApplication::serviceSimulatorStartup(const std::uint32_t nowMs) {
    if (simulatorLifecycle_ == SimulatorLifecycle::PoweredOff) {
        return;
    }
    if (simulatorLifecycle_ == SimulatorLifecycle::EasterEgg) {
        if (!serviceSelectedEasterEggForSimulator(nowMs)) {
            finishSimulatorStartup();
        }
        return;
    }

    std::uint32_t elapsedMs = nowMs - simulatorBootStartedAtMs_;
    if (elapsedMs > config::kBootDurationMs) {
        elapsedMs = config::kBootDurationMs;
    }
    simulatorEncoderHeldForEntireBoot_ = simulatorEncoderHeldForEntireBoot_ &&
        controlPanel_.sample(nowMs).encoderButton.pressed;

    if (simulatorLastBootRenderAtMs_ == UINT32_MAX ||
        elapsedMs - simulatorLastBootRenderAtMs_ >= config::kBootRefreshIntervalMs ||
        elapsedMs == config::kBootDurationMs) {
        renderer_.renderBootScreen(elapsedMs);
        simulatorLastBootRenderAtMs_ = elapsedMs;
    }
    if (elapsedMs < config::kBootDurationMs) {
        return;
    }

    if (simulatorEncoderHeldForEntireBoot_) {
        if (selectedLeaderboard_ != nullptr) {
            (void)selectedLeaderboard_->markStarted();
        }
        simulatorLifecycle_ = SimulatorLifecycle::EasterEgg;
        beginSelectedEasterEggForSimulator();
        return;
    }
    finishSimulatorStartup();
}

void ClockApplication::finishSimulatorStartup() {
    engine_.begin(state_);
    externalSyncController_.begin(state_);
    schedulerTimer_.start(config::kSchedulerFrequencyHz, schedulerInterruptThunk);
    gateOutputs_.setAllChannelsLow();
    gateOutputs_.enableOutputStage();
    uiController_.invalidate();
    simulatorLifecycle_ = SimulatorLifecycle::Running;
}
#endif

void ClockApplication::runSelectedEasterEgg() {
    switch (config::kEasterEgg) {
        case config::EasterEgg::Formula1:
            formula1Game_.run();
            break;
        case config::EasterEgg::Breakout:
            breakoutGame_.run();
            break;
        case config::EasterEgg::EggJourney:
            eggJourneyGame_.run();
            break;
        case config::EasterEgg::Beatknecht:
            beatknecht_.run();
            break;
        case config::EasterEgg::PixelRaid:
            pixelRaidGame_.run();
            break;
        default:
            beatknecht_.run();
            break;
    }
}

#ifdef CLOCK_SIMULATOR
void ClockApplication::beginSelectedEasterEggForSimulator() {
    switch (config::kEasterEgg) {
        case config::EasterEgg::Formula1:
            formula1Game_.beginForSimulator();
            break;
        case config::EasterEgg::Breakout:
            breakoutGame_.beginForSimulator();
            break;
        case config::EasterEgg::EggJourney:
            eggJourneyGame_.beginForSimulator();
            break;
        case config::EasterEgg::Beatknecht:
            beatknecht_.beginForSimulator();
            break;
        case config::EasterEgg::PixelRaid:
            pixelRaidGame_.beginForSimulator();
            break;
        default:
            beatknecht_.beginForSimulator();
            break;
    }
}

bool ClockApplication::serviceSelectedEasterEggForSimulator(const std::uint32_t nowMs) {
    switch (config::kEasterEgg) {
        case config::EasterEgg::Formula1:
            return formula1Game_.serviceForSimulator(nowMs);
        case config::EasterEgg::Breakout:
            return breakoutGame_.serviceForSimulator(nowMs);
        case config::EasterEgg::EggJourney:
            return eggJourneyGame_.serviceForSimulator(nowMs);
        case config::EasterEgg::Beatknecht:
            return beatknecht_.serviceForSimulator(nowMs);
        case config::EasterEgg::PixelRaid:
            return pixelRaidGame_.serviceForSimulator(nowMs);
        default:
            return beatknecht_.serviceForSimulator(nowMs);
    }
}
#endif


void ClockApplication::synchronizeDevicePreferences(const bool force) {
    if (force || !devicePreferencesApplied_ ||
        state_.device.encoderDirectionReversed != appliedEncoderDirectionReversed_) {
        controlPanel_.setEncoderDirectionReversed(state_.device.encoderDirectionReversed);
        appliedEncoderDirectionReversed_ = state_.device.encoderDirectionReversed;
    }
    if (force || !devicePreferencesApplied_ ||
        state_.device.displayRotated180 != appliedDisplayRotated180_) {
        display_.setRotation180(state_.device.displayRotated180);
        appliedDisplayRotated180_ = state_.device.displayRotated180;
        uiController_.invalidate();
    }
    devicePreferencesApplied_ = true;
}

void ClockApplication::schedulerInterruptThunk() {
    // The callback is registered only after begin() assigns activeInstance_, and
    // the firmware composition root has static lifetime. A nullable callback path
    // would therefore hide a lifecycle violation instead of making it safe.
    activeInstance_->externalSyncController_.processSchedulerTick(
        hal::SystemClock::microseconds());
    activeInstance_->engine_.processSchedulerTick();
}

bool ClockApplication::runBootSequence() {
    gateOutputs_.disableOutputStage();
    gateOutputs_.setAllChannelsLow();

    const std::uint32_t startedAtMs = hal::SystemClock::milliseconds();
    bool encoderHeldForEntireBoot = controlPanel_.sample(startedAtMs).encoderButton.pressed;
    std::uint32_t lastRenderedElapsedMs = UINT32_MAX;

    while (true) {
        const std::uint32_t nowMs = hal::SystemClock::milliseconds();
        std::uint32_t elapsedMs = nowMs - startedAtMs;
        if (elapsedMs > config::kBootDurationMs) {
            elapsedMs = config::kBootDurationMs;
        }

        if (lastRenderedElapsedMs == UINT32_MAX ||
            elapsedMs - lastRenderedElapsedMs >= config::kBootRefreshIntervalMs ||
            elapsedMs == config::kBootDurationMs) {
            renderer_.renderBootScreen(elapsedMs);
            lastRenderedElapsedMs = elapsedMs;
        }

        encoderHeldForEntireBoot = encoderHeldForEntireBoot &&
            controlPanel_.sample(nowMs).encoderButton.pressed;
        (void)display_.service();

        if (elapsedMs >= config::kBootDurationMs) {
            break;
        }
        hal::SystemClock::delayMilliseconds(1U);
    }

    gateOutputs_.setAllChannelsLow();
    return encoderHeldForEntireBoot;
}

[[noreturn]] void ClockApplication::haltSafely() {
    schedulerTimer_.stop();
    gateOutputs_.setAllChannelsLow();
    gateOutputs_.disableOutputStage();

    while (true) {
        hal::SystemClock::delayMilliseconds(1000U);
    }
}

}  // namespace clockfw::app
