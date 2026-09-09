/**
 * @file main.cpp
 * @brief SDL3 desktop entry point for the configurable native clock simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <SDL3/SDL.h>
#include <SDL3/SDL_main.h>

#include <algorithm>
#include <chrono>
#include <cstdint>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>

#include "panel_layout.h"
#include "panel_renderer.h"
#include "scope_timeline.h"
#include "simulator_input.h"
#include "simulator_runtime.h"

namespace {

/** Command-line options that affect only the native simulator host. */
struct SimulatorOptions {
    std::filesystem::path statePath = ".clock-simulator-state.bin";
    std::optional<std::filesystem::path> layoutPath{};
    std::optional<std::filesystem::path> panelImagePath{};
    std::optional<std::filesystem::path> screenshotPath{};
    std::uint64_t screenshotAfterMs = 0ULL;
    std::optional<int> displayPixelScale{};
    bool developerViewEnabled = true;
};

std::uint64_t parseUnsigned(const std::string& text, const char* const optionName) {
    std::size_t parsed = 0U;
    const unsigned long long value = std::stoull(text, &parsed, 10);
    if (parsed != text.size()) {
        throw std::runtime_error(std::string(optionName) + " requires an integer");
    }
    return static_cast<std::uint64_t>(value);
}

SimulatorOptions parseOptions(const int argc, char* argv[]) {
    SimulatorOptions options{};
    for (int index = 1; index < argc; ++index) {
        const std::string argument = argv[index];
        const auto requireValue = [&](const char* const optionName) -> std::string {
            if (index + 1 >= argc) {
                throw std::runtime_error(std::string(optionName) + " requires a value");
            }
            ++index;
            return argv[index];
        };

        if (argument == "--state") {
            options.statePath = requireValue("--state");
        } else if (argument == "--layout") {
            options.layoutPath = requireValue("--layout");
        } else if (argument == "--panel-image") {
            options.panelImagePath = requireValue("--panel-image");
        } else if (argument == "--display-scale") {
            options.displayPixelScale = static_cast<int>(
                parseUnsigned(requireValue("--display-scale"), "--display-scale"));
        } else if (argument == "--screenshot") {
            options.screenshotPath = requireValue("--screenshot");
        } else if (argument == "--screenshot-after-ms") {
            options.screenshotAfterMs = parseUnsigned(
                requireValue("--screenshot-after-ms"), "--screenshot-after-ms");
        } else if (argument == "--no-developer") {
            options.developerViewEnabled = false;
        } else if (argument == "--help" || argument == "-h") {
            std::cout
                << "clock-simulator [options]\n"
                << "  --layout FILE              load configurable panel geometry from INI\n"
                << "  --panel-image IMAGE        override the INI front-panel image\n"
                << "  --display-scale N          integer OLED pixel scale (1..8)\n"
                << "  --state FILE               use an isolated simulator persistence file\n"
                << "  --screenshot FILE.bmp      render one full-panel screenshot without a window\n"
                << "  --screenshot-after-ms N    advance virtual time before the screenshot\n"
                << "  --no-developer             omit the developer timing panel from screenshots/UI\n";
            std::exit(0);
        } else {
            throw std::runtime_error("unknown simulator option: " + argument);
        }
    }
    return options;
}

clockfw::sim::layout::PanelLayout resolvePanelLayout(const SimulatorOptions& options) {
    clockfw::sim::layout::PanelLayout panelLayout = clockfw::sim::layout::makeDefaultPanelLayout();
    if (options.layoutPath.has_value()) {
        panelLayout = clockfw::sim::layout::loadPanelLayout(*options.layoutPath);
    }
    if (options.panelImagePath.has_value()) {
        clockfw::sim::layout::overrideBackgroundImage(panelLayout, *options.panelImagePath);
    }
    if (options.displayPixelScale.has_value()) {
        clockfw::sim::layout::overrideDisplayPixelScale(panelLayout, *options.displayPixelScale);
    }
    return panelLayout;
}

void saveRendererScreenshot(SDL_Renderer* const renderer, const std::filesystem::path& path) {
    SDL_Surface* const screenshot = SDL_RenderReadPixels(renderer, nullptr);
    if (screenshot == nullptr) {
        throw std::runtime_error(std::string("cannot read simulator screenshot: ") + SDL_GetError());
    }
    const std::string screenshotPath = path.string();
    const bool saved = SDL_SaveBMP(screenshot, screenshotPath.c_str());
    SDL_DestroySurface(screenshot);
    if (!saved) {
        throw std::runtime_error(
            "cannot save simulator screenshot '" + screenshotPath + "': " + SDL_GetError());
    }
}

