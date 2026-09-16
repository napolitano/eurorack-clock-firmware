/**
 * @file main.cpp
 * @brief STM32CubeF4 entry point for the CLOCK eight-channel clock firmware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "app/clock_application.h"
#include "hal/platform_io.h"

#if defined(CLOCK_HOST_TEST)
namespace { clockfw::app::ClockApplication application; }
extern "C" void setup(){ application.begin(); }
extern "C" void loop(){ application.runOnce(); }
#else
int main(){
    // STM32duino initialized HAL/board hardware from its premain constructor
    // before sketch-level C++ objects were constructed. Preserve that ordering
    // after removing the Arduino runtime: no application object may be created
    // until the MCU, clocks and monotonic timers are ready.
    clockfw::hal::platform::initializeMcu();
    static clockfw::app::ClockApplication application;
    application.begin();
    while(true){ application.runOnce(); }
}
#endif
