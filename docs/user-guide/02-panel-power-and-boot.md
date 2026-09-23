<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 2 Panel, power and boot

![Numbered CLOCK front-panel illustration showing the OLED at the upper left, encoder at the upper right, PLAY/TAP/STOP buttons below them, SYNC and RST inputs in the middle, and eight output jacks with red activity LEDs in a 4×2 matrix.](../manual-source/assets/front-panel-anatomy.svg)

| No. | Element | Function |
| ---: | --- | --- |
| 1 | OLED | Performance view, menus, editors, prompts, and status feedback |
| 2 | Push encoder | Turn to select/change values; short press to select/confirm; long press (~650 ms) opens the current context settings |
| 3 | PLAY | Start/pause master transport; next page in the Sequencer editor |
| 4 | TAP | Tap Tempo; modifier for Settings and mode selection; previous page in the Sequencer editor |
| 5 | STOP / BACK | Stop and reset global phase from Performance; back/cancel elsewhere |
| 6 | SYNC IN | Conditioned external timing input |
| 7 | RST IN | Conditioned external reset/phase input |
| 8 | OUT 1–8 | Eight clock/gate outputs |
| 9 | Activity LEDs | One red indicator per output |

The panel layout in this illustration is generated from [`sim/panel_layout.ini`](../../sim/panel_layout.ini), the same geometry used by the desktop simulator. The drawing is an explanatory manual asset, not a drill or manufacturing template.

CLOCK always powers up in **STOP**. Stored configuration is restored, but a previously saved PLAY state is never allowed to start outputs automatically.

The boot sequence initializes the scheduler in STOP and keeps all eight gate source GPIOs LOW until startup is complete. Because the final pin map has no MCU-controlled shared `/OE`, firmware safety is enforced directly at the eight source GPIOs. This prevents a normal power-up from becoming eight accidental triggers in a patched rack.

![Boot screen halfway through its one-second progress sequence, showing the CLOCK wordmark and the two-pixel progress bar at the bottom.](../manual-source/assets/boot-500.png)

The normal boot screen lasts about one second. Holding the encoder push continuously through the complete boot sequence enters the compile-time selected Easter egg; see [Section 21](#21-hidden-boot-easter-eggs).

<h6 align="center">From Munich with &#9829;</h6>
