/**
 * @file panel_presentation.h
 * @brief Deterministic physical front-panel presentation state derived from Storybook events and simulator telemetry.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <vector>

#include "panel_layout.h"
#include "simulator_runtime.h"
#include "tutorial/story_presentation_sink.h"

namespace clockfw::sim::tutorial {

/** @brief Visible cable motion owned by tutorial presentation only. */
enum class PatchMotion : std::uint8_t { None, Inserting, Removing };

/** @brief Reconstructed visible interaction state at one presentation timestamp. */
struct PhysicalPresentationState {
    std::int64_t encoderDetentDelta = 0;
    bool encoderPressed = false;
    bool playPressed = false;
    bool tapPressed = false;
    bool stopPressed = false;
    bool recordedPowerOn = true;
    ScopeMode scopeMode = ScopeMode::Hidden;

    PatchMotion syncMotion = PatchMotion::None;
    PatchMotion resetMotion = PatchMotion::None;
    float syncInsertion = 0.0F;
    float resetInsertion = 0.0F;
};

/** @brief Geometry and state for one visible round pushbutton. */
struct ButtonPresentation {
    layout::Point center{};
    float radius = 0.0F;
    bool pressed = false;
};

/** @brief Geometry and state for the visible encoder. */
struct EncoderPresentation {
    layout::Point center{};
    float radius = 0.0F;
    std::int64_t visualPosition = 0;
    bool pressed = false;
};

/** @brief Geometry and current physical/animated state for one patch input. */
struct PatchPresentation {
    layout::Point center{};
    layout::JackGeometry jack{};
    bool connected = false;
    bool signalHigh = false;
    PatchMotion motion = PatchMotion::None;
    float insertion = 0.0F;
};

/** @brief Geometry and actual simulator-derived activity state for one output LED. */
struct LedPresentation {
    layout::Point center{};
    float radius = 0.0F;
    layout::Color color{};
    bool lit = false;
};

/**
 * @brief Complete dynamic front-panel snapshot consumed by later tutorial rendering.
 *
 * No product state is inferred here: power, patch connection, signals, encoder position and LEDs are
 * read from SimulatorRuntime. Only transient physical presentation such as pressed button faces and
 * plug insertion progress comes from the one-way Storybook presentation timeline.
 */
struct PanelPresentationSnapshot {
    bool poweredOn = false;
    EncoderPresentation encoder{};
    ButtonPresentation play{};
    ButtonPresentation tap{};
    ButtonPresentation stop{};
    PatchPresentation sync{};
    PatchPresentation reset{};
    std::array<LedPresentation, kChannelCount> leds{};
    ScopeMode scopeMode = ScopeMode::Hidden;
};

/**
 * @brief Records physical-presentation events emitted by StoryRunner and reconstructs deterministic state.
 */
class PanelPresentationTimeline final : public StoryPresentationSink {
public:
    /** @brief Clears all recorded physical-presentation events. */
    void clear();

    /** @brief Returns the number of recorded presentation events. */
    std::size_t eventCount() const;

    /** @brief Reconstructs transient physical state at one presentation timestamp. */
    PhysicalPresentationState stateAt(std::uint64_t presentationUs) const;

    void onEncoderDetent(std::uint64_t presentationUs, int direction) override;
    void onControlState(std::uint64_t presentationUs, ModuleControl control, bool pressed) override;
    void onPowerState(std::uint64_t presentationUs, bool poweredOn) override;
    void onPatchMotion(
        std::uint64_t presentationUs,
        PatchAction patch,
        bool targetConnected,
        std::uint64_t durationUs) override;
    void onPatchSettled(std::uint64_t presentationUs, PatchAction patch, bool connected) override;
    void onScopeState(std::uint64_t presentationUs, ScopeMode mode) override;

private:
    enum class EventKind : std::uint8_t { Encoder, Control, Power, PatchMotion, PatchSettled, Scope };

    struct Event {
        EventKind kind = EventKind::Encoder;
        std::uint64_t presentationUs = 0ULL;
        ModuleControl control = ModuleControl::Encoder;
        PatchAction patch = PatchAction::SyncCable;
        ScopeMode scope = ScopeMode::Hidden;
        int direction = 0;
        bool state = false;
        std::uint64_t durationUs = 0ULL;
    };

    std::vector<Event> events_{};
};

/**
 * @brief Builds one dynamic tutorial-panel snapshot from PanelLayout, real runtime telemetry, and transient presentation state.
 */
PanelPresentationSnapshot makePanelPresentationSnapshot(
    const layout::PanelLayout& panelLayout,
    const SimulatorRuntime& runtime,
    const PhysicalPresentationState& physicalState,
    double speedMultiplier);

}  // namespace clockfw::sim::tutorial
