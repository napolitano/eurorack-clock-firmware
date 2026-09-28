/**
 * @file story_validator.h
 * @brief Semantic and resource validation for typed CLOCK Storybook schema-1 stories.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <filesystem>
#include <vector>

#include "tutorial/story_model.h"

namespace clockfw::sim::tutorial {

/** @brief Validates one parsed story against Phase-1 semantics and docs/tutorials resources. */
std::vector<StoryIssue> validateStory(const Story& story, const std::filesystem::path& tutorialRoot);

/** @brief Validates story-ID uniqueness across one publication/catalog set. */
std::vector<StoryIssue> validateUniqueStoryIds(const std::vector<Story>& stories);

}  // namespace clockfw::sim::tutorial