int renderHeadlessScreenshot(
    const SimulatorOptions& options,
    const clockfw::sim::layout::PanelLayout& panelLayout) {
    SDL_Surface* const surface = SDL_CreateSurface(
        panelLayout.windowWidth,
        panelLayout.windowHeight,
        SDL_PIXELFORMAT_RGBA32);
    if (surface == nullptr) {
        throw std::runtime_error(std::string("cannot create screenshot surface: ") + SDL_GetError());
    }
    SDL_Renderer* const renderer = SDL_CreateSoftwareRenderer(surface);
    if (renderer == nullptr) {
        SDL_DestroySurface(surface);
        throw std::runtime_error(std::string("cannot create software renderer: ") + SDL_GetError());
    }

    const std::string screenshotPath = options.screenshotPath->string();
    bool saved = false;
    {
        clockfw::sim::SimulatorRuntime runtime(options.statePath);
        runtime.begin();
        if (options.screenshotAfterMs != 0ULL) {
            runtime.advanceMicroseconds(options.screenshotAfterMs * 1000ULL);
        }
        clockfw::sim::PanelRenderer panelRenderer(renderer, panelLayout);
        clockfw::sim::scope::SessionView scopeView{};
        scopeView.windowUs = clockfw::sim::scope::kWindowOptionsUs[clockfw::sim::scope::kDefaultWindowIndex];
        panelRenderer.draw(
            renderer,
            runtime,
            1.0,
            options.developerViewEnabled,
            scopeView);
        (void)SDL_FlushRenderer(renderer);

        saved = SDL_SaveBMP(surface, screenshotPath.c_str());
        runtime.flushPersistence();
    }
    SDL_DestroyRenderer(renderer);
    SDL_DestroySurface(surface);
    if (!saved) {
        throw std::runtime_error(
            "cannot save headless screenshot '" + screenshotPath + "': " + SDL_GetError());
    }
    return 0;
}

}  // namespace

int main(int argc, char* argv[]) {
    try {
        const SimulatorOptions options = parseOptions(argc, argv);
        const clockfw::sim::layout::PanelLayout panelLayout = resolvePanelLayout(options);

        if (options.screenshotPath.has_value()) {
            if (!SDL_Init(0U)) {
                std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
                return 1;
            }
            const int result = renderHeadlessScreenshot(options, panelLayout);
            SDL_Quit();
            return result;
        }

        if (!SDL_Init(SDL_INIT_VIDEO)) {
            std::cerr << "SDL_Init failed: " << SDL_GetError() << '\n';
            return 1;
        }

        SDL_Window* const window = SDL_CreateWindow(
            "CLOCK Simulator",
            panelLayout.windowWidth,
            panelLayout.windowHeight,
            SDL_WINDOW_RESIZABLE);
        if (window == nullptr) {
            std::cerr << "SDL_CreateWindow failed: " << SDL_GetError() << '\n';
            SDL_Quit();
            return 1;
        }

        SDL_Renderer* const renderer = SDL_CreateRenderer(window, nullptr);
        if (renderer == nullptr) {
            std::cerr << "SDL_CreateRenderer failed: " << SDL_GetError() << '\n';
            SDL_DestroyWindow(window);
            SDL_Quit();
            return 1;
        }

        (void)SDL_SetRenderLogicalPresentation(
            renderer,
            panelLayout.windowWidth,
            panelLayout.windowHeight,
            SDL_LOGICAL_PRESENTATION_INTEGER_SCALE);

        {
            clockfw::sim::SimulatorRuntime runtime(options.statePath);
            runtime.begin();
            clockfw::sim::SimulatorInput input(panelLayout);
            clockfw::sim::PanelRenderer panelRenderer(renderer, panelLayout);

            using Clock = std::chrono::steady_clock;
            auto previousTime = Clock::now();
            while (!input.quitRequested()) {
                SDL_Event event{};
                while (SDL_PollEvent(&event)) {
                    // SDL3 mouse/touch coordinates remain window-relative when logical
                    // presentation is active. Convert them explicitly so front-panel
                    // hit testing stays correct after resize and HiDPI scaling.
                    if (!SDL_ConvertEventToRenderCoordinates(renderer, &event)) {
                        continue;
                    }
                    input.handleEvent(event, runtime);
                }

                const auto currentTime = Clock::now();
                const auto realDelta = std::chrono::duration_cast<std::chrono::microseconds>(
                    currentTime - previousTime);
                previousTime = currentTime;
                const auto cappedDelta = std::min<std::int64_t>(realDelta.count(), 100000LL);
                const auto virtualDelta = static_cast<std::uint64_t>(
                    static_cast<double>(std::max<std::int64_t>(cappedDelta, 0LL)) * input.speedMultiplier());
                runtime.advanceMicroseconds(virtualDelta);
                input.updateScope(runtime);

                panelRenderer.render(
                    renderer,
                    runtime,
                    input.speedMultiplier(),
                    input.developerViewEnabled(),
                    input.scopeView());
                if (input.consumeScreenshotRequest()) {
                    saveRendererScreenshot(renderer, "clock-simulator-screenshot.bmp");
                }
                SDL_Delay(4U);
            }

            runtime.flushPersistence();
        }
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();
        return 0;
    } catch (const std::exception& error) {
        std::cerr << "Simulator failure: " << error.what() << '\n';
        SDL_Quit();
        return 2;
    }
}
