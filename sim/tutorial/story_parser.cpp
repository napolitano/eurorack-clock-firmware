/**
 * @file story_parser.cpp
 * @brief Decodes the strict CLOCK Storybook YAML subset into the typed schema-1 model.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_parser.h"
#include <algorithm>
#include <charconv>
#include <fstream>
#include <sstream>
#include <string>
#include <system_error>
#include <unordered_set>
#include "tutorial/story_yaml.h"
namespace clockfw::sim::tutorial {
namespace {
using Node = StoryYamlNode;
struct Decoder final {
    Story story;
    std::vector<StoryIssue> issues;
    void issue(const Node& node, const std::string& reason, std::optional<std::size_t> scene = std::nullopt,
               std::optional<std::size_t> action = std::nullopt) {
        issues.push_back({story.id, scene, action, node.line, reason});
    }
    bool mapping(const Node& node, const char* context, std::optional<std::size_t> scene = std::nullopt,
                 std::optional<std::size_t> action = std::nullopt) {
        if (node.type == Node::Type::Mapping) {
            return true;
        }
        issue(node, std::string(context) + " must be a mapping", scene, action);
        return false;
    }
    void rejectUnknown(const Node& node, std::initializer_list<const char*> allowed, const char* context,
                       std::optional<std::size_t> scene = std::nullopt,
                       std::optional<std::size_t> action = std::nullopt) {
        std::unordered_set<std::string> keys;
        for (const char* key : allowed) {
            keys.emplace(key);
        }
        for (const auto& entry : node.mapping) {
            if (keys.count(entry.first) == 0U) {
                issue(entry.second, std::string(context) + ": unknown field '" + entry.first + "'", scene, action);
            }
        }
    }
    const Node* required(const Node& node, const char* key, const char* context,
                         std::optional<std::size_t> scene = std::nullopt,
                         std::optional<std::size_t> action = std::nullopt) {
        const Node* value = node.find(key);
        if (value == nullptr) {
            issue(node, std::string(context) + ": missing required field '" + key + "'", scene, action);
        }
        return value;
    }
    std::optional<std::string> scalar(const Node& node, const char* context,
                                      std::optional<std::size_t> scene = std::nullopt,
                                      std::optional<std::size_t> action = std::nullopt) {
        if (node.type != Node::Type::Scalar || node.scalar.empty()) {
            issue(node, std::string(context) + " must be a non-empty scalar", scene, action);
            return std::nullopt;
        }
        return node.scalar;
    }
    std::optional<std::uint32_t> uintValue(const Node& node, const char* context, const std::uint32_t min,
                                           const std::uint32_t max, std::optional<std::size_t> scene = std::nullopt,
                                           std::optional<std::size_t> action = std::nullopt) {
        const auto text = scalar(node, context, scene, action);
        if (!text) {
            return std::nullopt;
        }
        std::uint32_t value = 0U;
        const auto* begin = text->data();
        const auto* end = begin + text->size();
        const auto result = std::from_chars(begin, end, value);
        if (result.ec != std::errc{} || result.ptr != end || value < min || value > max) {
            issue(node, std::string(context) + " must be an integer in range " + std::to_string(min) + ".." +
                            std::to_string(max), scene, action);
            return std::nullopt;
        }
        return value;
    }
    std::optional<bool> boolValue(const Node& node, const char* context, std::optional<std::size_t> scene = std::nullopt,
                                  std::optional<std::size_t> action = std::nullopt) {
        const auto text = scalar(node, context, scene, action);
        if (!text) {
            return std::nullopt;
        }
        if (*text == "true") return true;
        if (*text == "false") return false;
        issue(node, std::string(context) + " must be true or false", scene, action);
        return std::nullopt;
    }
    std::optional<TransitionKind> transition(const Node& node, std::optional<std::size_t> scene = std::nullopt) {
        if (!mapping(node, "transition", scene)) return std::nullopt;
        rejectUnknown(node, {"type", "duration_ms"}, "transition", scene);
        const Node* type = required(node, "type", "transition", scene);
        if (type == nullptr) return std::nullopt;
        const auto text = scalar(*type, "transition.type", scene);
        if (!text) return std::nullopt;
        if (*text == "cut") return TransitionKind::Cut;
        if (*text == "fade") return TransitionKind::Fade;
        issue(*type, "transition.type: unsupported transition '" + *text + "'", scene);
        return std::nullopt;
    }
    std::optional<InteractionProfile> profile(const Node& node) {
        const auto text = scalar(node, "interaction_profile");
        if (!text) return std::nullopt;
        if (*text == "HUMAN_SLOW") return InteractionProfile::HumanSlow;
        if (*text == "HUMAN_NORMAL") return InteractionProfile::HumanNormal;
        if (*text == "HUMAN_FAST") return InteractionProfile::HumanFast;
        issue(node, "interaction_profile: unknown profile '" + *text + "'");
        return std::nullopt;
    }
    std::optional<StoryWaveform> waveform(const Node& node, std::size_t scene, std::size_t action) {
        const auto text = scalar(node, "waveform", scene, action);
        if (!text) return std::nullopt;
        if (*text == "square") return StoryWaveform::Square;
        if (*text == "sine") return StoryWaveform::Sine;
        if (*text == "triangle") return StoryWaveform::Triangle;
        issue(node, "waveform: expected square, sine or triangle", scene, action);
        return std::nullopt;
    }
    void decodeOutput(const Node& node) {
        if (!mapping(node, "output")) return;
        rejectUnknown(node, {"width", "height", "fps"}, "output");
        if (const Node* value = node.find("width")) {
            if (const auto parsed = uintValue(*value, "output.width", 16U, 16384U)) story.output.width = static_cast<std::uint16_t>(*parsed);
        }
        if (const Node* value = node.find("height")) {
            if (const auto parsed = uintValue(*value, "output.height", 16U, 16384U)) story.output.height = static_cast<std::uint16_t>(*parsed);
        }
        if (const Node* value = node.find("fps")) {
            if (const auto parsed = uintValue(*value, "output.fps", 1U, 240U)) story.output.framesPerSecond = static_cast<std::uint16_t>(*parsed);
        }
    }
    void decodePublication(const Node& node) {
        if (!mapping(node, "publication")) return;
        rejectUnknown(node, {"intro_video", "outro_video"}, "publication");
        if (const Node* value = node.find("intro_video")) story.publication.introVideo = scalar(*value, "publication.intro_video");
        if (const Node* value = node.find("outro_video")) story.publication.outroVideo = scalar(*value, "publication.outro_video");
    }
    void decodeSetup(const Node& node) {
        if (!mapping(node, "setup")) return;
        rejectUnknown(node, {"factory_reset", "power"}, "setup");
        if (const Node* value = node.find("factory_reset")) {
            if (const auto parsed = boolValue(*value, "setup.factory_reset")) story.setup.factoryReset = *parsed;
        }
        if (const Node* value = node.find("power")) {
            const auto text = scalar(*value, "setup.power");
            if (text && (*text == "on" || *text == "off")) story.setup.powerOn = (*text == "on");
            else if (text) issue(*value, "setup.power must be 'on' or 'off'");
        }
    }
    StoryAction decodeAction(const std::string& name, const Node& node, const std::size_t sceneIndex, const std::size_t actionIndex) {
        StoryAction action;
        action.sourceLine = node.line;
        if (name == "wait_ms") {
            action.kind = StoryActionKind::Wait;
            if (const auto value = uintValue(node, "wait_ms", 1U, 3600000U, sceneIndex, actionIndex)) action.durationMs = *value;
            return action;
        }
        if (!mapping(node, name.c_str(), sceneIndex, actionIndex)) return action;
        if (name == "encoder") {
            action.kind = StoryActionKind::Encoder;
            rejectUnknown(node, {"direction", "detents"}, "encoder", sceneIndex, actionIndex);
            if (const Node* value = required(node, "direction", "encoder", sceneIndex, actionIndex)) {
                const auto text = scalar(*value, "encoder.direction", sceneIndex, actionIndex);
                if (text && *text == "clockwise") action.encoderDirection = 1;
                else if (text && (*text == "counter_clockwise" || *text == "counter-clockwise")) action.encoderDirection = -1;
                else if (text) issue(*value, "encoder.direction: expected clockwise or counter_clockwise", sceneIndex, actionIndex);
            }
            if (const Node* value = node.find("detents")) {
                if (const auto parsed = uintValue(*value, "encoder.detents", 1U, 4096U, sceneIndex, actionIndex)) action.detents = static_cast<std::uint16_t>(*parsed);
            }
        } else if (name == "encoder_push") {
            action.kind = StoryActionKind::EncoderPush;
            rejectUnknown(node, {"hold_ms"}, "encoder_push", sceneIndex, actionIndex);
            if (const Node* value = node.find("hold_ms")) {
                action.holdMs = uintValue(*value, "encoder_push.hold_ms", 1U, 10000U, sceneIndex, actionIndex);
            }
        } else if (name == "button") {
            action.kind = StoryActionKind::Button;
            rejectUnknown(node, {"name", "state"}, "button", sceneIndex, actionIndex);
            if (const Node* value = required(node, "name", "button", sceneIndex, actionIndex)) {
                const auto text = scalar(*value, "button.name", sceneIndex, actionIndex);
                if (text && *text == "PLAY") action.moduleControl = ModuleControl::Play;
                else if (text && *text == "TAP") action.moduleControl = ModuleControl::Tap;
                else if (text && (*text == "STOP" || *text == "STOP_BACK")) action.moduleControl = ModuleControl::StopBack;
                else if (text) issue(*value, "button.name: unknown CLOCK button '" + *text + "'", sceneIndex, actionIndex);
            }
            if (const Node* value = node.find("state")) {
                const auto text = scalar(*value, "button.state", sceneIndex, actionIndex);
                if (text && *text == "down") action.buttonState = true;
                else if (text && *text == "up") action.buttonState = false;
                else if (text) issue(*value, "button.state: expected 'down' or 'up'", sceneIndex, actionIndex);
            }
        } else if (name == "power" || name == "sync_cable" || name == "rst_cable" || name == "sync_generator" || name == "rst_generator") {
            action.kind = name == "power" ? StoryActionKind::Power : name == "sync_cable" ? StoryActionKind::SyncCable :
                          name == "rst_cable" ? StoryActionKind::ResetCable : name == "sync_generator" ? StoryActionKind::SyncGenerator : StoryActionKind::ResetGenerator;
            rejectUnknown(node, {"state"}, name.c_str(), sceneIndex, actionIndex);
            if (const Node* value = required(node, "state", name.c_str(), sceneIndex, actionIndex)) {
                const auto text = scalar(*value, (name + ".state").c_str(), sceneIndex, actionIndex);
                if (text) {
                    const bool onOff = name == "power";
                    const bool cable = name == "sync_cable" || name == "rst_cable";
                    const std::string yes = onOff ? "on" : cable ? "connect" : "run";
                    const std::string no = onOff ? "off" : cable ? "disconnect" : "hold";
                    if (*text == yes) action.state = true;
                    else if (*text == no) action.state = false;
                    else issue(*value, name + ".state: expected '" + yes + "' or '" + no + "'", sceneIndex, actionIndex);
                }
            }
        } else if (name == "sync_source") {
            action.kind = StoryActionKind::SyncSource;
            rejectUnknown(node, {"bpm", "ppqn", "waveform"}, "sync_source", sceneIndex, actionIndex);
            if (const Node* value = node.find("bpm")) action.bpm = uintValue(*value, "sync_source.bpm", 1U, 999U, sceneIndex, actionIndex);
            if (const Node* value = node.find("ppqn")) {
                if (const auto parsed = uintValue(*value, "sync_source.ppqn", 1U, 24U, sceneIndex, actionIndex)) action.ppqn = static_cast<std::uint8_t>(*parsed);
            }
            if (const Node* value = node.find("waveform")) action.waveform = waveform(*value, sceneIndex, actionIndex);
            if (!action.bpm && !action.ppqn && !action.waveform) issue(node, "sync_source requires bpm, ppqn and/or waveform", sceneIndex, actionIndex);
            if (action.ppqn && *action.ppqn != 1U && *action.ppqn != 2U && *action.ppqn != 4U && *action.ppqn != 24U) issue(node, "sync_source.ppqn must be 1, 2, 4 or 24", sceneIndex, actionIndex);
        } else if (name == "rst_pulse") {
            action.kind = StoryActionKind::ResetPulse;
            rejectUnknown(node, {}, "rst_pulse", sceneIndex, actionIndex);
        } else if (name == "subtitle") {
            action.kind = StoryActionKind::Subtitle;
            rejectUnknown(node, {"text"}, "subtitle", sceneIndex, actionIndex);
            if (const Node* value = required(node, "text", "subtitle", sceneIndex, actionIndex)) {
                if (const auto parsed = scalar(*value, "subtitle.text", sceneIndex, actionIndex)) action.text = *parsed;
            }
        } else if (name == "scope") {
            action.kind = StoryActionKind::Scope;
            rejectUnknown(node, {"state", "channel"}, "scope", sceneIndex, actionIndex);
            if (const Node* value = required(node, "state", "scope", sceneIndex, actionIndex)) {
                const auto text = scalar(*value, "scope.state", sceneIndex, actionIndex);
                if (text && *text == "hide") action.scopeMode = ScopeMode::Hidden;
                else if (text && *text == "show") action.scopeMode = ScopeMode::VisibleChannel;
                else if (text) issue(*value, "scope.state: expected show or hide", sceneIndex, actionIndex);
            }
            if (const Node* value = node.find("channel")) {
                const auto text = scalar(*value, "scope.channel", sceneIndex, actionIndex);
                if (text && *text != "visible") issue(*value, "scope.channel: Phase 1 supports only 'visible'", sceneIndex, actionIndex);
            } else if (action.scopeMode == ScopeMode::VisibleChannel) {
                issue(node, "scope show requires channel: visible", sceneIndex, actionIndex);
            }
        } else if (name == "focus") {
            action.kind = StoryActionKind::Focus;
            rejectUnknown(node, {"target", "label", "placement", "duration_ms", "x", "y", "width", "height", "narration"}, "focus", sceneIndex, actionIndex);
            if (const Node* value = required(node, "target", "focus", sceneIndex, actionIndex)) {
                const auto text = scalar(*value, "focus.target", sceneIndex, actionIndex);
                if (text && *text == "encoder") action.focusTarget = FocusTarget::Encoder;
                else if (text && *text == "play") action.focusTarget = FocusTarget::Play;
                else if (text && *text == "tap") action.focusTarget = FocusTarget::Tap;
                else if (text && (*text == "stop" || *text == "stop_back")) action.focusTarget = FocusTarget::StopBack;
                else if (text && *text == "sync") action.focusTarget = FocusTarget::Sync;
                else if (text && (*text == "rst" || *text == "reset")) action.focusTarget = FocusTarget::Reset;
                else if (text && *text == "oled_top_bar") { action.focusTarget = FocusTarget::OledRegion; action.focusX = 0; action.focusY = 0; action.focusWidth = 128; action.focusHeight = 12; }
                else if (text && *text == "oled_region") action.focusTarget = FocusTarget::OledRegion;
                else if (text) issue(*value, "focus.target: expected encoder/play/tap/stop/sync/rst/oled_top_bar/oled_region", sceneIndex, actionIndex);
            }
            if (const Node* value = node.find("label")) if (const auto parsed = scalar(*value, "focus.label", sceneIndex, actionIndex)) action.text = *parsed;
            if (const Node* value = node.find("narration")) if (const auto parsed = scalar(*value, "focus.narration", sceneIndex, actionIndex)) action.narrationId = *parsed;
            if (const Node* value = node.find("placement")) if (const auto parsed = scalar(*value, "focus.placement", sceneIndex, actionIndex)) {
                if (*parsed == "auto") action.focusPlacement = FocusPlacement::Auto;
                else if (*parsed == "left") action.focusPlacement = FocusPlacement::Left;
                else if (*parsed == "right") action.focusPlacement = FocusPlacement::Right;
                else if (*parsed == "above") action.focusPlacement = FocusPlacement::Above;
                else if (*parsed == "below") action.focusPlacement = FocusPlacement::Below;
                else issue(*value, "focus.placement: expected auto/left/right/above/below", sceneIndex, actionIndex);
            }
            if (const Node* value = node.find("duration_ms")) if (const auto parsed = uintValue(*value, "focus.duration_ms", 100U, 60000U, sceneIndex, actionIndex)) action.durationMs = *parsed;
            if (action.durationMs == 0U) action.durationMs = 2200U;
            if (action.focusTarget == FocusTarget::OledRegion && action.focusWidth == 0) {
                if (const Node* value = required(node, "x", "focus", sceneIndex, actionIndex)) if (const auto parsed = uintValue(*value, "focus.x", 0U, 127U, sceneIndex, actionIndex)) action.focusX = static_cast<int>(*parsed);
                if (const Node* value = required(node, "y", "focus", sceneIndex, actionIndex)) if (const auto parsed = uintValue(*value, "focus.y", 0U, 63U, sceneIndex, actionIndex)) action.focusY = static_cast<int>(*parsed);
                if (const Node* value = required(node, "width", "focus", sceneIndex, actionIndex)) if (const auto parsed = uintValue(*value, "focus.width", 1U, 128U, sceneIndex, actionIndex)) action.focusWidth = static_cast<int>(*parsed);
                if (const Node* value = required(node, "height", "focus", sceneIndex, actionIndex)) if (const auto parsed = uintValue(*value, "focus.height", 1U, 64U, sceneIndex, actionIndex)) action.focusHeight = static_cast<int>(*parsed);
            }
        } else if (name == "wait_until") {
            action.kind = StoryActionKind::WaitUntil;
            rejectUnknown(node, {"external_sync", "transport", "power", "timeout_ms"}, "wait_until", sceneIndex, actionIndex);
            const char* selected = nullptr;
            for (const char* key : {"external_sync", "transport", "power"}) if (node.find(key) != nullptr) selected = selected == nullptr ? key : "";
            if (selected == nullptr || selected[0] == '\0') issue(node, "wait_until requires exactly one condition", sceneIndex, actionIndex);
            else {
                const Node* value = node.find(selected);
                action.waitCondition = std::string(selected) == "external_sync" ? WaitCondition::ExternalSync : std::string(selected) == "transport" ? WaitCondition::Transport : WaitCondition::Power;
                if (value != nullptr) if (const auto parsed = scalar(*value, "wait_until expected state", sceneIndex, actionIndex)) action.expected = *parsed;
            }
            action.timeoutMs = 3000U;
            if (const Node* value = node.find("timeout_ms")) if (const auto parsed = uintValue(*value, "wait_until.timeout_ms", 1U, 3600000U, sceneIndex, actionIndex)) action.timeoutMs = *parsed;
        } else if (name == "assert") {
            action.kind = StoryActionKind::Assert;
            rejectUnknown(node, {"clock_source", "transport", "power", "sync_cable", "rst_cable"}, "assert", sceneIndex, actionIndex);
            const char* selected = nullptr;
            for (const char* key : {"clock_source", "transport", "power", "sync_cable", "rst_cable"}) if (node.find(key) != nullptr) selected = selected == nullptr ? key : "";
            if (selected == nullptr || selected[0] == '\0') issue(node, "assert requires exactly one assertion", sceneIndex, actionIndex);
            else {
                const std::string key(selected);
                action.assertion = key == "clock_source" ? AssertionKind::ClockSource : key == "transport" ? AssertionKind::Transport : key == "power" ? AssertionKind::Power : key == "sync_cable" ? AssertionKind::SyncCable : AssertionKind::ResetCable;
                const Node* value = node.find(selected);
                if (value != nullptr) if (const auto parsed = scalar(*value, "assert expected state", sceneIndex, actionIndex)) action.expected = *parsed;
            }
        } else {
            issue(node, "unknown action type '" + name + "'", sceneIndex, actionIndex);
        }
        return action;
    }

    StoryScene decodeScene(const std::string& name, const Node& node, const std::size_t sceneIndex) {
        StoryScene scene;
        scene.sourceLine = node.line;
        scene.transition = story.defaultTransition;
        if (!mapping(node, name.c_str(), sceneIndex)) return scene;
        if (name == "tutorial") {
            scene.kind = SceneKind::Tutorial;
            rejectUnknown(node, {"subtitle", "actions", "transition", "narration"}, "tutorial", sceneIndex);
            if (const Node* value = node.find("subtitle")) if (const auto parsed = scalar(*value, "tutorial.subtitle", sceneIndex)) scene.subtitle = *parsed;
            if (const Node* value = node.find("transition")) if (const auto parsed = transition(*value, sceneIndex)) scene.transition = *parsed;
            if (const Node* value = node.find("narration")) if (const auto parsed = scalar(*value, "tutorial.narration", sceneIndex)) scene.narrationId = *parsed;
            const Node* actions = required(node, "actions", "tutorial", sceneIndex);
            if (actions != nullptr) {
                if (actions->type != Node::Type::Sequence) issue(*actions, "tutorial.actions must be a sequence", sceneIndex);
                else {
                    for (std::size_t i = 0U; i < actions->sequence.size(); ++i) {
                        const Node& item = actions->sequence[i];
                        if (item.type != Node::Type::Mapping) {
                            issue(item, "tutorial action must be a mapping", sceneIndex, i);
                            continue;
                        }
                        const Node* narration = item.find("narration");
                        const std::size_t actionFieldCount = item.mapping.size() - (narration != nullptr ? 1U : 0U);
                        if (actionFieldCount != 1U) {
                            issue(item, "tutorial action must contain exactly one action type plus optional narration", sceneIndex, i);
                            continue;
                        }
                        const auto actionEntry = std::find_if(item.mapping.begin(), item.mapping.end(), [](const auto& entry) {
                            return entry.first != "narration";
                        });
                        StoryAction action = decodeAction(actionEntry->first, actionEntry->second, sceneIndex, i);
                        if (narration != nullptr) {
                            if (const auto parsed = scalar(*narration, "action.narration", sceneIndex, i)) action.narrationId = *parsed;
                        }
                        scene.actions.push_back(std::move(action));
                    }
                }
            }
            return scene;
        }
        rejectUnknown(node, {"number", "type", "title", "subtitle", "body", "steps", "duration_ms", "transition", "narration"}, name.c_str(), sceneIndex);
        if (name == "chapter") scene.kind = SceneKind::Chapter;
        else if (name == "text") scene.kind = SceneKind::Text;
        else if (name == "callout") scene.kind = SceneKind::Callout;
        else {
            issue(node, "unknown scene type '" + name + "'", sceneIndex);
            return scene;
        }
        if (const Node* value = node.find("number")) if (const auto parsed = uintValue(*value, "chapter.number", 1U, 9999U, sceneIndex)) scene.number = *parsed;
        if (const Node* value = node.find("title")) if (const auto parsed = scalar(*value, (name + ".title").c_str(), sceneIndex)) scene.title = *parsed;
        if (const Node* value = node.find("subtitle")) if (const auto parsed = scalar(*value, (name + ".subtitle").c_str(), sceneIndex)) scene.subtitle = *parsed;
        if (const Node* value = node.find("body")) if (const auto parsed = scalar(*value, (name + ".body").c_str(), sceneIndex)) scene.body = *parsed;
        if (const Node* value = node.find("narration")) if (const auto parsed = scalar(*value, (name + ".narration").c_str(), sceneIndex)) scene.narrationId = *parsed;
        if (const Node* value = required(node, "duration_ms", name.c_str(), sceneIndex)) if (const auto parsed = uintValue(*value, "duration_ms", 1U, 3600000U, sceneIndex)) scene.durationMs = *parsed;
        if (const Node* value = node.find("transition")) if (const auto parsed = transition(*value, sceneIndex)) scene.transition = *parsed;
        if (scene.kind == SceneKind::Callout) {
            const Node* type = required(node, "type", "callout", sceneIndex);
            if (type != nullptr) {
                const auto text = scalar(*type, "callout.type", sceneIndex);
                if (text && *text == "tip") scene.calloutKind = CalloutKind::Tip;
                else if (text && *text == "warning") scene.calloutKind = CalloutKind::Warning;
                else if (text && *text == "recipe") scene.calloutKind = CalloutKind::Recipe;
                else if (text) issue(*type, "callout.type must be tip, warning or recipe", sceneIndex);
            }
            if (const Node* steps = node.find("steps")) {
                if (steps->type != Node::Type::Sequence) issue(*steps, "callout.steps must be a sequence", sceneIndex);
                else for (const Node& step : steps->sequence) if (const auto parsed = scalar(step, "callout step", sceneIndex)) scene.recipeSteps.push_back(*parsed);
            }
        }
        return scene;
    }

    StoryParseResult decode(const Node& root) {
        if (!mapping(root, "story")) return {std::nullopt, std::move(issues)};
        if (const Node* value = root.find("id")) if (const auto parsed = scalar(*value, "id")) story.id = *parsed;
        rejectUnknown(root, {"schema", "id", "title", "language", "theme", "interaction_profile", "output", "publication", "defaults", "setup", "scenes"}, "story");
        if (const Node* value = required(root, "schema", "story")) if (const auto parsed = uintValue(*value, "schema", 1U, 0xFFFFU)) story.schema = *parsed;
        if (root.find("id") == nullptr) required(root, "id", "story");
        if (const Node* value = required(root, "title", "story")) if (const auto parsed = scalar(*value, "title")) story.title = *parsed;
        if (const Node* value = required(root, "language", "story")) if (const auto parsed = scalar(*value, "language")) story.language = *parsed;
        if (const Node* value = required(root, "theme", "story")) if (const auto parsed = scalar(*value, "theme")) story.theme = *parsed;
        if (const Node* value = required(root, "interaction_profile", "story")) if (const auto parsed = profile(*value)) story.interactionProfile = *parsed;
        if (const Node* value = root.find("output")) decodeOutput(*value);
        if (const Node* value = root.find("publication")) decodePublication(*value);
        if (const Node* value = root.find("setup")) decodeSetup(*value);
        if (const Node* defaults = root.find("defaults")) {
            if (mapping(*defaults, "defaults")) {
                rejectUnknown(*defaults, {"transition"}, "defaults");
                if (const Node* value = defaults->find("transition")) if (const auto parsed = transition(*value)) story.defaultTransition = *parsed;
            }
        }
        const Node* scenes = required(root, "scenes", "story");
        if (scenes != nullptr) {
            if (scenes->type != Node::Type::Sequence || scenes->sequence.empty()) issue(*scenes, "scenes must be a non-empty sequence");
            else for (std::size_t i = 0U; i < scenes->sequence.size(); ++i) {
                const Node& item = scenes->sequence[i];
                if (item.type != Node::Type::Mapping || item.mapping.size() != 1U) issue(item, "scene entry must contain exactly one scene type", i);
                else story.scenes.push_back(decodeScene(item.mapping.front().first, item.mapping.front().second, i));
            }
        }
        StoryParseResult result;
        result.issues = std::move(issues);
        if (result.issues.empty()) result.story = std::move(story);
        return result;
    }
};

}  // namespace

StoryParseResult::operator bool() const {
    return story.has_value() && issues.empty();
}

StoryParseResult parseStoryText(const std::string_view source) {
    const StoryYamlResult yaml = parseStoryYaml(source);
    if (!yaml) {
        StoryParseResult result;
        for (const auto& issue : yaml.issues) result.issues.push_back({{}, std::nullopt, std::nullopt, issue.line, issue.reason});
        return result;
    }
    Decoder decoder;
    return decoder.decode(*yaml.root);
}

StoryParseResult parseStoryFile(const std::filesystem::path& path) {
    std::ifstream input(path, std::ios::binary);
    if (!input) {
        StoryParseResult result;
        result.issues.push_back({{}, std::nullopt, std::nullopt, 0U, "cannot open story file: " + path.string()});
        return result;
    }
    std::ostringstream buffer;
    buffer << input.rdbuf();
    return parseStoryText(buffer.str());
}

}  // namespace clockfw::sim::tutorial
