/**
 * @file tutorial_renderer.cpp
 * @brief Implements deterministic full-frame Storybook presentation without desktop capture or UI reconstruction.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/tutorial_renderer.h"

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <cstdint>
#include <stdexcept>
#include <string>

#include "scope_timeline.h"
#include "tutorial/panel_dynamic_layer.h"
#include "tutorial/story_text_renderer.h"
#include "tutorial/story_theme.h"

namespace clockfw::sim::tutorial {
namespace {

constexpr int kFrameMargin = 56;
constexpr int kSubtitleHeight = 120;
constexpr int kTutorialGap = 56;
constexpr int kScopeHeight = 250;
constexpr int kPanelInternalTextPx = 14;

int rounded(const float value) {
    return static_cast<int>(std::lround(value));
}

TutorialColor colour(const layout::Color value, const std::uint8_t alpha = 255U) {
    return {value.red, value.green, value.blue, alpha};
}

TutorialSurface dynamicLayerToSurface(const PanelDynamicLayer& layer) {
    TutorialSurface surface(layer.width(), layer.height(), {0U, 0U, 0U, 0U});
    for (std::size_t y = 0U; y < layer.height(); ++y) {
        for (std::size_t x = 0U; x < layer.width(); ++x) {
            const RgbaPixel pixel = layer.pixel(x, y);
            surface.setPixel(
                static_cast<int>(x), static_cast<int>(y),
                {pixel.red, pixel.green, pixel.blue, pixel.alpha});
        }
    }
    return surface;
}

void drawJack(
    TutorialSurface& surface,
    const layout::Point center,
    const layout::JackGeometry& jack) {
    surface.fillCircle(rounded(center.x), rounded(center.y), rounded(jack.nutRadius), {148U, 150U, 154U, 255U});
    surface.fillCircle(rounded(center.x), rounded(center.y), rounded(jack.bushingRadius), {45U, 47U, 51U, 255U});
    surface.fillCircle(rounded(center.x), rounded(center.y), rounded(jack.openingRadius), {5U, 6U, 8U, 255U});
}

TutorialSurface renderPanelBase(const layout::PanelLayout& layout, const SimulatorRuntime& runtime) {
    TutorialSurface surface(
        static_cast<std::size_t>(layout.windowWidth),
        static_cast<std::size_t>(layout.windowHeight),
        {0U, 0U, 0U, 0U});
    const TutorialRect panel{
        rounded(layout.panel.x), rounded(layout.panel.y),
        rounded(layout.panel.width), rounded(layout.panel.height)};
    surface.fillRect(panel, {24U, 27U, 34U, 255U});
    surface.strokeRect(panel, {61U, 66U, 78U, 255U}, 2);

    const int oledScale = layout.displayPixelScale;
    const int oledWidth = 128 * oledScale;
    const int oledHeight = 64 * oledScale;
    const int oledX = rounded(layout.display.x + (layout.display.width - static_cast<float>(oledWidth)) * 0.5F);
    const int oledY = rounded(layout.display.y + (layout.display.height - static_cast<float>(oledHeight)) * 0.5F);
    surface.fillRect({oledX - 3, oledY - 3, oledWidth + 6, oledHeight + 6}, {5U, 6U, 8U, 255U});
    surface.strokeRect({oledX - 2, oledY - 2, oledWidth + 4, oledHeight + 4}, {82U, 85U, 92U, 255U}, 1);
    const auto& panelFramebuffer = runtime.framebuffer();
    for (int y = 0; y < 64; ++y) {
        for (int x = 0; x < 128; ++x) {
            const std::size_t byteIndex = static_cast<std::size_t>(x) + static_cast<std::size_t>(y / 8) * 128U;
            const bool on = (panelFramebuffer[byteIndex] & static_cast<std::uint8_t>(1U << (y & 7))) != 0U;
            if (on) {
                surface.fillRect({oledX + x * oledScale, oledY + y * oledScale, oledScale, oledScale},
                                 {236U, 239U, 242U, 255U});
            }
        }
    }

    surface.fillCircle(rounded(layout.encoderCenter.x), rounded(layout.encoderCenter.y), rounded(layout.encoderRadius), {55U, 58U, 65U, 255U});
    surface.strokeRect({rounded(layout.encoderCenter.x - layout.encoderRadius), rounded(layout.encoderCenter.y - layout.encoderRadius),
                        rounded(layout.encoderRadius * 2.0F), rounded(layout.encoderRadius * 2.0F)},
                       {84U, 88U, 97U, 80U}, 1);

    const auto drawButton = [&](const layout::CircleControl& button) {
        surface.fillCircle(rounded(button.center.x), rounded(button.center.y), rounded(button.bodyRadius), {12U, 13U, 16U, 255U});
        surface.fillCircle(rounded(button.center.x), rounded(button.center.y), rounded(button.radius), colour(button.color));
    };
    drawButton(layout.playButton);
    drawButton(layout.tapButton);
    drawButton(layout.stopButton);

    drawJack(surface, layout.syncInputCenter, layout.jackGeometry(layout.syncJackType));
    drawJack(surface, layout.resetInputCenter, layout.jackGeometry(layout.resetJackType));
    for (std::size_t index = 0U; index < layout.outputCenters.size(); ++index) {
        drawJack(surface, layout.outputCenters[index], layout.jackGeometry(layout.outputJackTypes[index]));
        surface.fillCircle(rounded(layout.ledCenters[index].x), rounded(layout.ledCenters[index].y),
                           rounded(layout.ledRadius), colour(layout.ledOffColor));
    }

    const StoryFontStyle labelFont{"CLOCK UI", static_cast<std::uint32_t>(kPanelInternalTextPx)};
    const TutorialColor label{218U, 220U, 224U, 255U};
    (void)drawStoryTextLine(surface, "CLOCK", labelFont, panel.x + 18, panel.y + 36, label);
    (void)drawStoryTextLine(surface, "PLAY", labelFont, rounded(layout.playButton.center.x) - 18,
                            rounded(layout.playButton.center.y + layout.playButton.bodyRadius + 10.0F), label);
    (void)drawStoryTextLine(surface, "TAP", labelFont, rounded(layout.tapButton.center.x) - 14,
                            rounded(layout.tapButton.center.y + layout.tapButton.bodyRadius + 10.0F), label);
    (void)drawStoryTextLine(surface, "STOP", labelFont, rounded(layout.stopButton.center.x) - 18,
                            rounded(layout.stopButton.center.y + layout.stopButton.bodyRadius + 10.0F), label);
    (void)drawStoryTextLine(surface, "SYNC", labelFont, rounded(layout.syncInputCenter.x) - 18,
                            rounded(layout.syncInputCenter.y + layout.jackGeometry(layout.syncJackType).nutRadius + 10.0F), label);
    (void)drawStoryTextLine(surface, "RST", labelFont, rounded(layout.resetInputCenter.x) - 13,
                            rounded(layout.resetInputCenter.y + layout.jackGeometry(layout.resetJackType).nutRadius + 10.0F), label);
    for (std::size_t index = 0U; index < layout.outputCenters.size(); ++index) {
        const std::string out = "OUT" + std::to_string(index + 1U);
        (void)drawStoryTextLine(surface, out, labelFont,
                                rounded(layout.outputCenters[index].x) - 20,
                                rounded(layout.outputCenters[index].y + layout.jackGeometry(layout.outputJackTypes[index]).nutRadius + 8.0F),
                                label);
    }
    return surface;
}

void renderOled(
    TutorialSurface& frame,
    const SimulatorRuntime& runtime,
    const TutorialRect box,
    const StoryTheme& theme) {
    constexpr int oledWidth = 128;
    constexpr int oledHeight = 64;
    const int scale = std::min(box.width / oledWidth, box.height / oledHeight);
    if (scale < 1) throw std::runtime_error("OLED area cannot fit integer nearest-neighbour scale");
    const int width = oledWidth * scale;
    const int height = oledHeight * scale;
    const int x0 = box.x + (box.width - width) / 2;
    const int y0 = box.y + (box.height - height) / 2;
    frame.fillRect({x0 - 12, y0 - 12, width + 24, height + 24}, {5U, 6U, 8U, 255U});
    frame.strokeRect({x0 - 12, y0 - 12, width + 24, height + 24}, theme.muted, 2);
    frame.fillRect({x0, y0, width, height}, {0U, 0U, 0U, 255U});
    const auto& framebuffer = runtime.framebuffer();
    for (int y = 0; y < oledHeight; ++y) {
        for (int x = 0; x < oledWidth; ++x) {
            const std::size_t byteIndex = static_cast<std::size_t>(x) + static_cast<std::size_t>(y / 8) * oledWidth;
            const bool on = (framebuffer[byteIndex] & static_cast<std::uint8_t>(1U << (y & 7))) != 0U;
            if (on) frame.fillRect({x0 + x * scale, y0 + y * scale, scale, scale}, {242U, 244U, 247U, 255U});
        }
    }
}

void renderPanel(
    TutorialSurface& frame,
    const layout::PanelLayout& layout,
    const SimulatorRuntime& runtime,
    const PhysicalPresentationState& physicalState,
    const double speedMultiplier,
    const TutorialRect box) {
    TutorialSurface panel = renderPanelBase(layout, runtime);
    const PanelPresentationSnapshot snapshot = makePanelPresentationSnapshot(layout, runtime, physicalState, speedMultiplier);
    const TutorialSurface dynamic = dynamicLayerToSurface(renderPanelDynamicLayer(layout, snapshot));
    panel.composite(dynamic, 0, 0);

    const TutorialRect source{
        rounded(layout.panel.x), rounded(layout.panel.y),
        rounded(layout.panel.width), rounded(layout.panel.height)};
    const double scale = std::min(
        static_cast<double>(box.width) / static_cast<double>(source.width),
        static_cast<double>(box.height) / static_cast<double>(source.height));
    if (!(scale > 0.0)) throw std::runtime_error("invalid panel destination box");
    const int width = std::max(1, static_cast<int>(std::floor(static_cast<double>(source.width) * scale)));
    const int height = std::max(1, static_cast<int>(std::floor(static_cast<double>(source.height) * scale)));
    const TutorialRect destination{box.x + (box.width - width) / 2, box.y + (box.height - height) / 2, width, height};
    frame.blitNearest(panel, source, destination);
}

void renderScope(
    TutorialSurface& frame,
    SimulatorRuntime& runtime,
    scope::Session& session,
    const TutorialRect box,
    const StoryTheme& theme) {
    if (box.width <= 160 || box.height <= 100) throw std::runtime_error("scope area is too small");
    session.update(runtime);
    scope::SessionView view = session.view();
    if (view.started && view.referenceUs == 0ULL && runtime.nowMicroseconds() > view.epochSimulatorUs) {
        session.update(runtime);
        view = session.view();
    }
    frame.fillRect(box, {16U, 18U, 22U, 255U});
    frame.strokeRect(box, theme.muted, 2);

    const std::uint8_t selected = std::min<std::uint8_t>(runtime.selectedChannelForPresentation(), 7U);
    const ChannelTelemetry& channel = runtime.telemetry()[selected];
    const StoryFontStyle labelFont{"CLOCK Mono", 14U};
    const std::string label = "CH" + std::to_string(static_cast<unsigned>(selected + 1U)) + "  VISIBLE CHANNEL";
    (void)drawStoryTextLine(frame, label, labelFont, box.x + 18, box.y + 16, theme.muted);

    const int graphLeft = box.x + 24;
    const int graphRight = box.x + box.width - 24;
    const int graphTop = box.y + 52;
    const int graphBottom = box.y + box.height - 24;
    const int highY = graphTop + 20;
    const int lowY = graphBottom - 18;
    frame.drawLine(graphLeft, lowY, graphRight, lowY, {64U, 68U, 76U, 255U});

    if (!view.started) {
        (void)drawStoryTextLine(frame, "ARMED", labelFont, graphRight - 72, box.y + 16, theme.muted);
        return;
    }

    const std::uint64_t windowUs = std::clamp<std::uint64_t>(
        view.windowUs, scope::kWindowOptionsUs.front(), scope::kWindowOptionsUs.back());
    const ClockState& state = runtime.state();
    const SyncInputTelemetry sync = runtime.syncInputTelemetry();
    const std::uint32_t referenceBpm = scope::effectiveReferenceBpmMilli(state, sync.locked, sync.engineBpmMilli);
    const scope::MusicalGridSpec grid = scope::musicalGridSpec(state, referenceBpm, windowUs);
    const double referenceUs = static_cast<double>(view.referenceUs);
    const double startUs = referenceUs - static_cast<double>(windowUs);
    const scope::GridAnchor anchor = scope::phaseLockedGridAnchor(state, runtime.engineSnapshot(), referenceUs, grid);
    const double serialStepUs = grid.intervalUs * static_cast<double>(grid.minorEvery);
    if (serialStepUs > 0.0) {
        double gridTime = anchor.timeUs;
        while (gridTime > startUs) gridTime -= serialStepUs;
        while (gridTime < startUs) gridTime += serialStepUs;
        for (double t = gridTime; t <= referenceUs; t += serialStepUs) {
            const double normalized = scope::normalizedPosition(t, startUs, windowUs);
            const int x = graphLeft + static_cast<int>(std::lround(normalized * static_cast<double>(graphRight - graphLeft)));
            frame.drawLine(x, graphTop, x, graphBottom, {48U, 52U, 59U, 255U});
        }
    }

    const std::int64_t gateReferenceUs = static_cast<std::int64_t>(view.referenceUs);
    const std::int64_t gateStartUs = gateReferenceUs - static_cast<std::int64_t>(windowUs);
    bool high = false;
    int previousX = graphLeft;
    for (const GateTransition& transition : channel.transitions) {
        if (transition.timestampUs < view.epochSimulatorUs) continue;
        const std::int64_t transitionUs = static_cast<std::int64_t>(transition.timestampUs - view.epochSimulatorUs);
        if (transitionUs < gateStartUs) {
            high = transition.high;
            continue;
        }
        if (transitionUs > gateReferenceUs) break;
        const double normalized = scope::normalizedPosition(
            static_cast<double>(transitionUs), static_cast<double>(gateStartUs), windowUs);
        const int x = graphLeft + static_cast<int>(std::lround(normalized * static_cast<double>(graphRight - graphLeft)));
        frame.drawLine(previousX, high ? highY : lowY, x, high ? highY : lowY, {150U, 220U, 168U, 255U}, 2);
        frame.drawLine(x, high ? highY : lowY, x, transition.high ? highY : lowY, {150U, 220U, 168U, 255U}, 2);
        previousX = x;
        high = transition.high;
    }
    frame.drawLine(previousX, high ? highY : lowY, graphRight, high ? highY : lowY, {150U, 220U, 168U, 255U}, 2);
    frame.drawLine(graphRight, graphTop, graphRight, graphBottom, theme.accent, 2);
}

void renderSubtitle(
    TutorialSurface& frame,
    const std::string& subtitle,
    const StoryTheme& theme) {
    const int width = static_cast<int>(frame.width());
    const int height = static_cast<int>(frame.height());
    const TutorialRect strip{0, height - kSubtitleHeight, width, kSubtitleHeight};
    frame.fillRect(strip, theme.background);
    frame.fillRect({0, strip.y, width, 3}, theme.accent);
    if (subtitle.empty()) return;
    const StoryTextLayout layout = layoutStoryText(
        subtitle, theme.subtitle, width - 2 * kFrameMargin, kSubtitleHeight - 28, theme.lineSpacingPx);
    const int x = (width - layout.widthPx) / 2;
    const int y = strip.y + (strip.height - layout.heightPx) / 2;
    drawStoryText(frame, layout, theme.subtitle, x, y, theme.foreground);
}

void renderChapter(TutorialSurface& frame, const StoryScene& scene, const StoryTheme& theme) {
    const int width = static_cast<int>(frame.width());
    const int contentBottom = static_cast<int>(frame.height()) - kSubtitleHeight;
    const StoryFontStyle numberFont{"CLOCK Mono", 42U};
    const std::string number = std::to_string(scene.number);
    const int numberWidth = measureStoryTextWidth(number, numberFont);
    (void)drawStoryTextLine(frame, number, numberFont, (width - numberWidth) / 2, 250, theme.accent);
    const StoryTextLayout title = layoutStoryText(scene.title, theme.chapter, width - 320, 180, theme.lineSpacingPx);
    drawStoryText(frame, title, theme.chapter, (width - title.widthPx) / 2, 360, theme.foreground);
    if (!scene.subtitle.empty()) {
        const StoryTextLayout subtitle = layoutStoryText(scene.subtitle, theme.heading, width - 360, 160, theme.lineSpacingPx);
        drawStoryText(frame, subtitle, theme.heading, (width - subtitle.widthPx) / 2,
                      std::min(contentBottom - subtitle.heightPx - 140, 520), theme.muted);
    }
}

void renderTextScene(TutorialSurface& frame, const StoryScene& scene, const StoryTheme& theme) {
    const int width = static_cast<int>(frame.width());
    const int contentBottom = static_cast<int>(frame.height()) - kSubtitleHeight;
    const int x = 180;
    const int maxWidth = width - 360;
    const StoryTextLayout title = layoutStoryText(scene.title, theme.heading, maxWidth, 140, theme.lineSpacingPx);
    drawStoryText(frame, title, theme.heading, x, 150, theme.foreground);
    const int bodyY = 150 + title.heightPx + 70;
    const StoryTextLayout body = layoutStoryText(scene.body, theme.body, maxWidth,
                                                 contentBottom - bodyY - 100, theme.lineSpacingPx);
    drawStoryText(frame, body, theme.body, x, bodyY, theme.foreground);
}

void renderCallout(TutorialSurface& frame, const StoryScene& scene, const StoryTheme& theme) {
    if (!scene.calloutKind) throw std::runtime_error("callout scene has no type");
    TutorialColor background{};
    TutorialColor foreground{};
    std::string label;
    switch (*scene.calloutKind) {
        case CalloutKind::Tip:
            background = theme.tipBackground; foreground = theme.tipForeground; label = "TIP"; break;
        case CalloutKind::Warning:
            background = theme.warningBackground; foreground = theme.warningForeground; label = "WARNING"; break;
        case CalloutKind::Recipe:
            background = theme.recipeBackground; foreground = theme.recipeForeground; label = "RECIPE"; break;
    }
    const int width = static_cast<int>(frame.width());
    const int contentBottom = static_cast<int>(frame.height()) - kSubtitleHeight;
    const TutorialRect box{180, 140, width - 360, contentBottom - 260};
    frame.fillRect(box, background);
    frame.fillRect({box.x, box.y, 8, box.height}, theme.accent);
    const StoryFontStyle labelFont{"CLOCK Mono", 21U};
    (void)drawStoryTextLine(frame, label, labelFont, box.x + 56, box.y + 48, theme.accent);
    const StoryTextLayout title = layoutStoryText(scene.title, theme.heading, box.width - 112, 120, theme.lineSpacingPx);
    drawStoryText(frame, title, theme.heading, box.x + 56, box.y + 110, foreground);
    std::string content = scene.body;
    if (*scene.calloutKind == CalloutKind::Recipe) {
        content.clear();
        for (std::size_t index = 0U; index < scene.recipeSteps.size(); ++index) {
            if (!content.empty()) content += '\n';
            content += std::to_string(index + 1U) + ". " + scene.recipeSteps[index];
        }
    }
    const int bodyY = box.y + 110 + title.heightPx + 58;
    const StoryTextLayout body = layoutStoryText(content, theme.body, box.width - 112,
                                                 box.y + box.height - bodyY - 48, theme.lineSpacingPx);
    drawStoryText(frame, body, theme.body, box.x + 56, bodyY, foreground);
}

void renderTutorial(
    TutorialSurface& frame,
    SimulatorRuntime& runtime,
    const layout::PanelLayout& panelLayout,
    const PhysicalPresentationState& physicalState,
    scope::Session& scopeSession,
    const StoryTheme& theme,
    const double speedMultiplier) {
    const OutputProfile output{
        static_cast<std::uint16_t>(frame.width()),
        static_cast<std::uint16_t>(frame.height()),
        30U};
    const bool scopeVisible = physicalState.scopeMode == ScopeMode::VisibleChannel;
    const TutorialCompositionLayout composition = resolveTutorialComposition(output, scopeVisible);
    renderOled(frame, runtime, composition.oledBox, theme);
    if (scopeVisible) {
        renderScope(frame, runtime, scopeSession, composition.scopeBox, theme);
    }
    renderPanel(frame, panelLayout, runtime, physicalState, speedMultiplier, composition.panelBox);
}

}  // namespace

TutorialCompositionLayout resolveTutorialComposition(const OutputProfile& output, const bool scopeVisible) {
    const int width = static_cast<int>(output.width);
    const int height = static_cast<int>(output.height);
    const int contentHeight = height - kSubtitleHeight - 2 * kFrameMargin;
    if (width < 1280 || contentHeight < 600) {
        throw std::runtime_error("tutorial output is too small for Phase-1 side-by-side composition");
    }
    const int contentWidth = width - 2 * kFrameMargin;
    const int leftWidth = (contentWidth * 62) / 100;
    const int rightWidth = contentWidth - leftWidth - kTutorialGap;
    TutorialCompositionLayout layout{};
    layout.content = {kFrameMargin, kFrameMargin, contentWidth, contentHeight};
    const TutorialRect left{kFrameMargin, kFrameMargin, leftWidth, contentHeight};
    layout.panelBox = {kFrameMargin + leftWidth + kTutorialGap, kFrameMargin, rightWidth, contentHeight};
    if (scopeVisible) {
        layout.oledBox = {left.x, left.y, left.width, left.height - kScopeHeight - 24};
        layout.scopeBox = {left.x + 20, left.y + left.height - kScopeHeight, left.width - 40, kScopeHeight};
    } else {
        layout.oledBox = left;
    }
    constexpr int oledWidth = 128;
    constexpr int oledHeight = 64;
    const int scale = std::min(layout.oledBox.width / oledWidth, layout.oledBox.height / oledHeight);
    if (scale < 1) throw std::runtime_error("OLED area cannot fit integer nearest-neighbour scale");
    const int rasterWidth = oledWidth * scale;
    const int rasterHeight = oledHeight * scale;
    layout.oledRaster = {
        layout.oledBox.x + (layout.oledBox.width - rasterWidth) / 2,
        layout.oledBox.y + (layout.oledBox.height - rasterHeight) / 2,
        rasterWidth,
        rasterHeight};
    layout.subtitleStrip = {0, height - kSubtitleHeight, width, kSubtitleHeight};
    return layout;
}

TutorialRenderer::TutorialRenderer(
    std::filesystem::path tutorialRoot,
    layout::PanelLayout panelLayout)
    : tutorialRoot_(std::move(tutorialRoot)), panelLayout_(std::move(panelLayout)) {}

TutorialSurface TutorialRenderer::renderScene(
    const Story& story,
    const StoryScene& scene,
    SimulatorRuntime& runtime,
    const PhysicalPresentationState& physicalState,
    const std::string& activeSubtitle,
    const double speedMultiplier) {
    const StoryTheme theme = loadStoryTheme(tutorialRoot_, story.theme);
    TutorialSurface frame(story.output.width, story.output.height, theme.background);
    switch (scene.kind) {
        case SceneKind::Chapter:
            renderChapter(frame, scene, theme);
            break;
        case SceneKind::Text:
            renderTextScene(frame, scene, theme);
            break;
        case SceneKind::Callout:
            renderCallout(frame, scene, theme);
            break;
        case SceneKind::Tutorial:
            renderTutorial(frame, runtime, panelLayout_, physicalState, scopeSession_, theme, speedMultiplier);
            break;
    }
    if (scene.kind == SceneKind::Tutorial) {
        const std::string subtitle = !activeSubtitle.empty() ? activeSubtitle : scene.subtitle;
        renderSubtitle(frame, subtitle, theme);
    }
    return frame;
}

}  // namespace clockfw::sim::tutorial
