/**
 * @file tutorial_surface.cpp
 * @brief Implements deterministic host-only RGBA8 composition primitives for CLOCK Storybook frames.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/tutorial_surface.h"

#include <algorithm>
#include <cmath>
#include <stdexcept>

namespace clockfw::sim::tutorial {
namespace {

std::uint8_t blendChannel(const std::uint8_t source, const std::uint8_t destination, const std::uint8_t alpha) {
    const std::uint32_t value = static_cast<std::uint32_t>(source) * alpha +
        static_cast<std::uint32_t>(destination) * (255U - alpha);
    return static_cast<std::uint8_t>((value + 127U) / 255U);
}

}  // namespace

TutorialSurface::TutorialSurface(
    const std::size_t width,
    const std::size_t height,
    const TutorialColor background)
    : width_(width), height_(height), pixels_(width * height, background) {
    if (width == 0U || height == 0U) {
        throw std::invalid_argument("tutorial surface dimensions must be positive");
    }
}

std::size_t TutorialSurface::width() const { return width_; }
std::size_t TutorialSurface::height() const { return height_; }

const TutorialColor& TutorialSurface::pixel(const std::size_t x, const std::size_t y) const {
    if (x >= width_ || y >= height_) {
        throw std::out_of_range("tutorial surface pixel outside raster");
    }
    return pixels_[y * width_ + x];
}

const std::vector<TutorialColor>& TutorialSurface::pixels() const { return pixels_; }

void TutorialSurface::clear(const TutorialColor color) {
    std::fill(pixels_.begin(), pixels_.end(), color);
}

void TutorialSurface::setPixel(const int x, const int y, const TutorialColor color) {
    if (x < 0 || y < 0 || static_cast<std::size_t>(x) >= width_ || static_cast<std::size_t>(y) >= height_) {
        return;
    }
    pixels_[static_cast<std::size_t>(y) * width_ + static_cast<std::size_t>(x)] = color;
}

void TutorialSurface::blendPixel(const int x, const int y, const TutorialColor color) {
    if (x < 0 || y < 0 || static_cast<std::size_t>(x) >= width_ || static_cast<std::size_t>(y) >= height_) {
        return;
    }
    TutorialColor& destination = pixels_[static_cast<std::size_t>(y) * width_ + static_cast<std::size_t>(x)];
    if (color.alpha == 255U) {
        destination = color;
        return;
    }
    if (color.alpha == 0U) return;
    destination = {
        blendChannel(color.red, destination.red, color.alpha),
        blendChannel(color.green, destination.green, color.alpha),
        blendChannel(color.blue, destination.blue, color.alpha),
        255U};
}

void TutorialSurface::fillRect(TutorialRect rect, const TutorialColor color) {
    if (rect.width <= 0 || rect.height <= 0 || color.alpha == 0U) return;
    const int left = std::max(0, rect.x);
    const int top = std::max(0, rect.y);
    const int right = std::min(static_cast<int>(width_), rect.x + rect.width);
    const int bottom = std::min(static_cast<int>(height_), rect.y + rect.height);
    if (left >= right || top >= bottom) return;
    if (color.alpha == 255U) {
        for (int y = top; y < bottom; ++y) {
            auto first = pixels_.begin() + static_cast<std::ptrdiff_t>(static_cast<std::size_t>(y) * width_ + static_cast<std::size_t>(left));
            std::fill(first, first + (right - left), color);
        }
        return;
    }
    for (int y = top; y < bottom; ++y) {
        for (int x = left; x < right; ++x) {
            blendPixel(x, y, color);
        }
    }
}

void TutorialSurface::strokeRect(const TutorialRect rect, const TutorialColor color, const int thickness) {
    if (rect.width <= 0 || rect.height <= 0 || thickness <= 0) return;
    fillRect({rect.x, rect.y, rect.width, thickness}, color);
    fillRect({rect.x, rect.y + rect.height - thickness, rect.width, thickness}, color);
    fillRect({rect.x, rect.y, thickness, rect.height}, color);
    fillRect({rect.x + rect.width - thickness, rect.y, thickness, rect.height}, color);
}

void TutorialSurface::drawLine(
    int x0,
    int y0,
    const int x1,
    const int y1,
    const TutorialColor color,
    const int thickness) {
    const int dx = std::abs(x1 - x0);
    const int sx = x0 < x1 ? 1 : -1;
    const int dy = -std::abs(y1 - y0);
    const int sy = y0 < y1 ? 1 : -1;
    int error = dx + dy;
    const int radius = std::max(0, thickness / 2);
    for (;;) {
        for (int oy = -radius; oy <= radius; ++oy) {
            for (int ox = -radius; ox <= radius; ++ox) {
                blendPixel(x0 + ox, y0 + oy, color);
            }
        }
        if (x0 == x1 && y0 == y1) break;
        const int doubled = 2 * error;
        if (doubled >= dy) {
            error += dy;
            x0 += sx;
        }
        if (doubled <= dx) {
            error += dx;
            y0 += sy;
        }
    }
}

void TutorialSurface::fillCircle(
    const int centerX,
    const int centerY,
    const int radius,
    const TutorialColor color) {
    if (radius <= 0) return;
    const int radiusSquared = radius * radius;
    for (int y = -radius; y <= radius; ++y) {
        for (int x = -radius; x <= radius; ++x) {
            if (x * x + y * y <= radiusSquared) {
                blendPixel(centerX + x, centerY + y, color);
            }
        }
    }
}

void TutorialSurface::composite(
    const TutorialSurface& source,
    const int destinationX,
    const int destinationY) {
    for (std::size_t y = 0U; y < source.height(); ++y) {
        for (std::size_t x = 0U; x < source.width(); ++x) {
            blendPixel(
                destinationX + static_cast<int>(x),
                destinationY + static_cast<int>(y),
                source.pixel(x, y));
        }
    }
}

void TutorialSurface::blitNearest(
    const TutorialSurface& source,
    const TutorialRect sourceRect,
    const TutorialRect destinationRect) {
    if (sourceRect.width <= 0 || sourceRect.height <= 0 || destinationRect.width <= 0 || destinationRect.height <= 0) {
        throw std::invalid_argument("nearest blit rectangles must be positive");
    }
    if (sourceRect.x < 0 || sourceRect.y < 0 ||
        sourceRect.x + sourceRect.width > static_cast<int>(source.width()) ||
        sourceRect.y + sourceRect.height > static_cast<int>(source.height())) {
        throw std::out_of_range("nearest blit source rectangle outside raster");
    }
    const auto& sourcePixels = source.pixels();
    for (int dy = 0; dy < destinationRect.height; ++dy) {
        const int destinationY = destinationRect.y + dy;
        if (destinationY < 0 || destinationY >= static_cast<int>(height_)) continue;
        const int sy = sourceRect.y + static_cast<int>(
            (static_cast<std::int64_t>(dy) * sourceRect.height) / destinationRect.height);
        const std::size_t sourceRow = static_cast<std::size_t>(sy) * source.width();
        const std::size_t destinationRow = static_cast<std::size_t>(destinationY) * width_;
        for (int dx = 0; dx < destinationRect.width; ++dx) {
            const int destinationX = destinationRect.x + dx;
            if (destinationX < 0 || destinationX >= static_cast<int>(width_)) continue;
            const int sx = sourceRect.x + static_cast<int>(
                (static_cast<std::int64_t>(dx) * sourceRect.width) / destinationRect.width);
            const TutorialColor pixel = sourcePixels[sourceRow + static_cast<std::size_t>(sx)];
            TutorialColor& destination = pixels_[destinationRow + static_cast<std::size_t>(destinationX)];
            if (pixel.alpha == 255U) {
                destination = pixel;
            } else if (pixel.alpha != 0U) {
                destination = {
                    blendChannel(pixel.red, destination.red, pixel.alpha),
                    blendChannel(pixel.green, destination.green, pixel.alpha),
                    blendChannel(pixel.blue, destination.blue, pixel.alpha),
                    255U};
            }
        }
    }
}

}  // namespace clockfw::sim::tutorial
