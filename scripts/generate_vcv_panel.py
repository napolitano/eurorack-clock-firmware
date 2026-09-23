#!/usr/bin/env python3
# Author: Axel Napolitano
# License: PolyForm-Noncommercial-1.0.0
"""Generate the functional VCV Rack panel directly from sim/panel_layout.ini.

The native simulator's millimetre layout is the single front-panel geometry source.
This generator maps those physical coordinates to Rack's 75-DPI panel coordinate
system without inventing a second placement model.
"""
from __future__ import annotations

import argparse
import configparser
import xml.etree.ElementTree as ET
from dataclasses import dataclass
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LAYOUT = ROOT / "sim" / "panel_layout.ini"
VCV_DIR = ROOT / "vcv"
DEFAULT_PANEL = VCV_DIR / "res" / "CLOCK.svg"
DEFAULT_HEADER = VCV_DIR / "generated_panel_layout.hpp"
CLOCK_LOGO = ROOT / "docs" / "manual-source" / "assets" / "clock-logo.svg"

RACK_DPI = 75.0
MM_PER_INCH = 25.4
PX_PER_MM = RACK_DPI / MM_PER_INCH
RACK_WIDTH_PX = 150.0  # 10 HP
RACK_HEIGHT_PX = 380.0


@dataclass(frozen=True)
class Point:
    x: float
    y: float


def fmt(value: float) -> str:
    return f"{value:.4f}".rstrip("0").rstrip(".")


def cpp_float(value: float) -> str:
    """Format a standard C++ floating literal with an explicit decimal point."""
    text = fmt(value)
    if "." not in text and "e" not in text.lower():
        text += ".0"
    return f"{text}F"


def read_layout(path: Path) -> configparser.ConfigParser:
    parser = configparser.ConfigParser(interpolation=None)
    with path.open("r", encoding="utf-8") as stream:
        parser.read_file(stream)
    return parser


def panel_offsets(parser: configparser.ConfigParser) -> tuple[float, float]:
    panel_width_px = parser.getfloat("panel", "width_mm") * PX_PER_MM
    panel_height_px = parser.getfloat("panel", "height_mm") * PX_PER_MM
    return ((RACK_WIDTH_PX - panel_width_px) * 0.5, (RACK_HEIGHT_PX - panel_height_px) * 0.5)


def point_mm(parser: configparser.ConfigParser, section: str) -> Point:
    return Point(parser.getfloat(section, "x_mm"), parser.getfloat(section, "y_mm"))


def to_px(value_mm: float) -> float:
    return value_mm * PX_PER_MM


def point_px(parser: configparser.ConfigParser, section: str) -> Point:
    ox, oy = panel_offsets(parser)
    p = point_mm(parser, section)
    return Point(ox + to_px(p.x), oy + to_px(p.y))


def clock_logo_paths() -> list[str]:
    """Return the project-owned CLOCK wordmark paths from the canonical manual asset."""
    root = ET.parse(CLOCK_LOGO).getroot()
    paths: list[str] = []
    for element in root.iter():
        if element.tag.endswith("path") and element.get("d"):
            paths.append(element.get("d", ""))
    if not paths:
        raise RuntimeError(f"CLOCK logo contains no paths: {CLOCK_LOGO}")
    return paths


