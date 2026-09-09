/**
 * @file simulator_input.cpp
 * @brief SDL input mapping for the native front-panel simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "simulator_input.h"

namespace clockfw::sim {

SimulatorInput::SimulatorInput(const layout::PanelLayout& panelLayout)
    : panelLayout_(panelLayout) {}

void SimulatorInput::handleEvent(const SDL_Event& event, SimulatorRuntime& runtime) {
    switch (event.type) {
        case SDL_EVENT_QUIT:
            quitRequested_ = true;
            break;
        case SDL_EVENT_KEY_DOWN:
        case SDL_EVENT_KEY_UP:
            handleKeyboard(event.key, runtime);
            break;
        case SDL_EVENT_MOUSE_BUTTON_DOWN:
        case SDL_EVENT_MOUSE_BUTTON_UP:
            handleMouseButton(event.button, runtime);
            break;
        case SDL_EVENT_MOUSE_WHEEL:
            handleMouseWheel(event.wheel, runtime);
            break;
        default:
            break;
    }
}

bool SimulatorInput::quitRequested() const {
    return quitRequested_;
}

double SimulatorInput::speedMultiplier() const {
    return speedMultiplier_;
}

bool SimulatorInput::developerViewEnabled() const {
    return developerViewEnabled_;
}

void SimulatorInput::updateScope(SimulatorRuntime& runtime) {
    scopeSession_.setWindowUs(scope::kWindowOptionsUs[scopeWindowIndex_]);
    scopeSession_.update(runtime);
}

scope::SessionView SimulatorInput::scopeView() const {
    return scopeSession_.view();
}

bool SimulatorInput::consumeScreenshotRequest() {
    const bool requested = screenshotRequested_;
    screenshotRequested_ = false;
    return requested;
}

void SimulatorInput::handleKeyboard(
    const SDL_KeyboardEvent& keyboard,
    SimulatorRuntime& runtime) {
    const bool pressed = keyboard.down;
    if (pressed && !keyboard.repeat) {
        if (keyboard.key == SDLK_MINUS || keyboard.key == SDLK_KP_MINUS) {
            scopeWindowIndex_ = scope::zoomInIndex(scopeWindowIndex_);
            return;
        }
        if (keyboard.key == SDLK_PLUS || keyboard.key == SDLK_EQUALS ||
            keyboard.key == SDLK_KP_PLUS) {
            scopeWindowIndex_ = scope::zoomOutIndex(scopeWindowIndex_);
            return;
        }
    }

    const bool encoderRotationKey =
        keyboard.scancode == SDL_SCANCODE_LEFT || keyboard.scancode == SDL_SCANCODE_A ||
        keyboard.scancode == SDL_SCANCODE_RIGHT || keyboard.scancode == SDL_SCANCODE_D;
    if (pressed && keyboard.repeat && !encoderRotationKey) {
        return;
    }

    switch (keyboard.scancode) {
        case SDL_SCANCODE_LEFT:
        case SDL_SCANCODE_A:
            if (pressed) runtime.rotateEncoder(-1);
            break;
        case SDL_SCANCODE_RIGHT:
        case SDL_SCANCODE_D:
            if (pressed) runtime.rotateEncoder(1);
            break;
        case SDL_SCANCODE_RETURN:
        case SDL_SCANCODE_E:
            runtime.setButton(SimButton::Encoder, pressed);
            break;
        case SDL_SCANCODE_SPACE:
        case SDL_SCANCODE_P:
            runtime.setButton(SimButton::Play, pressed);
            break;
        case SDL_SCANCODE_T:
            runtime.setButton(SimButton::Tap, pressed);
            break;
        case SDL_SCANCODE_S:
        case SDL_SCANCODE_BACKSPACE:
            runtime.setButton(SimButton::Stop, pressed);
            break;
        case SDL_SCANCODE_ESCAPE:
            if (pressed) quitRequested_ = true;
            break;
        case SDL_SCANCODE_C:
            if (pressed) {
                const SyncInputTelemetry sync = runtime.syncInputTelemetry();
                runtime.setSyncCableConnected(!sync.cableConnected);
            }
            break;
        case SDL_SCANCODE_G:
            if (pressed) {
                const SyncInputTelemetry sync = runtime.syncInputTelemetry();
                runtime.setSyncGeneratorRunning(!sync.generatorRunning);
            }
            break;
        case SDL_SCANCODE_Q:
            if (pressed) runtime.cycleSyncPpqn();
            break;
        case SDL_SCANCODE_LEFTBRACKET:
            if (pressed) runtime.adjustSyncBpm(-1);
            break;
        case SDL_SCANCODE_RIGHTBRACKET:
            if (pressed) runtime.adjustSyncBpm(1);
            break;
        case SDL_SCANCODE_PAGEUP:
            if (pressed) runtime.adjustSyncBpm(10);
            break;
        case SDL_SCANCODE_PAGEDOWN:
            if (pressed) runtime.adjustSyncBpm(-10);
            break;
        case SDL_SCANCODE_F1:
            if (pressed) developerViewEnabled_ = !developerViewEnabled_;
            break;
        case SDL_SCANCODE_F2:
            if (pressed) runtime.togglePower();
            break;
        case SDL_SCANCODE_R:
            if (pressed) runtime.triggerResetPulse();
            break;
        case SDL_SCANCODE_W:
            if (pressed) {
                if (selectedInput_ == InputTarget::Sync) runtime.cycleSyncWaveform();
                else runtime.cycleResetWaveform();
            }
            break;
        case SDL_SCANCODE_F12:
            if (pressed) screenshotRequested_ = true;
            break;
        case SDL_SCANCODE_1:
            if (pressed) speedMultiplier_ = 1.0;
            break;
        case SDL_SCANCODE_2:
            if (pressed) speedMultiplier_ = 4.0;
            break;
        case SDL_SCANCODE_3:
            if (pressed) speedMultiplier_ = 16.0;
            break;
        default:
            break;
    }
}

void SimulatorInput::handleMouseButton(
    const SDL_MouseButtonEvent& button,
    SimulatorRuntime& runtime) {
    if (developerViewEnabled_ && button.down && button.button == SDL_BUTTON_LEFT) {
        const layout::Rect& developer = panelLayout_.developerPanel;
        const layout::Rect powerBox{developer.x + developer.width - 118.0F, developer.y + 26.0F, 92.0F, 20.0F};
        if (powerBox.contains(button.x, button.y)) {
            runtime.togglePower();
            return;
        }
        const float boxX = developer.x + 26.0F;
        const float boxY = developer.y + 176.0F;
        if (button.x >= boxX && button.x <= boxX + 176.0F &&
            button.y >= boxY && button.y <= boxY + 20.0F) {
            scopeSession_.toggleFreezeOnStop();
            return;
        }
    }
    const float x = button.x;
    const float y = button.y;

    if (panelLayout_.syncInputContains(x, y) && button.down) {
        selectedInput_ = InputTarget::Sync;
        const SyncInputTelemetry sync = runtime.syncInputTelemetry();
        if (button.button == SDL_BUTTON_LEFT) {
            runtime.setSyncCableConnected(!sync.cableConnected);
        } else if (button.button == SDL_BUTTON_RIGHT) {
            runtime.setSyncGeneratorRunning(!sync.generatorRunning);
        } else if (button.button == SDL_BUTTON_MIDDLE) {
            runtime.cycleSyncWaveform();
        }
        return;
    }
    if (panelLayout_.resetInputContains(x, y) && button.down) {
        selectedInput_ = InputTarget::Reset;
        const ResetInputTelemetry reset = runtime.resetInputTelemetry();
        if (button.button == SDL_BUTTON_LEFT) {
            runtime.setResetCableConnected(!reset.cableConnected);
        } else if (button.button == SDL_BUTTON_RIGHT) {
            runtime.setResetGeneratorRunning(!reset.generatorRunning);
        } else if (button.button == SDL_BUTTON_MIDDLE) {
            runtime.cycleResetWaveform();
        }
        return;
    }

    if (button.button != SDL_BUTTON_LEFT) {
        return;
    }

    if (!button.down) {
        if (hasActiveMouseButton_) {
            runtime.setButton(activeMouseButton_, false);
            hasActiveMouseButton_ = false;
        }
        return;
    }

    if (panelLayout_.encoderContains(x, y)) {
        activeMouseButton_ = SimButton::Encoder;
    } else if (panelLayout_.playButton.contains(x, y)) {
        activeMouseButton_ = SimButton::Play;
    } else if (panelLayout_.tapButton.contains(x, y)) {
        activeMouseButton_ = SimButton::Tap;
    } else if (panelLayout_.stopButton.contains(x, y)) {
        activeMouseButton_ = SimButton::Stop;
    } else {
        return;
    }

    hasActiveMouseButton_ = true;
    runtime.setButton(activeMouseButton_, true);
}

void SimulatorInput::handleMouseWheel(
    const SDL_MouseWheelEvent& wheel,
    SimulatorRuntime& runtime) {
    if (wheel.y == 0.0F) {
        return;
    }
    int direction = wheel.y > 0.0F ? 1 : -1;
    if (wheel.direction == SDL_MOUSEWHEEL_FLIPPED) {
        direction = -direction;
    }
    if (panelLayout_.syncInputContains(wheel.mouse_x, wheel.mouse_y)) {
        selectedInput_ = InputTarget::Sync;
        runtime.adjustSyncBpm(direction);
        return;
    }
    if (panelLayout_.resetInputContains(wheel.mouse_x, wheel.mouse_y)) {
        selectedInput_ = InputTarget::Reset;
        runtime.adjustResetPeriod(direction);
        return;
    }
    if (panelLayout_.encoderContains(wheel.mouse_x, wheel.mouse_y) ||
        panelLayout_.panel.contains(wheel.mouse_x, wheel.mouse_y)) {
        runtime.rotateEncoder(direction);
    }
}

}  // namespace clockfw::sim
