/**
 * @file panel_renderer.cpp
 * @brief SDL renderer for the 10 HP front panel and developer timing instrumentation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "panel_renderer.h"

#include "scope_timeline.h"

#include <algorithm>
#include <array>
#include <cmath>
#include <cstdio>
#include <stdexcept>
#include <string>

namespace clockfw::sim {
namespace {

/** Minimum perceived LED-on time at 1x; logic annotations remain electrically exact. */
constexpr std::uint64_t kLedVisualPersistenceUs = 75000ULL;
constexpr float kPi = 3.14159265358979323846F;

void setColor(SDL_Renderer* renderer, Uint8 red, Uint8 green, Uint8 blue, Uint8 alpha = 255U) {
    (void)SDL_SetRenderDrawColor(renderer, red, green, blue, alpha);
}

void setColor(SDL_Renderer* renderer, const layout::Color& color) {
    setColor(renderer, color.red, color.green, color.blue);
}

void fillRect(SDL_Renderer* renderer, const layout::Rect& rect) {
    const SDL_FRect sdlRect{rect.x, rect.y, rect.width, rect.height};
    (void)SDL_RenderFillRect(renderer, &sdlRect);
}

void strokeRect(SDL_Renderer* renderer, const layout::Rect& rect) {
    const SDL_FRect sdlRect{rect.x, rect.y, rect.width, rect.height};
    (void)SDL_RenderRect(renderer, &sdlRect);
}

}  // namespace

PanelRenderer::PanelRenderer(SDL_Renderer* const renderer, const layout::PanelLayout& panelLayout)
    : panelLayout_(panelLayout) {
    if (panelLayout_.backgroundImagePath.empty()) {
        return;
    }

    const std::string imagePath = panelLayout_.backgroundImagePath.string();
    SDL_Surface* const surface = SDL_LoadSurface(imagePath.c_str());
    if (surface == nullptr) {
        throw std::runtime_error(
            "cannot load simulator panel image '" + imagePath + "': " + SDL_GetError());
    }

    panelBackgroundTexture_ = SDL_CreateTextureFromSurface(renderer, surface);
    SDL_DestroySurface(surface);
    if (panelBackgroundTexture_ == nullptr) {
        throw std::runtime_error(
            "cannot create simulator panel texture '" + imagePath + "': " + SDL_GetError());
    }
}

PanelRenderer::~PanelRenderer() {
    if (panelBackgroundTexture_ != nullptr) {
        SDL_DestroyTexture(panelBackgroundTexture_);
    }
}

void PanelRenderer::draw(
    SDL_Renderer* const renderer,
    const SimulatorRuntime& runtime,
    const double speedMultiplier,
    const bool developerViewEnabled,
    const scope::SessionView& scopeView) const {
    setColor(renderer, 18U, 19U, 21U);
    (void)SDL_RenderClear(renderer);

    renderFrontPanel(renderer, runtime, speedMultiplier);
    if (developerViewEnabled) {
        renderDeveloperPanel(renderer, runtime, speedMultiplier, scopeView);
    }
}

void PanelRenderer::render(
    SDL_Renderer* const renderer,
    const SimulatorRuntime& runtime,
    const double speedMultiplier,
    const bool developerViewEnabled,
    const scope::SessionView& scopeView) const {
    draw(renderer, runtime, speedMultiplier, developerViewEnabled, scopeView);
    (void)SDL_RenderPresent(renderer);
}