def generated_header(parser: configparser.ConfigParser) -> str:
    display = (
        parser.getfloat("display", "x_mm"),
        parser.getfloat("display", "y_mm"),
        parser.getfloat("display", "width_mm"),
        parser.getfloat("display", "height_mm"),
    )
    outputs = [point_mm(parser, f"output_{i}") for i in range(1, 9)]
    leds = [
        Point(
            parser.getfloat(f"output_{i}", "led_x_mm"),
            parser.getfloat(f"output_{i}", "led_y_mm"),
        )
        for i in range(1, 9)
    ]
    screws = [point_mm(parser, f"screw_{i}") for i in range(1, 5)]

    def p(point: Point) -> str:
        return f"{{{cpp_float(point.x)}, {cpp_float(point.y)}}}"

    return f'''/**
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

namespace clockfw::vcv::panel {{

struct PointMm final {{
    float x;
    float y;
}};

struct RectMm final {{
    float x;
    float y;
    float width;
    float height;
}};

inline constexpr float kPanelWidthMm = {cpp_float(parser.getfloat("panel", "width_mm"))};
inline constexpr float kPanelHeightMm = {cpp_float(parser.getfloat("panel", "height_mm"))};
inline constexpr float kPanelOffsetXmm = {cpp_float((RACK_WIDTH_PX / PX_PER_MM - parser.getfloat("panel", "width_mm")) * 0.5)};
inline constexpr float kPanelOffsetYmm = {cpp_float((RACK_HEIGHT_PX / PX_PER_MM - parser.getfloat("panel", "height_mm")) * 0.5)};
inline constexpr RectMm kDisplay{{{cpp_float(display[0])}, {cpp_float(display[1])}, {cpp_float(display[2])}, {cpp_float(display[3])}}};
inline constexpr PointMm kEncoderCenter{p(point_mm(parser, "encoder"))};
inline constexpr float kEncoderDiameterMm = {cpp_float(parser.getfloat("encoder", "knob_diameter_mm"))};
inline constexpr PointMm kPlayCenter{p(point_mm(parser, "play"))};
inline constexpr PointMm kTapCenter{p(point_mm(parser, "tap"))};
inline constexpr PointMm kStopCenter{p(point_mm(parser, "stop"))};
inline constexpr float kButtonActuatorDiameterMm = {cpp_float(parser.getfloat("play", "actuator_diameter_mm"))};
inline constexpr PointMm kSyncCenter{p(point_mm(parser, "sync"))};
inline constexpr PointMm kResetCenter{p(point_mm(parser, "reset"))};
inline constexpr float kJackNutDiameterMm = {cpp_float(parser.getfloat("jack_ts", "nut_diameter_mm"))};
inline constexpr float kLedDiameterMm = {cpp_float(parser.getfloat("leds", "diameter_mm"))};
inline constexpr std::array<PointMm, 8U> kOutputCenters{{{{
    {', '.join(p(x) for x in outputs)}
}}}};
inline constexpr std::array<PointMm, 8U> kLedCenters{{{{
    {', '.join(p(x) for x in leds)}
}}}};
inline constexpr std::array<PointMm, 4U> kScrewCenters{{{{
    {', '.join(p(x) for x in screws)}
}}}};

}}  // namespace clockfw::vcv::panel
'''


def circle_svg(size_px: float, fill: str, pressed: bool = False) -> str:
    center = size_px * 0.5
    radius = max(1.0, center - 1.0)
    highlight = 0.24 if pressed else 0.48
    return f'''<svg xmlns="http://www.w3.org/2000/svg" width="{fmt(size_px)}" height="{fmt(size_px)}" viewBox="0 0 {fmt(size_px)} {fmt(size_px)}">
  <circle cx="{fmt(center)}" cy="{fmt(center)}" r="{fmt(radius)}" fill="{fill}" stroke="#191a1c" stroke-width="1.4"/>
  <circle cx="{fmt(center - radius * 0.32)}" cy="{fmt(center - radius * 0.32)}" r="{fmt(max(0.8, radius * 0.10))}" fill="#ffffff" fill-opacity="{highlight}"/>
</svg>
'''


