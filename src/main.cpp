/**
 * @file main.cpp
 * @brief Arduino framework entry points for the CLOCK eight-channel clock firmware.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <Arduino.h>

#include "app/clock_application.h"

namespace {

/** Single top-level application instance; all subsystem ownership lives inside it. */
clockfw::app::ClockApplication application;

}  // namespace

/**
 * Initializes all hardware and firmware subsystems.
 *
 * Arduino.h declares setup() with C linkage. Keeping that declaration visible
 * here is required so STM32duino's framework main() can resolve this entry point.
 */
void setup() {
    application.begin();
}

/**
 * Runs one non-real-time control/UI iteration.
 *
 * Like setup(), this definition inherits the C linkage declared by Arduino.h.
 */
void loop() {
    application.runOnce();
}
