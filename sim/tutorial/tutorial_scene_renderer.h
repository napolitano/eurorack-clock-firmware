/**
 * @file tutorial_scene_renderer.h
 * @brief Presentation-only chapter, text, callout and subtitle rendering for CLOCK Storybook.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <string>

#include "tutorial/story_model.h"
#include "tutorial/story_theme.h"
#include "tutorial/tutorial_surface.h"

namespace clockfw::sim::tutorial {

void renderStorySubtitle(TutorialSurface& frame, const std::string& subtitle, const StoryTheme& theme);
void renderStoryChapter(TutorialSurface& frame, const StoryScene& scene, const StoryTheme& theme);
void renderStoryTextScene(TutorialSurface& frame, const StoryScene& scene, const StoryTheme& theme);
void renderStoryCallout(TutorialSurface& frame, const StoryScene& scene, const StoryTheme& theme);

}  // namespace clockfw::sim::tutorial
