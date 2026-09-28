/**
 * @file storybook_parser_tests.cpp
 * @brief Regression tests for CLOCK Storybook schema-1 model, YAML parser and validator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <string>
#include <vector>

#include "tutorial/story_parser.h"
#include "tutorial/story_validator.h"

namespace {

using namespace clockfw::sim::tutorial;

bool require(const bool condition, const std::string& message) {
    if (!condition) {
        std::cerr << "Storybook parser failure: " << message << '\n';
        return false;
    }
    return true;
}

bool containsReason(const std::vector<StoryIssue>& issues, const std::string& needle) {
    for (const StoryIssue& issue : issues) {
        if (issue.reason.find(needle) != std::string::npos) return true;
    }
    return false;
}

std::filesystem::path root() {
    return std::filesystem::path(CLOCK_SOURCE_ROOT);
}

bool validateFixture(const char* relative) {
    const StoryParseResult parsed = parseStoryFile(root() / relative);
    if (!parsed) return false;
    return validateStory(*parsed.story, root() / "docs/tutorials").empty();
}

}  // namespace

int main() {
    bool ok = true;
    const auto tutorialRoot = root() / "docs/tutorials";

    ok &= require(validateFixture("docs/tutorials/stories/examples/schema1-minimal.yaml"),
                  "minimal fixture must parse and validate");
    ok &= require(validateFixture("docs/tutorials/stories/examples/schema1-power-scope-publication.yaml"),
                  "power/scope/publication fixture must parse and validate");

    const StoryParseResult minimal = parseStoryFile(root() / "docs/tutorials/stories/examples/schema1-minimal.yaml");
    ok &= require(minimal && minimal.story->schema == 1U, "schema must decode");
    ok &= require(minimal && minimal.story->output.width == 1920U && minimal.story->output.height == 1080U &&
                      minimal.story->output.framesPerSecond == 30U,
                  "default output profile must survive parsing");
    ok &= require(minimal && minimal.story->scenes.size() == 2U, "minimal story must have two scenes");
    ok &= require(minimal && minimal.story->scenes[1].actions.size() == 3U, "tutorial actions must decode");

    const StoryParseResult power = parseStoryFile(root() / "docs/tutorials/stories/examples/schema1-power-scope-publication.yaml");
    ok &= require(power && power.story->publication.introVideo.has_value() && power.story->publication.outroVideo.has_value(),
                  "publication assets must decode as references");
    ok &= require(power && power.story->setup.powerOn.has_value() && !*power.story->setup.powerOn,
                  "setup power state must decode");

    const StoryParseResult fade = parseStoryFile(root() / "docs/tutorials/stories/examples/schema1-invalid-fade.yaml");
    ok &= require(static_cast<bool>(fade), "known future FADE must parse structurally");
    ok &= require(fade && containsReason(validateStory(*fade.story, tutorialRoot), "CUT transitions only"),
                  "FADE must fail Phase-1 semantic validation");

    const StoryParseResult generator = parseStoryFile(
        root() / "docs/tutorials/stories/examples/schema1-invalid-generator-without-cable.yaml");
    ok &= require(static_cast<bool>(generator), "generator fixture must parse structurally");
    const auto generatorIssues = generator ? validateStory(*generator.story, tutorialRoot) : std::vector<StoryIssue>{};
    ok &= require(containsReason(generatorIssues, "connected SYNC cable"),
                  "generator run without cable must fail semantic validation");
    ok &= require(!generatorIssues.empty() && generatorIssues.front().sceneIndex.has_value(),
                  "semantic errors must include scene context");

    const StoryParseResult badSchema = parseStoryText(R"YAML(
schema: 2
id: bad-schema
title: "Bad schema"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - chapter:
      number: 1
      title: "Nope"
      duration_ms: 1000
)YAML");
    ok &= require(badSchema && containsReason(validateStory(*badSchema.story, tutorialRoot), "unsupported schema version"),
                  "unsupported schema version must fail explicitly");

    const StoryParseResult unknownScene = parseStoryText(R"YAML(
schema: 1
id: unknown-scene
title: "Unknown scene"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - montage:
      duration_ms: 1000
)YAML");
    ok &= require(!unknownScene && containsReason(unknownScene.issues, "unknown scene type"),
                  "unknown scene type must fail parsing");

    const StoryParseResult unknownAction = parseStoryText(R"YAML(
schema: 1
id: unknown-action
title: "Unknown action"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - enable_sync: {}
)YAML");
    ok &= require(!unknownAction && containsReason(unknownAction.issues, "unknown action type"),
                  "unknown action type must fail parsing");

    const StoryParseResult badButton = parseStoryText(R"YAML(
schema: 1
id: bad-button
title: "Bad button"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - button:
            name: SYNC
)YAML");
    ok &= require(!badButton && containsReason(badButton.issues, "unknown CLOCK button"),
                  "nonexistent CLOCK button must fail parsing");

    const StoryParseResult unknownField = parseStoryText(R"YAML(
schema: 1
id: unknown-field
title: "Unknown field"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
magic: true
scenes:
  - chapter:
      number: 1
      title: "Nope"
      duration_ms: 1000
)YAML");
    ok &= require(!unknownField && containsReason(unknownField.issues, "unknown field 'magic'"),
                  "unknown top-level fields must fail parsing");
    ok &= require(!unknownField.issues.empty() && unknownField.issues.front().storyId == "unknown-field",
                  "structural errors must retain story ID context when available");

    const StoryParseResult badProfile = parseStoryText(R"YAML(
schema: 1
id: undefined-profile
title: "Undefined profile"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_FAST
scenes:
  - chapter:
      number: 1
      title: "Profile"
      duration_ms: 1000
)YAML");
    ok &= require(badProfile && validateStory(*badProfile.story, tutorialRoot).empty(),
                  "HUMAN_FAST must resolve from docs/tutorials interaction-profile resources");

    const StoryParseResult badTheme = parseStoryText(R"YAML(
schema: 1
id: missing-theme
title: "Missing theme"
language: en
theme: no-such-theme
interaction_profile: HUMAN_NORMAL
scenes:
  - chapter:
      number: 1
      title: "Theme"
      duration_ms: 1000
)YAML");
    ok &= require(badTheme && containsReason(validateStory(*badTheme.story, tutorialRoot), "theme not found"),
                  "theme lookup must use docs/tutorials/themes");

    const StoryParseResult validSync = parseStoryText(R"YAML(
schema: 1
id: valid-sync-semantics
title: "Valid sync semantics"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - sync_source:
            bpm: 123
            ppqn: 4
            waveform: square
        - sync_cable:
            state: connect
        - sync_generator:
            state: hold
        - sync_generator:
            state: run
        - wait_until:
            external_sync: locked
            timeout_ms: 4000
        - assert:
            clock_source: auto
)YAML");
    ok &= require(validSync && validateStory(*validSync.story, tutorialRoot).empty(),
                  "valid cable/source/generator separation must validate");

    const StoryParseResult rstWithoutCable = parseStoryText(R"YAML(
schema: 1
id: invalid-rst-pulse
title: "Invalid RST"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - rst_pulse: {}
)YAML");
    ok &= require(rstWithoutCable && containsReason(validateStory(*rstWithoutCable.story, tutorialRoot), "connected RST cable"),
                  "RST pulse without patch must fail semantic validation");

    if (minimal && power) {
        Story duplicate = *power.story;
        duplicate.id = minimal.story->id;
        ok &= require(containsReason(validateUniqueStoryIds({*minimal.story, duplicate}), "duplicate story id"),
                      "duplicate story IDs must fail catalog validation");
    }



    const StoryParseResult heldTap = parseStoryText(R"YAML(
schema: 1
id: held-tap
title: "Held TAP"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - button:
            name: TAP
            state: down
        - encoder_push: {}
        - button:
            name: TAP
            state: up
)YAML");
    ok &= require(heldTap && validateStory(*heldTap.story, tutorialRoot).empty(),
                  "explicit button down/up must validate for real modifier gestures");
    ok &= require(heldTap && heldTap.story->scenes[0].actions[0].buttonState == true &&
                      heldTap.story->scenes[0].actions[2].buttonState == false,
                  "button down/up states must decode explicitly");


    const StoryParseResult longPush = parseStoryText(R"YAML(
schema: 1
id: long-encoder-push
title: "Long push"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - encoder_push:
            hold_ms: 800
)YAML");
    ok &= require(longPush && longPush.story->scenes[0].actions[0].holdMs == 800U &&
                      validateStory(*longPush.story, tutorialRoot).empty(),
                  "encoder_push.hold_ms must preserve a real long-push duration");

    const StoryParseResult focusAction = parseStoryText(R"YAML(
schema: 1
id: focus-action
title: "Focus action"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - focus:
            target: oled_region
            x: 0
            y: 0
            width: 64
            height: 12
            label: "Top bar"
            duration_ms: 2400
)YAML");
    ok &= require(focusAction && focusAction.story->scenes[0].actions[0].focusTarget == FocusTarget::OledRegion &&
                      focusAction.story->scenes[0].actions[0].focusWidth == 64 &&
                      focusAction.story->scenes[0].actions[0].durationMs == 2400U &&
                      validateStory(*focusAction.story, tutorialRoot).empty(),
                  "presentation-only focus action must decode and validate OLED regions");

    const StoryParseResult orphanRelease = parseStoryText(R"YAML(
schema: 1
id: orphan-button-release
title: "Orphan release"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - button:
            name: TAP
            state: up
)YAML");
    ok &= require(orphanRelease && containsReason(validateStory(*orphanRelease.story, tutorialRoot), "held down"),
                  "button release without matching hold must fail semantic validation");

    const StoryParseResult badButtonState = parseStoryText(R"YAML(
schema: 1
id: bad-button-state
title: "Bad button state"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - tutorial:
      actions:
        - button:
            name: TAP
            state: toggle
)YAML");
    ok &= require(!badButtonState && containsReason(badButtonState.issues, "button.state"),
                  "unknown explicit button state must fail parsing");

    const StoryParseResult blockText = parseStoryText(R"YAML(
schema: 1
id: block-text
title: "Block text"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
scenes:
  - text:
      title: "Why AUTO"
      body: |
        AUTO keeps the internal clock available.
        External lock does not rewrite SOURCE.
      duration_ms: 2500
)YAML");
    ok &= require(blockText && blockText.story->scenes[0].body ==
                      "AUTO keeps the internal clock available.\nExternal lock does not rewrite SOURCE.",
                  "literal block text must decode without clipping or indentation artifacts");

    const StoryParseResult unsupportedYaml = parseStoryText(R"YAML(
schema: 1
id: unsupported-yaml
title: "Unsupported YAML"
language: en
theme: south-signal-lab-default
interaction_profile: HUMAN_NORMAL
meta: [one, two]
scenes: []
)YAML");
    ok &= require(!unsupportedYaml && containsReason(unsupportedYaml.issues, "flow collections"),
                  "unsupported YAML features must fail rather than silently degrade");

    if (!ok) return EXIT_FAILURE;
    std::cout << "CLOCK Storybook schema-1 parser/validator: PASS\n";
    return EXIT_SUCCESS;
}
