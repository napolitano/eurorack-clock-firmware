/**
 * @file screensaver_field.cpp
 * @brief Smooth dense monochrome scalar-field isocontour screensaver.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/screensaver_renderer.h"

#include <array>
#include <cstdint>

namespace clockfw::ui {
namespace {
constexpr std::array<std::int8_t, 64U> kFieldSine{{
    0,12,25,37,49,60,71,81,90,98,106,112,117,121,125,126,
    127,126,125,121,117,112,106,98,90,81,71,60,49,37,25,12,
    0,-12,-25,-37,-49,-60,-71,-81,-90,-98,-106,-112,-117,-121,-125,-126,
    -127,-126,-125,-121,-117,-112,-106,-98,-90,-81,-71,-60,-49,-37,-25,-12}};
std::int8_t fieldSine(const std::uint32_t phase) { return kFieldSine[phase % kFieldSine.size()]; }

struct FieldAttractor final { std::int16_t x; std::int16_t y; std::int32_t strength; };
struct Point final { std::int16_t x; std::int16_t y; };

std::int16_t sampleField(const std::int16_t x, const std::int16_t y, const std::array<FieldAttractor, 4U>& attractors) {
    std::int32_t value = 0;
    for (const FieldAttractor& attractor : attractors) {
        const std::int32_t dx = static_cast<std::int32_t>(x - attractor.x);
        const std::int32_t dy = static_cast<std::int32_t>(y - attractor.y);
        const std::int32_t distanceSquared = dx * dx + dy * dy + 58;
        value += attractor.strength / distanceSquared;
    }
    if (value < 0) return 0;
    return static_cast<std::int16_t>(value > 32767 ? 32767 : value);
}

std::int16_t interpolateCoordinate(
    const std::int16_t coordinate0,
    const std::int16_t coordinate1,
    const std::int16_t value0,
    const std::int16_t value1,
    const std::int16_t level) {
    const std::int32_t difference = static_cast<std::int32_t>(value1) - value0;
    if (difference == 0) return static_cast<std::int16_t>((coordinate0 + coordinate1) / 2);
    const std::int32_t numerator = static_cast<std::int32_t>(level - value0) * (coordinate1 - coordinate0);
    const std::int32_t result = static_cast<std::int32_t>(coordinate0) + numerator / difference;
    return static_cast<std::int16_t>(result);
}
}  // namespace

void ScreensaverRenderer::renderField(const std::uint32_t frameIndex) {
    display_.clear();

    // Long sine cycles and sub-unity phase ratios keep each source moving only a
    // few pixels per 100-ms frame. Spatial interpolation below removes the
    // blocky contour jumps of the first implementation.
    const std::uint32_t phase = frameIndex;
    const std::array<FieldAttractor, 4U> attractors{{
        {static_cast<std::int16_t>(63 + static_cast<std::int32_t>(fieldSine(phase + 3U)) * 30 / 127),
         static_cast<std::int16_t>(31 + static_cast<std::int32_t>(fieldSine((phase * 3U) / 4U + 17U)) * 18 / 127), 18'000},
        {static_cast<std::int16_t>(31 + static_cast<std::int32_t>(fieldSine((phase * 2U) / 3U + 29U)) * 19 / 127),
         static_cast<std::int16_t>(20 + static_cast<std::int32_t>(fieldSine(phase / 2U + 41U)) * 13 / 127), 12'500},
        {static_cast<std::int16_t>(96 + static_cast<std::int32_t>(fieldSine((phase * 3U) / 5U + 51U)) * 20 / 127),
         static_cast<std::int16_t>(43 + static_cast<std::int32_t>(fieldSine((phase * 4U) / 5U + 9U)) * 12 / 127), 12'000},
        {static_cast<std::int16_t>(66 + static_cast<std::int32_t>(fieldSine(phase / 2U + 57U)) * 24 / 127),
         static_cast<std::int16_t>(33 + static_cast<std::int32_t>(fieldSine((phase * 3U) / 5U + 5U)) * 20 / 127), 8'500}
    }};

    constexpr std::int16_t kStep = 3;
    constexpr std::size_t kColumns = static_cast<std::size_t>((hal::OledDisplay::kWidth + kStep - 1) / kStep) + 1U;
    constexpr std::size_t kRows = static_cast<std::size_t>((hal::OledDisplay::kHeight + kStep - 1) / kStep) + 1U;
    std::array<std::int16_t, kColumns * kRows> field{};
    for (std::size_t row = 0U; row < kRows; ++row) {
        const std::int16_t y = static_cast<std::int16_t>(row * static_cast<std::size_t>(kStep));
        for (std::size_t column = 0U; column < kColumns; ++column) {
            const std::int16_t x = static_cast<std::int16_t>(column * static_cast<std::size_t>(kStep));
            field[row * kColumns + column] = sampleField(x, y, attractors);
        }
    }

    // More closely spaced contour levels give the monochrome image the visual
    // density that a colour plasma normally obtains from gradients.
    constexpr std::array<std::int16_t, 11U> kLevels{{16,20,25,31,39,49,62,78,98,123,154}};
    for (const std::int16_t level : kLevels) {
        for (std::size_t row = 0U; row + 1U < kRows; ++row) {
            for (std::size_t column = 0U; column + 1U < kColumns; ++column) {
                const std::int16_t tl = field[row * kColumns + column];
                const std::int16_t tr = field[row * kColumns + column + 1U];
                const std::int16_t br = field[(row + 1U) * kColumns + column + 1U];
                const std::int16_t bl = field[(row + 1U) * kColumns + column];
                const std::uint8_t mask = static_cast<std::uint8_t>((tl >= level ? 1U : 0U) | (tr >= level ? 2U : 0U) | (br >= level ? 4U : 0U) | (bl >= level ? 8U : 0U));
                if (mask == 0U || mask == 15U) continue;
                const std::int16_t x0 = static_cast<std::int16_t>(column * static_cast<std::size_t>(kStep));
                const std::int16_t y0 = static_cast<std::int16_t>(row * static_cast<std::size_t>(kStep));
                const std::int16_t x1 = static_cast<std::int16_t>(x0 + kStep);
                const std::int16_t y1 = static_cast<std::int16_t>(y0 + kStep);
                const Point top{interpolateCoordinate(x0,x1,tl,tr,level), y0};
                const Point right{x1, interpolateCoordinate(y0,y1,tr,br,level)};
                const Point bottom{interpolateCoordinate(x0,x1,bl,br,level), y1};
                const Point left{x0, interpolateCoordinate(y0,y1,tl,bl,level)};
                auto segment = [this](const Point& a, const Point& b) { display_.drawLine(a.x,a.y,b.x,b.y); };
                switch (mask) {
                    case 1U: case 14U: segment(left,top); break;
                    case 2U: case 13U: segment(top,right); break;
                    case 3U: case 12U: segment(left,right); break;
                    case 4U: case 11U: segment(right,bottom); break;
                    case 6U: case 9U: segment(top,bottom); break;
                    case 7U: case 8U: segment(left,bottom); break;
                    case 5U: segment(left,top); segment(right,bottom); break;
                    case 10U: segment(top,right); segment(bottom,left); break;
                    default: break;
                }
            }
        }
    }
    display_.present();
}

}  // namespace clockfw::ui
