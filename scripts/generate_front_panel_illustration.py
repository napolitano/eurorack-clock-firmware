#!/usr/bin/env python3
# Author: Axel Napolitano
# License: PolyForm-Noncommercial-1.0.0
"""Generate the numbered CLOCK front-panel illustration used by the manual.

Geometry is read from sim/panel_layout.ini so documentation follows the
simulator/mechanical layout. The result is an SVG documentation illustration,
not a manufacturing drawing.
"""
from __future__ import annotations

import argparse
import configparser
from pathlib import Path
from xml.sax.saxutils import escape

ROOT = Path(__file__).resolve().parents[1]
DEFAULT_LAYOUT = ROOT / "sim" / "panel_layout.ini"
DEFAULT_OUTPUT = ROOT / "docs" / "manual" / "assets" / "front-panel-anatomy.svg"
ACCENT = "#0B4FC0"  # existing CLOCK manual accent


def f(v: float) -> str:
    return f"{v:.2f}".rstrip("0").rstrip(".")


def load(path: Path):
    c = configparser.ConfigParser(interpolation=None)
    c.read(path, encoding="utf-8")
    return c


def generate(layout_path: Path) -> str:
    c = load(layout_path)
    pw = c.getfloat("panel", "width_mm")
    ph = c.getfloat("panel", "height_mm")

    # Drawing coordinate system: 4 px per physical mm.
    scale = 4.0
    panel_w = pw * scale
    panel_h = ph * scale
    panel_x = 320.0
    panel_y = 44.0
    vb_x = 235.0
    vb_y = 30.0
    vb_w = 430.0
    vb_h = 545.0

    def xy(section: str):
        return (
            panel_x + c.getfloat(section, "x_mm") * scale,
            panel_y + c.getfloat(section, "y_mm") * scale,
        )

    def mm(v: float) -> float:
        return v * scale

    display_x = panel_x + c.getfloat("display", "x_mm") * scale
    display_y = panel_y + c.getfloat("display", "y_mm") * scale
    display_w = c.getfloat("display", "width_mm") * scale
    display_h = c.getfloat("display", "height_mm") * scale
    enc_x, enc_y = xy("encoder")
    knob_r = mm(c.getfloat("encoder", "knob_diameter_mm")) / 2

    play = xy("play")
    tap = xy("tap")
    stop = xy("stop")
    button_r = mm(c.getfloat("play", "actuator_diameter_mm")) / 2

    sync = xy("sync")
    reset = xy("reset")
    jack_r = mm(c.getfloat("jack_ts", "nut_diameter_mm")) / 2
    bush_r = mm(c.getfloat("jack_ts", "bushing_diameter_mm")) / 2
    hole_r = mm(c.getfloat("jack_ts", "opening_diameter_mm")) / 2
    led_r = mm(c.getfloat("leds", "diameter_mm")) / 2

    outs = []
    for i in range(1, 9):
        s = f"output_{i}"
        x, y = xy(s)
        lx = panel_x + c.getfloat(s, "led_x_mm") * scale
        ly = panel_y + c.getfloat(s, "led_y_mm") * scale
        outs.append((i, x, y, lx, ly))

    screws = [xy(f"screw_{i}") for i in range(1, 5)]

    # Callouts mimic the Quantizer/Drift manual style: simple leader + numbered disc.
    # Targets are based on real element geometry, while label positions are layout-only.
    callouts = [
        (1, "left",  display_x + 4, display_y + display_h * 0.52, 278, display_y + display_h * 0.52),
        (2, "right", enc_x + knob_r, enc_y, 610, enc_y),
        (3, "left",  play[0] - button_r, play[1], 278, play[1]),
        (4, "right", tap[0] + button_r * 0.65, tap[1] + button_r * 0.65, 610, tap[1] + 30),
        (5, "right", stop[0] + button_r, stop[1], 610, stop[1]),
        (6, "left",  sync[0] - jack_r, sync[1], 278, sync[1]),
        (7, "right", reset[0] + jack_r, reset[1], 610, reset[1]),
        (8, "left",  outs[4][1] - jack_r, outs[4][2], 278, outs[4][2]),
        (9, "right", outs[3][3] + led_r, outs[3][4], 610, outs[3][4]),
    ]

    s = []
    a = s.append
    a(f'''<svg xmlns="http://www.w3.org/2000/svg" viewBox="{f(vb_x)} {f(vb_y)} {f(vb_w)} {f(vb_h)}" role="img" aria-labelledby="title desc">''')
    a(f'  <rect x="{f(vb_x)}" y="{f(vb_y)}" width="{f(vb_w)}" height="{f(vb_h)}" fill="#fff"/>')
    a('  <title id="title">CLOCK front-panel controls</title>')
    a('  <desc id="desc">Numbered vector illustration of the CLOCK Eurorack front panel. Element positions are generated from the simulator panel layout.</desc>')
    a('  <!-- Generated from sim/panel_layout.ini; documentation illustration, not a manufacturing drawing. -->')
    a('  <defs>')
    a('    <style>')
    a('      .panel{fill:#fbfbfb;stroke:#202020;stroke-width:2.4}.ink{stroke:#202020;stroke-width:2.2;fill:none}.thin{stroke:#202020;stroke-width:1.4;fill:none}')
    a(f'      .accentLine{{stroke:{ACCENT};stroke-width:1.8;fill:none}}.accentDisc{{fill:{ACCENT}}}')
    a('      .panelText{font-family:Ubuntu,Arial,sans-serif;fill:#171717}.title{font-size:19px;font-weight:500;letter-spacing:.7px}.label{font-size:7.5px;font-weight:500;letter-spacing:.25px}.tiny{font-size:6px;font-weight:400;fill:#555}.num{font-family:Ubuntu,Arial,sans-serif;font-size:11px;font-weight:700;fill:white;text-anchor:middle;dominant-baseline:middle}')
    a('      .screen{fill:#111;stroke:#202020;stroke-width:1.6}.screenText{font-family:Ubuntu Mono,monospace;fill:#fff}.jackNut{fill:#fafafa;stroke:#202020;stroke-width:2}.jackBush{fill:#eee;stroke:#202020;stroke-width:1.3}.jackHole{fill:#fff;stroke:#202020;stroke-width:1.5}.led{fill:#e54b4b;stroke:#b83232;stroke-width:.8}.buttonEdge{stroke:#202020;stroke-width:2}')
    a('    </style>')
    a('  </defs>')

    # Panel.
    a(f'  <rect class="panel" x="{f(panel_x)}" y="{f(panel_y)}" width="{f(panel_w)}" height="{f(panel_h)}" rx="5"/>')
    for x, y in screws:
        a(f'  <circle cx="{f(x)}" cy="{f(y)}" r="3.3" fill="#fff" stroke="#202020" stroke-width="1.6"/>')
    a(f'  <text class="panelText title" x="{f(panel_x+panel_w/2)}" y="{f(panel_y+22)}" text-anchor="middle">CLOCK</text>')

    # OLED with a representative vectorised performance screen.
    a(f'  <rect class="screen" x="{f(display_x)}" y="{f(display_y)}" width="{f(display_w)}" height="{f(display_h)}" rx="2.5"/>')
    sx = display_x; sy = display_y
    a(f'  <polygon points="{f(sx+8)},{f(sy+8)} {f(sx+8)},{f(sy+16)} {f(sx+15)},{f(sy+12)}" fill="#fff"/>')
    a(f'  <rect x="{f(sx+display_w-18)}" y="{f(sy+6)}" width="10" height="8" rx="1" fill="none" stroke="#fff" stroke-width="1"/>')
    a(f'  <text class="screenText" x="{f(sx+display_w-13)}" y="{f(sy+12.2)}" text-anchor="middle" font-size="6">M</text>')
    a(f'  <text class="screenText" x="{f(sx+display_w/2)}" y="{f(sy+display_h*0.63)}" text-anchor="middle" font-size="25" font-weight="700">120</text>')
    a(f'  <text class="screenText" x="{f(sx+7)}" y="{f(sy+display_h-7)}" font-size="7">CH 1</text>')
    a(f'  <text class="screenText" x="{f(sx+display_w-7)}" y="{f(sy+display_h-7)}" text-anchor="end" font-size="7">×1</text>')

    # Encoder.
    a(f'  <circle cx="{f(enc_x)}" cy="{f(enc_y)}" r="{f(knob_r+2)}" fill="#f5f5f5" stroke="#202020" stroke-width="1.5"/>')
    a(f'  <circle cx="{f(enc_x)}" cy="{f(enc_y)}" r="{f(knob_r)}" fill="#2b2b2b" stroke="#111" stroke-width="1.5"/>')
    a(f'  <line x1="{f(enc_x)}" y1="{f(enc_y-knob_r+2)}" x2="{f(enc_x)}" y2="{f(enc_y-2)}" stroke="#fff" stroke-width="1.5"/>')
    a(f'  <text class="panelText tiny" x="{f(enc_x)}" y="{f(enc_y+knob_r+11)}" text-anchor="middle">ENCODER</text>')

    # Transport buttons.
    for (name, (x, y), color) in (("PLAY", play, c["play"]["color"]), ("TAP", tap, c["tap"]["color"]), ("STOP", stop, c["stop"]["color"])):
        a(f'  <circle class="buttonEdge" cx="{f(x)}" cy="{f(y)}" r="{f(button_r)}" fill="{escape(color)}"/>')
        # small highlight arc / dot to retain simplified illustrated feel
        a(f'  <circle cx="{f(x-2.3)}" cy="{f(y-2.3)}" r="1.2" fill="#fff" fill-opacity=".55"/>')
        a(f'  <text class="panelText label" x="{f(x)}" y="{f(y+button_r+10)}" text-anchor="middle">{name}</text>')

    # Input jacks.
    def jack(x, y):
        a(f'  <circle class="jackNut" cx="{f(x)}" cy="{f(y)}" r="{f(jack_r)}"/>')
        a(f'  <circle class="jackBush" cx="{f(x)}" cy="{f(y)}" r="{f(bush_r)}"/>')
        a(f'  <circle class="jackHole" cx="{f(x)}" cy="{f(y)}" r="{f(hole_r)}"/>')
    jack(*sync); jack(*reset)
    a(f'  <text class="panelText label" x="{f(sync[0])}" y="{f(sync[1]+jack_r+10)}" text-anchor="middle">SYNC</text>')
    a(f'  <text class="panelText label" x="{f(reset[0])}" y="{f(reset[1]+jack_r+10)}" text-anchor="middle">RST</text>')

    # Outputs and LEDs.
    for i, x, y, lx, ly in outs:
        a(f'  <circle class="led" cx="{f(lx)}" cy="{f(ly)}" r="{f(led_r)}"/>')
        jack(x, y)
        a(f'  <text class="panelText tiny" x="{f(x)}" y="{f(y+jack_r+9)}" text-anchor="middle">{i}</text>')

    a(f'  <text class="panelText tiny" x="{f(panel_x+panel_w/2)}" y="{f(panel_y+panel_h-10)}" text-anchor="middle" letter-spacing="1.2">SOUTH SIGNAL LAB</text>')

    # Callout leaders and numbered discs. Line terminates at disc edge rather than through it.
    disc_r = 12.5
    for n, side, tx, ty, cx, cy in callouts:
        if side == "left":
            line_end = cx + disc_r
            a(f'  <path class="accentLine" d="M {f(tx)} {f(ty)} L {f(line_end)} {f(cy)}"/>')
        else:
            line_end = cx - disc_r
            a(f'  <path class="accentLine" d="M {f(tx)} {f(ty)} L {f(line_end)} {f(cy)}"/>')
        a(f'  <circle class="accentDisc" cx="{f(cx)}" cy="{f(cy)}" r="{f(disc_r)}"/>')
        a(f'  <text class="num" x="{f(cx)}" y="{f(cy+.5)}">{n}</text>')

    a('</svg>')
    return "\n".join(s) + "\n"


def main() -> int:
    p = argparse.ArgumentParser()
    p.add_argument("--layout", type=Path, default=DEFAULT_LAYOUT)
    p.add_argument("--output", type=Path, default=DEFAULT_OUTPUT)
    p.add_argument("--check", action="store_true")
    args = p.parse_args()
    svg = generate(args.layout)
    if args.check:
        if not args.output.exists() or args.output.read_text(encoding="utf-8") != svg:
            raise SystemExit(f"front-panel illustration is stale: {args.output}")
        return 0
    args.output.parent.mkdir(parents=True, exist_ok=True)
    args.output.write_text(svg, encoding="utf-8")
    print(args.output)
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
