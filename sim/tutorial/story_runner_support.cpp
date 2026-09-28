/**
 * @file story_runner_support.cpp
 * @brief Implements internal StoryRunner naming, pacing, and waveform conversion helpers.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_runner_support.h"

namespace clockfw::sim::tutorial::detail {

const char* traceSceneName(const SceneKind kind) {
    switch (kind) {
        case SceneKind::Chapter: return "chapter";
        case SceneKind::Tutorial: return "tutorial";
        case SceneKind::Text: return "text";
        case SceneKind::Callout: return "callout";
    }
    return "scene";
}

const char* traceActionName(const StoryActionKind kind) {
    switch (kind) {
        case StoryActionKind::Encoder: return "encoder";
        case StoryActionKind::EncoderPush: return "encoder_push";
        case StoryActionKind::Button: return "button";
        case StoryActionKind::Power: return "power";
        case StoryActionKind::SyncCable: return "sync_cable";
        case StoryActionKind::ResetCable: return "rst_cable";
        case StoryActionKind::SyncGenerator: return "sync_generator";
        case StoryActionKind::SyncSource: return "sync_source";
        case StoryActionKind::ResetGenerator: return "rst_generator";
        case StoryActionKind::ResetPulse: return "rst_pulse";
        case StoryActionKind::Subtitle: return "subtitle";
        case StoryActionKind::Scope: return "scope";
        case StoryActionKind::Focus: return "focus";
        case StoryActionKind::Wait: return "wait";
        case StoryActionKind::WaitUntil: return "wait_until";
        case StoryActionKind::Assert: return "assert";
    }
    return "action";
}

const char* portErrorName(const StoryPortError error) {
    switch (error) {
        case StoryPortError::None: return "none";
        case StoryPortError::ModulePoweredOff: return "module powered off";
        case StoryPortError::CableRequired: return "cable required";
        case StoryPortError::InvalidParameter: return "invalid parameter";
        case StoryPortError::UnsupportedAction: return "unsupported action";
    }
    return "unknown port error";
}

bool isPacedAction(const StoryActionKind kind) {
    switch (kind) {
        case StoryActionKind::Encoder:
        case StoryActionKind::EncoderPush:
        case StoryActionKind::Button:
        case StoryActionKind::Power:
        case StoryActionKind::SyncCable:
        case StoryActionKind::ResetCable:
        case StoryActionKind::SyncGenerator:
        case StoryActionKind::SyncSource:
        case StoryActionKind::ResetGenerator:
        case StoryActionKind::ResetPulse:
            return true;
        case StoryActionKind::Subtitle:
        case StoryActionKind::Scope:
        case StoryActionKind::Focus:
        case StoryActionKind::Wait:
        case StoryActionKind::WaitUntil:
        case StoryActionKind::Assert:
            return false;
    }
    return false;
}

SignalWaveform toSimulatorWaveform(const StoryWaveform waveform) {
    switch (waveform) {
        case StoryWaveform::Square: return SignalWaveform::Square;
        case StoryWaveform::Sine: return SignalWaveform::Sine;
        case StoryWaveform::Triangle: return SignalWaveform::Triangle;
    }
    return SignalWaveform::Square;
}

}  // namespace clockfw::sim::tutorial::detail
