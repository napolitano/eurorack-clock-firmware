/**
 * @file interrupt_lock.h
 * @brief RAII critical-section primitive for synchronizing foreground code with timer ISRs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

namespace clockfw::hal {

/**
 * @brief Disables interrupts for one lexical scope and restores them on destruction.
 *
 * The firmware uses this only from foreground code when copying data shared with
 * the scheduler ISR. It must not be nested or constructed inside an ISR.
 */
class InterruptLock final {
public:
    /** @brief Enters the critical section by disabling interrupts. */
    InterruptLock();

    /** @brief Leaves the critical section by re-enabling interrupts. */
    ~InterruptLock();

    /** @brief Prevents copying a critical-section guard. */
    InterruptLock(const InterruptLock&) = delete;

    /** @brief Prevents copy-assignment of a critical-section guard. */
    InterruptLock& operator=(const InterruptLock&) = delete;
};

}  // namespace clockfw::hal
