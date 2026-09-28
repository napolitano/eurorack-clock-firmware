/**
 * @file story_parser.h
 * @brief Schema-1 CLOCK Storybook YAML decoder with strict structural validation.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <filesystem>
#include <optional>
#include <string_view>
#include <vector>

#include "tutorial/story_model.h"

namespace clockfw::sim::tutorial {

/** @brief Result of decoding one schema-1 story document. */
struct StoryParseResult {
    std::optional<Story> story;
    std::vector<StoryIssue> issues;

    /** @brief Returns true only when decoding completed with no issues. */
    explicit operator bool() const;
};

/** @brief Decodes one Storybook YAML document from memory. */
StoryParseResult parseStoryText(std::string_view source);

/** @brief Decodes one Storybook YAML document from a repository/local file. */
StoryParseResult parseStoryFile(const std::filesystem::path& path);

}  // namespace clockfw::sim::tutorial