void PanelRenderer::renderFrontPanel(
    SDL_Renderer* const renderer,
    const SimulatorRuntime& runtime,
    const double speedMultiplier) const {
    if (panelBackgroundTexture_ != nullptr) {
        const SDL_FRect target{
            panelLayout_.panel.x, panelLayout_.panel.y, panelLayout_.panel.width, panelLayout_.panel.height};
        (void)SDL_RenderTexture(renderer, panelBackgroundTexture_, nullptr, &target);
    } else {
        setColor(renderer, 205U, 207U, 209U);
        fillRect(renderer, panelLayout_.panel);
    }
    setColor(renderer, 75U, 77U, 80U);
    strokeRect(renderer, panelLayout_.panel);

    if (panelLayout_.drawBuiltinLabels) {
        setColor(renderer, 38U, 39U, 41U);
        drawText(renderer, panelLayout_.panel.x + 22.0F, panelLayout_.panel.y + 24.0F, "CLOCK");
        drawText(renderer, panelLayout_.panel.x + 22.0F, panelLayout_.panel.y + 46.0F, "8-CHANNEL CLOCK  -  NATIVE SIMULATOR");
    }

    renderOled(renderer, runtime);

    setColor(renderer, 58U, 59U, 62U);
    drawCircle(renderer, panelLayout_.encoderCenter.x, panelLayout_.encoderCenter.y, panelLayout_.encoderRadius, true);
    setColor(renderer, 220U, 221U, 222U);
    const float encoderInset = 0.7F * panelLayout_.pixelsPerMm;
    drawCircle(
        renderer,
        panelLayout_.encoderCenter.x,
        panelLayout_.encoderCenter.y,
        std::max(panelLayout_.encoderRadius - encoderInset, panelLayout_.encoderRadius * 0.6F),
        false);
    const float encoderIndicator = panelLayout_.encoderRadius * 0.47F;
    constexpr float kEncoderStepRadians = kPi / 12.0F;
    const float encoderAngle = -kPi * 0.25F +
        static_cast<float>(runtime.encoderVisualPosition() % 24) * kEncoderStepRadians;
    (void)SDL_RenderLine(
        renderer,
        panelLayout_.encoderCenter.x,
        panelLayout_.encoderCenter.y,
        panelLayout_.encoderCenter.x + std::cos(encoderAngle) * encoderIndicator,
        panelLayout_.encoderCenter.y + std::sin(encoderAngle) * encoderIndicator);
    if (panelLayout_.drawBuiltinLabels) {
        setColor(renderer, 38U, 39U, 41U);
        drawText(
            renderer,
            panelLayout_.encoderCenter.x - 16.0F,
            panelLayout_.encoderCenter.y + panelLayout_.encoderRadius + 12.0F,
            "ENC");
    }

    const std::array<std::pair<const layout::CircleControl*, const char*>, 3U> buttons{{
        {&panelLayout_.playButton, "PLAY"},
        {&panelLayout_.tapButton, "TAP"},
        {&panelLayout_.stopButton, "STOP"}
    }};
    for (const auto& item : buttons) {
        const layout::CircleControl& button = *item.first;
        setColor(renderer, button.color);
        drawCircle(renderer, button.center.x, button.center.y, button.radius, true);
        setColor(renderer, 25U, 26U, 28U);
        drawCircle(renderer, button.center.x, button.center.y, button.radius, false);
        if (panelLayout_.drawBuiltinLabels) {
            const float labelOffset = button.radius + 1.5F * panelLayout_.pixelsPerMm;
            drawText(renderer, button.center.x - 16.0F, button.center.y + labelOffset, item.second);
        }
    }

    const SyncInputTelemetry sync = runtime.syncInputTelemetry();
    const ResetInputTelemetry reset = runtime.resetInputTelemetry();
    if (panelLayout_.drawBuiltinLabels) {
        setColor(renderer, 36U, 37U, 39U);
        drawText(renderer, panelLayout_.syncInputCenter.x - 27.0F, panelLayout_.syncInputCenter.y - 48.0F, "SYNC IN");
        drawText(renderer, panelLayout_.resetInputCenter.x - 23.0F, panelLayout_.resetInputCenter.y - 48.0F, "RST IN");
    }
    const auto drawInputJack = [&](
        const layout::Point& center, const layout::JackType jackType,
        const bool connected, const bool high) {
        const layout::JackGeometry& jack = panelLayout_.jackGeometry(jackType);
        setColor(renderer, connected ? 88U : 145U, connected ? 98U : 147U, connected ? 88U : 150U);
        drawCircle(renderer, center.x, center.y, jack.nutRadius, true);
        setColor(renderer, connected ? 65U : 118U, connected ? 80U : 120U, connected ? 65U : 122U);
        drawCircle(renderer, center.x, center.y, jack.bushingRadius, true);
        setColor(renderer, high ? 200U : 25U, high ? 230U : 26U, high ? 180U : 28U);
        drawCircle(renderer, center.x, center.y, jack.openingRadius, true);
        setColor(renderer, high ? 25U : 220U, high ? 26U : 222U, high ? 28U : 224U);
        drawText(renderer, center.x - 7.0F, center.y - 4.0F, high ? "HI" : "LO");
    };
    drawInputJack(panelLayout_.syncInputCenter, panelLayout_.syncJackType, sync.cableConnected, sync.signalHigh);
    drawInputJack(panelLayout_.resetInputCenter, panelLayout_.resetJackType, reset.cableConnected, reset.signalHigh);

    const auto& channels = runtime.telemetry();
    const std::uint64_t nowUs = runtime.nowMicroseconds();
    const double safeSpeed = std::max(speedMultiplier, 1.0);
    const std::uint64_t ledPersistenceUs = static_cast<std::uint64_t>(
        static_cast<double>(kLedVisualPersistenceUs) * safeSpeed);
    for (std::size_t index = 0U; index < kChannelCount; ++index) {
        const auto& center = panelLayout_.outputCenters[index];
        const auto& led = panelLayout_.ledCenters[index];
        const bool recentPulse = channels[index].lastRisingUs != 0ULL &&
            nowUs >= channels[index].lastRisingUs &&
            nowUs - channels[index].lastRisingUs <= ledPersistenceUs;
        const bool ledLit = channels[index].logicHigh || recentPulse;
        setColor(renderer, ledLit ? panelLayout_.ledOnColor : panelLayout_.ledOffColor);
        drawCircle(renderer, led.x, led.y, panelLayout_.ledRadius, true);

        const layout::JackGeometry& jack = panelLayout_.jackGeometry(panelLayout_.outputJackTypes[index]);
        setColor(renderer, 145U, 147U, 150U);
        drawCircle(renderer, center.x, center.y, jack.nutRadius, true);
        setColor(renderer, 118U, 120U, 122U);
        drawCircle(renderer, center.x, center.y, jack.bushingRadius, true);
        setColor(renderer, 25U, 26U, 28U);
        drawCircle(renderer, center.x, center.y, jack.openingRadius, true);

        const char* const levelText = !runtime.outputStageEnabled()
            ? "HZ"
            : (channels[index].logicHigh ? "HI" : "LO");
        setColor(renderer, channels[index].logicHigh ? 225U : 170U, 225U, channels[index].logicHigh ? 170U : 180U);
        drawText(renderer, center.x - 7.0F, center.y - 4.0F, levelText);

        char label[8]{};
        std::snprintf(label, sizeof(label), "OUT%u", static_cast<unsigned>(index + 1U));
        if (panelLayout_.drawBuiltinLabels) {
            setColor(renderer, 38U, 39U, 41U);
            const layout::JackGeometry& labelJack = panelLayout_.jackGeometry(panelLayout_.outputJackTypes[index]);
            drawText(renderer, center.x - 17.0F, center.y + labelJack.nutRadius + 1.2F * panelLayout_.pixelsPerMm, label);
        }
    }

    if (panelLayout_.drawScrews) {
        setColor(renderer, 115U, 116U, 118U);
        for (const layout::Point& screw : panelLayout_.screwCenters) {
            drawCircle(renderer, screw.x, screw.y, 5.0F, true);
        }
    }
}

