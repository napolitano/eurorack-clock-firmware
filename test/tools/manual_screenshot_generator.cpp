/**
 * @file manual_screenshot_generator.cpp
 * @brief Deterministic simulator-side renderer for the complete manual screenshot catalog.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <array>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <stdexcept>
#include <string>

#include "domain/default_configuration.h"
#include "engine/clock_engine.h"
#include "game/breakout_game.h"
#include "game/arcade_leaderboard_store.h"
#include "game/arcade_shell.h"
#include "game/formula1_game.h"
#include "game/moon_buggy_game.h"
#include "game/pixel_raid_game.h"
#include "game/beatknecht.h"
#include "hal/control_panel.h"
#include "hal/gate_output_driver.h"
#include "hal/oled_display.h"
#include "hal/persistent_storage.h"
#include "services/persistent_state_service.h"
#include "ui/ui_renderer.h"

namespace clockfw::game {
struct Formula1GameTestAccess {
    static void forceCrash(Formula1Game& game, const std::uint32_t nowMs) { game.startCrash(nowMs); }
    static void render(Formula1Game& game) { game.render(); }
};
struct MoonBuggyGameTestAccess {
    static void configureActionFrame(MoonBuggyGame& game) {
        game.playerWorldX_ = 24;
        game.jumpHeightFp_ = 32;
        game.jumpVelocityFp_ = 5;
        game.asteroid_.targetWorldX = 58;
        game.asteroid_.y = 16;
        game.asteroid_.active = true;
        game.score_ = 428U;
    }
    static void forceBroken(MoonBuggyGame& game) {
        game.asteroid_.active = false;
        game.failureMode_ = MoonBuggyGame::FailureMode::Broken;
        game.lives_ = 2U;
        game.awaitingRetry_ = true;
    }
    static void forceFlat(MoonBuggyGame& game) {
        game.asteroid_.active = false;
        game.addImpactCrater(game.playerWorldX_, 9);
        game.lastPhysicsAtMs_ = 500U;
        game.impactFlashUntilMs_ = 900U;
        game.failureMode_ = MoonBuggyGame::FailureMode::Flattened;
        game.lives_ = 1U;
        game.awaitingRetry_ = true;
    }
    static void render(MoonBuggyGame& game) { game.render(); }
};
struct BreakoutGameTestAccess {
    static void configureActionFrame(BreakoutGame& game) {
        game.ballLaunched_ = true;
        game.ballX_ = 74;
        game.ballY_ = 43;
        game.velocityX_ = 2;
        game.velocityY_ = -1;
        game.applyExtra(BreakoutGame::ExtraType::LargeBat, 1000U);
        game.fallingExtra_.type = BreakoutGame::ExtraType::FastBat;
        game.fallingExtra_.x = 94;
        game.fallingExtra_.y = 45;
        game.fallingExtra_.active = true;
    }
    static void render(BreakoutGame& game) { game.render(); }
};
}  // namespace clockfw::game

namespace {

using namespace clockfw;

class ScreenshotCatalog final {
public:
    ScreenshotCatalog(
        std::filesystem::path outputDirectory,
        hal::OledDisplay& display,
        hal::PersistentStorage& storage,
        services::PersistentStateService& persistentState)
        : outputDirectory_(std::move(outputDirectory)),
          display_(display),
          renderer_(display, persistentState),
          persistentState_(persistentState),
          storage_(storage) {
        std::filesystem::create_directories(outputDirectory_);
        manifest_.open(outputDirectory_ / "manifest.tsv", std::ios::trunc);
        if (!manifest_) {
            throw std::runtime_error("Unable to create screenshot manifest");
        }
    }

    void renderAll() {
        renderBootStates();
        renderPerformanceStates();
        renderNavigationStates();
        renderSettingsStates();
        renderPresetStates();
        renderScreensavers();
        renderEasterEggs();
        renderPowerOff();
    }

private:
    void reset() {
        initializeFactoryDefaults(state_);
        navigation_ = ui::NavigationState{};
        snapshot_ = engine::EngineSnapshot{};
        state_.bpm = 124U;
    }

    void capture(const char* const name, const char* const description) {
        const std::filesystem::path path = outputDirectory_ / (std::string(name) + ".pgm");
        writePgm(path);
        manifest_ << name << '\t' << description << '\n';
    }

    void renderCurrent(
        const char* const name,
        const char* const description) {
        renderer_.render(state_, navigation_, snapshot_);
        capture(name, description);
    }

    void renderBootStates() {
        constexpr std::array<std::uint32_t, 5U> kBootTimes{{0U, 250U, 500U, 750U, 1000U}};
        constexpr std::array<const char*, 5U> kNames{{
            "boot-000", "boot-250", "boot-500", "boot-750", "boot-1000"}};
        constexpr std::array<const char*, 5U> kDescriptions{{
            "Boot screen at power-on",
            "Boot screen at 25 percent",
            "Boot screen at 50 percent",
            "Boot screen at 75 percent",
            "Boot screen at completion"}};
        for (std::size_t index = 0U; index < kBootTimes.size(); ++index) {
            renderer_.renderBootScreen(kBootTimes[index]);
            capture(kNames[index], kDescriptions[index]);
        }
    }

    void renderPerformanceStates() {
        reset();
        state_.transport = TransportState::Playing;
        state_.channels[0].common.mode = ChannelMode::Clock;
        state_.channels[0].common.swingPercent = 12U;
        state_.channels[0].common.rate.mode = ClockRatioMode::Divide;
        state_.channels[0].common.rate.factor = 4U;
        renderCurrent("performance-independent-clock-play", "Independent Clock channel while playing");

        state_.transport = TransportState::Paused;
        renderCurrent("performance-independent-clock-pause", "Independent Clock channel while paused");

        state_.transport = TransportState::Stopped;
        state_.bpm = 20U;
        renderCurrent("performance-independent-clock-stop", "Independent Clock channel while stopped");

        reset();
        state_.transport = TransportState::Playing;
        state_.channels[0].common.mode = ChannelMode::Off;
        renderCurrent("performance-independent-off", "Independent channel disabled");

        reset();
        state_.transport = TransportState::Playing;
        state_.channels[2].common.mode = ChannelMode::Euclid;
        state_.channels[2].common.swingPercent = 18U;
        state_.channels[2].euclid.steps = 13U;
        state_.channels[2].euclid.hits = 5U;
        state_.channels[2].euclid.rotation = 2U;
        navigation_.selectedChannel = 2U;
        snapshot_.channelStep[2] = 6U;
        renderCurrent("performance-independent-euclid-play", "Independent Euclid channel while playing");

        reset();
        state_.transport = TransportState::Playing;
        state_.channels[6].common.mode = ChannelMode::Sequencer;
        state_.channels[6].sequencer.length = 64U;
        state_.channels[6].sequencer.pattern = 0xA55A0F0F33CC5AA5ULL;
        navigation_.selectedChannel = 6U;
        snapshot_.channelStep[6] = 34U;
        renderCurrent("performance-independent-sequencer-play", "Independent 64-step Sequencer while playing");

        reset();
        state_.transport = TransportState::Playing;
        state_.operatingMode = OperatingMode::UnifiedClock;
        state_.unifiedClock.swingPercent = 9U;
        state_.unifiedClock.humanizeUs = 2000U;
        renderCurrent("performance-one-clock-play", "One Clock mode with Humanize while playing");

        reset();
        state_.transport = TransportState::Playing;
        state_.operatingMode = OperatingMode::DividerBank;
        state_.dividerBank.bank = DividerBank::Primes;
        navigation_.selectedChannel = 7U;
        renderCurrent("performance-divider-bank-play", "Divider Bank in prime-divider mode while playing");

        reset();
        state_.transport = TransportState::Playing;
        state_.source = ClockSource::External;
        snapshot_.externalLocked = false;
        renderCurrent("performance-external-unlocked", "External clock source before lock");

        snapshot_.externalLocked = true;
        snapshot_.externalBpmMilli = 128000U;
        renderCurrent("performance-external-locked", "External clock source with lock");

        reset();
        state_.transport = TransportState::Playing;
        state_.source = ClockSource::Auto;
        snapshot_.externalLocked = false;
        renderCurrent("performance-auto-internal", "AUTO clock source using the internal master");

        snapshot_.externalLocked = true;
        snapshot_.externalBpmMilli = 128000U;
        renderCurrent("performance-auto-external", "AUTO clock source locked to external timing");
    }

    void renderNavigationStates() {
        reset();
        state_.channels[0].common.mode = ChannelMode::Clock;
        state_.channels[1].common.mode = ChannelMode::Euclid;
        state_.channels[2].common.mode = ChannelMode::Sequencer;
        state_.channels[3].common.mode = ChannelMode::Off;
        navigation_.screen = ui::Screen::ChannelQuickSelect;
        navigation_.cursor = 2U;
        renderCurrent("channel-overview-independent", "Independent-mode eight-channel overview");

        state_.operatingMode = OperatingMode::UnifiedClock;
        renderCurrent("channel-overview-one-clock", "Global overview in One Clock mode");

        state_.operatingMode = OperatingMode::DividerBank;
        renderCurrent("channel-overview-divider-bank", "Global overview in Divider Bank mode");

        constexpr std::array<const char*, 6U> kModeNames{{
            "one-clock", "divider-bank", "clock", "euclid", "sequencer", "off"}};
        for (std::uint8_t mode = 0U; mode < 6U; ++mode) {
            reset();
            navigation_.screen = ui::Screen::ModeSelect;
            navigation_.cursor = mode;
            renderCurrent(
                (std::string("mode-select-") + kModeNames[mode]).c_str(),
                "Six-function mode palette");
        }

        reset();
        navigation_.screen = ui::Screen::ModeChangeConfirm;
        navigation_.pendingModeFunction = ui::ModeFunction::DividerBank;
        navigation_.cursor = 0U;
        renderCurrent("mode-change-confirm-no", "Mode-change confirmation with safe default NO");
        navigation_.cursor = 1U;
        renderCurrent("mode-change-confirm-yes", "Mode-change confirmation with YES selected");

        reset();
        state_.channels[0].common.mode = ChannelMode::Sequencer;
        state_.channels[0].sequencer.length = 64U;
        state_.channels[0].sequencer.pattern = 0xA55A0F0F33CC5AA5ULL;
        navigation_.screen = ui::Screen::SequencerEditor;
        navigation_.sequencerPage = 2U;
        navigation_.sequencerCursor = 4U;
        snapshot_.channelStep[0] = 36U;
        renderCurrent("sequencer-editor", "64-step Sequencer editor");

        reset();
        navigation_.screen = ui::Screen::Templates;
        navigation_.cursor = 2U;
        navigation_.scrollOffset = 0U;
        renderCurrent("templates", "Factory template selection");
    }

    void renderSettingsStates() {
        constexpr std::array<ui::SettingsPage, 16U> kPages{{
            ui::SettingsPage::Root,
            ui::SettingsPage::General,
            ui::SettingsPage::Master,
            ui::SettingsPage::Sync,
            ui::SettingsPage::Preferences,
            ui::SettingsPage::Screensaver,
            ui::SettingsPage::Info,
            ui::SettingsPage::Licenses,
            ui::SettingsPage::Updates,
            ui::SettingsPage::Channel,
            ui::SettingsPage::Rate,
            ui::SettingsPage::Clock,
            ui::SettingsPage::Euclid,
            ui::SettingsPage::Sequencer,
            ui::SettingsPage::UnifiedClock,
            ui::SettingsPage::DividerBank}};
        constexpr std::array<const char*, 16U> kNames{{
            "settings-root",
            "settings-general",
            "settings-master",
            "settings-sync",
            "settings-preferences",
            "settings-screensaver",
            "settings-info",
            "settings-licenses",
            "settings-updates",
            "settings-channel",
            "settings-rate",
            "settings-clock",
            "settings-euclid",
            "settings-sequencer",
            "settings-one-clock",
            "settings-divider-bank"}};
        constexpr std::array<const char*, 16U> kDescriptions{{
            "Settings root",
            "General settings",
            "Master clock settings",
            "External Sync settings",
            "Preset settings",
            "Screensaver settings",
            "Firmware information",
            "Licensing information",
            "Scannable update QR code",
            "Selected-channel settings",
            "Selected-channel rate settings",
            "Clock generator settings",
            "Euclid generator settings",
            "Sequencer settings",
            "One Clock settings",
            "Divider Bank settings"}};

        for (std::size_t index = 0U; index < kPages.size(); ++index) {
            reset();
            navigation_.screen = ui::Screen::Settings;
            navigation_.settingsPage = kPages[index];
            navigation_.cursor = 0U;
            if (kPages[index] == ui::SettingsPage::Euclid) {
                state_.channels[0].common.mode = ChannelMode::Euclid;
            } else if (kPages[index] == ui::SettingsPage::Sequencer) {
                state_.channels[0].common.mode = ChannelMode::Sequencer;
            } else if (kPages[index] == ui::SettingsPage::UnifiedClock) {
                state_.operatingMode = OperatingMode::UnifiedClock;
                state_.unifiedClock.humanizeUs = 1000U;
            } else if (kPages[index] == ui::SettingsPage::DividerBank) {
                state_.operatingMode = OperatingMode::DividerBank;
            }
            renderCurrent(kNames[index], kDescriptions[index]);
        }

        reset();
        navigation_.screen = ui::Screen::Settings;
        navigation_.settingsPage = ui::SettingsPage::Master;
        navigation_.cursor = 0U;
        navigation_.editing = true;
        renderCurrent("settings-editing", "Settings value while actively editing");
    }

    void renderPresetStates() {
        reset();
        (void)persistentState_.savePreset(2U, "LIVE SET", state_);

        navigation_.screen = ui::Screen::PresetSlots;
        navigation_.presetSlotAction = ui::PresetSlotAction::Load;
        navigation_.cursor = 2U;
        renderCurrent("preset-load", "User preset load list");

        navigation_.presetSlotAction = ui::PresetSlotAction::Save;
        renderCurrent("preset-save", "User preset save list");

        navigation_.screen = ui::Screen::OverwriteConfirm;
        navigation_.selectedPresetSlot = 2U;
        navigation_.cursor = 0U;
        renderCurrent("preset-overwrite-confirm", "Occupied preset overwrite confirmation");

        navigation_.screen = ui::Screen::NameEntry;
        navigation_.presetNameBuffer.fill(' ');
        navigation_.presetNameBuffer.back() = '\0';
        constexpr char kExamplePresetName[] = "LIVE SET";
        for (std::size_t index = 0U; index < sizeof(kExamplePresetName) - 1U; ++index) {
            navigation_.presetNameBuffer[index] = kExamplePresetName[index];
        }
        navigation_.nameCharacterIndex = 5U;
        renderCurrent("preset-name-entry", "Preset-name character-band editor");
    }

    void renderScreensavers() {
        renderer_.renderScreensaver(ScreensaverMode::Clock, 6U);
        capture("screensaver-clock", "Clock screensaver with eight oscilloscope-style channel traces");
        renderer_.renderScreensaver(ScreensaverMode::Plug, 6U);
        capture("screensaver-plug", "Plug screensaver with a damped plucked string");
        renderer_.renderScreensaver(ScreensaverMode::Heartbeat, 2U);
        capture("screensaver-heartbeat", "Heartbeat screensaver pulse frame");
        renderer_.renderScreensaver(ScreensaverMode::Acid, 2U);
        capture("screensaver-acid", "Acid screensaver bouncing and rotating smiley");
        renderer_.renderScreensaver(ScreensaverMode::Spectrum, 12U);
        capture("screensaver-spectrum", "Synthetic spectrum analyser with peak hold and decay");
        renderer_.renderScreensaver(ScreensaverMode::Field, 24U);
        capture("screensaver-field", "Dense smooth scalar-field isocontours around moving attractors");
        for (std::uint32_t frame = 0U; frame <= 240U; ++frame) renderer_.renderScreensaver(ScreensaverMode::Blox, frame);
        capture("screensaver-blox", "Blox falling triangle-built bodies accumulating on the display");
        renderer_.renderScreensaver(ScreensaverMode::Matrix, 37U);
        capture("screensaver-matrix", "Original monochrome procedural digital rain");
        renderer_.renderScreensaver(ScreensaverMode::CubeCover, 75U);
        capture("screensaver-cube-cover", "Cube Cover filling the OLED with 8x8 cube tiles");
        renderer_.renderScreensaver(ScreensaverMode::Fractal, 6U);
        capture("screensaver-fractal", "Fractal screensaver frame");
        renderer_.renderScreensaver(ScreensaverMode::Orbit, 6U);
        capture("screensaver-orbit", "Orbit screensaver frame");
    }

    void renderEasterEggs() {
        hal::ControlPanel controls;
        hal::GateOutputDriver gateOutputs;
        controls.begin();
        gateOutputs.beginDisabled();

        game::ArcadeLeaderboardStore pixelStore(storage_, game::ArcadeGameId::PixelRaid);
        game::ArcadeLeaderboardStore formulaStore(storage_, game::ArcadeGameId::Formula1);
        game::ArcadeLeaderboardStore breakoutStore(storage_, game::ArcadeGameId::Breakout);
        game::ArcadeLeaderboardStore eggStore(storage_, game::ArcadeGameId::EggJourney);

        const std::array<char, 4U> axl{{'A','X','L','\0'}};
        const std::array<char, 4U> egg{{'E','G','G','\0'}};
        (void)pixelStore.insertAndSave(48290U, axl);
        (void)pixelStore.insertAndSave(39120U, egg);

        game::ArcadeShell pixelIntro(display_, game::ArcadeTitle::PixelRaid, &pixelStore);
        pixelIntro.begin(0U); pixelIntro.render(1200U); capture("pixel-raid-intro", "Pixel Raid individual retro intro with marquee");
        game::PixelRaidGame pixelRaid(display_, controls, gateOutputs, pixelStore);
        pixelRaid.run();
        capture("pixel-raid", "Pixel Raid boot Easter egg");

        game::ArcadeShell formulaIntro(display_, game::ArcadeTitle::Formula1, &formulaStore);
        formulaIntro.begin(0U); formulaIntro.render(1200U); capture("formula-1-intro", "Formula 1 individual retro intro with marquee");
        game::Formula1Game formula1(display_, controls, gateOutputs, formulaStore);
        formula1.run();
        capture("formula-1", "Formula 1 boot Easter egg");
        game::Formula1GameTestAccess::forceCrash(formula1, 1000U);
        game::Formula1GameTestAccess::render(formula1);
        capture("formula-1-crash", "Formula 1 visible crash burst state");

        game::ArcadeShell breakoutIntro(display_, game::ArcadeTitle::Breakout, &breakoutStore);
        breakoutIntro.begin(0U); breakoutIntro.render(1200U); capture("breakout-intro", "Breakout individual retro intro with marquee");
        game::BreakoutGame breakout(display_, controls, gateOutputs, breakoutStore);
        breakout.run();
        capture("breakout", "Breakout boot Easter egg before Tap launch");
        game::BreakoutGameTestAccess::configureActionFrame(breakout);
        game::BreakoutGameTestAccess::render(breakout);
        capture("breakout-modifier", "Breakout action frame with enlarged paddle and falling speed modifier");

        game::ArcadeShell eggIntro(display_, game::ArcadeTitle::EggJourney, &eggStore);
        eggIntro.begin(0U); eggIntro.render(1200U); capture("egg-journey-intro", "Egg Journey individual retro intro with marquee");
        game::MoonBuggyGame moonBuggy(display_, controls, gateOutputs, eggStore);
        moonBuggy.run();
        game::MoonBuggyGameTestAccess::configureActionFrame(moonBuggy);
        game::MoonBuggyGameTestAccess::render(moonBuggy);
        capture("egg-journey", "Egg Journey boot Easter egg with jumping egg and incoming asteroid");
        game::MoonBuggyGameTestAccess::forceBroken(moonBuggy);
        game::MoonBuggyGameTestAccess::render(moonBuggy);
        capture("egg-journey-broken", "Egg Journey crater crash with broken egg shell");
        game::MoonBuggyGameTestAccess::forceFlat(moonBuggy);
        game::MoonBuggyGameTestAccess::render(moonBuggy);
        capture("egg-journey-flat", "Egg Journey asteroid collision with flattened egg");

        game::ArcadeShell beatIntro(display_, game::ArcadeTitle::Beatknecht, nullptr);
        beatIntro.begin(0U); beatIntro.render(1200U); capture("beatknecht-intro", "BEATKNECHT individual retro intro with marquee");
        game::Beatknecht drummer(display_, controls, gateOutputs);
        drummer.run();
        capture("beatknecht", "Eight-channel BEATKNECHT gate-pattern Easter egg");

        game::ArcadeShell nameEntry(display_, game::ArcadeTitle::PixelRaid, &pixelStore);
        nameEntry.begin(0U);
        nameEntry.startImmediatelyForTest();
        nameEntry.finishRun(99999U);
        nameEntry.render(0U);
        capture("arcade-name-entry", "Shared three-letter initials editor for a qualifying Top-100 score");

        game::ArcadeShell leaderboard(display_, game::ArcadeTitle::PixelRaid, &pixelStore);
        leaderboard.begin(0U);
        leaderboard.forceLeaderboardForTest(48290U, 0);
        leaderboard.render(0U);
        capture("arcade-top-100", "Scrollable shared Top-100 leaderboard presentation");
    }

    void renderPowerOff() {
        display_.clear();
        display_.present();
        capture("power-off", "Display appearance while module power is off");
    }

    void writePgm(const std::filesystem::path& path) const {
        const auto& framebuffer = display_.framebufferForTest();
        std::ofstream output(path, std::ios::binary);
        if (!output) {
            throw std::runtime_error("Unable to create manual screenshot image");
        }

        output << "P5\n" << hal::OledDisplay::kWidth << ' ' << hal::OledDisplay::kHeight << "\n255\n";
        for (std::int16_t y = 0; y < hal::OledDisplay::kHeight; ++y) {
            for (std::int16_t x = 0; x < hal::OledDisplay::kWidth; ++x) {
                const std::size_t byteIndex = static_cast<std::size_t>(x) +
                    static_cast<std::size_t>(y / 8) * static_cast<std::size_t>(hal::OledDisplay::kWidth);
                const std::uint8_t bitMask = static_cast<std::uint8_t>(1U << (y & 7));
                const std::uint8_t pixel = (framebuffer[byteIndex] & bitMask) != 0U ? 255U : 0U;
                output.write(reinterpret_cast<const char*>(&pixel), 1);
            }
        }
    }

    std::filesystem::path outputDirectory_;
    hal::OledDisplay& display_;
    ui::UiRenderer renderer_;
    services::PersistentStateService& persistentState_;
    hal::PersistentStorage& storage_;
    ClockState state_{};
    ui::NavigationState navigation_{};
    engine::EngineSnapshot snapshot_{};
    std::ofstream manifest_{};
};

}  // namespace

int main(int argc, char** argv) {
    if (argc != 2) {
        return 2;
    }

    try {
        clockfw::hal::OledDisplay display;
        clockfw::hal::PersistentStorage storage;
        clockfw::services::PersistentStateService persistentState(storage);
        persistentState.begin();

        ScreenshotCatalog catalog(argv[1], display, storage, persistentState);
        catalog.renderAll();
        return 0;
    } catch (const std::exception&) {
        return 1;
    }
}
