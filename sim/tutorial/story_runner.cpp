/**
 * @file story_runner.cpp
 * @brief Implements deterministic CLOCK Storybook scene sequencing, interaction pacing, waits and assertions.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_runner.h"
#include <algorithm>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <utility>
#include "tutorial/interaction_profile_loader.h"
#include "tutorial/story_narration_beat.h"
#include "tutorial/story_runner_support.h"
#include "tutorial/story_validator.h"
namespace clockfw::sim::tutorial { namespace {
constexpr std::uint64_t kUsPerMs = 1000ULL;
constexpr std::uint64_t kWaitPollUs = 1000ULL;
using detail::isPacedAction;
using detail::portErrorName;
using detail::toSimulatorWaveform;
using detail::traceActionName;
using detail::traceSceneName;
class RunContext final {
public:
    RunContext(StorySimulatorPort& port, const Story& story, const InteractionTiming& timing,
               StoryPresentationSink* presentationSink, StoryExecutionObserver* executionObserver)
        : port_(port), story_(story), timing_(timing), presentationSink_(presentationSink), executionObserver_(executionObserver) {}
    StoryRunResult execute() {
        if (!applySetup()) {
            finish();
            return std::move(result_);
        }
        for (std::size_t sceneIndex = 0U; sceneIndex < story_.scenes.size(); ++sceneIndex) {
            if (!executeScene(story_.scenes[sceneIndex], sceneIndex)) {
                break;
            }
        }
        finish();
        return std::move(result_);
    }
private:
    bool applySetup() {
        if (story_.setup.factoryReset) {
            const StoryPortResult reset = port_.resetPersistenceToFactory();
            if (!reset) {
                addIssue(std::nullopt, std::nullopt, 0U, std::string("setup factory_reset failed: ") + portErrorName(reset.error));
                return false;
            }
            emit(StoryTraceKind::Setup, std::nullopt, std::nullopt, "factory_reset", "applied");
        }
        if (story_.setup.powerOn.has_value()) {
            const StoryPortResult power = port_.setPower(*story_.setup.powerOn);
            if (!power) {
                addIssue(std::nullopt, std::nullopt, 0U, std::string("setup power failed: ") + portErrorName(power.error));
                return false;
            }
            emit(StoryTraceKind::Setup, std::nullopt, std::nullopt, "power", *story_.setup.powerOn ? "on" : "off");
        }
        return true;
    }
    bool executeScene(const StoryScene& scene, const std::size_t sceneIndex) {
        currentScene_ = &scene;
        currentSceneIndex_ = sceneIndex;
        activeSubtitle_ = scene.kind == SceneKind::Tutorial ? scene.subtitle : std::string{};
        emit(StoryTraceKind::SceneBegin, sceneIndex, std::nullopt, traceSceneName(scene.kind), scene.title);
        if (scene.kind != SceneKind::Tutorial) {
            if (scene.narrationId) emit(StoryTraceKind::NarrationBegin, sceneIndex, std::nullopt, *scene.narrationId, {});
            advancePresentationMs(scene.durationMs);
            if (scene.narrationId) emit(StoryTraceKind::NarrationEnd, sceneIndex, std::nullopt, *scene.narrationId, {});
            emit(StoryTraceKind::SceneEnd, sceneIndex, std::nullopt, traceSceneName(scene.kind), scene.title);
            currentScene_ = nullptr;
            activeSubtitle_.clear();
            return true;
        }
        if (!scene.subtitle.empty()) {
            emit(StoryTraceKind::Subtitle, sceneIndex, std::nullopt, "subtitle", scene.subtitle);
        }
        for (std::size_t actionIndex = 0U; actionIndex < scene.actions.size(); ++actionIndex) {
            const StoryAction& action = scene.actions[actionIndex];
            emit(StoryTraceKind::ActionBegin, sceneIndex, actionIndex, traceActionName(action.kind), {});
            const bool beatBoundary = action.kind == StoryActionKind::BeatBegin || action.kind == StoryActionKind::BeatEnd;
            if (action.narrationId && !beatBoundary) {
                emit(StoryTraceKind::NarrationBegin, sceneIndex, actionIndex, *action.narrationId, {});
            }
            if (isPacedAction(action.kind)) {
                advanceActiveMs(timing_.beforeActionMs);
            }
            if (!executeAction(action, sceneIndex, actionIndex)) {
                return false;
            }
            if (action.narrationId && !beatBoundary) {
                emit(StoryTraceKind::NarrationEnd, sceneIndex, actionIndex, *action.narrationId, {});
            }
            emit(StoryTraceKind::ActionEnd, sceneIndex, actionIndex, traceActionName(action.kind), {});
        }
        emit(StoryTraceKind::SceneEnd, sceneIndex, std::nullopt, traceSceneName(scene.kind), scene.title);
        currentScene_ = nullptr;
        activeSubtitle_.clear();
        return true;
    }
    bool executeAction(
        const StoryAction& action,
        const std::size_t sceneIndex,
        const std::size_t actionIndex) {
        switch (action.kind) {
            case StoryActionKind::BeatBegin:
                return beginBeat(action, sceneIndex, actionIndex);
            case StoryActionKind::BeatEnd:
                return endBeat(action, sceneIndex, actionIndex);
            case StoryActionKind::Encoder:
                for (std::uint16_t detent = 0U; detent < action.detents; ++detent) {
                    if (!requirePort(port_.rotateEncoderDetent(action.encoderDirection), action, sceneIndex, actionIndex)) {
                        return false;
                    }
                    if (presentationSink_ != nullptr) {
                        presentationSink_->onEncoderDetent(presentationUs_, action.encoderDirection);
                    }
                    emit(StoryTraceKind::EncoderDetent, sceneIndex, actionIndex, "encoder",
                         action.encoderDirection > 0 ? "clockwise" : "counter_clockwise");
                    advanceActiveMs(timing_.encoderDetentMs);
                }
                advanceActiveMs(timing_.afterValueChangeMs);
                return true;
            case StoryActionKind::EncoderPush:
                return executeButton(
                    ModuleControl::EncoderPush,
                    action.holdMs.value_or(timing_.encoderPushDownMs),
                    timing_.encoderPushReleasePauseMs + timing_.afterNavigationMs,
                    action, sceneIndex, actionIndex);
            case StoryActionKind::Button:
                if (action.buttonState.has_value()) {
                    if (!requirePort(port_.setModuleControl(action.moduleControl, *action.buttonState),
                                     action, sceneIndex, actionIndex)) {
                        return false;
                    }
                    if (presentationSink_ != nullptr) {
                        presentationSink_->onControlState(presentationUs_, action.moduleControl, *action.buttonState);
                    }
                    emit(StoryTraceKind::ControlState, sceneIndex, actionIndex,
                         std::string(moduleControlName(action.moduleControl)),
                         *action.buttonState ? "down" : "up");
                    advanceActiveMs(*action.buttonState ? timing_.buttonDownMs : timing_.buttonReleasePauseMs);
                    return true;
                }
                return executeButton(action.moduleControl, timing_.buttonDownMs,
                                     timing_.buttonReleasePauseMs, action, sceneIndex, actionIndex);
            case StoryActionKind::Power: {
                if (!requirePort(port_.setPower(action.state), action, sceneIndex, actionIndex)) return false;
                if (presentationSink_ != nullptr) {
                    presentationSink_->onPowerState(presentationUs_, action.state);
                }
                emit(StoryTraceKind::PowerState, sceneIndex, actionIndex, "power", action.state ? "on" : "off");
                advanceActiveMs(timing_.afterMajorScreenChangeMs);
                return true;
            }
            case StoryActionKind::SyncCable:
            case StoryActionKind::ResetCable: {
                const PatchAction patch = action.kind == StoryActionKind::SyncCable
                    ? PatchAction::SyncCable : PatchAction::ResetCable;
                if (!requirePort(port_.setPatchConnected(patch, action.state), action, sceneIndex, actionIndex)) return false;
                if (presentationSink_ != nullptr) {
                    presentationSink_->onPatchMotion(
                        presentationUs_,
                        patch,
                        action.state,
                        static_cast<std::uint64_t>(timing_.patchActionMs) * kUsPerMs);
                }
                emit(StoryTraceKind::PatchState, sceneIndex, actionIndex,
                     action.kind == StoryActionKind::SyncCable ? "sync_cable" : "rst_cable",
                     action.state ? "connected" : "disconnected");
                advanceActiveMs(timing_.patchActionMs);
                if (presentationSink_ != nullptr) {
                    presentationSink_->onPatchSettled(presentationUs_, patch, action.state);
                }
                return true;
            }
            case StoryActionKind::SyncGenerator:
            case StoryActionKind::ResetGenerator: {
                const ExternalStimulus stimulus = action.kind == StoryActionKind::SyncGenerator
                    ? ExternalStimulus::SyncGenerator : ExternalStimulus::ResetGenerator;
                if (!requirePort(port_.setGeneratorRunning(stimulus, action.state), action, sceneIndex, actionIndex)) return false;
                emit(StoryTraceKind::GeneratorState, sceneIndex, actionIndex,
                     action.kind == StoryActionKind::SyncGenerator ? "sync_generator" : "rst_generator",
                     action.state ? "run" : "hold");
                advanceActiveMs(timing_.afterValueChangeMs);
                return true;
            }
            case StoryActionKind::SyncSource: {
                const SyncInputTelemetry current = port_.syncTelemetry();
                const std::uint32_t bpmMilli = action.bpm ? (*action.bpm * 1000U) : current.bpmMilli;
                const std::uint8_t ppqn = action.ppqn.value_or(current.ppqn);
                const SignalWaveform waveform = action.waveform
                    ? toSimulatorWaveform(*action.waveform) : current.waveform;
                if (!requirePort(port_.configureSyncSource(bpmMilli, ppqn, waveform), action, sceneIndex, actionIndex)) return false;
                emit(StoryTraceKind::SourceConfigured, sceneIndex, actionIndex, "sync_source",
                     std::to_string(bpmMilli) + "mBPM/" + std::to_string(ppqn) + "PPQN");
                advanceActiveMs(timing_.afterValueChangeMs);
                return true;
            }
            case StoryActionKind::ResetPulse:
                if (!requirePort(port_.triggerResetPulse(), action, sceneIndex, actionIndex)) return false;
                emit(StoryTraceKind::ResetPulse, sceneIndex, actionIndex, "rst_pulse", "triggered");
                advanceActiveMs(timing_.afterValueChangeMs);
                return true;
            case StoryActionKind::Subtitle:
                activeSubtitle_ = action.text;
                emit(StoryTraceKind::Subtitle, sceneIndex, actionIndex, "subtitle", action.text);
                return true;
            case StoryActionKind::Scope:
                if (presentationSink_ != nullptr) {
                    presentationSink_->onScopeState(presentationUs_, action.scopeMode);
                }
                emit(StoryTraceKind::Scope, sceneIndex, actionIndex, "scope",
                     action.scopeMode == ScopeMode::VisibleChannel ? "visible_channel" : "hidden");
                return true;
            case StoryActionKind::Focus:
                if (presentationSink_ != nullptr) presentationSink_->onFocusState(
                    presentationUs_, action.focusTarget, true, action.focusPlacement, action.focusX, action.focusY, action.focusWidth, action.focusHeight, action.text);
                emit(StoryTraceKind::Focus, sceneIndex, actionIndex, "focus", action.text);
                if (action.durationMs == 0U && beat_.active()) {
                    beat_.retainFocus(action.focusTarget);
                    return true;
                }
                advancePresentationMs(action.durationMs);
                if (presentationSink_ != nullptr) presentationSink_->onFocusState(
                    presentationUs_, action.focusTarget, false, FocusPlacement::Auto, 0, 0, 0, 0, {});
                return true;
            case StoryActionKind::Wait:
                advanceActiveMs(action.durationMs);
                emit(StoryTraceKind::WaitSatisfied, sceneIndex, actionIndex, "wait_ms", std::to_string(action.durationMs));
                return true;
            case StoryActionKind::WaitUntil:
                return executeWaitUntil(action, sceneIndex, actionIndex);
            case StoryActionKind::Assert:
                if (!assertionMatches(action)) {
                    addIssue(sceneIndex, actionIndex, action.sourceLine,
                             "assertion failed: expected " + action.expected);
                    return false;
                }
                emit(StoryTraceKind::AssertionPassed, sceneIndex, actionIndex, "assert", action.expected);
                return true;
        }
        return false;
    }
    bool beginBeat(const StoryAction& action, const std::size_t sceneIndex, const std::size_t actionIndex) {
        std::string error;
        if (!beat_.begin(action, presentationUs_, error)) { addIssue(sceneIndex, actionIndex, action.sourceLine, std::move(error)); return false; }
        emit(StoryTraceKind::NarrationBegin, sceneIndex, actionIndex, *action.narrationId, {});
        return true;
    }
    bool endBeat(const StoryAction& action, const std::size_t sceneIndex, const std::size_t actionIndex) {
        StoryNarrationBeatFinish finish;
        std::string error;
        if (!beat_.finish(action, presentationUs_, finish, error)) { addIssue(sceneIndex, actionIndex, action.sourceLine, std::move(error)); return false; }
        if (finish.remainingUs > 0ULL) sampleUntil(presentationUs_ + finish.remainingUs, false);
        if (finish.focusToClear && presentationSink_ != nullptr)
            presentationSink_->onFocusState(presentationUs_, *finish.focusToClear, false, FocusPlacement::Auto, 0, 0, 0, 0, {});
        emit(StoryTraceKind::NarrationEnd, sceneIndex, actionIndex, finish.id, {});
        return true;
    }
    bool executeButton(
        const ModuleControl control,
        const std::uint32_t downMs,
        const std::uint32_t releaseAndPauseMs,
        const StoryAction& action,
        const std::size_t sceneIndex,
        const std::size_t actionIndex) {
        if (!requirePort(port_.setModuleControl(control, true), action, sceneIndex, actionIndex)) return false;
        if (presentationSink_ != nullptr) {
            presentationSink_->onControlState(presentationUs_, control, true);
        }
        emit(StoryTraceKind::ControlState, sceneIndex, actionIndex, std::string(moduleControlName(control)), "down");
        advanceActiveMs(downMs);
        if (!requirePort(port_.setModuleControl(control, false), action, sceneIndex, actionIndex)) return false;
        if (presentationSink_ != nullptr) {
            presentationSink_->onControlState(presentationUs_, control, false);
        }
        emit(StoryTraceKind::ControlState, sceneIndex, actionIndex, std::string(moduleControlName(control)), "up");
        advanceActiveMs(releaseAndPauseMs);
        return true;
    }
    bool executeWaitUntil(
        const StoryAction& action,
        const std::size_t sceneIndex,
        const std::size_t actionIndex) {
        if (waitConditionMatches(action)) {
            emit(StoryTraceKind::WaitSatisfied, sceneIndex, actionIndex, "wait_until", action.expected);
            return true;
        }
        std::uint64_t remainingUs = static_cast<std::uint64_t>(action.timeoutMs) * kUsPerMs;
        while (remainingUs > 0ULL) {
            const std::uint64_t stepUs = std::min(kWaitPollUs, remainingUs);
            advanceActiveUs(stepUs);
            remainingUs -= stepUs;
            if (waitConditionMatches(action)) {
                emit(StoryTraceKind::WaitSatisfied, sceneIndex, actionIndex, "wait_until", action.expected);
                return true;
            }
        }
        addIssue(sceneIndex, actionIndex, action.sourceLine,
                 "wait_until timeout: expected " + action.expected);
        return false;
    }
    bool waitConditionMatches(const StoryAction& action) const {
        if (!action.waitCondition) return false;
        switch (*action.waitCondition) {
            case WaitCondition::ExternalSync:
                return port_.syncTelemetry().locked == (action.expected == "locked");
            case WaitCondition::Transport:
                return transportMatches(action.expected);
            case WaitCondition::Power:
                return port_.poweredOn() == (action.expected == "on");
        }
        return false;
    }
    bool assertionMatches(const StoryAction& action) const {
        if (!action.assertion) return false;
        switch (*action.assertion) {
            case AssertionKind::ClockSource:
                if (action.expected == "auto") return port_.clockSource() == ClockSource::Auto;
                if (action.expected == "internal") return port_.clockSource() == ClockSource::Internal;
                if (action.expected == "external") return port_.clockSource() == ClockSource::External;
                return false;
            case AssertionKind::Transport:
                return transportMatches(action.expected);
            case AssertionKind::Power:
                return port_.poweredOn() == (action.expected == "on");
            case AssertionKind::SyncCable:
                return port_.syncTelemetry().cableConnected == (action.expected == "connected");
            case AssertionKind::ResetCable:
                return port_.resetTelemetry().cableConnected == (action.expected == "connected");
        }
        return false;
    }
    bool transportMatches(const std::string& expected) const {
        if (expected == "playing") return port_.transportState() == TransportState::Playing;
        if (expected == "paused") return port_.transportState() == TransportState::Paused;
        if (expected == "stopped") return port_.transportState() == TransportState::Stopped;
        return false;
    }
    bool requirePort(
        const StoryPortResult result,
        const StoryAction& action,
        const std::size_t sceneIndex,
        const std::size_t actionIndex) {
        if (result) return true;
        addIssue(sceneIndex, actionIndex, action.sourceLine,
                 std::string(traceActionName(action.kind)) + " failed: " + portErrorName(result.error));
        return false;
    }
    void addIssue(
        const std::optional<std::size_t> sceneIndex,
        const std::optional<std::size_t> actionIndex,
        const std::size_t sourceLine,
        std::string reason) {
        result_.issues.push_back({story_.id, sceneIndex, actionIndex, sourceLine, std::move(reason)});
    }
    void emit(
        const StoryTraceKind kind,
        const std::optional<std::size_t> sceneIndex,
        const std::optional<std::size_t> actionIndex,
        std::string name,
        std::string value) {
        result_.trace.push_back({kind, presentationUs_, port_.runtime().nowMicroseconds(),
                                 sceneIndex, actionIndex, std::move(name), std::move(value)});
    }
    void sampleUntil(const std::uint64_t targetPresentationUs, const bool firmwareAdvances) {
        while (executionObserver_ != nullptr) {
            const std::optional<std::uint64_t> next = executionObserver_->nextPresentationSampleUs();
            if (!next.has_value() || *next >= targetPresentationUs) break;
            if (*next < presentationUs_) {
                throw std::runtime_error("Story execution observer requested a past presentation timestamp");
            }
            const std::uint64_t deltaUs = *next - presentationUs_;
            if (firmwareAdvances && deltaUs > 0ULL) {
                port_.advanceFirmwareMicroseconds(deltaUs);
            }
            presentationUs_ = *next;
            if (currentScene_ == nullptr || !currentSceneIndex_.has_value()) {
                throw std::runtime_error("Story execution observer sampled outside a scene");
            }
            executionObserver_->onPresentationSample({
                presentationUs_, port_.runtime().nowMicroseconds(), *currentSceneIndex_, currentScene_,
                activeSubtitle_, &port_.presentationRuntime()});
        }
        const std::uint64_t remainingUs = targetPresentationUs - presentationUs_;
        if (firmwareAdvances && remainingUs > 0ULL) {
            port_.advanceFirmwareMicroseconds(remainingUs);
        }
        presentationUs_ = targetPresentationUs;
    }
    void advancePresentationMs(const std::uint32_t durationMs) {
        const std::uint64_t target = presentationUs_ + static_cast<std::uint64_t>(durationMs) * kUsPerMs;
        sampleUntil(target, false);
    }
    void advanceActiveMs(const std::uint32_t durationMs) {
        advanceActiveUs(static_cast<std::uint64_t>(durationMs) * kUsPerMs);
    }
    void advanceActiveUs(const std::uint64_t durationUs) {
        if (durationUs == 0ULL) return;
        sampleUntil(presentationUs_ + durationUs, true);
    }
    void finish() {
        result_.presentationDurationUs = presentationUs_;
    }
    StorySimulatorPort& port_;
    const Story& story_;
    const InteractionTiming& timing_;
    StoryRunResult result_;
    std::uint64_t presentationUs_ = 0ULL;
    StoryPresentationSink* presentationSink_ = nullptr;
    StoryExecutionObserver* executionObserver_ = nullptr;
    const StoryScene* currentScene_ = nullptr;
    std::optional<std::size_t> currentSceneIndex_;
    std::string activeSubtitle_;
    StoryNarrationBeat beat_;
};
}  // namespace
StoryRunResult::operator bool() const {
    return issues.empty();
}
StoryRunner::StoryRunner(
    StorySimulatorPort& port,
    std::filesystem::path tutorialRoot,
    StoryPresentationSink* presentationSink,
    StoryExecutionObserver* executionObserver)
    : port_(port), tutorialRoot_(std::move(tutorialRoot)), presentationSink_(presentationSink),
      executionObserver_(executionObserver) {}
StoryRunResult StoryRunner::run(const Story& story) {
    StoryRunResult rejected;
    rejected.issues = validateStory(story, tutorialRoot_);
    if (!rejected.issues.empty()) {
        return rejected;
    }
    const InteractionTimingResult timing = loadInteractionTiming(tutorialRoot_, story.interactionProfile);
    if (!timing) {
        rejected.issues = timing.issues;
        for (StoryIssue& issue : rejected.issues) {
            issue.storyId = story.id;
        }
        return rejected;
    }
    RunContext context(port_, story, *timing.timing, presentationSink_, executionObserver_);
    return context.execute();
}
}  // namespace clockfw::sim::tutorial