def generated_panel_svg(parser: configparser.ConfigParser) -> str:
    ox, oy = panel_offsets(parser)
    pw = to_px(parser.getfloat("panel", "width_mm"))
    ph = to_px(parser.getfloat("panel", "height_mm"))

    def pp(section: str) -> Point:
        return point_px(parser, section)

    display_x = ox + to_px(parser.getfloat("display", "x_mm"))
    display_y = oy + to_px(parser.getfloat("display", "y_mm"))
    display_w = to_px(parser.getfloat("display", "width_mm"))
    display_h = to_px(parser.getfloat("display", "height_mm"))
    encoder = pp("encoder")
    play, tap, stop = pp("play"), pp("tap"), pp("stop")
    sync, reset = pp("sync"), pp("reset")
    button_r = to_px(parser.getfloat("play", "actuator_diameter_mm")) * 0.5
    jack_nut_r = to_px(parser.getfloat("jack_ts", "nut_diameter_mm")) * 0.5
    jack_bush_r = to_px(parser.getfloat("jack_ts", "bushing_diameter_mm")) * 0.5
    jack_hole_r = to_px(parser.getfloat("jack_ts", "opening_diameter_mm")) * 0.5
    led_r = to_px(parser.getfloat("leds", "diameter_mm")) * 0.5
    knob_r = to_px(parser.getfloat("encoder", "knob_diameter_mm")) * 0.5

    outputs: list[tuple[int, Point, Point]] = []
    for index in range(1, 9):
        center = pp(f"output_{index}")
        led = Point(
            ox + to_px(parser.getfloat(f"output_{index}", "led_x_mm")),
            oy + to_px(parser.getfloat(f"output_{index}", "led_y_mm")),
        )
        outputs.append((index, center, led))

    screws = [pp(f"screw_{i}") for i in range(1, 5)]

    panel_center_x = ox + pw * 0.5
    brand_y = oy + to_px(123.15)
    button_label_y = play.y + button_r + to_px(1.25)
    input_label_y = sync.y + jack_nut_r + to_px(2.4)
    label_line_gap = to_px(2.25)

    # Reuse the canonical project-owned CLOCK wordmark instead of a text approximation.
    logo_width = to_px(33.0)
    logo_scale = logo_width / 128.0
    logo_height = 41.0 * logo_scale
    logo_x = panel_center_x - logo_width * 0.5
    logo_y = oy + to_px(0.8)
    logo_paths = clock_logo_paths()

    lines: list[str] = []
    a = lines.append
    a('<svg xmlns="http://www.w3.org/2000/svg" width="150" height="380" viewBox="0 0 150 380" role="img" aria-labelledby="title desc">')
    a('  <title id="title">South Signal Lab Clock functional VCV Rack panel</title>')
    a('  <desc id="desc">Black 10 HP South Signal Lab Clock panel generated from sim/panel_layout.ini. Positions and physical sizes follow the native simulator geometry.</desc>')
    a('  <!-- GENERATED from sim/panel_layout.ini by scripts/generate_vcv_panel.py. -->')
    a('  <defs><style>')
    a('    .label{font-family:DejaVu Sans,Arial,sans-serif;fill:#f4f4f4;font-size:7.2px;font-weight:650}.secondary{fill:#8d8f93;font-size:6.5px;font-weight:600}.input{font-size:7.2px;font-weight:650}.output{font-size:7.6px;font-weight:700}.brand{font-size:6.5px;font-weight:650;letter-spacing:.5px}')
    a('    .jackNut{fill:#d8d9db;stroke:#77797c;stroke-width:.8}.jackBush{fill:#afb1b4;stroke:#707276;stroke-width:.55}.jackHole{fill:#08090a}.led{fill:#111113;stroke:#4a2426;stroke-width:.45}.screw{fill:#a8aaad;stroke:#696b6e;stroke-width:.5}')
    a('  </style></defs>')
    a('  <rect width="150" height="380" fill="#050506"/>')
    a(f'  <rect x="{fmt(ox)}" y="{fmt(oy)}" width="{fmt(pw)}" height="{fmt(ph)}" fill="#0a0a0b" stroke="#343438" stroke-width="0.8"/>')
    a(f'  <g transform="translate({fmt(logo_x)} {fmt(logo_y)}) scale({fmt(logo_scale)})" fill="none" stroke="#f4f4f4" stroke-width="3" stroke-linecap="round" stroke-linejoin="round">')
    for path_data in logo_paths:
        a(f'    <path d="{path_data}"/>')
    a('  </g>')
    a(f'  <rect x="{fmt(display_x)}" y="{fmt(display_y)}" width="{fmt(display_w)}" height="{fmt(display_h)}" rx="1.2" fill="#050607" stroke="#525457" stroke-width="0.8"/>')

    for screw in screws:
        a(f'  <circle class="screw" cx="{fmt(screw.x)}" cy="{fmt(screw.y)}" r="2.0"/>')

    # Ghosted control silhouettes make the standalone SVG readable; real Rack widgets cover them.
    a(f'  <circle cx="{fmt(encoder.x)}" cy="{fmt(encoder.y)}" r="{fmt(knob_r)}" fill="#3a3b3e" stroke="#17181a" stroke-width="0.8"/>')
    for line1, line2, center, color in (("PLAY", "PAUSE", play, parser["play"]["color"]), ("TAP", "SHIFT", tap, parser["tap"]["color"]), ("STOP", "BACK", stop, parser["stop"]["color"])):
        a(f'  <circle cx="{fmt(center.x)}" cy="{fmt(center.y)}" r="{fmt(button_r)}" fill="{color}" stroke="#191a1c" stroke-width="0.8"/>')
        a(f'  <text class="label" x="{fmt(center.x)}" y="{fmt(button_label_y)}" text-anchor="middle">{line1}</text>')
        a(f'  <text class="label secondary" x="{fmt(center.x)}" y="{fmt(button_label_y + label_line_gap)}" text-anchor="middle">{line2}</text>')

    def jack(center: Point) -> None:
        a(f'  <circle class="jackNut" cx="{fmt(center.x)}" cy="{fmt(center.y)}" r="{fmt(jack_nut_r)}"/>')
        a(f'  <circle class="jackBush" cx="{fmt(center.x)}" cy="{fmt(center.y)}" r="{fmt(jack_bush_r)}"/>')
        a(f'  <circle class="jackHole" cx="{fmt(center.x)}" cy="{fmt(center.y)}" r="{fmt(jack_hole_r)}"/>')

    jack(sync)
    jack(reset)
    a(f'  <text class="label input" x="{fmt(sync.x)}" y="{fmt(input_label_y)}" text-anchor="middle">IN 1</text>')
    a(f'  <text class="label input" x="{fmt(reset.x)}" y="{fmt(input_label_y)}" text-anchor="middle">IN 2</text>')

    for index, center, led in outputs:
        a(f'  <circle class="led" cx="{fmt(led.x)}" cy="{fmt(led.y)}" r="{fmt(led_r)}"/>')
        jack(center)
        label_y = center.y + jack_nut_r + to_px(2.5)
        a(f'  <text class="label output" x="{fmt(center.x)}" y="{fmt(label_y)}" text-anchor="middle">{index}</text>')

    a(f'  <text class="label brand" x="{fmt(panel_center_x)}" y="{fmt(brand_y)}" text-anchor="middle">SOUTH SIGNAL LAB</text>')
    a('</svg>')
    return "\n".join(lines) + "\n"


