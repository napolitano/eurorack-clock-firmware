/**
 * @file oled_display_transport.cpp
 * @brief STM32duino I2C/SPI transport implementation for the configurable OLED HAL.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/oled_display.h"

#include <Arduino.h>
#include <SPI.h>
#include <Wire.h>

#include <algorithm>

#include "pin_map.h"

namespace clockfw::hal {
namespace {

/** Solomon-compatible control byte selecting command bytes on I2C. */
constexpr std::uint8_t kSsd1306CommandControlByte = 0x00U;

/** Solomon-compatible control byte selecting display RAM data on I2C. */
constexpr std::uint8_t kSsd1306DataControlByte = 0x40U;

/** Conservative payload size that fits common Arduino Wire TX buffers. */
constexpr std::size_t kI2cPayloadChunkSize = 24U;

#if !defined(CLOCK_HOST_TEST) && !defined(CLOCK_SIMULATOR)
/** Dedicated SPI1 object for the OLED. SSEL stays unassigned because PA4 is controlled explicitly as GPIO. */
SPIClass gDisplaySpi(
    pinmap::kDisplaySpiDataPin,
    pinmap::kDisplaySpiMisoPin,
    pinmap::kDisplaySpiClockPin,
    PNUM_NOT_DEFINED);
#endif

SPIClass& displaySpi() {
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
    return SPI;
#else
    return gDisplaySpi;
#endif
}

}  // namespace

bool OledDisplay::beginI2c() {
    Wire.setSDA(pinmap::kDisplayI2cDataPin);
    Wire.setSCL(pinmap::kDisplayI2cClockPin);
    Wire.begin();
    Wire.setClock(config::kDisplayI2cFrequencyHz);

    if (config::kDisplayI2cAddress != 0U) {
        i2cAddress_ = config::kDisplayI2cAddress;
        return isI2cDevicePresent(i2cAddress_);
    }
    if (isI2cDevicePresent(0x3CU)) {
        i2cAddress_ = 0x3CU;
        return true;
    }
    if (isI2cDevicePresent(0x3DU)) {
        i2cAddress_ = 0x3DU;
        return true;
    }
    return false;
}

bool OledDisplay::beginSpi() {
    // The display owns a dedicated SPI object built from the configured
    // Arduino digital-pin numbers. MISO is required by STM32duino's SPIClass
    // setup but remains electrically unconnected at the write-only OLED.
    // Chip select is deliberately managed as an ordinary GPIO.
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
    // Host shims use the global SPI object, so mirror the production pin
    // assignment explicitly for regression visibility.
    displaySpi().setSCLK(pinmap::kDisplaySpiClockPin);
    displaySpi().setMOSI(pinmap::kDisplaySpiDataPin);
#endif
    displaySpi().begin();

    pinMode(pinmap::kDisplaySpiChipSelectPin, OUTPUT);
    pinMode(pinmap::kDisplaySpiDataCommandPin, OUTPUT);
    digitalWrite(pinmap::kDisplaySpiChipSelectPin, HIGH);
    digitalWrite(pinmap::kDisplaySpiDataCommandPin, LOW);
    delay(1U);
    return true;
}

void OledDisplay::resetController() {
    if (pinmap::kDisplayResetPin == pinmap::kUnassignedDigitalPin) {
        return;
    }
    pinMode(pinmap::kDisplayResetPin, OUTPUT);
    digitalWrite(pinmap::kDisplayResetPin, HIGH);
    // Let the module supply and reset line settle before asserting RESET low.
    delay(20U);
    digitalWrite(pinmap::kDisplayResetPin, LOW);
    delay(10U);
    digitalWrite(pinmap::kDisplayResetPin, HIGH);
    // Keep the controller out of command traffic briefly after reset release.
    delay(20U);
}

void OledDisplay::sendCommand(const std::uint8_t command) {
    sendCommands(&command, 1U);
}

void OledDisplay::sendCommands(
    const std::uint8_t* const commands,
    const std::size_t size) {
    if (commands == nullptr || size == 0U) {
        return;
    }

    if (config::kDisplayTransport == config::DisplayTransport::I2c) {
        Wire.beginTransmission(i2cAddress_);
        Wire.write(kSsd1306CommandControlByte);
        Wire.write(commands, size);
        Wire.endTransmission();
        return;
    }

    displaySpi().beginTransaction(SPISettings(config::kDisplaySpiFrequencyHz, MSBFIRST, SPI_MODE0));
    digitalWrite(pinmap::kDisplaySpiDataCommandPin, LOW);
    digitalWrite(pinmap::kDisplaySpiChipSelectPin, LOW);
    for (std::size_t index = 0U; index < size; ++index) {
        displaySpi().transfer(commands[index], SPI_TRANSMITONLY);
    }
    digitalWrite(pinmap::kDisplaySpiChipSelectPin, HIGH);
    displaySpi().endTransaction();
}

