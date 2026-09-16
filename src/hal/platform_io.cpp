/**
 * @file platform_io.cpp
 * @brief STM32CubeF4 production implementation and deterministic host/simulator adapter.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "hal/platform_io.h"

#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
#include <Arduino.h>
#else
#include <stm32f4xx_hal.h>
#endif

namespace clockfw::hal::platform {
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
void initializeMcu() {}
void configureInputPullup(const mcu::Pin pin) { pinMode(pin, INPUT_PULLUP); }
void configureOutput(const mcu::Pin pin) { pinMode(pin, OUTPUT); }
bool read(const mcu::Pin pin) { return digitalRead(pin) == HIGH; }
void write(const mcu::Pin pin, const bool high) { digitalWrite(pin, high ? HIGH : LOW); }
void attachInterrupt(const mcu::Pin pin, const InterruptCallback callback, const InterruptEdge edge) {
    const int mode = edge == InterruptEdge::Rising ? RISING : (edge == InterruptEdge::Falling ? FALLING : CHANGE);
    ::attachInterrupt(digitalPinToInterrupt(pin), callback, mode);
}
std::uint32_t milliseconds() { return millis(); }
std::uint32_t microseconds() { return micros(); }
void delayMilliseconds(const std::uint32_t durationMs) { delay(durationMs); }
std::uint32_t enterCritical() { noInterrupts(); return 0U; }
void exitCritical(const std::uint32_t) { interrupts(); }
#else
namespace {
TIM_HandleTypeDef gMicrosTimer{};
InterruptCallback gExtiCallbacks[16]{};
constexpr std::uint32_t kExtiPreemptPriority = 4U;
constexpr std::uint32_t kExtiSubPriority = 0U;

GPIO_TypeDef* portFor(const mcu::Pin pin) {
    switch ((pin >> 8U) & 0xFFU) {
        case 0xAU: return GPIOA;
        case 0xBU: return GPIOB;
        case 0xCU: return GPIOC;
        default: return nullptr;
    }
}
std::uint16_t maskFor(const mcu::Pin pin) { return static_cast<std::uint16_t>(1U << (pin & 0x0FU)); }
std::uint8_t indexFor(const mcu::Pin pin) { return static_cast<std::uint8_t>(pin & 0x0FU); }
void enablePortClock(const mcu::Pin pin) {
    switch ((pin >> 8U) & 0xFFU) {
        case 0xAU: __HAL_RCC_GPIOA_CLK_ENABLE(); break;
        case 0xBU: __HAL_RCC_GPIOB_CLK_ENABLE(); break;
        case 0xCU: __HAL_RCC_GPIOC_CLK_ENABLE(); break;
        default: break;
    }
}
IRQn_Type extiIrq(const std::uint8_t line) {
    if (line <= 4U) return static_cast<IRQn_Type>(EXTI0_IRQn + line);
    return line <= 9U ? EXTI9_5_IRQn : EXTI15_10_IRQn;
}
void configureSystemClock() {
    __HAL_RCC_PWR_CLK_ENABLE();
    __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE2);
    RCC_OscInitTypeDef osc{};
    osc.OscillatorType = RCC_OSCILLATORTYPE_HSE;
    osc.HSEState = RCC_HSE_ON;
    osc.PLL.PLLState = RCC_PLL_ON;
    osc.PLL.PLLSource = RCC_PLLSOURCE_HSE;
    osc.PLL.PLLM = 25U;
    osc.PLL.PLLN = 336U;
    osc.PLL.PLLP = RCC_PLLP_DIV4;
    osc.PLL.PLLQ = 7U;
    if (HAL_RCC_OscConfig(&osc) != HAL_OK) { while (true) {} }
    RCC_ClkInitTypeDef clk{};
    clk.ClockType = RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2;
    clk.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
    clk.AHBCLKDivider = RCC_SYSCLK_DIV1;
    clk.APB1CLKDivider = RCC_HCLK_DIV2;
    clk.APB2CLKDivider = RCC_HCLK_DIV1;
    if (HAL_RCC_ClockConfig(&clk, FLASH_LATENCY_2) != HAL_OK) { while (true) {} }
}
void configureMicrosTimer() {
    __HAL_RCC_TIM5_CLK_ENABLE();
    gMicrosTimer.Instance = TIM5;
    gMicrosTimer.Init.Prescaler = (HAL_RCC_GetPCLK1Freq() * 2U / 1000000U) - 1U;
    gMicrosTimer.Init.CounterMode = TIM_COUNTERMODE_UP;
    gMicrosTimer.Init.Period = 0xFFFFFFFFU;
    gMicrosTimer.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
    gMicrosTimer.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&gMicrosTimer) != HAL_OK || HAL_TIM_Base_Start(&gMicrosTimer) != HAL_OK) { while (true) {} }
}
}

void initializeMcu() { HAL_Init(); configureSystemClock(); configureMicrosTimer(); }
void configureInputPullup(const mcu::Pin pin) {
    enablePortClock(pin); GPIO_TypeDef* const port = portFor(pin); if (port == nullptr) return;
    GPIO_InitTypeDef init{}; init.Pin=maskFor(pin); init.Mode=GPIO_MODE_INPUT; init.Pull=GPIO_PULLUP; init.Speed=GPIO_SPEED_FREQ_LOW; HAL_GPIO_Init(port,&init);
}
void configureOutput(const mcu::Pin pin) {
    enablePortClock(pin); GPIO_TypeDef* const port = portFor(pin); if (port == nullptr) return;
    GPIO_InitTypeDef init{}; init.Pin=maskFor(pin); init.Mode=GPIO_MODE_OUTPUT_PP; init.Pull=GPIO_NOPULL; init.Speed=GPIO_SPEED_FREQ_HIGH; HAL_GPIO_Init(port,&init);
}
bool read(const mcu::Pin pin) { GPIO_TypeDef* const port=portFor(pin); return port != nullptr && HAL_GPIO_ReadPin(port,maskFor(pin)) == GPIO_PIN_SET; }
void write(const mcu::Pin pin, const bool high) { GPIO_TypeDef* const port=portFor(pin); if (port != nullptr) HAL_GPIO_WritePin(port,maskFor(pin),high?GPIO_PIN_SET:GPIO_PIN_RESET); }
void attachInterrupt(const mcu::Pin pin, const InterruptCallback callback, const InterruptEdge edge) {
    enablePortClock(pin); GPIO_TypeDef* const port=portFor(pin); if (port == nullptr) return;
    const std::uint8_t index=indexFor(pin); gExtiCallbacks[index]=callback;
    GPIO_InitTypeDef init{}; init.Pin=maskFor(pin); init.Pull=GPIO_PULLUP; init.Speed=GPIO_SPEED_FREQ_HIGH;
    init.Mode = edge==InterruptEdge::Rising ? GPIO_MODE_IT_RISING : (edge==InterruptEdge::Falling ? GPIO_MODE_IT_FALLING : GPIO_MODE_IT_RISING_FALLING);
    HAL_GPIO_Init(port,&init); const IRQn_Type irq=extiIrq(index); HAL_NVIC_SetPriority(irq,kExtiPreemptPriority,kExtiSubPriority); HAL_NVIC_EnableIRQ(irq);
}
std::uint32_t milliseconds() { return HAL_GetTick(); }
std::uint32_t microseconds() { return __HAL_TIM_GET_COUNTER(&gMicrosTimer); }
void delayMilliseconds(const std::uint32_t durationMs) { HAL_Delay(durationMs); }
std::uint32_t enterCritical() { const std::uint32_t primask=__get_PRIMASK(); __disable_irq(); return primask; }
void exitCritical(const std::uint32_t previousPrimask) { if (previousPrimask == 0U) __enable_irq(); }

extern "C" void HAL_GPIO_EXTI_Callback(const std::uint16_t gpioPin) {
    for (std::uint8_t i=0U;i<16U;++i) if (gpioPin == static_cast<std::uint16_t>(1U<<i)) { if (gExtiCallbacks[i] != nullptr) gExtiCallbacks[i](); return; }
}
#endif
} // namespace clockfw::hal::platform

#if !defined(CLOCK_HOST_TEST) && !defined(CLOCK_SIMULATOR)
extern "C" void SysTick_Handler(){ HAL_IncTick(); }
extern "C" void EXTI0_IRQHandler(){HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_0);} extern "C" void EXTI1_IRQHandler(){HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_1);} extern "C" void EXTI2_IRQHandler(){HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_2);} extern "C" void EXTI3_IRQHandler(){HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_3);} extern "C" void EXTI4_IRQHandler(){HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_4);}
extern "C" void EXTI9_5_IRQHandler(){for(std::uint16_t p=GPIO_PIN_5;p<=GPIO_PIN_9;p<<=1U) if(__HAL_GPIO_EXTI_GET_IT(p)!=RESET) HAL_GPIO_EXTI_IRQHandler(p);}
extern "C" void EXTI15_10_IRQHandler(){for(std::uint16_t p=GPIO_PIN_10;p<=GPIO_PIN_15;p<<=1U) if(__HAL_GPIO_EXTI_GET_IT(p)!=RESET) HAL_GPIO_EXTI_IRQHandler(p);}
#endif
