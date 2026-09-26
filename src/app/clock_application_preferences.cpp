/**
 * @file clock_application_preferences.cpp
 * @brief Device-local preference synchronization for the CLOCK composition root.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "app/clock_application.h"

namespace clockfw::app {
void ClockApplication::synchronizeDevicePreferences(const bool force) {
    if (force || !devicePreferencesApplied_ ||
        state_.device.encoderDirectionReversed != appliedEncoderDirectionReversed_) {
        controlPanel_.setEncoderDirectionReversed(state_.device.encoderDirectionReversed);
        appliedEncoderDirectionReversed_ = state_.device.encoderDirectionReversed;
    }
    if (force || !devicePreferencesApplied_ ||
        state_.device.displayRotated180 != appliedDisplayRotated180_) {
        display_.setRotation180(state_.device.displayRotated180);
        appliedDisplayRotated180_ = state_.device.displayRotated180;
        uiController_.invalidate();
    }
    devicePreferencesApplied_ = true;
}

}  // namespace clockfw::app
