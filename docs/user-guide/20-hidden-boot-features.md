<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 20 Hidden boot features

Hold the encoder push continuously from power-up until the boot screen finishes to enter the compile-time selected Easter egg. `CLOCK_EASTER_EGG` selects one of five implementations; the shipped/default build selects **BEATKNECHT**.

The four ranked games — **Pixel Raid, Formula 1, Breakout, and Egg Journey** — share the same presentation flow: game-specific intro, gameplay, optional three-letter initials entry for a qualifying score, and a scrollable Top 100. A long encoder hold exits back to the normal firmware lifecycle.

<table>
<tr>
<td align="center"><img src="../manual-source/assets/pixel-raid-intro.png" alt="Pixel Raid retro intro with its game-specific pixel motif and scrolling marquee." width="220"><br><sub>Pixel Raid — ranked arcade shooter</sub></td>
<td align="center"><img src="../manual-source/assets/formula-1-intro.png" alt="Formula 1 retro intro with its game-specific pixel motif and scrolling marquee." width="220"><br><sub>Formula 1 — ranked driving game</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/breakout-intro.png" alt="Breakout retro intro with its game-specific pixel motif and scrolling marquee." width="220"><br><sub>Breakout — ranked brick game</sub></td>
<td align="center"><img src="../manual-source/assets/egg-journey-intro.png" alt="Egg Journey retro intro with its game-specific pixel motif and scrolling marquee." width="220"><br><sub>Egg Journey — ranked lunar runner</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/beatknecht-intro.png" alt="BEATKNECHT retro intro shown before the eight-channel rhythm utility starts." width="220"><br><sub>BEATKNECHT — eight-channel rhythm utility</sub></td>
<td align="center"><img src="../manual-source/assets/arcade-top-100.png" alt="Shared scrollable Top 100 presentation used by the four ranked arcade games." width="220"><br><sub>Top 100 — shared ranked leaderboard</sub></td>
</tr>
</table>

- **Pixel Raid** — encoder moves the cannon; TAP fires.
- **Formula 1** — encoder steers through changing road geometry and traffic; three crashes end the run.
- **Breakout** — encoder moves the paddle; TAP launches the waiting ball; the run has three lives and scored bricks/board clears.
- **Egg Journey** — encoder shifts the egg within the scrolling lunar landscape; TAP jumps; craters and asteroids consume one of three lives.
- **BEATKNECHT** — not a ranked game. **PLAY/PAUSE** starts and pauses the rhythm, **STOP/BACK** stops it and resets the pattern to step 1, TAP cycles curated one-bar rhythm styles, and the encoder changes BPM. OUT 1–8 intentionally emit the displayed eight gate patterns. Pause forces every gate LOW while preserving the remaining step interval; PLAY resumes from that point. STOP additionally mutes all gate source GPIOs until PLAY is pressed again. A long encoder push opens an **EXIT GAME? / NO / YES** confirmation instead of leaving immediately. NO returns to the exact previous transport state; if the rhythm was playing it resumes from the preserved phase. YES stops the rhythm, forces all gates LOW, keeps gate output muted, and returns to the normal clock firmware.

After a ranked Easter egg has been launched at least once, the Settings root gains **HI-SCORES / CLEAR**. The entry is hidden beforehand and is not shown for BEATKNECHT. Clearing uses a guarded **NO / YES** confirmation, stops transport before Flash is written, clears the complete Top 100 for that selected ranked game, and keeps the menu entry available for later resets.

<table>
<tr>
<td align="center"><img src="../manual-source/assets/formula-1-crash.png" alt="Formula 1 crash frame showing the visible collision burst used during recovery." width="220"><br><sub>Formula 1 crash recovery</sub></td>
<td align="center"><img src="../manual-source/assets/breakout-modifier.png" alt="Breakout action frame with the enlarged paddle and a falling speed modifier." width="220"><br><sub>Breakout modifier</sub></td>
</tr>
<tr>
<td align="center"><img src="../manual-source/assets/egg-journey.png" alt="Egg Journey gameplay frame showing the jumping egg, lunar terrain, and an incoming asteroid." width="220"><br><sub>Egg Journey gameplay</sub></td>
<td align="center"><sub><b>Leaderboard management</b><br>After a ranked Easter egg has been launched once, Settings exposes a guarded HI-SCORES / CLEAR action.</sub></td>
</tr>
</table>

Pixel Raid, Formula 1, Breakout, and Egg Journey keep all gate source GPIOs muted. BEATKNECHT is the deliberate exception: its intro is started with PLAY, it allows gate HIGH requests only while transport is PLAYING, drives the rhythm gates, forces all channels LOW on PAUSE, and returns all channels LOW plus mutes gate output on STOP or confirmed exit. Opening the exit confirmation also silences the gates; cancelling restores the prior PLAY/PAUSE/STOP state. Normal firmware resumes in STOP.

<h6 align="center">From Munich with &#9829;</h6>
