<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 19 Gate outputs and LEDs

CLOCK has eight digital gate/clock outputs with one activity LED per output. The target output level is nominally **0 / +5 V** through the HCT output stage. The LED follows the same logical source event but is not part of the timing decision itself.

The eight channels remain deterministic even when they fire at the same musical instant. The embedded scheduler owns gate timing; OLED drawing, menus and persistence do not generate musical edges.

### Gate length

Gate length is selected in milliseconds: **1 / 2 / 5 / 10 / 20 / 50 / 100 ms**. This is a fixed requested HIGH time, not a duty-cycle setting. If a fast rate would schedule the next rising edge sooner, CLOCK bounds the current pulse so the next onset is preserved. Timing requests are resolved on the 50 µs scheduler service quantum.

### LEDs and diagnostics

The front-panel LEDs are activity indicators for OUT 1–8. When troubleshooting, `GENERAL SETTINGS → DIAGNOSTICS → OUTPUTS` shows the eight logical gate-source states independently of the external patch. This helps distinguish a configuration/timing problem from wiring or downstream-module behavior.

CLOCK always boots in STOP and keeps the eight gate-source GPIOs LOW through initialization. The final hardware has no MCU-controlled common output-enable line; safe startup is therefore enforced directly at the source GPIOs.

<h6 align="center">From Munich with &#9829;</h6>
