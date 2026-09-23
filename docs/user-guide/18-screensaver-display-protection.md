<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 18 Screensaver and display protection

`SETTINGS → GENERAL SETTINGS → HARDWARE` contains the two persistent installation preferences:

- **ENCODER DIR** — `NORMAL / REVERSED`. `REVERSED` flips the user-facing rotary direction after quadrature decoding; detent recovery, bounce handling and Fast Turn buffering are unchanged.
- **ORIENTATION** — `0 DEG / 180 DEG`. Firmware rotates the complete 128×64 transfer framebuffer before sending it to the OLED, so the boot screen, settings, screensavers and Easter eggs remain readable with the module mounted upside down. The controller itself stays in the proven `A1/C8` scan orientation; this avoids the mirrored-text behavior seen with controller-remap rotation on interchangeable SSD1306/SSD1315 modules.

<p align="center"><img src="../manual-source/assets/settings-hardware.png" alt="HARDWARE settings page with encoder direction and display orientation." width="220"><br><sub>Device-local hardware preferences</sub></p>

Both preferences take effect immediately and survive power cycling. They are device-local: loading a named preset or applying a factory template does not change them. Factory defaults are `NORMAL` and `0 DEG`.

### Hardware diagnostics

Open `SETTINGS → GENERAL SETTINGS → DIAGNOSTICS` for a live digital signal view. `INPUTS` shows the two conditioned comparator levels as centered `INPUT 1` and `INPUT 2` indicators, independent of their currently assigned musical roles. `OUTPUTS` shows channels 1–8 as a 4×2 grid of rectangular indicators. An inactive signal is shown as an outlined rectangle; an active signal is filled with its label inverted.

The input page reports the digital comparator levels seen by the MCU; the output page reports the digital source levels actually written by firmware to the eight gate-output GPIOs. The current hardware does not provide ADC voltage measurements at these points, so Diagnostics deliberately does not display inferred voltage values.

Display protection is active only while transport is STOP. The factory timing is:

- screensaver after 2 minutes;
- dim after 5 minutes;
- OLED off after 10 minutes.

The configured order is constrained to `START <= DIM <= OFF`. Any front-panel activity wakes the OLED immediately. `OFF` disables animation but does not disable the later dim/panel-off protection stages.

Four additional visual modes are available: **MAKE MUSIC** builds `MAKE·MUSIC·NOT·WAR` one character at a time and continues row by row; **LABYRINTH** repeatedly generates and draws a random perfect maze; **STARFIELD** scrolls three parallax depth layers; and **FIREWORKS** launches a pixel rocket from changing positions before an upper-screen burst.

![Screensaver settings page showing the selected animation and the STOP-mode start, dim, and OLED-off timing values.](../manual-source/assets/settings-screensaver.png)

<table>
<tr>
<td align="center"><img src="../manual-source/assets/screensaver-clock.png" alt="CLOCK screensaver with eight independently phased oscilloscope-style digital traces." width="220"><br><sub>CLOCK — eight phased digital traces</sub></td>
<td align="center"><img src="../manual-source/assets/screensaver-plug.png" alt="PLUG screensaver showing a damped plucked string between fixed endpoints." width="220"><br><sub>PLUG — damped plucked string</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/screensaver-heartbeat.png" alt="HEARTBEAT screensaver showing the animated heart pulse." width="220"><br><sub>HEARTBEAT — animated pulse</sub></td>
<td align="center"><img src="../manual-source/assets/screensaver-acid.png" alt="ACID screensaver showing the rotating gravity-driven bouncing smiley." width="220"><br><sub>ACID — bouncing smiley physics</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/screensaver-spectrum.png" alt="SPECTRUM screensaver showing the synthetic segmented spectrum bars and peak markers." width="220"><br><sub>SPECTRUM — 16-band spectrum with peaks</sub></td>
<td align="center"><img src="../manual-source/assets/screensaver-field.png" alt="FIELD screensaver showing dense moving monochrome scalar-field contours." width="220"><br><sub>FIELD — moving metaball field</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/screensaver-blox.png" alt="BLOX screensaver showing falling triangle-built bodies accumulating on the display." width="220"><br><sub>BLOX — falling geometric bodies</sub></td>
<td align="center"><img src="../manual-source/assets/screensaver-matrix.png" alt="MATRIX screensaver showing original monochrome procedural digital rain." width="220"><br><sub>MATRIX — procedural digital rain</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/screensaver-cube-cover.png" alt="CUBE COVER screensaver filling the OLED with small cube tiles." width="220"><br><sub>CUBE COVER — tiled cube fill</sub></td>
<td align="center"><img src="../manual-source/assets/screensaver-fractal.png" alt="FRACTAL screensaver progressively revealing a curated Barnsley-fern crop." width="220"><br><sub>FRACTAL — progressive fern reveal</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/screensaver-orbit.png" alt="ORBIT screensaver showing the sparse animated orbital display." width="220"><br><sub>ORBIT — sparse orbital motion</sub></td>
<td align="center"><img src="../manual-source/assets/screensaver-make-music.png" alt="MAKE MUSIC screensaver progressively building the MAKE MUSIC NOT WAR text stream with mid-dot separators." width="220"><br><sub>MAKE MUSIC — progressive text stream</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/screensaver-labyrinth.png" alt="LABYRINTH screensaver showing a generated perfect maze." width="220"><br><sub>LABYRINTH — generated maze</sub></td>
<td align="center"><img src="../manual-source/assets/screensaver-starfield.png" alt="STARFIELD screensaver showing three layers of parallax stars." width="220"><br><sub>STARFIELD — three-depth parallax</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/screensaver-fireworks.png" alt="FIREWORKS screensaver showing an upper-screen pixel burst." width="220"><br><sub>FIREWORKS — pixel rocket and burst</sub></td>
<td align="center"><img src="../manual-source/assets/power-off.png" alt="OLED OFF display-protection state with the panel blank." width="220"><br><sub>OLED OFF — final display-protection stage</sub></td>
</tr>
</table>

<h6 align="center">From Munich with &#9829;</h6>
