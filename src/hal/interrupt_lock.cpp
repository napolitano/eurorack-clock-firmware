/**
 * @file interrupt_lock.cpp
 * @brief RAII critical-section primitive for synchronizing foreground code with timer ISRs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/interrupt_lock.h"

#include <Arduino.h>

namespace clockfw::hal {

InterruptLock::InterruptLock() {
    noInterrupts();
}

InterruptLock::~InterruptLock() {
    interrupts();
}

}  // namespace clockfw::hal
