<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# 5 Read the Performance screen

The Performance screen is deliberately sparse. It keeps the information required while playing visible and moves configuration detail into contextual pages.

![Independent Clock performance screen while playing: master badge, selected channel and Clock pictogram, 4/4 meter, PLAY state, centered BPM, and only relevant non-default timing modifiers.](../manual-source/assets/performance-independent-clock-play.png)

The header shows:

- **filled `M`** when CLOCK is the active master;
- **outlined `S`** when CLOCK follows external timing;
- a lock icon beside `S` when external timing is locked;
- the selected channel and compact mode pictogram;
- master meter in the center;
- transport state at the right.

The BPM numerals are mathematically centered. Non-zero Swing appears at the left of the tempo area and a non-`×1` rate appears at the right. When Groove is active, `G:<name>` appears lower-left for Clock/One Clock and directly above the live pattern strip for Euclid/Sequencer. Factory presets show their preset name; Custom Grooves show the actual stored user name. Default or ineffective values are omitted rather than filling the screen with redundant status.

Clock uses the larger BPM role. Euclid and Sequencer use a smaller tempo role because the lower display area is reserved for live pattern feedback.

### Euclid performance view

![Independent Euclid performance screen while playing, with a live step strip along the bottom: filled cells are hits, outlined cells are rests, and the underline marks the current step.](../manual-source/assets/performance-independent-euclid-play.png)

The strip is rendered from the same Euclidean pattern data used by the scheduler, so the display is not a decorative approximation of the rhythm.

### Sequencer performance view

![Independent 64-step Sequencer performance screen while playing, with the active 16-step gate block and a lower block indicator showing which 16-step segment of the longer pattern is active.](../manual-source/assets/performance-independent-sequencer-play.png)

For lengths above 16 steps, the lowest display rows show the active 16-step block. A 64-step sequence therefore exposes four logical display segments.

### STOP

![Independent Clock performance screen in STOP, with transport stopped and no beat animation.](../manual-source/assets/performance-independent-clock-stop.png)

With factory display preferences, STOP-mode inactivity starts the configured screensaver after 2 minutes, dims the OLED after 5 minutes, and powers the OLED panel off after 10 minutes. Any front-panel interaction or conditioned SYNC/RST transition wakes it immediately.

<h6 align="center">From Munich with &#9829;</h6>
