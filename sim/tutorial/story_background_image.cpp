/**
 * @file story_background_image.cpp
 * @brief Implements dependency-free BMP background loading for Storybook.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_background_image.h"

#include <algorithm>
#include <cstdint>
#include <fstream>
#include <stdexcept>
#include <vector>

namespace clockfw::sim::tutorial {
namespace {

std::uint16_t u16(const std::vector<std::uint8_t>& bytes, const std::size_t offset) {
    if (offset + 2U > bytes.size()) throw std::runtime_error("truncated BMP header");
    return static_cast<std::uint16_t>(bytes[offset]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(bytes[offset + 1U]) << 8U);
}

std::uint32_t u32(const std::vector<std::uint8_t>& bytes, const std::size_t offset) {
    if (offset + 4U > bytes.size()) throw std::runtime_error("truncated BMP header");
    return static_cast<std::uint32_t>(bytes[offset]) |
        (static_cast<std::uint32_t>(bytes[offset + 1U]) << 8U) |
        (static_cast<std::uint32_t>(bytes[offset + 2U]) << 16U) |
        (static_cast<std::uint32_t>(bytes[offset + 3U]) << 24U);
}

std::int32_t i32(const std::vector<std::uint8_t>& bytes, const std::size_t offset) {
    return static_cast<std::int32_t>(u32(bytes, offset));
}

}  // namespace

TutorialSurface loadStoryBackgroundImage(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("cannot open Storybook background image: " + path.string());
    const std::vector<std::uint8_t> bytes(
        (std::istreambuf_iterator<char>(input)), std::istreambuf_iterator<char>());
    if (bytes.size() < 54U || bytes[0] != static_cast<std::uint8_t>('B') || bytes[1] != static_cast<std::uint8_t>('M')) {
        throw std::runtime_error("Storybook background image must be a Windows BMP: " + path.string());
    }
    const std::uint32_t pixelOffset = u32(bytes, 10U);
    const std::uint32_t dibSize = u32(bytes, 14U);
    if (dibSize < 40U) throw std::runtime_error("unsupported BMP DIB header: " + path.string());
    const std::int32_t signedWidth = i32(bytes, 18U);
    const std::int32_t signedHeight = i32(bytes, 22U);
    const std::uint16_t planes = u16(bytes, 26U);
    const std::uint16_t bitsPerPixel = u16(bytes, 28U);
    const std::uint32_t compression = u32(bytes, 30U);
    if (planes != 1U || signedWidth <= 0 || signedHeight == 0 ||
        (bitsPerPixel != 24U && bitsPerPixel != 32U) || compression != 0U) {
        throw std::runtime_error("Storybook background BMP must be uncompressed 24-bit or 32-bit RGB");
    }
    const bool topDown = signedHeight < 0;
    const std::uint32_t width = static_cast<std::uint32_t>(signedWidth);
    const std::uint32_t height = static_cast<std::uint32_t>(topDown ? -static_cast<std::int64_t>(signedHeight) : signedHeight);
    if (width == 0U || height == 0U || width > 16384U || height > 16384U) {
        throw std::runtime_error("Storybook background BMP dimensions are invalid");
    }
    const std::uint32_t bytesPerPixel = bitsPerPixel / 8U;
    const std::uint64_t rowStride = ((static_cast<std::uint64_t>(width) * bytesPerPixel + 3ULL) / 4ULL) * 4ULL;
    const std::uint64_t required = static_cast<std::uint64_t>(pixelOffset) + rowStride * height;
    if (required > bytes.size()) throw std::runtime_error("Storybook background BMP pixel data is truncated");

    TutorialSurface surface(width, height, {0U, 0U, 0U, 255U});
    for (std::uint32_t y = 0U; y < height; ++y) {
        const std::uint32_t sourceY = topDown ? y : (height - 1U - y);
        const std::size_t row = static_cast<std::size_t>(static_cast<std::uint64_t>(pixelOffset) + rowStride * sourceY);
        for (std::uint32_t x = 0U; x < width; ++x) {
            const std::size_t offset = row + static_cast<std::size_t>(x * bytesPerPixel);
            const std::uint8_t blue = bytes[offset];
            const std::uint8_t green = bytes[offset + 1U];
            const std::uint8_t red = bytes[offset + 2U];
            const std::uint8_t alpha = bitsPerPixel == 32U ? bytes[offset + 3U] : 255U;
            surface.setPixel(static_cast<int>(x), static_cast<int>(y), {red, green, blue, static_cast<std::uint8_t>(alpha == 0U ? 255U : alpha)});
        }
    }
    return surface;
}

}  // namespace clockfw::sim::tutorial
