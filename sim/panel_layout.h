/**
 * @file panel_layout.h
 * @brief Runtime-configurable physical geometry for the native 10 HP / 3U simulator panel.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <filesystem>

namespace clockfw::sim::layout {

/** @brief Lightweight rectangle used for drawing and hit testing without SDL dependency. */
struct Rect {
    float x = 0.0F;
    float y = 0.0F;
    float width = 0.0F;
    float height = 0.0F;

    /** @brief Returns true when a point lies inside this rectangle. */
    bool contains(float pointX, float pointY) const;
};

/** @brief Simple point coordinate used for circular controls. */
struct Point {
    float x = 0.0F;
    float y = 0.0F;
};

/** @brief RGB appearance color used by configurable front-panel controls. */
struct Color {
    std::uint8_t red = 0U;
    std::uint8_t green = 0U;
    std::uint8_t blue = 0U;
};

/** @brief Resolved circular front-panel control. */
struct CircleControl {
    Point center{};
    float radius = 1.0F;
    float bodyRadius = 1.0F;
    Color color{};

    /** @brief Returns true when a point lies inside this control. */
    bool contains(float pointX, float pointY) const;
};

/** @brief Supported Thonkiconn-family front-panel jack geometries. */
enum class JackType : std::uint8_t {
    Ts,
    Trs
};

/** @brief Front-visible dimensions of one jack profile after mm-to-pixel resolution. */
struct JackGeometry {
    float nutRadius = 1.0F;
    float bushingRadius = 0.8F;
    float openingRadius = 0.5F;
};

/**
 * @brief Fully resolved simulator layout in logical render coordinates.
 *
 * The committed INI stores all physical control positions and sizes in millimetres.
 * A single pixels-per-mm factor preserves physical proportions exactly on screen.
 */
struct PanelLayout {
    int windowWidth = 1180;
    int windowHeight = 952;
    float pixelsPerMm = 7.0F;

    Rect panel{20.0F, 20.0F, 353.5F, 899.5F};
    Rect developerPanel{404.0F, 20.0F, 756.0F, 912.0F};
    Rect display{37.675F, 122.5745F, 251.3777F, 126.2457F};
    int displayPixelScale = 2;

    Point encoderCenter{328.3304F, 181.7525F};
    float encoderRadius = 29.4581F;

    CircleControl playButton{{87.7544F, 361.2577F}, 31.5F, 42.0F, {176U, 32U, 32U}};
    CircleControl tapButton{{186.9304F, 361.2577F}, 31.5F, 42.0F, {112U, 112U, 112U}};
    CircleControl stopButton{{286.1064F, 361.2577F}, 31.5F, 42.0F, {28U, 28U, 28U}};

    Point syncInputCenter{71.0608F, 511.1746F};
    JackType syncJackType = JackType::Ts;
    Point resetInputCenter{150.5983F, 511.1746F};
    JackType resetJackType = JackType::Ts;
    JackGeometry tsJack{27.475F, 21.0F, 12.6F};
    JackGeometry trsJack{27.475F, 21.0F, 12.6F};

    float ledRadius = 10.5F;
    Color ledOnColor{235U, 52U, 52U};
    Color ledOffColor{90U, 52U, 52U};

    std::array<Point, 8U> outputCenters{{
        {66.1517F, 690.6798F}, {145.6892F, 690.6798F}, {225.2267F, 690.6798F}, {304.7642F, 690.6798F},
        {66.1517F, 807.0625F}, {145.6892F, 807.0625F}, {225.2267F, 807.0625F}, {304.7642F, 807.0625F}
    }};

    std::array<JackType, 8U> outputJackTypes{{
        JackType::Ts, JackType::Ts, JackType::Ts, JackType::Ts,
        JackType::Ts, JackType::Ts, JackType::Ts, JackType::Ts
    }};

    std::array<Point, 8U> ledCenters{{
        {66.1517F, 649.2552F}, {145.6892F, 649.2552F}, {225.2267F, 649.2552F}, {304.7642F, 649.2552F},
        {66.1517F, 765.6379F}, {145.6892F, 765.6379F}, {225.2267F, 765.6379F}, {304.7642F, 765.6379F}
    }};

    std::array<Point, 4U> screwCenters{{
        {34.7294F, 35.7731F}, {358.8679F, 35.7731F}, {34.7294F, 903.7269F}, {358.8679F, 903.7269F}
    }};

    std::filesystem::path backgroundImagePath{};
    bool drawBuiltinLabels = true;
    bool drawScrews = true;

    /** @brief Returns true when the supplied point lies inside the encoder hit area. */
    bool encoderContains(float x, float y) const;

    /** @brief Returns true when the supplied point lies inside the SYNC IN jack hit area. */
    bool syncInputContains(float x, float y) const;

    /** @brief Returns true when the supplied point lies inside the RST IN jack hit area. */
    bool resetInputContains(float x, float y) const;

    /** @brief Returns resolved geometry for one TS/TRS jack profile. */
    const JackGeometry& jackGeometry(JackType type) const;
};

/** @brief Returns built-in mm-accurate layout matching the committed simulator geometry. */
PanelLayout makeDefaultPanelLayout();

/**
 * @brief Loads a strict INI layout file and resolves millimetre coordinates/sizes.
 *
 * Relative background-image paths are resolved relative to the INI file.
 * Unknown sections/keys and invalid values are rejected to catch layout typos.
 *
 * @param path Layout INI file to load.
 * @return Fully resolved simulator layout.
 * @throws std::runtime_error when the file cannot be read or validated.
 */
PanelLayout loadPanelLayout(const std::filesystem::path& path);

/** @brief Replaces the configured panel background image without changing geometry. */
void overrideBackgroundImage(PanelLayout& layout, const std::filesystem::path& imagePath);

/** @brief Overrides the integer nearest-neighbour OLED scale used by the simulator. */
void overrideDisplayPixelScale(PanelLayout& layout, int pixelScale);

}  // namespace clockfw::sim::layout
