/**
 * @file story_theme.cpp
 * @brief Loads CLOCK Storybook themes without silent font substitution.
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

const Node* optionalMapping(const Node& parent, const char* key) {
    const Node* value = parent.find(key);
    if (value == nullptr) return nullptr;
    if (value->type != Node::Type::Mapping) {
        throw std::runtime_error(std::string("theme.") + key + " must be a mapping");
    }
    return value;
}

std::string requireScalar(const Node& parent, const char* key, const std::string& context) {
    const Node* value = parent.find(key);
    if (value == nullptr || value->type != Node::Type::Scalar || value->scalar.empty()) {
        throw std::runtime_error(context + "." + key + " is required");
    }
    return value->scalar;
}

std::optional<std::string> optionalScalar(const Node& parent, const char* key, const std::string& context) {
    const Node* value = parent.find(key);
    if (value == nullptr) return std::nullopt;
    if (value->type != Node::Type::Scalar) {
        throw std::runtime_error(context + "." + key + " must be a scalar");
    }
    return value->scalar;
}

std::uint32_t parseUnsigned(const std::string& text, const std::string& context, const std::uint32_t maximum) {
    if (text.empty() || !std::all_of(text.begin(), text.end(), [](const unsigned char ch) { return std::isdigit(ch) != 0; })) {
        throw std::runtime_error(context + " must be an unsigned integer");
    }
    const unsigned long value = std::stoul(text);
    if (value == 0UL || value > maximum) {
        throw std::runtime_error(context + " must be in range 1.." + std::to_string(maximum));
    }
    return static_cast<std::uint32_t>(value);
}

std::uint32_t requireUnsigned(const Node& parent, const char* key, const std::string& context) {
    return parseUnsigned(requireScalar(parent, key, context), context + "." + key, 1000U);
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

TutorialColor optionalColour(const Node& colours, const char* key, const TutorialColor fallback) {
    const auto value = optionalScalar(colours, key, "theme.colours");
    return value ? parseColour(*value, std::string("theme.colours.") + key) : fallback;
}

StoryFontBackend parseBackend(const std::string& text, const std::string& context) {
    if (text == "builtin") return StoryFontBackend::Builtin;
    if (text == "system") return StoryFontBackend::System;
    throw std::runtime_error(context + " must be builtin or system");
}

StoryFontStyle loadFont(const Node& fonts, const char* key) {
    const Node& node = requireMapping(fonts, key);
    const std::string context = std::string("theme.fonts.") + key;
    StoryFontStyle style{};
    style.family = requireScalar(node, "family", context);
    style.sizePx = requireUnsigned(node, "size_px", context);
    const auto backend = optionalScalar(node, "backend", context);
    style.backend = backend ? parseBackend(*backend, context + ".backend") : StoryFontBackend::Builtin;
    if (const auto weight = optionalScalar(node, "weight", context)) {
        const std::uint32_t parsed = parseUnsigned(*weight, context + ".weight", 1000U);
        if (parsed < 100U) throw std::runtime_error(context + ".weight must be in range 100..1000");
        style.weight = static_cast<std::uint16_t>(parsed);
    }
    if (style.backend == StoryFontBackend::Builtin && (style.sizePx % 7U) != 0U) {
        throw std::runtime_error(context + ".size_px must be a multiple of 7 for builtin bitmap scaling");
    }
    if (!storybookFontAvailable(style)) {
        throw std::runtime_error("requested Storybook font is not available: " + style.family);
    }
    return style;
}

StoryBackgroundImageMode parseImageMode(const std::string& text) {
    if (text == "cover") return StoryBackgroundImageMode::Cover;
    if (text == "contain") return StoryBackgroundImageMode::Contain;
    if (text == "stretch") return StoryBackgroundImageMode::Stretch;
    throw std::runtime_error("theme.presentation.background_image_mode must be cover, contain or stretch");
}

}  // namespace

bool storybookFontAvailable(const StoryFontStyle& font) {
    if (font.backend == StoryFontBackend::Builtin) {
        return font.family == "CLOCK UI" || font.family == "CLOCK Mono";
    }
    return !font.family.empty();
}

StoryTheme loadStoryTheme(const std::filesystem::path& tutorialRoot, const std::string& themeId) {
    const std::filesystem::path path = tutorialRoot / "themes" / (themeId + ".yaml");
    std::ifstream input(path, std::ios::binary);
    if (!input) throw std::runtime_error("theme not found: " + path.string());
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const StoryYamlResult parsed = parseStoryYaml(buffer.str());
    if (!parsed) {
        const auto& issue = parsed.issues.front();
        throw std::runtime_error("theme YAML line " + std::to_string(issue.line) + ": " + issue.reason);
    }
    if (!parsed.root || parsed.root->type != Node::Type::Mapping) throw std::runtime_error("theme root must be a mapping");
    const std::string id = requireScalar(*parsed.root, "id", "theme");
    if (id != themeId) throw std::runtime_error("theme id does not match requested theme: " + themeId);

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
    theme.tutorialBackground = optionalColour(colours, "tutorial_background", theme.background);
    theme.tutorialSurface = optionalColour(colours, "tutorial_surface", theme.background);
    theme.tutorialSurfaceBorder = optionalColour(colours, "tutorial_surface_border", theme.muted);
    theme.chapterBackground = optionalColour(colours, "chapter_background", theme.background);
    theme.chapterForeground = optionalColour(colours, "chapter_foreground", theme.foreground);
    theme.chapterMuted = optionalColour(colours, "chapter_muted", theme.muted);
    theme.subtitleBackground = optionalColour(colours, "subtitle_background", theme.background);
    theme.subtitleForeground = optionalColour(colours, "subtitle_foreground", theme.foreground);
    theme.tipBackground = parseColour(requireScalar(colours, "tip_background", "theme.colours"), "theme.colours.tip_background");
    theme.tipForeground = parseColour(requireScalar(colours, "tip_foreground", "theme.colours"), "theme.colours.tip_foreground");
    theme.warningBackground = parseColour(requireScalar(colours, "warning_background", "theme.colours"), "theme.colours.warning_background");
    theme.warningForeground = parseColour(requireScalar(colours, "warning_foreground", "theme.colours"), "theme.colours.warning_foreground");
    theme.recipeBackground = parseColour(requireScalar(colours, "recipe_background", "theme.colours"), "theme.colours.recipe_background");
    theme.recipeForeground = parseColour(requireScalar(colours, "recipe_foreground", "theme.colours"), "theme.colours.recipe_foreground");

    if (const Node* presentation = optionalMapping(*parsed.root, "presentation")) {
        if (const auto image = optionalScalar(*presentation, "background_image", "theme.presentation"); image && !image->empty()) {
            std::filesystem::path resolved = *image;
            if (resolved.is_relative()) resolved = tutorialRoot / resolved;
            theme.tutorialBackgroundImage = resolved.lexically_normal();
        }
        if (const auto mode = optionalScalar(*presentation, "background_image_mode", "theme.presentation")) {
            theme.tutorialBackgroundImageMode = parseImageMode(*mode);
        }
    }
    return theme;
}

}  // namespace clockfw::sim::tutorial
