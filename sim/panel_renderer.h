/**
 * @file panel_renderer.h
 * @brief SDL renderer for the configurable 10 HP front panel and timing instrumentation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <SDL3/SDL.h>

#include "panel_layout.h"
#include "scope_session.h"
#include "simulator_runtime.h"

namespace clockfw::sim {

/** @brief Draws the virtual Eurorack panel using the firmware's actual OLED framebuffer and gate states. */
class PanelRenderer final {
public:
    /**
     * @brief Creates a renderer using one resolved panel layout.
     *
     * A configured BMP/PNG/JPEG front-panel image is loaded once into an SDL texture.
     * The built-in panel art remains the fallback when no image is configured.
     */
    PanelRenderer(SDL_Renderer* renderer, const layout::PanelLayout& panelLayout);

    /** @brief Releases the optional front-panel background texture. */
    ~PanelRenderer();

    /** @brief Prevents copying an SDL renderer resource owner. */
    PanelRenderer(const PanelRenderer&) = delete;

    /** @brief Prevents copy-assignment of an SDL renderer resource owner. */
    PanelRenderer& operator=(const PanelRenderer&) = delete;

    /** @brief Draws one complete simulator frame without presenting it. */
    void draw(
        SDL_Renderer* renderer,
        const SimulatorRuntime& runtime,
        double speedMultiplier,
        bool developerViewEnabled,
        const scope::SessionView& scopeView) const;

    /** @brief Draws and presents one complete interactive simulator frame. */
    void render(
        SDL_Renderer* renderer,
        const SimulatorRuntime& runtime,
        double speedMultiplier,
        bool developerViewEnabled,
        const scope::SessionView& scopeView) const;

private:
    /** @brief Draws the physical 10 HP / 3U front-panel representation. */
    void renderFrontPanel(
        SDL_Renderer* renderer,
        const SimulatorRuntime& runtime,
        double speedMultiplier) const;

    /** @brief Draws the true 128x64 firmware framebuffer inside the configured display rectangle. */
    void renderOled(SDL_Renderer* renderer, const SimulatorRuntime& runtime) const;

    /** @brief Draws eight output waveforms and scheduler/debug information. */
    void renderDeveloperPanel(
        SDL_Renderer* renderer,
        const SimulatorRuntime& runtime,
        double speedMultiplier,
        const scope::SessionView& scopeView) const;

    /** @brief Draws one outlined or filled circle using simple SDL line primitives. */
    static void drawCircle(SDL_Renderer* renderer, float centerX, float centerY, float radius, bool filled);

    /** @brief Writes compact ASCII debug text using SDL's built-in 8x8 debug font. */
    static void drawText(SDL_Renderer* renderer, float x, float y, const char* text);

    const layout::PanelLayout& panelLayout_;
    SDL_Texture* panelBackgroundTexture_ = nullptr;
};

}  // namespace clockfw::sim
