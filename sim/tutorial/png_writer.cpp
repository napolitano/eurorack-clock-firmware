/**
 * @file png_writer.cpp
 * @brief Implements deterministic PNG output with a small fixed-Huffman DEFLATE encoder.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/png_writer.h"

#include <array>
#include <cstddef>
#include <cstdint>
#include <fstream>
#include <iomanip>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

namespace clockfw::sim::tutorial {
namespace {

class BitWriter final {
public:
    void bits(std::uint32_t value, const unsigned count) {
        for (unsigned bit = 0U; bit < count; ++bit) {
            current_ |= static_cast<std::uint8_t>(((value >> bit) & 1U) << used_);
            ++used_;
            if (used_ == 8U) {
                bytes_.push_back(current_);
                current_ = 0U;
                used_ = 0U;
            }
        }
    }

    std::vector<std::uint8_t> finish() {
        if (used_ != 0U) bytes_.push_back(current_);
        return std::move(bytes_);
    }

private:
    std::vector<std::uint8_t> bytes_{};
    std::uint8_t current_ = 0U;
    unsigned used_ = 0U;
};

std::uint32_t reverseBits(std::uint32_t value, const unsigned count) {
    std::uint32_t reversed = 0U;
    for (unsigned index = 0U; index < count; ++index) {
        reversed = (reversed << 1U) | ((value >> index) & 1U);
    }
    return reversed;
}

void fixedSymbol(BitWriter& writer, const unsigned symbol) {
    if (symbol <= 143U) {
        writer.bits(reverseBits(0x30U + symbol, 8U), 8U);
    } else if (symbol <= 255U) {
        writer.bits(reverseBits(0x190U + (symbol - 144U), 9U), 9U);
    } else if (symbol <= 279U) {
        writer.bits(reverseBits(symbol - 256U, 7U), 7U);
    } else if (symbol <= 287U) {
        writer.bits(reverseBits(0xC0U + (symbol - 280U), 8U), 8U);
    } else {
        throw std::runtime_error("invalid DEFLATE fixed Huffman symbol");
    }
}

void fixedLengthDistanceOne(BitWriter& writer, const unsigned length) {
    static constexpr std::array<unsigned, 29U> bases{{
        3U, 4U, 5U, 6U, 7U, 8U, 9U, 10U, 11U, 13U, 15U, 17U, 19U, 23U, 27U,
        31U, 35U, 43U, 51U, 59U, 67U, 83U, 99U, 115U, 131U, 163U, 195U, 227U, 258U}};
    static constexpr std::array<unsigned, 29U> extras{{
        0U, 0U, 0U, 0U, 0U, 0U, 0U, 0U, 1U, 1U, 1U, 1U, 2U, 2U, 2U,
        2U, 3U, 3U, 3U, 3U, 4U, 4U, 4U, 4U, 5U, 5U, 5U, 5U, 0U}};
    if (length < 3U || length > 258U) throw std::runtime_error("invalid DEFLATE match length");
    for (std::size_t index = 0U; index < bases.size(); ++index) {
        const unsigned maxValue = index + 1U < bases.size() ? bases[index + 1U] - 1U : 258U;
        if (length < bases[index] || length > maxValue) continue;
        fixedSymbol(writer, 257U + static_cast<unsigned>(index));
        const unsigned extraCount = extras[index];
        if (extraCount != 0U) writer.bits(length - bases[index], extraCount);
        writer.bits(0U, 5U);  // Fixed distance code 0 = distance 1.
        return;
    }
    throw std::runtime_error("unable to encode DEFLATE match length");
}

std::uint32_t adler32(const std::vector<std::uint8_t>& bytes) {
    constexpr std::uint32_t modulus = 65521U;
    std::uint32_t a = 1U;
    std::uint32_t b = 0U;
    for (const std::uint8_t byte : bytes) {
        a = (a + byte) % modulus;
        b = (b + a) % modulus;
    }
    return (b << 16U) | a;
}

std::uint32_t crc32(const std::uint8_t* const data, const std::size_t size) {
    std::uint32_t crc = 0xFFFFFFFFU;
    for (std::size_t index = 0U; index < size; ++index) {
        crc ^= data[index];
        for (unsigned bit = 0U; bit < 8U; ++bit) {
            const std::uint32_t mask = 0U - (crc & 1U);
            crc = (crc >> 1U) ^ (0xEDB88320U & mask);
        }
    }
    return crc ^ 0xFFFFFFFFU;
}

void appendBe32(std::vector<std::uint8_t>& out, const std::uint32_t value) {
    out.push_back(static_cast<std::uint8_t>((value >> 24U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 16U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>((value >> 8U) & 0xFFU));
    out.push_back(static_cast<std::uint8_t>(value & 0xFFU));
}

void appendChunk(
    std::vector<std::uint8_t>& png,
    const std::array<std::uint8_t, 4U>& type,
    const std::vector<std::uint8_t>& data) {
    if (data.size() > 0xFFFFFFFFULL) throw std::runtime_error("PNG chunk too large");
    appendBe32(png, static_cast<std::uint32_t>(data.size()));
    const std::size_t crcStart = png.size();
    png.insert(png.end(), type.begin(), type.end());
    png.insert(png.end(), data.begin(), data.end());
    appendBe32(png, crc32(png.data() + crcStart, 4U + data.size()));
}

std::vector<std::uint8_t> filteredRgba(const TutorialSurface& surface) {
    const std::size_t rowBytes = surface.width() * 4U;
    std::vector<std::uint8_t> filtered;
    filtered.reserve((rowBytes + 1U) * surface.height());
    for (std::size_t y = 0U; y < surface.height(); ++y) {
        filtered.push_back(1U);  // PNG Sub filter.
        for (std::size_t x = 0U; x < surface.width(); ++x) {
            const TutorialColor pixel = surface.pixel(x, y);
            const std::array<std::uint8_t, 4U> channels{{pixel.red, pixel.green, pixel.blue, pixel.alpha}};
            for (std::size_t channel = 0U; channel < channels.size(); ++channel) {
                const std::uint8_t left = x == 0U ? 0U : [&]() {
                    const TutorialColor previous = surface.pixel(x - 1U, y);
                    const std::array<std::uint8_t, 4U> previousChannels{{
                        previous.red, previous.green, previous.blue, previous.alpha}};
                    return previousChannels[channel];
                }();
                filtered.push_back(static_cast<std::uint8_t>(channels[channel] - left));
            }
        }
    }
    return filtered;
}

std::vector<std::uint8_t> deflateFixed(const std::vector<std::uint8_t>& raw) {
    BitWriter writer;
    writer.bits(1U, 1U);  // BFINAL
    writer.bits(1U, 2U);  // BTYPE=01, fixed Huffman.

    std::size_t index = 0U;
    while (index < raw.size()) {
        if (index > 0U && raw[index] == raw[index - 1U]) {
            unsigned run = 1U;
            while (index + run < raw.size() && raw[index + run] == raw[index - 1U] && run < 258U) {
                ++run;
            }
            if (run >= 3U) {
                fixedLengthDistanceOne(writer, run);
                index += run;
                continue;
            }
        }
        fixedSymbol(writer, raw[index]);
        ++index;
    }
    fixedSymbol(writer, 256U);
    return writer.finish();
}

std::vector<std::uint8_t> makePng(const TutorialSurface& surface) {
    if (surface.width() == 0U || surface.height() == 0U ||
        surface.width() > 0xFFFFFFFFULL || surface.height() > 0xFFFFFFFFULL) {
        throw std::runtime_error("invalid PNG dimensions");
    }
    std::vector<std::uint8_t> png{
        137U, 80U, 78U, 71U, 13U, 10U, 26U, 10U};

    std::vector<std::uint8_t> ihdr;
    appendBe32(ihdr, static_cast<std::uint32_t>(surface.width()));
    appendBe32(ihdr, static_cast<std::uint32_t>(surface.height()));
    ihdr.insert(ihdr.end(), {8U, 6U, 0U, 0U, 0U});  // RGBA8, default compression/filter/interlace.
    appendChunk(png, {{'I', 'H', 'D', 'R'}}, ihdr);

    const std::vector<std::uint8_t> raw = filteredRgba(surface);
    std::vector<std::uint8_t> zlib{0x78U, 0x01U};
    const std::vector<std::uint8_t> deflated = deflateFixed(raw);
    zlib.insert(zlib.end(), deflated.begin(), deflated.end());
    appendBe32(zlib, adler32(raw));
    appendChunk(png, {{'I', 'D', 'A', 'T'}}, zlib);
    appendChunk(png, {{'I', 'E', 'N', 'D'}}, {});
    return png;
}

}  // namespace

void writeTutorialPng(const std::filesystem::path& path, const TutorialSurface& surface) {
    const std::vector<std::uint8_t> png = makePng(surface);
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) throw std::runtime_error("cannot create PNG frame: " + path.string());
    stream.write(reinterpret_cast<const char*>(png.data()), static_cast<std::streamsize>(png.size()));
    if (!stream) throw std::runtime_error("cannot write PNG frame: " + path.string());
}

std::uint64_t tutorialRgbaFnv1a64(const TutorialSurface& surface) {
    constexpr std::uint64_t offset = 14695981039346656037ULL;
    constexpr std::uint64_t prime = 1099511628211ULL;
    std::uint64_t hash = offset;
    for (const TutorialColor pixel : surface.pixels()) {
        const std::array<std::uint8_t, 4U> channels{{pixel.red, pixel.green, pixel.blue, pixel.alpha}};
        for (const std::uint8_t channel : channels) {
            hash ^= channel;
            hash *= prime;
        }
    }
    return hash;
}

std::string formatDigest64(const std::uint64_t digest) {
    std::ostringstream stream;
    stream << std::hex << std::nouppercase << std::setfill('0') << std::setw(16) << digest;
    return stream.str();
}

}  // namespace clockfw::sim::tutorial
