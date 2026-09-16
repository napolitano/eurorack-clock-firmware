/**
 * @file oled_display_transport.cpp
 * @brief STM32CubeF4 I2C/SPI transport implementation with host-test adapters.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/oled_display.h"

#include "hal/platform_io.h"

#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
#include <SPI.h>
#include <Wire.h>
#else
#include <stm32f4xx_hal.h>
#endif

#include <algorithm>

#include "pin_map.h"

namespace clockfw::hal {
namespace {

/** Solomon-compatible control byte selecting command bytes on I2C. */
constexpr std::uint8_t kSsd1306CommandControlByte = 0x00U;

/** Solomon-compatible control byte selecting display RAM data on I2C. */
constexpr std::uint8_t kSsd1306DataControlByte = 0x40U;

/** Bounded I2C payload size used to keep display transactions short and preemptible. */
constexpr std::size_t kI2cPayloadChunkSize = 24U;

#if !defined(CLOCK_HOST_TEST) && !defined(CLOCK_SIMULATOR)
I2C_HandleTypeDef gDisplayI2c{};
SPI_HandleTypeDef gDisplaySpi{};

void enableGpioClock(GPIO_TypeDef* const port) {
    if (port == GPIOA) __HAL_RCC_GPIOA_CLK_ENABLE();
    else if (port == GPIOB) __HAL_RCC_GPIOB_CLK_ENABLE();
}
GPIO_TypeDef* portFor(const mcu::Pin pin) {
    const std::uint32_t code = (pin >> 8U) & 0xFFU;
    return code == 0xAU ? GPIOA : (code == 0xBU ? GPIOB : nullptr);
}
std::uint16_t maskFor(const mcu::Pin pin) { return static_cast<std::uint16_t>(1U << (pin & 0x0FU)); }
std::uint32_t spiPrescalerFor(const std::uint32_t peripheralClockHz, const std::uint32_t requestedHz) {
    if (requestedHz == 0U) return SPI_BAUDRATEPRESCALER_256;
    if (peripheralClockHz / 2U <= requestedHz) return SPI_BAUDRATEPRESCALER_2;
    if (peripheralClockHz / 4U <= requestedHz) return SPI_BAUDRATEPRESCALER_4;
    if (peripheralClockHz / 8U <= requestedHz) return SPI_BAUDRATEPRESCALER_8;
    if (peripheralClockHz / 16U <= requestedHz) return SPI_BAUDRATEPRESCALER_16;
    if (peripheralClockHz / 32U <= requestedHz) return SPI_BAUDRATEPRESCALER_32;
    if (peripheralClockHz / 64U <= requestedHz) return SPI_BAUDRATEPRESCALER_64;
    if (peripheralClockHz / 128U <= requestedHz) return SPI_BAUDRATEPRESCALER_128;
    return SPI_BAUDRATEPRESCALER_256;
}
#endif

bool i2cTransmit(const std::uint8_t address, const std::uint8_t control, const std::uint8_t* const data, const std::size_t size) {
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
    Wire.beginTransmission(address); Wire.write(control); Wire.write(data, size); return Wire.endTransmission() == 0U;
#else
    std::uint8_t buffer[32]{};
    if (size + 1U > sizeof(buffer)) return false;
    buffer[0] = control;
    std::copy_n(data, size, buffer + 1U);
    return HAL_I2C_Master_Transmit(&gDisplayI2c, static_cast<std::uint16_t>(address << 1U), buffer, static_cast<std::uint16_t>(size + 1U), 20U) == HAL_OK;
#endif
}

bool i2cProbe(const std::uint8_t address) {
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
    Wire.beginTransmission(address); return Wire.endTransmission() == 0U;
#else
    return HAL_I2C_IsDeviceReady(&gDisplayI2c, static_cast<std::uint16_t>(address << 1U), 2U, 10U) == HAL_OK;
#endif
}

