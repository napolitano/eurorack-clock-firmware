/**
 * @file simulator_contract_tests.cpp
 * @brief Static layout and timing-grid contract tests for the native simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "simulator_contract_tests.h"

#include <array>
#include <cmath>
#include <cstdint>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>

#include "domain/clock_types.h"
#include "panel_layout.h"
#include "scope_timeline.h"

namespace clockfw::sim::tests {
namespace {

int fail(const char* const message) {
    std::cerr << "simulator test failed: " << message << '\n';
    return 1;
}

}  // namespace

int runStaticContracts() {
    // The developer-scope ruler is a real timeline, not a screen-fixed decoration.
    if (scope::kWindowOptionsUs != std::array<std::uint64_t, 7U>{{
            500000ULL, 1000000ULL, 2000000ULL, 4000000ULL, 8000000ULL,
            16000000ULL, 32000000ULL}} ||
        scope::kWindowOptionsUs[scope::kDefaultWindowIndex] != 4000000ULL ||
        scope::kMaximumWindowUs != 32000000ULL) {
        return fail("scope zoom stages must remain fixed at 0.5/1/2/4/8/16/32 seconds");
    }
    if (scope::zoomInIndex(0U) != 0U || scope::zoomInIndex(3U) != 2U ||
        scope::zoomOutIndex(3U) != 4U || scope::zoomOutIndex(6U) != 6U) {
        return fail("scope zoom controls must clamp to the committed fixed span set");
    }
    clockfw::ClockState gridState{};
    gridState.bpm = 130U;
    gridState.masterMeter = {4U, 4U};
    gridState.source = clockfw::ClockSource::Internal;

    // Independent mode uses the shared 1/16 musical lattice. At 130 BPM that
    // is 115.384615 ms; a CLOCK x2 output therefore lands on every second line
    // instead of drifting against the old fixed 250 ms ruler.
    gridState.operatingMode = clockfw::OperatingMode::Independent;
    const std::uint32_t internalBpmMilli = scope::effectiveReferenceBpmMilli(
        gridState, false, 0U);
    const scope::MusicalGridSpec independentGrid = scope::musicalGridSpec(
        gridState, internalBpmMilli, 4000000ULL);
    if (std::fabs(independentGrid.intervalUs - (15000000000.0 / 130000.0)) > 0.001 ||
        independentGrid.minorEvery != 1U || independentGrid.majorEvery != 4U) {
        return fail("independent scope grid must follow the BPM-derived 1/16 musical lattice");
    }
    const double secondClockX2Reference = scope::referenceTimeUs(2U, independentGrid);
    if (std::fabs(secondClockX2Reference - (30000000000.0 / 130000.0)) > 0.001) {
        return fail("130 BPM x2 CLOCK references must be 230.769 ms apart, not fixed at 250 ms");
    }

    // ONE CLOCK follows the exact shared output rate, so swing/humanize are
    // visible as displacement from each expected gate boundary.
    gridState.operatingMode = clockfw::OperatingMode::UnifiedClock;
    gridState.unifiedClock.rate = {clockfw::ClockRatioMode::Multiply, 2U, 1U, 1U};
    const scope::MusicalGridSpec unifiedGrid = scope::musicalGridSpec(
        gridState, internalBpmMilli, 4000000ULL);
    if (std::fabs(unifiedGrid.intervalUs - (30000000000.0 / 130000.0)) > 0.001 ||
        std::string(unifiedGrid.basis) != "ONE CLOCK") {
        return fail("ONE CLOCK scope grid must follow the unswung shared-clock event interval");
    }

    // External tempo must drive the reference ruler even when it lies outside
    // the user's manual MIN/MAX BPM range.
    gridState.source = clockfw::ClockSource::External;
    gridState.externalSync.lossMode = clockfw::SyncLossMode::Freewheel;
    if (scope::effectiveReferenceBpmMilli(gridState, true, 480000U) != 480000U) {
        return fail("scope grid must follow the effective external tempo");
    }

    // Very fast clocks are decimated only by integer serial strides, so every
    // displayed line remains an exact member of the musical lattice.
    gridState.source = clockfw::ClockSource::Internal;
    gridState.bpm = 999U;
    gridState.operatingMode = clockfw::OperatingMode::Independent;
    const scope::MusicalGridSpec fastGrid = scope::musicalGridSpec(
        gridState, 999000U, 32000000ULL);
    if (fastGrid.minorEvery <= 1U ||
        scope::referenceTimeUs(fastGrid.minorEvery, fastGrid) <= 0.0) {
        return fail("high-rate scope grids must decimate by whole musical reference serials");
    }

    const std::uint64_t firstVisible = scope::firstMinorReferenceSerialAtOrAfter(
        4123456LL, unifiedGrid);
    const double firstVisibleTime = scope::referenceTimeUs(firstVisible, unifiedGrid);
    if (firstVisibleTime < 4123456.0 || !scope::isMajorReference(0U, unifiedGrid)) {
        return fail("scope reference serials must stay anchored to transport t=0");
    }
    const double gridXBefore = scope::normalizedPosition(firstVisibleTime, 4000000.0, 1000000ULL);
    const double gridXAfter = scope::normalizedPosition(firstVisibleTime, 4100000.0, 1000000ULL);
    if (gridXAfter >= gridXBefore ||
        std::fabs(scope::normalizedPosition(5100000.0, 4100000.0, 1000000ULL) - 1.0) > 0.000001) {
        return fail("musical scope references must scroll left while NOW remains the right edge");
    }

    // Keep the built-in desktop panel faithful to the intended physical hierarchy.
    const layout::PanelLayout defaultLayout = layout::makeDefaultPanelLayout();
    if (!(defaultLayout.display.y < defaultLayout.playButton.center.y &&
          defaultLayout.playButton.center.y < defaultLayout.syncInputCenter.y &&
          defaultLayout.syncInputCenter.y < defaultLayout.outputCenters[0].y &&
          defaultLayout.outputCenters[0].y < defaultLayout.outputCenters[4].y)) {
        return fail("panel controls must follow display -> buttons -> sync -> two output rows");
    }
    if (!(defaultLayout.encoderCenter.x > defaultLayout.display.x + defaultLayout.display.width &&
          defaultLayout.outputCenters[3].y == defaultLayout.outputCenters[0].y &&
          defaultLayout.outputCenters[7].y == defaultLayout.outputCenters[4].y)) {
        return fail("encoder and two-by-four output layout must match panel intent");
    }
    for (std::size_t index = 0U; index < defaultLayout.outputCenters.size(); ++index) {
        if (defaultLayout.ledCenters[index].y >= defaultLayout.outputCenters[index].y ||
            defaultLayout.ledCenters[index].x != defaultLayout.outputCenters[index].x) {
            return fail("each output LED must sit directly above its matching jack");
        }
    }
    if (defaultLayout.displayPixelScale != 2) {
        return fail("default OLED scaling must use an explicit integer factor");
    }
    if (defaultLayout.playButton.radius != 31.5F || defaultLayout.playButton.bodyRadius != 42.0F ||
        defaultLayout.tapButton.radius != 31.5F || defaultLayout.stopButton.radius != 31.5F) {
        return fail("default D6R geometry must resolve 9 mm actuator and 12 mm body at 7 px/mm");
    }
    if (std::fabs(defaultLayout.tsJack.nutRadius - 27.475F) > 0.001F ||
        std::fabs(defaultLayout.tsJack.bushingRadius - 21.0F) > 0.001F ||
        std::fabs(defaultLayout.tsJack.openingRadius - 12.6F) > 0.001F ||
        std::fabs(defaultLayout.trsJack.nutRadius - 27.475F) > 0.001F ||
        std::fabs(defaultLayout.trsJack.bushingRadius - 21.0F) > 0.001F ||
        std::fabs(defaultLayout.trsJack.openingRadius - 12.6F) > 0.001F) {
        return fail("default Thonkiconn TS/TRS front geometry must use documented 6/3.6 mm diameters");
    }
    if (defaultLayout.ledRadius != 10.5F) {
        return fail("default output LEDs must be 3 mm diameter");
    }

    // Layout configuration is expressed in physical millimetres and must drive
    // both drawing and hit-test geometry without modifying firmware code.
    const std::filesystem::path layoutPath = ".clock-simulator-test-layout.ini";
    {
        std::ofstream layoutFile(layoutPath);
        layoutFile
            << "[simulator]\n"
            << "panel_x = 10\n"
            << "panel_y = 20\n"
            << "pixels_per_mm = 10\n"
            << "[panel]\n"
            << "width_mm = 50.5\n"
            << "height_mm = 128.5\n"
            << "background_image = panel-test.png\n"
            << "draw_builtin_labels = false\n"
            << "[display]\n"
            << "pixel_scale = 3\n"
            << "[encoder]\n"
            << "x_mm = 25.25\n"
            << "y_mm = 64.25\n"
            << "knob_diameter_mm = 10\n"
            << "[play]\n"
            << "x_mm = 10\n"
            << "y_mm = 40\n"
            << "actuator_diameter_mm = 8\n"
            << "body_diameter_mm = 11\n"
            << "color = #123456\n"
            << "[jack_trs]\n"
            << "nut_diameter_mm = 8\n"
            << "bushing_diameter_mm = 7\n"
            << "opening_diameter_mm = 3.5\n"
            << "[sync]\n"
            << "jack_type = trs\n"
            << "[leds]\n"
            << "diameter_mm = 4\n"
            << "[output_1]\n"
            << "x_mm = 5.05\n"
            << "y_mm = 102.8\n"
            << "led_x_mm = 5.05\n"
            << "led_y_mm = 90\n";
    }
    const layout::PanelLayout configuredLayout = layout::loadPanelLayout(layoutPath);
    if (configuredLayout.encoderCenter.x != 262.5F || configuredLayout.encoderCenter.y != 662.5F) {
        return fail("millimetre coordinates must resolve against configured panel bounds");
    }
    if (!configuredLayout.encoderContains(262.5F, 662.5F) || configuredLayout.drawBuiltinLabels) {
        return fail("configured hit testing and panel flags must be applied");
    }
    if (configuredLayout.displayPixelScale != 3) {
        return fail("configured OLED scale must remain an integer setting");
    }
    if (configuredLayout.panel.width != 505.0F || configuredLayout.panel.height != 1285.0F) {
        return fail("pixels_per_mm must preserve exact panel physical proportions");
    }
    if (configuredLayout.playButton.center.x != 110.0F || configuredLayout.playButton.center.y != 420.0F ||
        configuredLayout.playButton.radius != 40.0F || configuredLayout.playButton.bodyRadius != 55.0F ||
        configuredLayout.playButton.color.red != 0x12U || configuredLayout.playButton.color.green != 0x34U ||
        configuredLayout.playButton.color.blue != 0x56U) {
        return fail("round button size, body envelope and RGB color must resolve from millimetres/config");
    }
    if (configuredLayout.syncJackType != layout::JackType::Trs ||
        std::fabs(configuredLayout.trsJack.nutRadius - 40.0F) > 0.001F ||
        std::fabs(configuredLayout.trsJack.bushingRadius - 35.0F) > 0.001F ||
        std::fabs(configuredLayout.trsJack.openingRadius - 17.5F) > 0.001F) {
        return fail("TS/TRS jack profile selection and millimetre diameters must be configurable");
    }
    if (configuredLayout.ledRadius != 20.0F) {
        return fail("LED diameter must resolve from physical millimetres");
    }
    if (!configuredLayout.syncInputContains(
            configuredLayout.syncInputCenter.x, configuredLayout.syncInputCenter.y)) {
        return fail("SYNC IN hit testing must follow the configured layout");
    }
    if (!configuredLayout.resetInputContains(
            configuredLayout.resetInputCenter.x, configuredLayout.resetInputCenter.y)) {
        return fail("RST IN hit testing must follow the configured layout");
    }
    if (configuredLayout.outputCenters[0].x != 60.5F || configuredLayout.outputCenters[0].y != 1048.0F) {
        return fail("configured output jack coordinates must be resolved from millimetres");
    }
    if (configuredLayout.backgroundImagePath.filename() != "panel-test.png") {
        return fail("relative panel image must resolve relative to the layout file");
    }
    layout::PanelLayout overriddenLayout = configuredLayout;
    layout::overrideBackgroundImage(overriddenLayout, "override-panel.jpg");
    if (overriddenLayout.backgroundImagePath.filename() != "override-panel.jpg") {
        return fail("command-line panel image override must replace configured image");
    }
    std::filesystem::remove(layoutPath);

    return 0;
}

}  // namespace clockfw::sim::tests
