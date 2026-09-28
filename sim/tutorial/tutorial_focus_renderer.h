/**
 * @file tutorial_focus_renderer.h
 * @brief Presentation-only focus geometry and guidance overlays for CLOCK Storybook tutorials.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <optional>

#include "panel_layout.h"
#include "tutorial/panel_presentation.h"
#include "tutorial/story_theme.h"
#include "tutorial/tutorial_surface.h"

namespace clockfw::sim::tutorial {

/** @brief One resolved focus overlay after explicit-focus and automatic-interaction precedence. */
struct ResolvedTutorialFocus {
    FocusTarget target = FocusTarget::None;
    FocusPlacement placement = FocusPlacement::Auto;
    int oledX = 0;
    int oledY = 0;
    int oledWidth = 0;
    int oledHeight = 0;
    std::string label;
    bool explicitFocus = false;
};

ResolvedTutorialFocus resolveTutorialFocus(const PhysicalPresentationState& state, const StoryTheme& theme);
std::optional<layout::Point> panelFocusPoint(const layout::PanelLayout& layout, FocusTarget target);
TutorialRect panelFocusSourceRect(const layout::PanelLayout& layout, FocusTarget target);
std::optional<TutorialRect> panelFocusDestinationRect(
    const layout::PanelLayout& layout, FocusTarget target, TutorialRect source, TutorialRect destination);
std::string focusTargetLabel(FocusTarget target);
void drawTutorialFocusRing(TutorialSurface& frame, int centerX, int centerY, int radius, const StoryTheme& theme);
void drawTutorialFocusArrow(
    TutorialSurface& frame, TutorialRect target, const std::string& label,
    const StoryTheme& theme, FocusPlacement placement = FocusPlacement::Auto);
void drawOledRegionFocus(
    TutorialSurface& frame, TutorialRect oledRaster,
    const ResolvedTutorialFocus& focus, const StoryTheme& theme);

}  // namespace clockfw::sim::tutorial