void spiTransmit(const bool dataMode, const std::uint8_t* const data, const std::size_t size) {
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
    SPI.beginTransaction(SPISettings(config::kDisplaySpiFrequencyHz, MSBFIRST, SPI_MODE0));
    platform::write(pinmap::kDisplaySpiDataCommandPin, dataMode);
    platform::write(pinmap::kDisplaySpiChipSelectPin, false);
    for (std::size_t index=0U; index<size; ++index) SPI.transfer(data[index], SPI_TRANSMITONLY);
    platform::write(pinmap::kDisplaySpiChipSelectPin, true);
    SPI.endTransaction();
#else
    platform::write(pinmap::kDisplaySpiDataCommandPin, dataMode);
    platform::write(pinmap::kDisplaySpiChipSelectPin, false);
    (void)HAL_SPI_Transmit(&gDisplaySpi, const_cast<std::uint8_t*>(data), static_cast<std::uint16_t>(size), 20U);
    platform::write(pinmap::kDisplaySpiChipSelectPin, true);
#endif
}

}  // namespace

bool OledDisplay::beginI2c() {
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
    Wire.setSDA(pinmap::kDisplayI2cDataPin); Wire.setSCL(pinmap::kDisplayI2cClockPin); Wire.begin(); Wire.setClock(config::kDisplayI2cFrequencyHz);
#else
    __HAL_RCC_I2C1_CLK_ENABLE();
    GPIO_TypeDef* const sdaPort = portFor(pinmap::kDisplayI2cDataPin);
    GPIO_TypeDef* const sclPort = portFor(pinmap::kDisplayI2cClockPin);
    if (sdaPort == nullptr || sclPort == nullptr) return false;
    enableGpioClock(sdaPort); enableGpioClock(sclPort);
    GPIO_InitTypeDef gpio{}; gpio.Mode = GPIO_MODE_AF_OD; gpio.Pull = GPIO_PULLUP; gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH; gpio.Alternate = GPIO_AF4_I2C1;
    gpio.Pin = maskFor(pinmap::kDisplayI2cDataPin); HAL_GPIO_Init(sdaPort, &gpio);
    gpio.Pin = maskFor(pinmap::kDisplayI2cClockPin); HAL_GPIO_Init(sclPort, &gpio);
    gDisplayI2c.Instance = I2C1; gDisplayI2c.Init.ClockSpeed = config::kDisplayI2cFrequencyHz; gDisplayI2c.Init.DutyCycle = I2C_DUTYCYCLE_2; gDisplayI2c.Init.OwnAddress1 = 0U; gDisplayI2c.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT; gDisplayI2c.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE; gDisplayI2c.Init.OwnAddress2 = 0U; gDisplayI2c.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE; gDisplayI2c.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
    if (HAL_I2C_Init(&gDisplayI2c) != HAL_OK) return false;
#endif
    if (config::kDisplayI2cAddress != 0U) { i2cAddress_ = config::kDisplayI2cAddress; return isI2cDevicePresent(i2cAddress_); }
    if (isI2cDevicePresent(0x3CU)) { i2cAddress_=0x3CU; return true; }
    if (isI2cDevicePresent(0x3DU)) { i2cAddress_=0x3DU; return true; }
    return false;
}

