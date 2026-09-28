/**
 * @file tutorial_renderer.h
 * @brief Complete deterministic CLOCK Storybook scene compositor for Phase 1.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <filesystem>
#include <string>

#include "panel_layout.h"
#include "scope_session.h"
#include "simulator_runtime.h"
#include "tutorial/panel_presentation.h"
#include "tutorial/story_model.h"
#include "tutorial/tutorial_surface.h"

namespace clockfw::sim::tutorial {

/** @brief Resolved Phase-1 frame regions used by tests and later frame scheduling. */
struct TutorialCompositionLayout {
    TutorialRect content{};
    TutorialRect oledBox{};
    TutorialRect oledRaster{};
    TutorialRect scopeBox{};
    TutorialRect panelBox{};
    TutorialRect subtitleStrip{};
};

/** @brief Resolves fixed side-by-side composition without reading simulator state. */
TutorialCompositionLayout resolveTutorialComposition(const OutputProfile& output, bool scopeVisible);

/** @brief Deterministic scene renderer combining production OLED, panel telemetry and Story presentation. */
class TutorialRenderer final {
public:
    TutorialRenderer(std::filesystem::path tutorialRoot, layout::PanelLayout panelLayout);

    /**
     * @brief Renders one complete Phase-1 frame.
     * @param story Parsed/validated story owning output/theme settings.
     * @param scene Scene to present.
     * @param runtime Real simulator runtime; required for tutorial scenes.
     * @param physicalState One-way physical presentation state at this exact presentation timestamp.
     * @param activeSubtitle Timed subtitle event; empty uses the tutorial scene subtitle when present.
     * @param speedMultiplier Simulator speed used only by the shared LED visual persistence rule.
     * @throws std::runtime_error on missing fonts, invalid layout or any text overflow.
     */
    TutorialSurface renderScene(
        const Story& story,
        const StoryScene& scene,
        SimulatorRuntime& runtime,
        const PhysicalPresentationState& physicalState,
        const std::string& activeSubtitle = {},
        double speedMultiplier = 1.0);

private:
    std::filesystem::path tutorialRoot_;
    layout::PanelLayout panelLayout_;
    scope::Session scopeSession_{};
};

}  // namespace clockfw::sim::tutorial
