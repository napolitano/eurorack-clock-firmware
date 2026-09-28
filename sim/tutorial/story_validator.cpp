/**
 * @file story_validator.cpp
 * @brief Enforces Phase-1 Storybook semantics after strict YAML decoding.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_validator.h"

#include <algorithm>
#include <array>
#include <cctype>
#include <fstream>
#include <set>
#include <sstream>
#include <string>
#include <unordered_set>

#include "tutorial/story_theme.h"
#include "tutorial/story_yaml.h"

namespace clockfw::sim::tutorial {
namespace {

bool validNarrationId(const std::string& id) {
    if (id.empty()) return false;
    return std::all_of(id.begin(), id.end(), [](const unsigned char ch) {
        return std::islower(ch) != 0 || std::isdigit(ch) != 0 || ch == '-';
    });
}

bool validStoryId(const std::string& id) {
    if (id.empty() || ((!std::islower(static_cast<unsigned char>(id.front()))) && (!std::isdigit(static_cast<unsigned char>(id.front()))))) {
        return false;
    }
    return std::all_of(id.begin(), id.end(), [](const unsigned char ch) {
        return std::islower(ch) != 0 || std::isdigit(ch) != 0 || ch == '-';
    });
}


std::optional<StoryYamlNode> loadYaml(const std::filesystem::path& path, std::vector<StoryIssue>& issues,
                                      const Story& story, const std::string& label) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        issues.push_back({story.id, std::nullopt, std::nullopt, 0U, label + " not found: " + path.string()});
        return std::nullopt;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    const StoryYamlResult parsed = parseStoryYaml(buffer.str());
    if (!parsed) {
        for (const auto& issue : parsed.issues) {
            issues.push_back({story.id, std::nullopt, std::nullopt, issue.line, label + ": " + issue.reason});
        }
        return std::nullopt;
    }
    return parsed.root;
}

void validateTheme(const Story& story, const std::filesystem::path& root, std::vector<StoryIssue>& issues) {
    try {
        (void)loadStoryTheme(root, story.theme);
    } catch (const std::exception& exception) {
        issues.push_back({story.id, std::nullopt, std::nullopt, 0U, std::string("theme: ") + exception.what()});
    }
}

void validateProfile(const Story& story, const std::filesystem::path& root, std::vector<StoryIssue>& issues) {
    const auto node = loadYaml(root / "interaction_profiles.yaml", issues, story, "interaction profiles");
    if (!node || node->type != StoryYamlNode::Type::Mapping) return;
    const StoryYamlNode* profiles = node->find("interaction_profiles");
    if (profiles == nullptr || profiles->type != StoryYamlNode::Type::Mapping) {
        issues.push_back({story.id, std::nullopt, std::nullopt, node->line, "interaction_profiles mapping is required"});
        return;
    }
    const std::string name(interactionProfileName(story.interactionProfile));
    const StoryYamlNode* profile = profiles->find(name);
    if (profile == nullptr || profile->type != StoryYamlNode::Type::Mapping) {
        issues.push_back({story.id, std::nullopt, std::nullopt, profiles->line, "interaction profile is not defined: " + name});
        return;
    }
    const std::array<const char*, 11U> required = {
        "encoder_detent_ms", "encoder_fast_detent_ms", "button_down_ms", "button_release_pause_ms",
        "encoder_push_down_ms", "encoder_push_release_pause_ms", "patch_action_ms", "before_action_ms",
        "after_navigation_ms", "after_value_change_ms", "after_major_screen_change_ms"};
    for (const char* key : required) {
        const StoryYamlNode* value = profile->find(key);
        if (value == nullptr || value->type != StoryYamlNode::Type::Scalar || value->scalar.empty() ||
            !std::all_of(value->scalar.begin(), value->scalar.end(), [](const unsigned char ch) { return std::isdigit(ch) != 0; })) {
            issues.push_back({story.id, std::nullopt, std::nullopt, profile->line,
                              std::string("interaction profile field is missing/invalid: ") + key});
        }
    }
}

bool expectedValid(const StoryAction& action) {
    if (action.waitCondition) {
        switch (*action.waitCondition) {
            case WaitCondition::ExternalSync: return action.expected == "locked" || action.expected == "unlocked";
            case WaitCondition::Transport: return action.expected == "playing" || action.expected == "paused" || action.expected == "stopped";
            case WaitCondition::Power: return action.expected == "on" || action.expected == "off";
        }
    }
    if (action.assertion) {
        switch (*action.assertion) {
            case AssertionKind::ClockSource: return action.expected == "auto" || action.expected == "internal" || action.expected == "external";
            case AssertionKind::Transport: return action.expected == "playing" || action.expected == "paused" || action.expected == "stopped";
            case AssertionKind::Power: return action.expected == "on" || action.expected == "off";
            case AssertionKind::SyncCable:
            case AssertionKind::ResetCable: return action.expected == "connected" || action.expected == "disconnected";
        }
    }
    return true;
}

}  // namespace

std::vector<StoryIssue> validateStory(const Story& story, const std::filesystem::path& tutorialRoot) {
    std::vector<StoryIssue> issues;
    if (story.schema != currentSchemaVersion()) {
        issues.push_back({story.id, std::nullopt, std::nullopt, 0U, "unsupported schema version " + std::to_string(story.schema)});
    }
    if (!validStoryId(story.id)) {
        issues.push_back({story.id, std::nullopt, std::nullopt, 0U, "story id must use lower-case kebab-case"});
    }
    if (story.scenes.empty()) {
        issues.push_back({story.id, std::nullopt, std::nullopt, 0U, "story must contain at least one scene"});
    }
    if (!phase1SupportsTransition(story.defaultTransition)) {
        issues.push_back({story.id, std::nullopt, std::nullopt, 0U, "Phase 1 supports CUT transitions only"});
    }
    validateTheme(story, tutorialRoot, issues);
    validateProfile(story, tutorialRoot, issues);

    bool syncCable = false;
    bool resetCable = false;
    std::array<bool, 3U> heldButtons{};
    std::unordered_set<std::string> narrationIds;
    for (std::size_t sceneIndex = 0U; sceneIndex < story.scenes.size(); ++sceneIndex) {
        const StoryScene& scene = story.scenes[sceneIndex];
        if (!phase1SupportsTransition(scene.transition)) {
            issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine, "Phase 1 supports CUT transitions only"});
        }
        if (scene.narrationId) {
            if (!validNarrationId(*scene.narrationId)) {
                issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine,
                                  "narration id must use lower-case kebab-case"});
            } else if (!narrationIds.emplace(*scene.narrationId).second) {
                issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine,
                                  "duplicate narration id: " + *scene.narrationId});
            }
        }
        if (scene.kind == SceneKind::Chapter && (scene.number == 0U || scene.title.empty())) {
            issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine, "chapter requires number and title"});
        }
        if (scene.kind == SceneKind::Text && (scene.title.empty() || scene.body.empty())) {
            issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine, "text scene requires title and body"});
        }
        if (scene.kind == SceneKind::Callout) {
            if (!scene.calloutKind || scene.title.empty()) {
                issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine, "callout requires type and title"});
            } else if (*scene.calloutKind == CalloutKind::Recipe && scene.recipeSteps.empty()) {
                issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine, "RECIPE requires at least one step"});
            } else if (*scene.calloutKind != CalloutKind::Recipe && scene.body.empty()) {
                issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine, "TIP/WARNING requires body text"});
            }
        }
        if (scene.kind == SceneKind::Tutorial && scene.actions.empty()) {
            issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine, "tutorial scene requires actions"});
        }
        if (scene.kind == SceneKind::Tutorial && scene.narrationId) {
            issues.push_back({story.id, sceneIndex, std::nullopt, scene.sourceLine,
                              "tutorial scene narration must be attached to a timed action"});
        }
        for (std::size_t actionIndex = 0U; actionIndex < scene.actions.size(); ++actionIndex) {
            const StoryAction& action = scene.actions[actionIndex];
            auto semanticIssue = [&](const std::string& reason) {
                issues.push_back({story.id, sceneIndex, actionIndex, action.sourceLine, reason});
            };
            if (action.narrationId) {
                if (!validNarrationId(*action.narrationId)) {
                    semanticIssue("narration id must use lower-case kebab-case");
                } else if (!narrationIds.emplace(*action.narrationId).second) {
                    semanticIssue("duplicate narration id: " + *action.narrationId);
                }
            }
            switch (action.kind) {
                case StoryActionKind::Button:
                    if (action.buttonState.has_value()) {
                        std::size_t index = 0U;
                        if (action.moduleControl == ModuleControl::Tap) index = 1U;
                        else if (action.moduleControl == ModuleControl::StopBack) index = 2U;
                        if (*action.buttonState && heldButtons[index]) {
                            semanticIssue("button is already held down");
                        } else if (!*action.buttonState && !heldButtons[index]) {
                            semanticIssue("button up requires the same button to be held down");
                        } else {
                            heldButtons[index] = *action.buttonState;
                        }
                    }
                    break;
                case StoryActionKind::SyncCable: syncCable = action.state; break;
                case StoryActionKind::ResetCable: resetCable = action.state; break;
                case StoryActionKind::SyncGenerator:
                    if (!syncCable) semanticIssue("sync_generator requires a connected SYNC cable");
                    break;
                case StoryActionKind::ResetGenerator:
                case StoryActionKind::ResetPulse:
                    if (!resetCable) semanticIssue("RST stimulus requires a connected RST cable");
                    break;
                case StoryActionKind::Focus:
                    if (action.focusTarget == FocusTarget::None) semanticIssue("focus requires a valid target");
                    if (action.focusTarget == FocusTarget::OledRegion &&
                        (action.focusX < 0 || action.focusY < 0 || action.focusWidth <= 0 || action.focusHeight <= 0 ||
                         action.focusX + action.focusWidth > 128 || action.focusY + action.focusHeight > 64)) {
                        semanticIssue("focus OLED region must stay inside the 128x64 framebuffer");
                    }
                    break;
                default: break;
            }
            if (!expectedValid(action)) semanticIssue("invalid expected state for wait_until/assert");
        }
    }
    if (heldButtons[0] || heldButtons[1] || heldButtons[2]) {
        issues.push_back({story.id, std::nullopt, std::nullopt, 0U,
                          "all explicitly held buttons must be released before the story ends"});
    }
    return issues;
}

std::vector<StoryIssue> validateUniqueStoryIds(const std::vector<Story>& stories) {
    std::vector<StoryIssue> issues;
    std::unordered_set<std::string> ids;
    for (const Story& story : stories) {
        if (!ids.emplace(story.id).second) {
            issues.push_back({story.id, std::nullopt, std::nullopt, 0U, "duplicate story id"});
        }
    }
    return issues;
}

}  // namespace clockfw::sim::tutorial
