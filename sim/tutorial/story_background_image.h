/**
 * @file story_background_image.h
 * @brief Dependency-free BMP background loading for CLOCK Storybook tutorial frames.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <filesystem>

#include "tutorial/tutorial_surface.h"

namespace clockfw::sim::tutorial {

/** @brief Loads an uncompressed 24/32-bit BMP into an RGBA8 surface. */
TutorialSurface loadStoryBackgroundImage(const std::filesystem::path& path);

}  // namespace clockfw::sim::tutorial
