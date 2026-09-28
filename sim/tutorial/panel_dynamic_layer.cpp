/**
 * @file panel_dynamic_layer.cpp
 * @brief Implements deterministic headless rasterization for Storybook physical interaction overlays.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/panel_dynamic_layer.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace clockfw::sim::tutorial {
namespace {

constexpr float kPi = 3.14159265358979323846F;

void drawFilledCircle(
    PanelDynamicLayer& layer,
    const layout::Point center,
    const float radius,
    const RgbaPixel color) {
    const int minX = static_cast<int>(std::floor(center.x - radius));
    const int maxX = static_cast<int>(std::ceil(center.x + radius));
    const int minY = static_cast<int>(std::floor(center.y - radius));
    const int maxY = static_cast<int>(std::ceil(center.y + radius));
    const float radiusSquared = radius * radius;
    for (int y = minY; y <= maxY; ++y) {
        for (int x = minX; x <= maxX; ++x) {
            const float dx = static_cast<float>(x) - center.x;
            const float dy = static_cast<float>(y) - center.y;
            if (dx * dx + dy * dy <= radiusSquared) {
                layer.setPixel(x, y, color);
            }
        }
    }
}

void drawLine(
    PanelDynamicLayer& layer,
    const layout::Point from,
    const layout::Point to,
    const RgbaPixel color) {
    const float dx = to.x - from.x;
    const float dy = to.y - from.y;
    const int steps = std::max(1, static_cast<int>(std::ceil(std::max(std::fabs(dx), std::fabs(dy)))));
    for (int step = 0; step <= steps; ++step) {
        const float t = static_cast<float>(step) / static_cast<float>(steps);
        layer.setPixel(
            static_cast<int>(std::lround(from.x + dx * t)),
            static_cast<int>(std::lround(from.y + dy * t)),
            color);
    }
}

void drawPressedControl(
    PanelDynamicLayer& layer,
    const ButtonPresentation& button) {
    if (!button.pressed) return;
    drawFilledCircle(layer, button.center, button.radius * 0.78F, {0U, 0U, 0U, 108U});
}

void drawPatchPlug(
    PanelDynamicLayer& layer,
    const PatchPresentation& patch) {
    if (patch.insertion <= 0.0F) return;
    const float progress = std::clamp(patch.insertion, 0.0F, 1.0F);
    const float plugRadius = patch.jack.bushingRadius * (0.55F + 0.25F * progress);
    const float cableLength = patch.jack.nutRadius * 2.4F * progress;
    const layout::Point cableEnd{patch.center.x, patch.center.y + cableLength};
    drawLine(layer, patch.center, cableEnd, {60U, 62U, 66U, 255U});
    drawFilledCircle(layer, patch.center, plugRadius, {78U, 80U, 84U, 255U});
    drawFilledCircle(layer, patch.center, patch.jack.openingRadius * 0.7F, {24U, 25U, 27U, 255U});
}

}  // namespace

PanelDynamicLayer::PanelDynamicLayer(const std::size_t width, const std::size_t height)
    : width_(width), height_(height), pixels_(width * height) {
    if (width == 0U || height == 0U) {
        throw std::invalid_argument("panel dynamic layer dimensions must be positive");
    }
}

std::size_t PanelDynamicLayer::width() const {
    return width_;
}

std::size_t PanelDynamicLayer::height() const {
    return height_;
}

const RgbaPixel& PanelDynamicLayer::pixel(const std::size_t x, const std::size_t y) const {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("panel dynamic layer pixel outside raster");
    }
    return pixels_[y * width_ + x];
}

void PanelDynamicLayer::setPixel(const int x, const int y, const RgbaPixel pixelValue) {
    if (x < 0 || y < 0 || static_cast<std::size_t>(x) >= width_ || static_cast<std::size_t>(y) >= height_) {
        return;
    }
    pixels_[static_cast<std::size_t>(y) * width_ + static_cast<std::size_t>(x)] = pixelValue;
}

PanelDynamicLayer renderPanelDynamicLayer(
    const layout::PanelLayout& panelLayout,
    const PanelPresentationSnapshot& snapshot) {
    PanelDynamicLayer layer(
        static_cast<std::size_t>(panelLayout.windowWidth),
        static_cast<std::size_t>(panelLayout.windowHeight));

    for (const LedPresentation& led : snapshot.leds) {
        drawFilledCircle(
            layer,
            led.center,
            led.radius,
            {led.color.red, led.color.green, led.color.blue, 255U});
    }

    constexpr float kEncoderStepRadians = kPi / 12.0F;
    const float encoderAngle = -kPi * 0.25F +
        static_cast<float>(snapshot.encoder.visualPosition % 24) * kEncoderStepRadians;
    const float indicatorRadius = snapshot.encoder.radius * 0.47F;
    const layout::Point encoderEnd{
        snapshot.encoder.center.x + std::cos(encoderAngle) * indicatorRadius,
        snapshot.encoder.center.y + std::sin(encoderAngle) * indicatorRadius};
    drawLine(layer, snapshot.encoder.center, encoderEnd, {220U, 221U, 222U, 255U});
    if (snapshot.encoder.pressed) {
        drawFilledCircle(layer, snapshot.encoder.center, snapshot.encoder.radius * 0.7F, {0U, 0U, 0U, 96U});
    }

    drawPressedControl(layer, snapshot.play);
    drawPressedControl(layer, snapshot.tap);
    drawPressedControl(layer, snapshot.stop);
    drawPatchPlug(layer, snapshot.sync);
    drawPatchPlug(layer, snapshot.reset);
    return layer;
}

}  // namespace clockfw::sim::tutorial
