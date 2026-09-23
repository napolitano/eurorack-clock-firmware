/**
 * @file generated_panel_layout.hpp
 * @brief Generated VCV Rack front-panel geometry sourced from sim/panel_layout.ini.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 *
 * GENERATED FILE. Run scripts/generate_vcv_panel.py after changing simulator geometry.
 */
#pragma once

#include <array>

namespace clockfw::vcv::panel {

struct PointMm final {
    float x;
    float y;
};

struct RectMm final {
    float x;
    float y;
    float width;
    float height;
};

inline constexpr float kPanelWidthMm = 50.5F;
inline constexpr float kPanelHeightMm = 128.5F;
inline constexpr float kPanelOffsetXmm = 0.15F;
inline constexpr float kPanelOffsetYmm = 0.0967F;
inline constexpr RectMm kDisplay{2.525F, 14.6535F, 35.9111F, 18.0351F};
inline constexpr PointMm kEncoderCenter{44.0472F, 23.1075F};
inline constexpr float kEncoderDiameterMm = 8.4166F;
inline constexpr PointMm kPlayCenter{9.6792F, 48.7511F};
inline constexpr PointMm kTapCenter{23.8472F, 48.7511F};
inline constexpr PointMm kStopCenter{38.0153F, 48.7511F};
inline constexpr float kButtonActuatorDiameterMm = 9F;
inline constexpr PointMm kSyncCenter{7.2944F, 70.1678F};
inline constexpr PointMm kResetCenter{18.6569F, 70.1678F};
inline constexpr float kJackNutDiameterMm = 7.85F;
inline constexpr float kLedDiameterMm = 3F;
inline constexpr std::array<PointMm, 8U> kOutputCenters{{
    {6.5931F, 95.8114F}, {17.9556F, 95.8114F}, {29.3181F, 95.8114F}, {40.6806F, 95.8114F}, {6.5931F, 112.4375F}, {17.9556F, 112.4375F}, {29.3181F, 112.4375F}, {40.6806F, 112.4375F}
}};
inline constexpr std::array<PointMm, 8U> kLedCenters{{
    {6.5931F, 89.8936F}, {17.9556F, 89.8936F}, {29.3181F, 89.8936F}, {40.6806F, 89.8936F}, {6.5931F, 106.5197F}, {17.9556F, 106.5197F}, {29.3181F, 106.5197F}, {40.6806F, 106.5197F}
}};
inline constexpr std::array<PointMm, 4U> kScrewCenters{{
    {2.1042F, 2.2533F}, {48.4097F, 2.2533F}, {2.1042F, 126.2467F}, {48.4097F, 126.2467F}
}};

}  // namespace clockfw::vcv::panel
