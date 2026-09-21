/**
 * @file external_sync_controller_inputs.cpp
 * @brief Role and transport interpretation for the two configurable comparator inputs.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/external_sync_controller.h"

namespace clockfw::services {

void ExternalSyncController::applyInputConfiguration() {
    if (inputRolesChanged_) {
        // The same electrical edge must never be reinterpreted under a newly
        // selected role.
        drainInput(0);
        drainInput(1);
        inputRolesChanged_ = false;
        runLevelInitialized_ = false;
    }

    if (!configurationDirty_) {
        return;
    }
    configurationDirty_ = false;

    const int resetInput = assignedInput(InputFunction::Reset);
    if (resetInput >= 0 && settings_.resetMode == ExternalResetMode::Gate) {
        resetGateApplied_ = inputLevelHigh(resetInput);
        engine_.setExternalResetGateFromIsr(resetGateApplied_);
    } else if (resetGateApplied_) {
        resetGateApplied_ = false;
        engine_.setExternalResetGateFromIsr(false);
    }
}

void ExternalSyncController::processResetEdges() {
    const int inputIndex = assignedInput(InputFunction::Reset);
    if (inputIndex < 0) {
        return;
    }

    hal::ExternalInputEdge edge{};
    bool sawEdge = false;
    bool triggerRequested = false;
    while (popInputEdge(inputIndex, edge)) {
        sawEdge = true;
        if (settings_.resetMode == ExternalResetMode::Trigger) {
            // Continuity loss is conservative for RESET: a reset edge may have
            // been dropped, so one reset is safer than silently missing it.
            triggerRequested = triggerRequested || edge.high || edge.continuityLost;
        }
    }

    if (settings_.resetMode == ExternalResetMode::Trigger) {
        if (triggerRequested) {
            engine_.resetGlobalPhaseFromIsr();
        }
        return;
    }
    if (!sawEdge) {
        return;
    }

    const bool high = inputLevelHigh(inputIndex);
    if (high != resetGateApplied_) {
        resetGateApplied_ = high;
        engine_.setExternalResetGateFromIsr(high);
    }
}

void ExternalSyncController::processTransportEdge(const InputFunction function) {
    const int inputIndex = assignedInput(function);
    if (inputIndex < 0) {
        return;
    }

    hal::ExternalInputEdge edge{};
    while (popInputEdge(inputIndex, edge)) {
        // START/STOP/RESTART/TAP are positive-edge commands. An overflow marker
        // is not sufficient evidence that such a command occurred, so it is
        // deliberately ignored rather than synthesized.
        if (edge.continuityLost || !edge.high) {
            continue;
        }

        switch (function) {
            case InputFunction::Start:
                requestPlayFromInput();
                break;
            case InputFunction::Stop:
                requestStopFromInput();
                break;
            case InputFunction::Restart:
                requestRestartFromInput();
                break;
            case InputFunction::Tap:
                pendingTapTimestampUs_ = edge.timestampUs;
                pendingTap_ = true;
                break;
            default:
                break;
        }
    }
}

void ExternalSyncController::processRunLevel() {
    const int inputIndex = assignedInput(InputFunction::Run);
    if (inputIndex < 0) {
        runLevelInitialized_ = false;
        return;
    }

    // RUN is level-authoritative, not edge-authoritative. Drain transitions and
    // use the current comparator latch so queue collapse cannot leave transport
    // in the wrong state.
    drainInput(inputIndex);
    const bool high = inputLevelHigh(inputIndex);
    if (runLevelInitialized_ && high == runLevelApplied_) {
        return;
    }

    runLevelInitialized_ = true;
    runLevelApplied_ = high;
    if (high) {
        requestPlayFromInput();
    } else {
        requestStopFromInput();
    }
}

void ExternalSyncController::drainInactiveInputs() {
    if (inputs_.input1 == InputFunction::Off || inputs_.input1 == InputFunction::Fill) {
        drainInput(0);
    }
    if (inputs_.input2 == InputFunction::Off || inputs_.input2 == InputFunction::Fill) {
        drainInput(1);
    }
}


}  // namespace clockfw::services
