/**
 * @file panel_dynamic_layer.h
 * @brief Deterministic software raster for Storybook front-panel interaction/telemetry overlays.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

#include "panel_layout.h"
#include "tutorial/panel_presentation.h"

namespace clockfw::sim::tutorial {

/** @brief One straight-alpha RGBA8 pixel used by the host-only Storybook overlay. */
struct RgbaPixel {
    std::uint8_t red = 0U;
    std::uint8_t green = 0U;
    std::uint8_t blue = 0U;
    std::uint8_t alpha = 0U;
};

/** @brief Transparent RGBA8 raster containing only dynamic physical interaction/LED presentation. */
class PanelDynamicLayer final {
public:
    /** @brief Creates one transparent deterministic raster. */
    PanelDynamicLayer(std::size_t width, std::size_t height);

    /** @brief Returns raster width in pixels. */
    std::size_t width() const;

    /** @brief Returns raster height in pixels. */
    std::size_t height() const;

    /** @brief Returns one pixel; out-of-range access throws. */
    const RgbaPixel& pixel(std::size_t x, std::size_t y) const;

    /** @brief Replaces one pixel when coordinates lie inside the raster. */
    void setPixel(int x, int y, RgbaPixel pixel);

private:
    std::size_t width_ = 0U;
    std::size_t height_ = 0U;
    std::vector<RgbaPixel> pixels_{};
};

/**
 * @brief Renders only dynamic physical state; static panel art and OLED composition remain later renderer responsibilities.
 */
PanelDynamicLayer renderPanelDynamicLayer(
    const layout::PanelLayout& panelLayout,
    const PanelPresentationSnapshot& snapshot);

}  // namespace clockfw::sim::tutorial
