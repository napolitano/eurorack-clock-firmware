/**
 * @file story_yaml.h
 * @brief Strict project-local YAML subset used by CLOCK Storybook schema files.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstddef>
#include <optional>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace clockfw::sim::tutorial {

/** @brief One parsed YAML node from the intentionally narrow Storybook authoring subset. */
struct StoryYamlNode {
    enum class Type { Null, Scalar, Mapping, Sequence };

    Type type = Type::Null;
    std::size_t line = 0U;
    std::string scalar;
    std::vector<std::pair<std::string, StoryYamlNode>> mapping;
    std::vector<StoryYamlNode> sequence;

    /** @brief Returns one mapping child by exact key, or null when absent/not a mapping. */
    const StoryYamlNode* find(std::string_view key) const;
};

/** @brief Syntax issue emitted by the strict Storybook YAML reader. */
struct StoryYamlIssue {
    std::size_t line = 0U;
    std::string reason;
};

/** @brief Result of parsing one Storybook YAML document. */
struct StoryYamlResult {
    std::optional<StoryYamlNode> root;
    std::vector<StoryYamlIssue> issues;

    /** @brief Returns true only when a root node exists and no syntax issues were produced. */
    explicit operator bool() const;
};

/**
 * @brief Parses the deterministic YAML subset accepted by Storybook schema 1.
 *
 * Supported constructs are block mappings/sequences, quoted or plain scalars, literal `|`
 * blocks and empty `{}` / `[]` collections. Tabs, anchors, aliases, tags and non-empty flow
 * collections are rejected deliberately instead of being interpreted ambiguously.
 */
StoryYamlResult parseStoryYaml(std::string_view source);

}  // namespace clockfw::sim::tutorial
