/**
 * @file panel_layout.cpp
 * @brief Physical mm-to-render mapping for the simulator panel.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "panel_layout.h"

#include <array>
#include <stdexcept>
#include <string>

#include "panel_layout_ini.h"

namespace clockfw::sim::layout {
namespace {

constexpr float kDefaultPanelWidthMm = 50.5F;
constexpr float kDefaultPanelHeightMm = 128.5F;

struct PhysicalRect {
    float xMm = 0.0F;
    float yMm = 0.0F;
    float widthMm = 0.0F;
    float heightMm = 0.0F;
};

struct PhysicalPoint {
    float xMm = 0.0F;
    float yMm = 0.0F;
};

struct PhysicalCircle {
    PhysicalPoint center{};
    float actuatorDiameterMm = 1.0F;
    float bodyDiameterMm = 1.0F;
    Color color{};
};

struct PhysicalJack {
    float nutDiameterMm = 7.85F;
    float bushingDiameterMm = 6.0F;
    float openingDiameterMm = 3.6F;
};

struct LayoutDefinition {
    int windowWidth = 1180;
    int windowHeight = 952;
    float pixelsPerMm = 7.0F;
    Rect panel{20.0F, 20.0F, 0.0F, 0.0F};
    Rect developerPanel{404.0F, 20.0F, 756.0F, 912.0F};
    float physicalWidthMm = kDefaultPanelWidthMm;
    float physicalHeightMm = kDefaultPanelHeightMm;
    std::filesystem::path backgroundImagePath{};
    bool drawBuiltinLabels = true;
    bool drawScrews = true;

    PhysicalRect display{2.525F, 14.6535F, 35.9111F, 18.0351F};
    int displayPixelScale = 2;
    PhysicalPoint encoder{44.0472F, 23.1075F};
    float encoderDiameterMm = 8.4166F;

    PhysicalCircle play{{9.6792F, 48.7511F}, 9.0F, 12.0F, {176U, 32U, 32U}};
    PhysicalCircle tap{{23.8472F, 48.7511F}, 9.0F, 12.0F, {112U, 112U, 112U}};
    PhysicalCircle stop{{38.0153F, 48.7511F}, 9.0F, 12.0F, {28U, 28U, 28U}};

    PhysicalJack tsJack{};
    PhysicalJack trsJack{};
    PhysicalPoint sync{7.2944F, 70.1678F};
    JackType syncJackType = JackType::Ts;
    PhysicalPoint reset{18.6569F, 70.1678F};
    JackType resetJackType = JackType::Ts;

    float ledDiameterMm = 3.0F;
    Color ledOnColor{235U, 52U, 52U};
    Color ledOffColor{90U, 52U, 52U};

    std::array<PhysicalPoint, 8U> outputs{{
        {6.5931F, 95.8114F}, {17.9556F, 95.8114F}, {29.3181F, 95.8114F}, {40.6806F, 95.8114F},
        {6.5931F, 112.4375F}, {17.9556F, 112.4375F}, {29.3181F, 112.4375F}, {40.6806F, 112.4375F}
    }};
    std::array<JackType, 8U> outputJackTypes{{
        JackType::Ts, JackType::Ts, JackType::Ts, JackType::Ts,
        JackType::Ts, JackType::Ts, JackType::Ts, JackType::Ts
    }};
    std::array<PhysicalPoint, 8U> leds{{
        {6.5931F, 89.8936F}, {17.9556F, 89.8936F}, {29.3181F, 89.8936F}, {40.6806F, 89.8936F},
        {6.5931F, 106.5197F}, {17.9556F, 106.5197F}, {29.3181F, 106.5197F}, {40.6806F, 106.5197F}
    }};
    std::array<PhysicalPoint, 4U> screws{{
        {2.1042F, 2.2533F}, {48.4097F, 2.2533F}, {2.1042F, 126.2467F}, {48.4097F, 126.2467F}
    }};
};

PhysicalPoint consumePoint(ini::Section& section, const PhysicalPoint& fallback, const std::string& context) {
    return {ini::takeFloat(section, "x_mm", fallback.xMm, context),
            ini::takeFloat(section, "y_mm", fallback.yMm, context)};
}

PhysicalRect consumeRect(ini::Section& section, const PhysicalRect& fallback, const std::string& context) {
    return {
        ini::takeFloat(section, "x_mm", fallback.xMm, context),
        ini::takeFloat(section, "y_mm", fallback.yMm, context),
        ini::takeFloat(section, "width_mm", fallback.widthMm, context),
        ini::takeFloat(section, "height_mm", fallback.heightMm, context)
    };
}

void consumeCircle(ini::Section& section, PhysicalCircle& circle, const std::string& context) {
    circle.center = consumePoint(section, circle.center, context);
    circle.actuatorDiameterMm = ini::takeFloat(
        section, "actuator_diameter_mm", circle.actuatorDiameterMm, context);
    circle.bodyDiameterMm = ini::takeFloat(section, "body_diameter_mm", circle.bodyDiameterMm, context);
    circle.color = ini::takeColor(section, "color", circle.color, context);
}

Point mapPoint(const LayoutDefinition& definition, const PhysicalPoint& point) {
    return {definition.panel.x + point.xMm * definition.pixelsPerMm,
            definition.panel.y + point.yMm * definition.pixelsPerMm};
}

Rect mapRect(const LayoutDefinition& definition, const PhysicalRect& rect) {
    const Point origin = mapPoint(definition, {rect.xMm, rect.yMm});
    return {origin.x, origin.y, rect.widthMm * definition.pixelsPerMm, rect.heightMm * definition.pixelsPerMm};
}

float mapRadius(const LayoutDefinition& definition, const float diameterMm) {
    return diameterMm * definition.pixelsPerMm * 0.5F;
}

CircleControl mapCircle(const LayoutDefinition& definition, const PhysicalCircle& circle) {
    return {
        mapPoint(definition, circle.center),
        mapRadius(definition, circle.actuatorDiameterMm),
        mapRadius(definition, circle.bodyDiameterMm),
        circle.color};
}

JackGeometry mapJack(const LayoutDefinition& definition, const PhysicalJack& jack) {
    return {
        mapRadius(definition, jack.nutDiameterMm),
        mapRadius(definition, jack.bushingDiameterMm),
        mapRadius(definition, jack.openingDiameterMm)};
}

void validateJack(const PhysicalJack& jack, const std::string& name) {
    if (jack.nutDiameterMm <= 0.0F || jack.bushingDiameterMm <= 0.0F || jack.openingDiameterMm <= 0.0F ||
        jack.openingDiameterMm >= jack.bushingDiameterMm || jack.bushingDiameterMm > jack.nutDiameterMm) {
        throw std::runtime_error(
            name + " requires 0 < opening_diameter_mm < bushing_diameter_mm <= nut_diameter_mm");
    }
}

void validateDefinition(const LayoutDefinition& definition) {
    if (definition.windowWidth <= 0 || definition.windowHeight <= 0 || definition.pixelsPerMm <= 0.0F) {
        throw std::runtime_error("simulator window dimensions and pixels_per_mm must be positive");
    }
    if (definition.physicalWidthMm <= 0.0F || definition.physicalHeightMm <= 0.0F) {
        throw std::runtime_error("panel dimensions must be positive");
    }
    if (definition.display.widthMm <= 0.0F || definition.display.heightMm <= 0.0F ||
        definition.encoderDiameterMm <= 0.0F || definition.play.actuatorDiameterMm <= 0.0F ||
        definition.tap.actuatorDiameterMm <= 0.0F || definition.stop.actuatorDiameterMm <= 0.0F ||
        definition.play.bodyDiameterMm < definition.play.actuatorDiameterMm ||
        definition.tap.bodyDiameterMm < definition.tap.actuatorDiameterMm ||
        definition.stop.bodyDiameterMm < definition.stop.actuatorDiameterMm ||
        definition.ledDiameterMm <= 0.0F) {
        throw std::runtime_error("display/control dimensions must be positive and button body >= actuator");
    }
    validateJack(definition.tsJack, "jack_ts");
    validateJack(definition.trsJack, "jack_trs");
    if (definition.displayPixelScale < 1 || definition.displayPixelScale > 8) {
        throw std::runtime_error("display.pixel_scale must be an integer in the range 1..8");
    }
}

PanelLayout resolve(const LayoutDefinition& definition) {
    validateDefinition(definition);
    PanelLayout layout{};
    layout.windowWidth = definition.windowWidth;
    layout.windowHeight = definition.windowHeight;
    layout.pixelsPerMm = definition.pixelsPerMm;
    layout.panel = {
        definition.panel.x, definition.panel.y,
        definition.physicalWidthMm * definition.pixelsPerMm,
        definition.physicalHeightMm * definition.pixelsPerMm};
    layout.developerPanel = definition.developerPanel;
    layout.display = mapRect(definition, definition.display);
    layout.displayPixelScale = definition.displayPixelScale;
    layout.encoderCenter = mapPoint(definition, definition.encoder);
    layout.encoderRadius = mapRadius(definition, definition.encoderDiameterMm);
    layout.playButton = mapCircle(definition, definition.play);
    layout.tapButton = mapCircle(definition, definition.tap);
    layout.stopButton = mapCircle(definition, definition.stop);
    layout.syncInputCenter = mapPoint(definition, definition.sync);
    layout.syncJackType = definition.syncJackType;
    layout.resetInputCenter = mapPoint(definition, definition.reset);
    layout.resetJackType = definition.resetJackType;
    layout.tsJack = mapJack(definition, definition.tsJack);
    layout.trsJack = mapJack(definition, definition.trsJack);
    layout.ledRadius = mapRadius(definition, definition.ledDiameterMm);
    layout.ledOnColor = definition.ledOnColor;
    layout.ledOffColor = definition.ledOffColor;
    for (std::size_t index = 0U; index < layout.outputCenters.size(); ++index) {
        layout.outputCenters[index] = mapPoint(definition, definition.outputs[index]);
        layout.outputJackTypes[index] = definition.outputJackTypes[index];
        layout.ledCenters[index] = mapPoint(definition, definition.leds[index]);
    }
    for (std::size_t index = 0U; index < layout.screwCenters.size(); ++index) {
        layout.screwCenters[index] = mapPoint(definition, definition.screws[index]);
    }
    layout.backgroundImagePath = definition.backgroundImagePath;
    layout.drawBuiltinLabels = definition.drawBuiltinLabels;
    layout.drawScrews = definition.drawScrews;
    return layout;
}

}  // namespace

bool Rect::contains(const float pointX, const float pointY) const {
    return pointX >= x && pointX <= x + width && pointY >= y && pointY <= y + height;
}

bool CircleControl::contains(const float pointX, const float pointY) const {
    const float dx = pointX - center.x;
    const float dy = pointY - center.y;
    return dx * dx + dy * dy <= bodyRadius * bodyRadius;
}

bool PanelLayout::encoderContains(const float x, const float y) const {
    const float dx = x - encoderCenter.x;
    const float dy = y - encoderCenter.y;
    return dx * dx + dy * dy <= encoderRadius * encoderRadius;
}

bool PanelLayout::syncInputContains(const float x, const float y) const {
    const JackGeometry& geometry = jackGeometry(syncJackType);
    const float dx = x - syncInputCenter.x;
    const float dy = y - syncInputCenter.y;
    return dx * dx + dy * dy <= geometry.nutRadius * geometry.nutRadius;
}

bool PanelLayout::resetInputContains(const float x, const float y) const {
    const JackGeometry& geometry = jackGeometry(resetJackType);
    const float dx = x - resetInputCenter.x;
    const float dy = y - resetInputCenter.y;
    return dx * dx + dy * dy <= geometry.nutRadius * geometry.nutRadius;
}

const JackGeometry& PanelLayout::jackGeometry(const JackType type) const {
    return type == JackType::Trs ? trsJack : tsJack;
}

PanelLayout makeDefaultPanelLayout() {
    return resolve(LayoutDefinition{});
}

PanelLayout loadPanelLayout(const std::filesystem::path& path) {
    LayoutDefinition definition{};
    ini::Document document = ini::parse(path);

    ini::consumeSection(document, "simulator", [&](ini::Section& section) {
        definition.windowWidth = ini::takeInt(section, "window_width", definition.windowWidth, "simulator");
        definition.windowHeight = ini::takeInt(section, "window_height", definition.windowHeight, "simulator");
        definition.pixelsPerMm = ini::takeFloat(section, "pixels_per_mm", definition.pixelsPerMm, "simulator");
        definition.panel.x = ini::takeFloat(section, "panel_x", definition.panel.x, "simulator");
        definition.panel.y = ini::takeFloat(section, "panel_y", definition.panel.y, "simulator");
        definition.developerPanel.x = ini::takeFloat(section, "developer_x", definition.developerPanel.x, "simulator");
        definition.developerPanel.y = ini::takeFloat(section, "developer_y", definition.developerPanel.y, "simulator");
        definition.developerPanel.width = ini::takeFloat(section, "developer_width", definition.developerPanel.width, "simulator");
        definition.developerPanel.height = ini::takeFloat(section, "developer_height", definition.developerPanel.height, "simulator");
    });

    ini::consumeSection(document, "panel", [&](ini::Section& section) {
        definition.physicalWidthMm = ini::takeFloat(section, "width_mm", definition.physicalWidthMm, "panel");
        definition.physicalHeightMm = ini::takeFloat(section, "height_mm", definition.physicalHeightMm, "panel");
        definition.drawBuiltinLabels = ini::takeBool(section, "draw_builtin_labels", definition.drawBuiltinLabels, "panel");
        definition.drawScrews = ini::takeBool(section, "draw_screws", definition.drawScrews, "panel");
        const std::string image = ini::take(section, "background_image", "");
        if (!image.empty()) {
            const std::filesystem::path imagePath(image);
            definition.backgroundImagePath = imagePath.is_absolute() ? imagePath : path.parent_path() / imagePath;
        }
    });

    ini::consumeSection(document, "display", [&](ini::Section& section) {
        definition.display = consumeRect(section, definition.display, "display");
        definition.displayPixelScale = ini::takeInt(section, "pixel_scale", definition.displayPixelScale, "display");
    });
    ini::consumeSection(document, "encoder", [&](ini::Section& section) {
        definition.encoder = consumePoint(section, definition.encoder, "encoder");
        definition.encoderDiameterMm = ini::takeFloat(
            section, "knob_diameter_mm", definition.encoderDiameterMm, "encoder");
    });
    ini::consumeSection(document, "play", [&](ini::Section& section) { consumeCircle(section, definition.play, "play"); });
    ini::consumeSection(document, "tap", [&](ini::Section& section) { consumeCircle(section, definition.tap, "tap"); });
    ini::consumeSection(document, "stop", [&](ini::Section& section) { consumeCircle(section, definition.stop, "stop"); });

    ini::consumeSection(document, "jack_ts", [&](ini::Section& section) {
        definition.tsJack.nutDiameterMm = ini::takeFloat(
            section, "nut_diameter_mm", definition.tsJack.nutDiameterMm, "jack_ts");
        definition.tsJack.bushingDiameterMm = ini::takeFloat(
            section, "bushing_diameter_mm", definition.tsJack.bushingDiameterMm, "jack_ts");
        definition.tsJack.openingDiameterMm = ini::takeFloat(
            section, "opening_diameter_mm", definition.tsJack.openingDiameterMm, "jack_ts");
    });
    ini::consumeSection(document, "jack_trs", [&](ini::Section& section) {
        definition.trsJack.nutDiameterMm = ini::takeFloat(
            section, "nut_diameter_mm", definition.trsJack.nutDiameterMm, "jack_trs");
        definition.trsJack.bushingDiameterMm = ini::takeFloat(
            section, "bushing_diameter_mm", definition.trsJack.bushingDiameterMm, "jack_trs");
        definition.trsJack.openingDiameterMm = ini::takeFloat(
            section, "opening_diameter_mm", definition.trsJack.openingDiameterMm, "jack_trs");
    });
    ini::consumeSection(document, "sync", [&](ini::Section& section) {
        definition.sync = consumePoint(section, definition.sync, "sync");
        definition.syncJackType = ini::takeJackType(section, "jack_type", definition.syncJackType, "sync");
    });
    ini::consumeSection(document, "reset", [&](ini::Section& section) {
        definition.reset = consumePoint(section, definition.reset, "reset");
        definition.resetJackType = ini::takeJackType(section, "jack_type", definition.resetJackType, "reset");
    });
    ini::consumeSection(document, "leds", [&](ini::Section& section) {
        definition.ledDiameterMm = ini::takeFloat(section, "diameter_mm", definition.ledDiameterMm, "leds");
        definition.ledOnColor = ini::takeColor(section, "on_color", definition.ledOnColor, "leds");
        definition.ledOffColor = ini::takeColor(section, "off_color", definition.ledOffColor, "leds");
    });

    for (std::size_t index = 0U; index < definition.outputs.size(); ++index) {
        const std::string sectionName = "output_" + std::to_string(index + 1U);
        ini::consumeSection(document, sectionName, [&](ini::Section& section) {
            definition.outputs[index] = consumePoint(section, definition.outputs[index], sectionName);
            definition.outputJackTypes[index] = ini::takeJackType(
                section, "jack_type", definition.outputJackTypes[index], sectionName);
            definition.leds[index].xMm = ini::takeFloat(section, "led_x_mm", definition.leds[index].xMm, sectionName);
            definition.leds[index].yMm = ini::takeFloat(section, "led_y_mm", definition.leds[index].yMm, sectionName);
        });
    }

    for (std::size_t index = 0U; index < definition.screws.size(); ++index) {
        const std::string sectionName = "screw_" + std::to_string(index + 1U);
        ini::consumeSection(document, sectionName, [&](ini::Section& section) {
            definition.screws[index] = consumePoint(section, definition.screws[index], sectionName);
        });
    }

    if (!document.empty()) throw std::runtime_error("unknown panel layout section [" + document.begin()->first + "]");
    return resolve(definition);
}

void overrideBackgroundImage(PanelLayout& layout, const std::filesystem::path& imagePath) {
    layout.backgroundImagePath = imagePath;
}

void overrideDisplayPixelScale(PanelLayout& layout, const int pixelScale) {
    if (pixelScale < 1 || pixelScale > 8) throw std::runtime_error("display pixel scale must be in the range 1..8");
    layout.displayPixelScale = pixelScale;
}

}  // namespace clockfw::sim::layout
