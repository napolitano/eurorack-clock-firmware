/**
 * @file main.cpp
 * @brief STM32CubeF4 entry point for the CLOCK eight-channel clock firmware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "app/clock_application.h"
#include "hal/platform_io.h"

namespace { clockfw::app::ClockApplication application; }

#if defined(CLOCK_HOST_TEST)
extern "C" void setup(){ application.begin(); }
extern "C" void loop(){ application.runOnce(); }
#else
int main(){
    clockfw::hal::platform::initializeMcu();
    application.begin();
    while(true){ application.runOnce(); }
}
#endif
