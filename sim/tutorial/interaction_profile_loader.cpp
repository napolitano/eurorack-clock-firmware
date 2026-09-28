/**
 * @file interaction_profile_loader.cpp
 * @brief Implements deterministic Storybook interaction-profile resource loading.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/interaction_profile_loader.h"

#include <array>
#include <charconv>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>

#include "tutorial/story_contract.h"
#include "tutorial/story_yaml.h"

namespace clockfw::sim::tutorial {
namespace {

std::optional<std::uint32_t> parseUnsigned(const StoryYamlNode& node) {
    if (node.type != StoryYamlNode::Type::Scalar || node.scalar.empty()) {
        return std::nullopt;
    }
    std::uint32_t value = 0U;
    const char* const begin = node.scalar.data();
    const char* const end = begin + node.scalar.size();
    const auto result = std::from_chars(begin, end, value);
    if (result.ec != std::errc{} || result.ptr != end || value == 0U) {
        return std::nullopt;
    }
    return value;
}

std::string readFile(const std::filesystem::path& path) {
    std::ifstream stream(path);
    if (!stream) {
        return {};
    }
    std::ostringstream buffer;
    buffer << stream.rdbuf();
    return buffer.str();
}

}  // namespace

InteractionTimingResult::operator bool() const {
    return timing.has_value() && issues.empty();
}

InteractionTimingResult loadInteractionTiming(
    const std::filesystem::path& tutorialRoot,
    const InteractionProfile profile) {
    InteractionTimingResult result;
    const std::string profileName(interactionProfileName(profile));
    const std::filesystem::path path = tutorialRoot / "interaction_profiles.yaml";
    const std::string source = readFile(path);
    if (source.empty()) {
        result.issues.push_back({profileName, std::nullopt, std::nullopt, 0U,
                                 "could not read interaction profile resource: " + path.string()});
        return result;
    }

    const StoryYamlResult yaml = parseStoryYaml(source);
    if (!yaml || !yaml.root) {
        for (const StoryYamlIssue& issue : yaml.issues) {
            result.issues.push_back({profileName, std::nullopt, std::nullopt, issue.line,
                                     "interaction profile YAML: " + issue.reason});
        }
        return result;
    }

    const StoryYamlNode* schema = yaml.root->find("schema");
    if (schema == nullptr || schema->type != StoryYamlNode::Type::Scalar ||
        schema->scalar != std::to_string(currentSchemaVersion())) {
        result.issues.push_back({profileName, std::nullopt, std::nullopt, yaml.root->line,
                                 "interaction profile schema must match Storybook schema 1"});
        return result;
    }

    const StoryYamlNode* profiles = yaml.root->find("interaction_profiles");
    const StoryYamlNode* selected = profiles != nullptr ? profiles->find(profileName) : nullptr;
    if (profiles == nullptr || profiles->type != StoryYamlNode::Type::Mapping ||
        selected == nullptr || selected->type != StoryYamlNode::Type::Mapping) {
        result.issues.push_back({profileName, std::nullopt, std::nullopt,
                                 profiles != nullptr ? profiles->line : yaml.root->line,
                                 "interaction profile is not defined: " + profileName});
        return result;
    }

    InteractionTiming timing;
    struct Field final {
        const char* name;
        std::uint32_t InteractionTiming::* member;
    };
    constexpr std::array<Field, 11U> fields{{
        {"encoder_detent_ms", &InteractionTiming::encoderDetentMs},
        {"encoder_fast_detent_ms", &InteractionTiming::encoderFastDetentMs},
        {"button_down_ms", &InteractionTiming::buttonDownMs},
        {"button_release_pause_ms", &InteractionTiming::buttonReleasePauseMs},
        {"encoder_push_down_ms", &InteractionTiming::encoderPushDownMs},
        {"encoder_push_release_pause_ms", &InteractionTiming::encoderPushReleasePauseMs},
        {"patch_action_ms", &InteractionTiming::patchActionMs},
        {"before_action_ms", &InteractionTiming::beforeActionMs},
        {"after_navigation_ms", &InteractionTiming::afterNavigationMs},
        {"after_value_change_ms", &InteractionTiming::afterValueChangeMs},
        {"after_major_screen_change_ms", &InteractionTiming::afterMajorScreenChangeMs},
    }};

    for (const Field& field : fields) {
        const StoryYamlNode* node = selected->find(field.name);
        const std::optional<std::uint32_t> value = node != nullptr ? parseUnsigned(*node) : std::nullopt;
        if (!value) {
            result.issues.push_back({profileName, std::nullopt, std::nullopt,
                                     node != nullptr ? node->line : selected->line,
                                     std::string("interaction profile field is missing/invalid: ") + field.name});
            continue;
        }
        timing.*(field.member) = *value;
    }

    if (result.issues.empty()) {
        result.timing = timing;
    }
    return result;
}

}  // namespace clockfw::sim::tutorial
