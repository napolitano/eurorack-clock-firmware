/**
 * @file story_model.h
 * @brief Typed CLOCK Storybook schema-1 model independent of simulator and renderer implementations.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

#include "tutorial/story_contract.h"

namespace clockfw::sim::tutorial {

/** @brief Deterministic human interaction profiles referenced by schema 1 stories. */
enum class InteractionProfile : std::uint8_t { HumanSlow, HumanNormal, HumanFast };

/** @brief External source waveforms exposed by the simulator contract. */
enum class StoryWaveform : std::uint8_t { Square, Sine, Triangle };

/** @brief Callout presentation variants supported by Phase 1. */
enum class CalloutKind : std::uint8_t { Tip, Warning, Recipe };

/** @brief Typed Storybook action kinds after YAML decoding. */
enum class StoryActionKind : std::uint8_t {
    Encoder,
    EncoderPush,
    Button,
    Power,
    SyncCable,
    ResetCable,
    SyncGenerator,
    SyncSource,
    ResetGenerator,
    ResetPulse,
    Subtitle,
    Scope,
    Focus,
    Wait,
    WaitUntil,
    Assert,
};

/** @brief Conditions supported by Phase-1 wait_until. */
enum class WaitCondition : std::uint8_t { ExternalSync, Transport, Power };

/** @brief Assertions supported by Phase-1 stories. */
enum class AssertionKind : std::uint8_t { ClockSource, Transport, Power, SyncCable, ResetCable };

/** @brief One fully typed recorded or flow action. Unused fields retain neutral defaults. */
struct StoryAction {
    StoryActionKind kind = StoryActionKind::Wait;
    std::size_t sourceLine = 0U;
    int encoderDirection = 0;
    std::uint16_t detents = 1U;
    ModuleControl moduleControl = ModuleControl::Play;
    /** Optional explicit button level. Missing means one normal press/release click. */
    std::optional<bool> buttonState;
    /** Optional explicit encoder-push hold duration; missing uses the interaction-profile short push. */
    std::optional<std::uint32_t> holdMs;
    bool state = false;
    std::optional<std::uint32_t> bpm;
    std::optional<std::uint8_t> ppqn;
    std::optional<StoryWaveform> waveform;
    std::string text;
    ScopeMode scopeMode = ScopeMode::Hidden;
    FocusTarget focusTarget = FocusTarget::None;
    FocusPlacement focusPlacement = FocusPlacement::Auto;
    int focusX = 0;
    int focusY = 0;
    int focusWidth = 0;
    int focusHeight = 0;
    std::uint32_t durationMs = 0U;
    std::uint32_t timeoutMs = 0U;
    std::optional<WaitCondition> waitCondition;
    std::optional<AssertionKind> assertion;
    std::string expected;
};

/** @brief One schema-1 scene after parsing and structural validation. */
struct StoryScene {
    SceneKind kind = SceneKind::Tutorial;
    std::size_t sourceLine = 0U;
    std::uint32_t number = 0U;
    std::string title;
    std::string subtitle;
    std::string body;
    std::optional<CalloutKind> calloutKind;
    std::vector<std::string> recipeSteps;
    std::uint32_t durationMs = 0U;
    TransitionKind transition = TransitionKind::Cut;
    std::vector<StoryAction> actions;
};

/** @brief Optional media assets appended only by the publication stage. */
struct StoryPublication {
    std::optional<std::string> introVideo;
    std::optional<std::string> outroVideo;
};

/** @brief Narrow pre-recording setup contract. */
struct StorySetup {
    bool factoryReset = false;
    std::optional<bool> powerOn;
};

/** @brief Complete typed schema-1 story. */
struct Story {
    std::uint32_t schema = 0U;
    std::string id;
    std::string title;
    std::string language;
    std::string theme;
    InteractionProfile interactionProfile = InteractionProfile::HumanNormal;
    OutputProfile output = defaultOutputProfile();
    StoryPublication publication;
    StorySetup setup;
    TransitionKind defaultTransition = TransitionKind::Cut;
    std::vector<StoryScene> scenes;
};

/** @brief Parse/validation failure carrying stable story/scene/action context. */
struct StoryIssue {
    std::string storyId;
    std::optional<std::size_t> sceneIndex;
    std::optional<std::size_t> actionIndex;
    std::size_t sourceLine = 0U;
    std::string reason;
};

/** @brief Human profile timing values loaded from docs/tutorials/interaction_profiles.yaml. */
struct InteractionTiming {
    std::uint32_t encoderDetentMs = 0U;
    std::uint32_t encoderFastDetentMs = 0U;
    std::uint32_t buttonDownMs = 0U;
    std::uint32_t buttonReleasePauseMs = 0U;
    std::uint32_t encoderPushDownMs = 0U;
    std::uint32_t encoderPushReleasePauseMs = 0U;
    std::uint32_t patchActionMs = 0U;
    std::uint32_t beforeActionMs = 0U;
    std::uint32_t afterNavigationMs = 0U;
    std::uint32_t afterValueChangeMs = 0U;
    std::uint32_t afterMajorScreenChangeMs = 0U;
};

/** @brief Returns the schema spelling for one interaction profile. */
std::string_view interactionProfileName(InteractionProfile profile);

}  // namespace clockfw::sim::tutorial