void OledDisplay::sendData(const std::uint8_t* const data, const std::size_t size) {
    if (config::kDisplayTransport == config::DisplayTransport::I2c) {
        std::size_t offset = 0U;
        while (offset < size) {
            const std::size_t chunkSize = std::min(kI2cPayloadChunkSize, size - offset);
            Wire.beginTransmission(i2cAddress_);
            Wire.write(kSsd1306DataControlByte);
            Wire.write(data + offset, chunkSize);
            Wire.endTransmission();
            offset += chunkSize;
        }
        return;
    }

    displaySpi().beginTransaction(SPISettings(config::kDisplaySpiFrequencyHz, MSBFIRST, SPI_MODE0));
    digitalWrite(pinmap::kDisplaySpiDataCommandPin, HIGH);
    digitalWrite(pinmap::kDisplaySpiChipSelectPin, LOW);
    for (std::size_t index = 0U; index < size; ++index) {
        displaySpi().transfer(data[index], SPI_TRANSMITONLY);
    }
    digitalWrite(pinmap::kDisplaySpiChipSelectPin, HIGH);
    displaySpi().endTransaction();
}


bool OledDisplay::sendI2cDataChunk(
    const std::uint8_t* const data,
    const std::size_t size) {
    if (data == nullptr || size == 0U) {
        return false;
    }
    Wire.beginTransmission(i2cAddress_);
    Wire.write(kSsd1306DataControlByte);
    Wire.write(data, size);
    return Wire.endTransmission() == 0U;
}

bool OledDisplay::sendI2cCommandChunk(
    const std::uint8_t* const commands,
    const std::size_t size) {
    if (commands == nullptr || size == 0U) {
        return false;
    }
    Wire.beginTransmission(i2cAddress_);
    Wire.write(kSsd1306CommandControlByte);
    Wire.write(commands, size);
    return Wire.endTransmission() == 0U;
}

bool OledDisplay::isI2cDevicePresent(const std::uint8_t address) {
    Wire.beginTransmission(address);
    return Wire.endTransmission() == 0U;
}


void OledDisplay::present() {
    if (config::kDisplayTransport == config::DisplayTransport::I2c) {
        queueI2cPresent();
        return;
    }
    presentImmediate();
}

bool OledDisplay::service() {
    if (config::kDisplayTransport != config::DisplayTransport::I2c) {
        return false;
    }
    return serviceI2cRefresh();
}

void OledDisplay::presentImmediate() {
    constexpr std::size_t kPageWidth = static_cast<std::size_t>(config::kDisplayWidth);
    constexpr std::size_t kPageCount = config::kDisplayHeight / 8U;

    std::size_t page = 0U;
    while (page < kPageCount) {
        const std::size_t pageOffset = page * kPageWidth;
        const bool pageChanged = !hasPresentedFramebuffer_ || !std::equal(
            framebuffer_.begin() + static_cast<std::ptrdiff_t>(pageOffset),
            framebuffer_.begin() + static_cast<std::ptrdiff_t>(pageOffset + kPageWidth),
            presentedFramebuffer_.begin() + static_cast<std::ptrdiff_t>(pageOffset));
        if (!pageChanged) {
            ++page;
            continue;
        }

        const std::size_t firstDirtyPage = page;
        std::size_t lastDirtyPage = page;
        while (lastDirtyPage + 1U < kPageCount) {
            const std::size_t nextPage = lastDirtyPage + 1U;
            const std::size_t nextOffset = nextPage * kPageWidth;
            const bool nextPageChanged = !hasPresentedFramebuffer_ || !std::equal(
                framebuffer_.begin() + static_cast<std::ptrdiff_t>(nextOffset),
                framebuffer_.begin() + static_cast<std::ptrdiff_t>(nextOffset + kPageWidth),
                presentedFramebuffer_.begin() + static_cast<std::ptrdiff_t>(nextOffset));
            if (!nextPageChanged) {
                break;
            }
            lastDirtyPage = nextPage;
        }

        const std::uint8_t addressWindow[] = {
            0x21U, 0x00U, 0x7FU,
            0x22U,
            static_cast<std::uint8_t>(firstDirtyPage),
            static_cast<std::uint8_t>(lastDirtyPage)};
        sendCommands(addressWindow, sizeof(addressWindow));

        const std::size_t firstByte = firstDirtyPage * kPageWidth;
        const std::size_t byteCount = (lastDirtyPage - firstDirtyPage + 1U) * kPageWidth;
        sendData(framebuffer_.data() + firstByte, byteCount);
        std::copy_n(
            framebuffer_.begin() + static_cast<std::ptrdiff_t>(firstByte),
            byteCount,
            presentedFramebuffer_.begin() + static_cast<std::ptrdiff_t>(firstByte));
        page = lastDirtyPage + 1U;
    }

    hasPresentedFramebuffer_ = true;
}

