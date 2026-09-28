/**
 * @file story_theme.cpp
 * @brief Loads deterministic CLOCK Storybook themes without system-font substitution.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_theme.h"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <sstream>
#include <stdexcept>

#include "tutorial/story_yaml.h"

namespace clockfw::sim::tutorial {
namespace {

using Node = StoryYamlNode;

const Node& requireMapping(const Node& parent, const char* key) {
    const Node* value = parent.find(key);
    if (value == nullptr || value->type != Node::Type::Mapping) {
        throw std::runtime_error(std::string("theme.") + key + " mapping is required");
    }
    return *value;
}

std::string requireScalar(const Node& parent, const char* key, const std::string& context) {
    const Node* value = parent.find(key);
    if (value == nullptr || value->type != Node::Type::Scalar || value->scalar.empty()) {
        throw std::runtime_error(context + "." + key + " is required");
    }
    return value->scalar;
}

std::uint32_t requireUnsigned(const Node& parent, const char* key, const std::string& context) {
    const std::string text = requireScalar(parent, key, context);
    if (!std::all_of(text.begin(), text.end(), [](const unsigned char ch) { return std::isdigit(ch) != 0; })) {
        throw std::runtime_error(context + "." + key + " must be an unsigned integer");
    }
    const unsigned long value = std::stoul(text);
    if (value == 0UL || value > 1000UL) {
        throw std::runtime_error(context + "." + key + " must be in range 1..1000");
    }
    return static_cast<std::uint32_t>(value);
}

TutorialColor parseColour(const std::string& text, const std::string& context) {
    if (text.size() != 7U || text.front() != '#' ||
        !std::all_of(text.begin() + 1, text.end(), [](const unsigned char ch) { return std::isxdigit(ch) != 0; })) {
        throw std::runtime_error(context + " must use #RRGGBB syntax");
    }
    const auto component = [&](const std::size_t offset) {
        return static_cast<std::uint8_t>(std::stoul(text.substr(offset, 2U), nullptr, 16));
    };
    return {component(1U), component(3U), component(5U), 255U};
}

StoryFontStyle loadFont(const Node& fonts, const char* key) {
    const Node& node = requireMapping(fonts, key);
    StoryFontStyle style{};
    style.family = requireScalar(node, "family", std::string("theme.fonts.") + key);
    style.sizePx = requireUnsigned(node, "size_px", std::string("theme.fonts.") + key);
    if ((style.sizePx % 7U) != 0U) {
        throw std::runtime_error(std::string("theme.fonts.") + key + ".size_px must be a multiple of 7 for deterministic bitmap scaling");
    }
    if (!storybookFontAvailable(style.family)) {
        throw std::runtime_error(std::string("requested Storybook font is not available: ") + style.family);
    }
    return style;
}

}  // namespace

bool storybookFontAvailable(const std::string& family) {
    return family == "CLOCK UI" || family == "CLOCK Mono";
}

StoryTheme loadStoryTheme(const std::filesystem::path& tutorialRoot, const std::string& themeId) {
    const std::filesystem::path path = tutorialRoot / "themes" / (themeId + ".yaml");
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        throw std::runtime_error("theme not found: " + path.string());
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const StoryYamlResult parsed = parseStoryYaml(buffer.str());
    if (!parsed) {
        const auto& issue = parsed.issues.front();
        throw std::runtime_error("theme YAML line " + std::to_string(issue.line) + ": " + issue.reason);
    }
    if (!parsed.root || parsed.root->type != Node::Type::Mapping) {
        throw std::runtime_error("theme root must be a mapping");
    }
    const std::string id = requireScalar(*parsed.root, "id", "theme");
    if (id != themeId) {
        throw std::runtime_error("theme id does not match requested theme: " + themeId);
    }
    const Node& fonts = requireMapping(*parsed.root, "fonts");
    const Node& colours = requireMapping(*parsed.root, "colours");
    StoryTheme theme{};
    theme.id = id;
    theme.body = loadFont(fonts, "body");
    theme.heading = loadFont(fonts, "heading");
    theme.chapter = loadFont(fonts, "chapter");
    theme.subtitle = loadFont(fonts, "subtitle");
    theme.monospace = loadFont(fonts, "monospace");
    theme.lineSpacingPx = requireUnsigned(*parsed.root, "line_spacing_px", "theme");
    theme.background = parseColour(requireScalar(colours, "background", "theme.colours"), "theme.colours.background");
    theme.foreground = parseColour(requireScalar(colours, "foreground", "theme.colours"), "theme.colours.foreground");
    theme.muted = parseColour(requireScalar(colours, "muted", "theme.colours"), "theme.colours.muted");
    theme.accent = parseColour(requireScalar(colours, "accent", "theme.colours"), "theme.colours.accent");
    theme.tipBackground = parseColour(requireScalar(colours, "tip_background", "theme.colours"), "theme.colours.tip_background");
    theme.tipForeground = parseColour(requireScalar(colours, "tip_foreground", "theme.colours"), "theme.colours.tip_foreground");
    theme.warningBackground = parseColour(requireScalar(colours, "warning_background", "theme.colours"), "theme.colours.warning_background");
    theme.warningForeground = parseColour(requireScalar(colours, "warning_foreground", "theme.colours"), "theme.colours.warning_foreground");
    theme.recipeBackground = parseColour(requireScalar(colours, "recipe_background", "theme.colours"), "theme.colours.recipe_background");
    theme.recipeForeground = parseColour(requireScalar(colours, "recipe_foreground", "theme.colours"), "theme.colours.recipe_foreground");
    return theme;
}

}  // namespace clockfw::sim::tutorial
