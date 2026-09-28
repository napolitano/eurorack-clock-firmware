/**
 * @file storybook_renderer_tests.cpp
 * @brief Headless regression tests for Phase-1 Storybook theme, text, scene, OLED, panel and scope rendering.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>
#include <string>

#include "panel_layout.h"
#include "simulator_runtime.h"
#include "tutorial/panel_presentation.h"
#include "tutorial/story_background_image.h"
#include "tutorial/story_parser.h"
#include "tutorial/story_runner.h"
#include "tutorial/story_simulator_port.h"
#include "tutorial/story_text_renderer.h"
#include "tutorial/story_theme.h"
#include "tutorial/tutorial_renderer.h"

namespace {

using clockfw::sim::SimulatorRuntime;
using clockfw::sim::layout::loadPanelLayout;
using clockfw::sim::tutorial::CalloutKind;
using clockfw::sim::tutorial::FocusTarget;
using clockfw::sim::tutorial::PhysicalPresentationState;
using clockfw::sim::tutorial::PanelPresentationTimeline;
using clockfw::sim::tutorial::SceneKind;
using clockfw::sim::tutorial::ScopeMode;
using clockfw::sim::tutorial::Story;
using clockfw::sim::tutorial::StoryFontBackend;
using clockfw::sim::tutorial::StoryFontStyle;
using clockfw::sim::tutorial::loadStoryBackgroundImage;
using clockfw::sim::tutorial::StoryParseResult;
using clockfw::sim::tutorial::StoryRunner;
using clockfw::sim::tutorial::StoryScene;
using clockfw::sim::tutorial::StorySimulatorPort;
using clockfw::sim::tutorial::TutorialColor;
using clockfw::sim::tutorial::TutorialRenderer;
using clockfw::sim::tutorial::TutorialSurface;
using clockfw::sim::tutorial::layoutStoryText;
using clockfw::sim::tutorial::loadStoryTheme;
using clockfw::sim::tutorial::parseStoryText;
using clockfw::sim::tutorial::resolveTutorialComposition;

bool require(const bool condition, const char* const message) {
    if (!condition) {
        std::cerr << "Storybook renderer failure: " << message << '\n';
        return false;
    }
    return true;
}

std::size_t countDifferent(const TutorialSurface& surface, const TutorialColor background) {
    std::size_t count = 0U;
    for (const TutorialColor pixel : surface.pixels()) {
        if (!(pixel == background)) ++count;
    }
    return count;
}

template <typename Callable>
bool throwsContaining(const Callable& callable, const std::string& needle) {
    try {
        callable();
    } catch (const std::exception& exception) {
        return std::string(exception.what()).find(needle) != std::string::npos;
    }
    return false;
}


void writeSolidBmp(const std::filesystem::path& path) {
    const unsigned char bytes[] = {
        'B','M', 70,0,0,0, 0,0,0,0, 54,0,0,0,
        40,0,0,0, 2,0,0,0, 2,0,0,0, 1,0, 24,0, 0,0,0,0, 16,0,0,0,
        0,0,0,0, 0,0,0,0, 0,0,0,0, 0,0,0,0,
        0,0,255, 0,0,255, 0,0,
        0,0,255, 0,0,255, 0,0
    };
    std::ofstream output(path, std::ios::binary);
    output.write(reinterpret_cast<const char*>(bytes), static_cast<std::streamsize>(sizeof(bytes)));
}

Story baseStory() {
    Story story{};
    story.schema = 1U;
    story.id = "renderer-contract";
    story.title = "Renderer contract";
    story.language = "en";
    story.theme = "south-signal-lab-ci";
    return story;
}

}  // namespace

int main() {
    const std::filesystem::path sourceRoot = CLOCK_SOURCE_ROOT;
    const std::filesystem::path tutorialRoot = sourceRoot / "docs" / "tutorials";
    const auto panelLayout = loadPanelLayout(sourceRoot / "sim" / "panel_layout.ini");
    const auto publicationTheme = loadStoryTheme(tutorialRoot, "south-signal-lab-default");
    const auto theme = loadStoryTheme(tutorialRoot, "south-signal-lab-ci");
    bool ok = true;

    ok &= require(publicationTheme.body.family == "Ubuntu" && publicationTheme.body.backend == StoryFontBackend::System &&
                  publicationTheme.monospace.family == "Ubuntu Mono",
                  "publication theme must use configurable Ubuntu-family system fonts by default");
    ok &= require(publicationTheme.chapterBackground == TutorialColor{11U, 79U, 192U, 255U},
                  "publication chapter background must use the CLOCK manual blue #0B4FC0");
    ok &= require(publicationTheme.tutorialBackground == TutorialColor{0U, 0U, 0U, 255U},
                  "publication tutorial background must default to black");
    ok &= require(theme.body.family == "CLOCK UI" && theme.monospace.family == "CLOCK Mono",
                  "CI theme must retain deterministic project-local font faces");
    ok &= require(theme.body.sizePx == 28U && theme.chapter.sizePx == 56U,
                  "theme font sizes must remain data-driven");

    const StoryFontStyle bodyFont{"CLOCK UI", 28U};
    const auto wrapped = layoutStoryText(
        "This text must wrap deterministically without clipping.", bodyFont, 420, 200, 12U);
    ok &= require(wrapped.lines.size() > 1U && wrapped.widthPx <= 420 && wrapped.heightPx <= 200,
                  "text must be measured and wrapped before rendering");
    ok &= require(throwsContaining([&]() {
        (void)layoutStoryText(std::string(80U, 'W'), bodyFont, 180, 200, 12U);
    }, "unbreakable word"), "unbreakable horizontal overflow must fail explicitly");
    ok &= require(throwsContaining([&]() {
        (void)layoutStoryText("ONE TWO THREE FOUR FIVE SIX SEVEN EIGHT NINE TEN", bodyFont, 180, 40, 12U);
    }, "text overflow"), "vertical text overflow must fail explicitly");

    const std::filesystem::path badRoot = ".clock-storybook-bad-theme";
    std::filesystem::remove_all(badRoot);
    std::filesystem::create_directories(badRoot / "themes");
    {
        std::ofstream out(badRoot / "themes" / "bad.yaml");
        out << R"YAML(schema: 1
id: bad
line_spacing_px: 12
fonts:
  body:
    family: "Missing Font"
    size_px: 28
  heading:
    family: "CLOCK UI"
    size_px: 42
  chapter:
    family: "CLOCK UI"
    size_px: 56
  subtitle:
    family: "CLOCK UI"
    size_px: 28
  monospace:
    family: "CLOCK Mono"
    size_px: 21
colours:
  background: "#FFFFFF"
  foreground: "#111111"
  muted: "#666666"
  accent: "#0B4FC0"
  tip_background: "#EAF0FF"
  tip_foreground: "#12213A"
  warning_background: "#FFF2CC"
  warning_foreground: "#2A2416"
  recipe_background: "#E8F5EE"
  recipe_foreground: "#123328"
)YAML";
    }
    ok &= require(throwsContaining([&]() { (void)loadStoryTheme(badRoot, "bad"); }, "not available"),
                  "missing requested font must fail without silent substitution");
    std::filesystem::remove_all(badRoot);

    const std::filesystem::path bmpPath = ".clock-storybook-background-test.bmp";
    writeSolidBmp(bmpPath);
    const TutorialSurface background = loadStoryBackgroundImage(bmpPath);
    ok &= require(background.width() == 2U && background.height() == 2U &&
                  background.pixel(0U, 0U) == TutorialColor{255U, 0U, 0U, 255U},
                  "configurable tutorial background loader must decode uncompressed RGB BMP data");
    std::filesystem::remove(bmpPath);

    const std::filesystem::path statePath = ".clock-storybook-renderer-state.bin";
    std::filesystem::remove(statePath);
    SimulatorRuntime runtime(statePath);
    runtime.begin();
    runtime.advanceMicroseconds(1200000ULL);
    TutorialRenderer renderer(tutorialRoot, panelLayout);
    Story story = baseStory();

    StoryScene chapter{};
    chapter.kind = SceneKind::Chapter;
    chapter.number = 3U;
    chapter.title = "External Sync";
    chapter.subtitle = "Following another clock source";
    const TutorialSurface chapterFrame = renderer.renderScene(story, chapter, runtime, {});
    ok &= require(chapterFrame.width() == 1920U && chapterFrame.height() == 1080U,
                  "renderer must use the story output profile");
    ok &= require(countDifferent(chapterFrame, theme.background) > 1000U,
                  "chapter scene must visibly render number/title/subtitle");

    StoryScene text{};
    text.kind = SceneKind::Text;
    text.title = "What AUTO means";
    text.body = "AUTO uses the internal clock until a valid external clock has been detected and locked.\nManual STOP still takes precedence.";
    const TutorialSurface textFrame = renderer.renderScene(story, text, runtime, {});
    ok &= require(countDifferent(textFrame, theme.background) > 1000U,
                  "full-frame text scene must render measured title/body text");

    StoryScene tip{};
    tip.kind = SceneKind::Callout;
    tip.calloutKind = CalloutKind::Tip;
    tip.title = "Two edges establish lock";
    tip.body = "The first accepted edge starts acquisition. The second establishes the first measurable period.";
    const TutorialSurface tipFrame = renderer.renderScene(story, tip, runtime, {});
    ok &= require(tipFrame.pixel(200U, 160U) == theme.tipBackground,
                  "TIP must use theme-provided styling");

    StoryScene warning = tip;
    warning.calloutKind = CalloutKind::Warning;
    warning.title = "Manual STOP still wins";
    const TutorialSurface warningFrame = renderer.renderScene(story, warning, runtime, {});
    ok &= require(warningFrame.pixel(200U, 160U) == theme.warningBackground,
                  "WARNING must use theme-provided styling");

    StoryScene recipe{};
    recipe.kind = SceneKind::Callout;
    recipe.calloutKind = CalloutKind::Recipe;
    recipe.title = "External Sync recipe";
    recipe.recipeSteps = {"Select AUTO or EXTERNAL on CLOCK.", "Connect the external clock to SYNC.", "Wait for lock.", "Use STOP for manual transport control."};
    const TutorialSurface recipeFrame = renderer.renderScene(story, recipe, runtime, {});
    ok &= require(recipeFrame.pixel(200U, 160U) == theme.recipeBackground,
                  "RECIPE must use theme-provided styling");

    StoryScene overflow = text;
    overflow.body = std::string(300U, 'W');
    ok &= require(throwsContaining([&]() { (void)renderer.renderScene(story, overflow, runtime, {}); }, "overflow"),
                  "renderer must reject scene overflow instead of clipping");

    const auto readabilityLayout = resolveTutorialComposition(
        story.output, false, theme.subtitleHeightPx, theme.subtitleSafeBottomPx);
    ok &= require(readabilityLayout.oledRaster.width >= 1152 && readabilityLayout.oledRaster.height >= 576,
                  "default 1080p tutorial composition must keep the primary OLED readable at 9x scale");
    ok &= require(readabilityLayout.subtitleStrip.y + readabilityLayout.subtitleStrip.height <=
                      static_cast<int>(story.output.height) - static_cast<int>(theme.subtitleSafeBottomPx),
                  "burned-in subtitles must remain above the configured player-control safe area");
    ok &= require(readabilityLayout.detailBox.width > readabilityLayout.panelBox.width / 2 &&
                      readabilityLayout.detailBox.height > readabilityLayout.panelBox.height,
                  "tutorial composition must reserve a readable control-detail area above the locator panel");

    const StoryParseResult tutorialParsed = parseStoryText(R"YAML(
schema: 1
id: renderer-live-contract
title: "Renderer live contract"
language: en
theme: south-signal-lab-ci
interaction_profile: HUMAN_FAST
setup:
  factory_reset: true
  power: on
scenes:
  - tutorial:
      subtitle: "Watch the real CLOCK state."
      actions:
        - wait_ms: 1200
        - button:
            name: PLAY
        - scope:
            state: show
            channel: visible
        - wait_ms: 1200
)YAML");
    ok &= require(static_cast<bool>(tutorialParsed), "live tutorial renderer story must parse");
    if (tutorialParsed) {
        StorySimulatorPort port(runtime);
        PanelPresentationTimeline timeline;
        StoryRunner runner(port, tutorialRoot, &timeline);
        const auto run = runner.run(*tutorialParsed.story);
        ok &= require(static_cast<bool>(run), "live tutorial renderer story must execute");
        const PhysicalPresentationState physical = timeline.stateAt(run.presentationDurationUs);
        const StoryScene& tutorialScene = tutorialParsed.story->scenes.front();
        const TutorialSurface tutorialFrame = renderer.renderScene(
            *tutorialParsed.story, tutorialScene, runtime, physical, "Watch the real CLOCK state.");
        const auto composition = resolveTutorialComposition(tutorialParsed.story->output, true);
        const int oledScale = composition.oledRaster.width / 128;
        ok &= require(oledScale >= 1 && composition.oledRaster.height == 64 * oledScale,
                      "OLED composition must use one exact integer scale");

        bool checkedOn = false;
        bool checkedOff = false;
        const auto& framebuffer = runtime.framebuffer();
        for (int y = 0; y < 64 && (!checkedOn || !checkedOff); ++y) {
            for (int x = 0; x < 128 && (!checkedOn || !checkedOff); ++x) {
                const std::size_t byteIndex = static_cast<std::size_t>(x) + static_cast<std::size_t>(y / 8) * 128U;
                const bool on = (framebuffer[byteIndex] & static_cast<std::uint8_t>(1U << (y & 7))) != 0U;
                const TutorialColor pixel = tutorialFrame.pixel(
                    static_cast<std::size_t>(composition.oledRaster.x + x * oledScale + oledScale / 2),
                    static_cast<std::size_t>(composition.oledRaster.y + y * oledScale + oledScale / 2));
                if (on && !checkedOn) {
                    ok &= require(pixel == TutorialColor{242U, 244U, 247U, 255U},
                                  "OLED ON pixel must come directly from the production framebuffer");
                    checkedOn = true;
                }
                if (!on && !checkedOff) {
                    ok &= require(pixel == TutorialColor{0U, 0U, 0U, 255U},
                                  "OLED OFF pixel must remain black at the same integer scale");
                    checkedOff = true;
                }
            }
        }
        ok &= require(checkedOn && checkedOff, "test framebuffer must contain both OLED ON and OFF pixels");

        std::size_t scopeSignalPixels = 0U;
        for (int y = composition.scopeBox.y; y < composition.scopeBox.y + composition.scopeBox.height; ++y) {
            for (int x = composition.scopeBox.x; x < composition.scopeBox.x + composition.scopeBox.width; ++x) {
                const TutorialColor pixel = tutorialFrame.pixel(static_cast<std::size_t>(x), static_cast<std::size_t>(y));
                if (pixel.green > 180U && pixel.red < 190U && pixel.blue < 190U) ++scopeSignalPixels;
            }
        }
        ok &= require(scopeSignalPixels > 10U,
                      "visible-channel scope must draw real selected-channel gate telemetry");

        ok &= require(tutorialFrame.pixel(0U, static_cast<std::size_t>(composition.subtitleStrip.y)) == theme.accent,
                      "subtitle strip must be dedicated below tutorial content");

        PhysicalPresentationState focusState = physical;
        focusState.explicitFocus = FocusTarget::OledRegion;
        focusState.focusX = 0;
        focusState.focusY = 0;
        focusState.focusWidth = 128;
        focusState.focusHeight = 12;
        focusState.focusLabel = "Top bar";
        const TutorialSurface focusedFrame = renderer.renderScene(
            *tutorialParsed.story, tutorialScene, runtime, focusState, "Watch the real CLOCK state.");
        std::size_t changedPixels = 0U;
        for (std::size_t index = 0U; index < focusedFrame.pixels().size(); ++index) {
            if (!(focusedFrame.pixels()[index] == tutorialFrame.pixels()[index])) ++changedPixels;
        }
        ok &= require(changedPixels > 1000U,
                      "manual OLED focus must render a clearly visible explanatory arrow/highlight overlay");
    }

    runtime.flushPersistence();
    std::filesystem::remove(statePath);
    return ok ? EXIT_SUCCESS : EXIT_FAILURE;
}