void OledDisplay::queueI2cPresent() {
    queuedI2cFramebuffer_ = framebuffer_;
    i2cFrameQueued_ = true;
    i2cRefreshPage_ = 0U;
    i2cRefreshOffset_ = 0U;
    i2cRefreshPhase_ = I2cRefreshPhase::SetPageWindow;
}

bool OledDisplay::serviceI2cRefresh() {
    constexpr std::size_t kPageWidth = static_cast<std::size_t>(config::kDisplayWidth);
    constexpr std::uint8_t kPageCount = static_cast<std::uint8_t>(config::kDisplayHeight / 8U);
    constexpr std::size_t kChunkSize = 24U;

    if (!i2cFrameQueued_) {
        return false;
    }

    if (i2cRefreshPhase_ == I2cRefreshPhase::SetPageWindow) {
        while (i2cRefreshPage_ < kPageCount) {
            const std::size_t pageOffset = static_cast<std::size_t>(i2cRefreshPage_) * kPageWidth;
            const bool pageChanged = !hasPresentedFramebuffer_ || !std::equal(
                queuedI2cFramebuffer_.begin() + static_cast<std::ptrdiff_t>(pageOffset),
                queuedI2cFramebuffer_.begin() + static_cast<std::ptrdiff_t>(pageOffset + kPageWidth),
                presentedFramebuffer_.begin() + static_cast<std::ptrdiff_t>(pageOffset));
            if (pageChanged) {
                break;
            }
            ++i2cRefreshPage_;
        }

        if (i2cRefreshPage_ >= kPageCount) {
            i2cFrameQueued_ = false;
            i2cRefreshPhase_ = I2cRefreshPhase::Idle;
            hasPresentedFramebuffer_ = true;
            return false;
        }

        const std::uint8_t addressWindow[] = {
            0x21U, 0x00U, 0x7FU,
            0x22U, i2cRefreshPage_, i2cRefreshPage_};
        if (sendI2cCommandChunk(addressWindow, sizeof(addressWindow))) {
            i2cRefreshOffset_ = 0U;
            i2cRefreshPhase_ = I2cRefreshPhase::SendPageData;
        }
        return true;
    }

    if (i2cRefreshPhase_ == I2cRefreshPhase::SendPageData) {
        const std::size_t pageOffset = static_cast<std::size_t>(i2cRefreshPage_) * kPageWidth;
        const std::size_t remaining = kPageWidth - i2cRefreshOffset_;
        const std::size_t chunkSize = std::min(kChunkSize, remaining);
        const std::size_t byteOffset = pageOffset + i2cRefreshOffset_;
        if (sendI2cDataChunk(queuedI2cFramebuffer_.data() + byteOffset, chunkSize)) {
            std::copy_n(
                queuedI2cFramebuffer_.begin() + static_cast<std::ptrdiff_t>(byteOffset),
                chunkSize,
                presentedFramebuffer_.begin() + static_cast<std::ptrdiff_t>(byteOffset));
            i2cRefreshOffset_ = static_cast<std::uint8_t>(i2cRefreshOffset_ + chunkSize);
            if (i2cRefreshOffset_ >= kPageWidth) {
                ++i2cRefreshPage_;
                i2cRefreshOffset_ = 0U;
                i2cRefreshPhase_ = I2cRefreshPhase::SetPageWindow;
            }
        }
        return true;
    }

    return false;
}

}  // namespace clockfw::hal
