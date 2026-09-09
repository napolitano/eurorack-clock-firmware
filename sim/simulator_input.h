/**
 * @file simulator_input.h
 * @brief SDL input mapping for the native front-panel simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <SDL3/SDL.h>

#include "panel_layout.h"
#include "scope_session.h"
#include "scope_timeline.h"
#include "simulator_runtime.h"

namespace clockfw::sim {

/** @brief Maps mouse and keyboard events onto the physical control-panel HAL. */
class SimulatorInput final {
public:
    /** @brief Creates an input mapper using the same runtime geometry as the renderer. */
    explicit SimulatorInput(const layout::PanelLayout& panelLayout);

    /** @brief Processes one SDL event and updates the simulated controls. */
    void handleEvent(const SDL_Event& event, SimulatorRuntime& runtime);

    /** @brief Returns true when the window should close. */
    bool quitRequested() const;

    /** @brief Returns current virtual-time multiplier selected by keyboard shortcuts. */
    double speedMultiplier() const;

    /** @brief Returns true when the developer waveform area should be rendered. */
    bool developerViewEnabled() const;

    /** @brief Updates transport-referenced developer-scope timing after virtual MCU advancement. */
    void updateScope(SimulatorRuntime& runtime);

    /** @brief Returns current developer-scope session state for rendering. */
    scope::SessionView scopeView() const;

    /** @brief Returns and clears the interactive screenshot hotkey request. */
    bool consumeScreenshotRequest();

private:
    /** @brief Applies keyboard down/up state to one physical control or simulator tool. */
    void handleKeyboard(const SDL_KeyboardEvent& keyboard, SimulatorRuntime& runtime);

    /** @brief Applies mouse press/release to front-panel hit targets. */
    void handleMouseButton(const SDL_MouseButtonEvent& button, SimulatorRuntime& runtime);

    /** @brief Routes the wheel to encoder rotation or the virtual SYNC IN source. */
    void handleMouseWheel(const SDL_MouseWheelEvent& wheel, SimulatorRuntime& runtime);

    enum class InputTarget : std::uint8_t { Sync, Reset };

    const layout::PanelLayout& panelLayout_;
    bool quitRequested_ = false;
    bool developerViewEnabled_ = true;
    bool screenshotRequested_ = false;
    double speedMultiplier_ = 1.0;
    std::size_t scopeWindowIndex_ = scope::kDefaultWindowIndex;
    scope::Session scopeSession_{};
    SimButton activeMouseButton_ = SimButton::Encoder;
    bool hasActiveMouseButton_ = false;
    InputTarget selectedInput_ = InputTarget::Sync;
};

}  // namespace clockfw::sim
