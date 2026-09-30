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

std::uint32_t ClockApplication::translateExternalTapTimestampMs(
    const std::uint32_t nowMs,
    const std::uint32_t nowUs,
    const std::uint32_t edgeTimestampUs) {
    const std::uint32_t edgeAgeUs = nowUs - edgeTimestampUs;
    return nowMs - edgeAgeUs / 1000U;
}

#ifdef CLOCK_HOST_TEST
std::uint32_t ClockApplication::translateExternalTapTimestampMsForTest(
    const std::uint32_t nowMs,
    const std::uint32_t nowUs,
    const std::uint32_t edgeTimestampUs) {
    return translateExternalTapTimestampMs(nowMs, nowUs, edgeTimestampUs);
}
#endif

#ifdef CLOCK_SIMULATOR
void ClockApplication::flushPersistenceForSimulator() {
    persistentState_.flushPendingForSimulator();
    (void)customGrooveStore_.service(true);
    (void)sequencerPatternStore_.flush();
    (void)sequencerStepStore_.flush();
}
#endif

}  // namespace clockfw::app