bool OledDisplay::beginSpi() {
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
    SPI.setSCLK(pinmap::kDisplaySpiClockPin); SPI.setMOSI(pinmap::kDisplaySpiDataPin); SPI.begin();
#else
    __HAL_RCC_SPI1_CLK_ENABLE();
    GPIO_TypeDef* const sckPort = portFor(pinmap::kDisplaySpiClockPin);
    GPIO_TypeDef* const mosiPort = portFor(pinmap::kDisplaySpiDataPin);
    GPIO_TypeDef* const misoPort = portFor(pinmap::kDisplaySpiMisoPin);
    if (sckPort == nullptr || mosiPort == nullptr || misoPort == nullptr) return false;
    enableGpioClock(sckPort); enableGpioClock(mosiPort); enableGpioClock(misoPort);
    GPIO_InitTypeDef gpio{}; gpio.Mode = GPIO_MODE_AF_PP; gpio.Pull = GPIO_NOPULL; gpio.Speed = GPIO_SPEED_FREQ_VERY_HIGH; gpio.Alternate = GPIO_AF5_SPI1;
    gpio.Pin = maskFor(pinmap::kDisplaySpiClockPin); HAL_GPIO_Init(sckPort, &gpio);
    gpio.Pin = maskFor(pinmap::kDisplaySpiDataPin); HAL_GPIO_Init(mosiPort, &gpio);
    // STM32duino's dedicated SPIClass was constructed with PA6 as MISO and
    // therefore ran SPI1 in the ordinary 2-line master configuration even
    // though the OLED itself is write-only. Keep the production HAL equivalent
    // electrically and register-compatible with that proven hardware path.
    gpio.Pin = maskFor(pinmap::kDisplaySpiMisoPin); HAL_GPIO_Init(misoPort, &gpio);
    gDisplaySpi.Instance = SPI1; gDisplaySpi.Init.Mode = SPI_MODE_MASTER; gDisplaySpi.Init.Direction = SPI_DIRECTION_2LINES; gDisplaySpi.Init.DataSize = SPI_DATASIZE_8BIT; gDisplaySpi.Init.CLKPolarity = SPI_POLARITY_LOW; gDisplaySpi.Init.CLKPhase = SPI_PHASE_1EDGE; gDisplaySpi.Init.NSS = SPI_NSS_SOFT; gDisplaySpi.Init.BaudRatePrescaler = spiPrescalerFor(HAL_RCC_GetPCLK2Freq(), config::kDisplaySpiFrequencyHz); gDisplaySpi.Init.FirstBit = SPI_FIRSTBIT_MSB; gDisplaySpi.Init.TIMode = SPI_TIMODE_DISABLE; gDisplaySpi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE; gDisplaySpi.Init.CRCPolynomial = 7U;
    if (HAL_SPI_Init(&gDisplaySpi) != HAL_OK) return false;
#endif
    platform::configureOutput(pinmap::kDisplaySpiChipSelectPin); platform::configureOutput(pinmap::kDisplaySpiDataCommandPin); platform::write(pinmap::kDisplaySpiChipSelectPin,true); platform::write(pinmap::kDisplaySpiDataCommandPin,false); platform::delayMilliseconds(1U); return true;
}

void OledDisplay::resetController() {
    if (pinmap::kDisplayResetPin == pinmap::kUnassignedDigitalPin) {
        return;
    }
    platform::configureOutput(pinmap::kDisplayResetPin);
    platform::write(pinmap::kDisplayResetPin, true);
    // Let the module supply and reset line settle before asserting RESET low.
    platform::delayMilliseconds(20U);
    platform::write(pinmap::kDisplayResetPin, false);
    platform::delayMilliseconds(10U);
    platform::write(pinmap::kDisplayResetPin, true);
    // Keep the controller out of command traffic briefly after reset release.
    platform::delayMilliseconds(20U);
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

#if defined(CLOCK_SIMULATOR)
    // Feed the same command bytes that go to the transport into the virtual OLED
    // controller. The simulator therefore follows A0/A1 and C0/C8 exactly like
    // the physical SSD1306/SSD1315 instead of mirroring application state.
    for (std::size_t index = 0U; index < size; ++index) {
        observeControllerCommandForSimulator(commands[index]);
    }
#endif

    if (config::kDisplayTransport == config::DisplayTransport::I2c) { (void)i2cTransmit(i2cAddress_, kSsd1306CommandControlByte, commands, size); return; }
    spiTransmit(false, commands, size);
}

void OledDisplay::sendData(const std::uint8_t* const data, const std::size_t size) {
    if (config::kDisplayTransport == config::DisplayTransport::I2c) {
        std::size_t offset=0U; while (offset<size) { const std::size_t chunkSize=std::min(kI2cPayloadChunkSize,size-offset); (void)i2cTransmit(i2cAddress_,kSsd1306DataControlByte,data+offset,chunkSize); offset+=chunkSize; } return;
    }
    spiTransmit(true, data, size);
}


bool OledDisplay::sendI2cDataChunk(const std::uint8_t* const data, const std::size_t size) { return data != nullptr && size != 0U && i2cTransmit(i2cAddress_, kSsd1306DataControlByte, data, size); }

bool OledDisplay::sendI2cCommandChunk(const std::uint8_t* const commands, const std::size_t size) { return commands != nullptr && size != 0U && i2cTransmit(i2cAddress_, kSsd1306CommandControlByte, commands, size); }

bool OledDisplay::isI2cDevicePresent(const std::uint8_t address) { return i2cProbe(address); }


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