def outputs(parser: configparser.ConfigParser) -> dict[Path, str]:
    button_size = to_px(parser.getfloat("play", "actuator_diameter_mm"))
    return {
        DEFAULT_PANEL: generated_panel_svg(parser),
        DEFAULT_HEADER: generated_header(parser),
        VCV_DIR / "res" / "button-play-0.svg": circle_svg(button_size, parser["play"]["color"], False),
        VCV_DIR / "res" / "button-play-1.svg": circle_svg(button_size, "#7A1616", True),
        VCV_DIR / "res" / "button-tap-0.svg": circle_svg(button_size, parser["tap"]["color"], False),
        VCV_DIR / "res" / "button-tap-1.svg": circle_svg(button_size, "#4F4F4F", True),
        VCV_DIR / "res" / "button-stop-0.svg": circle_svg(button_size, parser["stop"]["color"], False),
        VCV_DIR / "res" / "button-stop-1.svg": circle_svg(button_size, "#090909", True),
    }


def main() -> int:
    argp = argparse.ArgumentParser()
    argp.add_argument("--layout", type=Path, default=DEFAULT_LAYOUT)
    argp.add_argument("--check", action="store_true")
    args = argp.parse_args()

    parser = read_layout(args.layout)
    generated = outputs(parser)
    if args.check:
        stale = [path for path, content in generated.items() if not path.exists() or path.read_text(encoding="utf-8") != content]
        if stale:
            raise SystemExit("stale generated VCV panel files: " + ", ".join(str(p.relative_to(ROOT)) for p in stale))
        return 0

    for path, content in generated.items():
        path.parent.mkdir(parents=True, exist_ok=True)
        path.write_text(content, encoding="utf-8")
        print(path.relative_to(ROOT))
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
