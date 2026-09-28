/**
 * @file story_runner_support.h
 * @brief Internal naming, pacing, and simulator-conversion helpers for StoryRunner.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include "tutorial/story_model.h"
#include "tutorial/story_simulator_port.h"

namespace clockfw::sim::tutorial::detail {

const char* traceSceneName(SceneKind kind);
const char* traceActionName(StoryActionKind kind);
const char* portErrorName(StoryPortError error);
bool isPacedAction(StoryActionKind kind);
SignalWaveform toSimulatorWaveform(StoryWaveform waveform);

}  // namespace clockfw::sim::tutorial::detail
