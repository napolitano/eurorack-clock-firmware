/**
 * @file story_contract.h
 * @brief Host-only semantic contract shared by CLOCK Storybook parser, runner, and tests.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <string_view>

namespace clockfw::sim::tutorial {

/** @brief Scene kinds supported by Storybook schema 1. */
enum class SceneKind : std::uint8_t { Chapter, Tutorial, Text, Callout };

/** @brief Physical CLOCK controls that recorded stories may operate. */
enum class ModuleControl : std::uint8_t {
    Encoder,
    EncoderPush,
    Play,
    Tap,
    StopBack,
    Power,
};

/** @brief Physical patch operations that recorded stories may perform. */
enum class PatchAction : std::uint8_t { SyncCable, ResetCable };

/** @brief External source operations that are not CLOCK controls. */
enum class ExternalStimulus : std::uint8_t {
    SyncGenerator,
    SyncSource,
    ResetGenerator,
    ResetPulse,
};

/** @brief Presentation-only actions that never mutate firmware or simulator product state. */
enum class PresentationAction : std::uint8_t { Subtitle, Scope, Focus };

/** @brief Flow and validation operations owned by the deterministic story runner. */
enum class FlowAction : std::uint8_t { Wait, WaitUntil, Assert };

/** @brief Scene transition kinds known to the versioned story model. */
enum class TransitionKind : std::uint8_t { Cut, Fade };

/** @brief Scope presentation modes supported by Phase 1. */
enum class ScopeMode : std::uint8_t { Hidden, VisibleChannel };

/** @brief Presentation-only focus targets for tutorial guidance overlays. */
enum class FocusTarget : std::uint8_t { None, Encoder, Play, Tap, StopBack, Sync, Reset, OledRegion };

/** @brief Publication containers supported by the planned media encoder stage. */
enum class PublicationFormat : std::uint8_t { Mp4H264, WebM };

/** @brief Deterministic raster profile for generated tutorial frames. */
struct OutputProfile {
    std::uint16_t width = 1920U;
    std::uint16_t height = 1080U;
    std::uint16_t framesPerSecond = 30U;
};

/** @brief Returns the only Storybook schema version accepted by the initial implementation. */
std::uint32_t currentSchemaVersion();

/** @brief Returns the default deterministic tutorial raster profile. */
OutputProfile defaultOutputProfile();

/** @brief Returns whether one transition is supported by Phase 1 rendering. */
bool phase1SupportsTransition(TransitionKind transition);

/** @brief Returns whether pure chapter/text/callout presentation advances firmware time in Phase 1. */
bool presentationScenesAdvanceFirmwareTime();

/** @brief Returns the default final publication container/codec combination. */
PublicationFormat defaultPublicationFormat();

/** @brief Returns the stable schema spelling for one module-control value. */
std::string_view moduleControlName(ModuleControl control);

/** @brief Returns the stable schema spelling for one patch-action value. */
std::string_view patchActionName(PatchAction action);

/** @brief Returns the stable schema spelling for one external-stimulus value. */
std::string_view externalStimulusName(ExternalStimulus stimulus);

}  // namespace clockfw::sim::tutorial