void PanelRenderer::renderOled(SDL_Renderer* const renderer, const SimulatorRuntime& runtime) const {
    const int scale = panelLayout_.displayPixelScale;
    const float renderedWidth = static_cast<float>(hal::OledDisplay::kWidth * scale);
    const float renderedHeight = static_cast<float>(hal::OledDisplay::kHeight * scale);
    const float displayX = std::round(panelLayout_.display.x +
        (panelLayout_.display.width - renderedWidth) * 0.5F);
    const float displayY = std::round(panelLayout_.display.y +
        (panelLayout_.display.height - renderedHeight) * 0.5F);
    const layout::Rect pixelArea{displayX, displayY, renderedWidth, renderedHeight};

    setColor(renderer, 5U, 6U, 7U);
    fillRect(renderer, panelLayout_.display);

    const auto& framebuffer = runtime.framebuffer();
    setColor(renderer, 236U, 239U, 242U);
    for (std::int16_t y = 0; y < hal::OledDisplay::kHeight; ++y) {
        for (std::int16_t x = 0; x < hal::OledDisplay::kWidth; ++x) {
            const std::size_t byteIndex = static_cast<std::size_t>(x) +
                static_cast<std::size_t>(y / 8) * static_cast<std::size_t>(hal::OledDisplay::kWidth);
            const std::uint8_t bitMask = static_cast<std::uint8_t>(1U << (y & 7));
            if ((framebuffer[byteIndex] & bitMask) == 0U) {
                continue;
            }
            const SDL_FRect pixel{
                pixelArea.x + static_cast<float>(x * scale),
                pixelArea.y + static_cast<float>(y * scale),
                static_cast<float>(scale),
                static_cast<float>(scale)};
            (void)SDL_RenderFillRect(renderer, &pixel);
        }
    }
    setColor(renderer, 82U, 84U, 87U);
    strokeRect(renderer, pixelArea);
}

void PanelRenderer::drawCircle(
    SDL_Renderer* const renderer,
    const float centerX,
    const float centerY,
    const float radius,
    const bool filled) {
    if (filled) {
        const int integerRadius = static_cast<int>(std::ceil(radius));
        for (int y = -integerRadius; y <= integerRadius; ++y) {
            const float yFloat = static_cast<float>(y);
            const float extentSquared = radius * radius - yFloat * yFloat;
            if (extentSquared < 0.0F) {
                continue;
            }
            const float extent = std::sqrt(extentSquared);
            (void)SDL_RenderLine(renderer, centerX - extent, centerY + yFloat, centerX + extent, centerY + yFloat);
        }
        return;
    }

    constexpr int kSegments = 40;
    float previousX = centerX + radius;
    float previousY = centerY;
    for (int segment = 1; segment <= kSegments; ++segment) {
        const float angle = 2.0F * kPi * static_cast<float>(segment) / static_cast<float>(kSegments);
        const float x = centerX + std::cos(angle) * radius;
        const float y = centerY + std::sin(angle) * radius;
        (void)SDL_RenderLine(renderer, previousX, previousY, x, y);
        previousX = x;
        previousY = y;
    }
}

void PanelRenderer::drawText(
    SDL_Renderer* const renderer,
    const float x,
    const float y,
    const char* const text) {
    (void)SDL_RenderDebugText(renderer, x, y, text);
}

}  // namespace clockfw::sim
