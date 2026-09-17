/**
 * @file ui_controller_factory_reset.cpp
 * @brief Guarded full factory-reset control flow.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/ui_controller_factory_reset.h"

#include "domain/default_configuration.h"

namespace clockfw::ui {
void UiController::confirmFactoryReset(const std::uint32_t nowMs) {
    (void)nowMs;
    if (navigation_.cursor == 0U) {
        openSettingsPage(SettingsPage::Info, 5U);
        return;
    }

    // A full-image Flash commit stalls STM32F4 instruction fetch. Stop transport
    // first so every gate is LOW and no musical timing can be interrupted.
    state_.transport = TransportState::Stopped;
    engine_.stop();

    ClockState factoryState{};
    initializeFactoryDefaults(factoryState);
    factoryState.transport = TransportState::Stopped;

    if (!persistentState_.factoryReset(factoryState)) {
        // Keep the current in-RAM configuration when persistence could not be
        // replaced. The previous committed A/B slot remains power-loss safe.
        openSettingsPage(SettingsPage::Info, 5U);
        return;
    }

    state_ = factoryState;
    engine_.updateConfiguration(state_, true);
    engine_.stop();
    navigation_.highScoreResetAvailable = false;
    navigation_.selectedChannel = 0U;
    navigation_.screen = Screen::Performance;
    navigation_.cursor = 0U;
    navigation_.scrollOffset = 0U;
    navigation_.editing = false;
    invalidate();
}

}  // namespace clockfw::ui
