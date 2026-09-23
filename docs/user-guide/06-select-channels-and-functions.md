<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 6 Select channels and change functions

In Independent topology, a short encoder push opens the eight-channel overview. All channels remain visible in a 2×4 grid; each tile contains only its number and mode pictogram. The selected tile is fully inverted.

![Independent channel overview showing all eight channels at once in a 2×4 grid, with the selected channel fully inverted.](../manual-source/assets/channel-overview-independent.png)

Turn the encoder to move the highlight. Short press commits the highlighted channel and returns to Performance. Long press opens that channel's settings directly.

One Clock and Divider Bank are global topologies, so their overview deliberately does not pretend that eight independent channel tiles exist.

<table>
<tr>
<td align="center"><img src="../manual-source/assets/channel-overview-one-clock.png" alt="One Clock global overview showing the large One Clock pictogram and mode name." width="256"><br><sub><b>One Clock:</b> one shared configuration drives all outputs.</sub></td>
<td align="center"><img src="../manual-source/assets/channel-overview-divider-bank.png" alt="Divider Bank global overview showing the large Divider Bank pictogram and mode name." width="256"><br><sub><b>Divider Bank:</b> one source feeds the eight fixed divider outputs.</sub></td>
</tr>
</table>

Short press returns from a global overview. Long press opens that topology's settings.

Hold **TAP** and turn the encoder. CLOCK opens a horizontal mode carousel. The selected function stays centered and is the only item drawn on an inverted background; its immediate neighbors remain unframed, with a pictogram and label underneath. Turning moves the band rather than moving a highlight through a fixed 2×3 tile grid.

The current catalog order is **One Clock → Divider Bank → Clock → Euclid → Sequencer → Off**. The renderer and navigation use this catalog directly, so future channel modes can be added without inventing another fixed palette layout.

One Clock and Divider Bank change the global output topology. Clock, Euclid, Sequencer, and Off select Independent topology for the highlighted channel.

<table>
<tr>
<td align="center"><img src="../manual-source/assets/mode-select-one-clock.png" alt="Horizontal mode carousel with One Clock centered and selected." width="220"><br><sub>One Clock centered</sub></td>
<td align="center"><img src="../manual-source/assets/mode-select-euclid.png" alt="Horizontal mode carousel with Euclid centered and selected." width="220"><br><sub>Euclid centered</sub></td>
<td align="center"><img src="../manual-source/assets/mode-select-divider-bank.png" alt="Horizontal mode carousel with Divider Bank centered and selected." width="220"><br><sub>Divider Bank centered</sub></td>
</tr>
</table>

Releasing TAP does not silently mutate the running setup. If the highlighted function differs from the active function, CLOCK opens `CHANGE MODE?` with **NO** selected by default.

![Mode-change confirmation dialog showing CHANGE MODE? with NO selected as the safe default.](../manual-source/assets/mode-change-confirm-no.png)

After confirmation CLOCK opens the most useful destination: Euclid goes to algorithm settings, Sequencer to its editor, One Clock to shared settings, Divider Bank to divider-family settings, while Clock and Off return to Performance.

<h6 align="center">From Munich with &#9829;</h6>
