/**
 * @file periodic_timer.cpp
 * @brief STM32CubeF4 TIM3 scheduler implementation with deterministic host adapter.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "hal/periodic_timer.h"
#include "config.h"
#if !defined(CLOCK_HOST_TEST) && !defined(CLOCK_SIMULATOR)
#include <stm32f4xx_hal.h>
#endif
namespace clockfw::hal {
#if defined(CLOCK_HOST_TEST) || defined(CLOCK_SIMULATOR)
PeriodicTimer::PeriodicTimer() : timer_(TIM3) {}
void PeriodicTimer::start(const std::uint32_t frequencyHz, const Callback callback) {
    timer_.pause(); timer_.setOverflow(frequencyHz,HERTZ_FORMAT); timer_.setInterruptPriority(config::kSchedulerInterruptPreemptPriority,config::kSchedulerInterruptSubPriority); timer_.attachInterrupt(callback); timer_.resume();
}
void PeriodicTimer::stop(){timer_.pause();}
#else
namespace { TIM_HandleTypeDef gSchedulerTimer{}; PeriodicTimer::Callback gCallback=nullptr; }
PeriodicTimer::PeriodicTimer() = default;
void PeriodicTimer::start(const std::uint32_t frequencyHz, const Callback callback) {
    stop(); __HAL_RCC_TIM3_CLK_ENABLE(); gCallback=callback;
    const std::uint32_t timerClockHz = HAL_RCC_GetPCLK1Freq() * 2U;
    const std::uint32_t prescaler = 83U;
    const std::uint32_t tickHz = timerClockHz / (prescaler + 1U);
    gSchedulerTimer.Instance=TIM3; gSchedulerTimer.Init.Prescaler=prescaler; gSchedulerTimer.Init.CounterMode=TIM_COUNTERMODE_UP; gSchedulerTimer.Init.Period=(tickHz/frequencyHz)-1U; gSchedulerTimer.Init.ClockDivision=TIM_CLOCKDIVISION_DIV1; gSchedulerTimer.Init.AutoReloadPreload=TIM_AUTORELOAD_PRELOAD_DISABLE;
    if (HAL_TIM_Base_Init(&gSchedulerTimer) != HAL_OK) return;
    __HAL_TIM_CLEAR_FLAG(&gSchedulerTimer,TIM_FLAG_UPDATE); __HAL_TIM_ENABLE_IT(&gSchedulerTimer,TIM_IT_UPDATE);
    HAL_NVIC_SetPriority(TIM3_IRQn,config::kSchedulerInterruptPreemptPriority,config::kSchedulerInterruptSubPriority); HAL_NVIC_EnableIRQ(TIM3_IRQn); (void)HAL_TIM_Base_Start_IT(&gSchedulerTimer);
}
void PeriodicTimer::stop(){ if(gSchedulerTimer.Instance==TIM3){(void)HAL_TIM_Base_Stop_IT(&gSchedulerTimer); HAL_NVIC_DisableIRQ(TIM3_IRQn);} }
#endif
} // namespace clockfw::hal
#if !defined(CLOCK_HOST_TEST) && !defined(CLOCK_SIMULATOR)
extern "C" void TIM3_IRQHandler(){ if(__HAL_TIM_GET_FLAG(&clockfw::hal::gSchedulerTimer,TIM_FLAG_UPDATE)!=RESET && __HAL_TIM_GET_IT_SOURCE(&clockfw::hal::gSchedulerTimer,TIM_IT_UPDATE)!=RESET){__HAL_TIM_CLEAR_IT(&clockfw::hal::gSchedulerTimer,TIM_IT_UPDATE); if(clockfw::hal::gCallback!=nullptr) clockfw::hal::gCallback();}}
#endif
