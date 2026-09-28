/**
 * @file tutorial_surface.h
 * @brief Deterministic host-only RGBA8 surface and composition primitives for CLOCK Storybook frames.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <vector>

namespace clockfw::sim::tutorial {

/** @brief Straight-alpha RGBA8 colour used by Storybook software rendering. */
struct TutorialColor {
    std::uint8_t red = 0U;
    std::uint8_t green = 0U;
    std::uint8_t blue = 0U;
    std::uint8_t alpha = 255U;

    bool operator==(const TutorialColor& other) const {
        return red == other.red && green == other.green && blue == other.blue && alpha == other.alpha;
    }
};

/** @brief Integer rectangle used by the fixed-resolution tutorial compositor. */
struct TutorialRect {
    int x = 0;
    int y = 0;
    int width = 0;
    int height = 0;
};

/** @brief Deterministic in-memory RGBA8 raster independent of SDL/windowing. */
class TutorialSurface final {
public:
    TutorialSurface(std::size_t width, std::size_t height, TutorialColor background = {});

    std::size_t width() const;
    std::size_t height() const;
    const TutorialColor& pixel(std::size_t x, std::size_t y) const;
    const std::vector<TutorialColor>& pixels() const;

    void clear(TutorialColor color);
    void setPixel(int x, int y, TutorialColor color);
    void blendPixel(int x, int y, TutorialColor color);
    void fillRect(TutorialRect rect, TutorialColor color);
    void strokeRect(TutorialRect rect, TutorialColor color, int thickness = 1);
    void drawLine(int x0, int y0, int x1, int y1, TutorialColor color, int thickness = 1);
    void fillCircle(int centerX, int centerY, int radius, TutorialColor color);

    /** @brief Alpha-composites another surface at 1:1 pixel scale. */
    void composite(const TutorialSurface& source, int destinationX, int destinationY);

    /** @brief Nearest-neighbour blits one source rectangle into an arbitrary destination rectangle. */
    void blitNearest(const TutorialSurface& source, TutorialRect sourceRect, TutorialRect destinationRect);

private:
    std::size_t width_ = 0U;
    std::size_t height_ = 0U;
    std::vector<TutorialColor> pixels_{};
};

}  // namespace clockfw::sim::tutorial
