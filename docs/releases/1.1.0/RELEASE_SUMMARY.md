<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK 1.1.0

CLOCK 1.1.0 is the first musical feature release after the stable 1.0 line. It keeps the same STM32F401 hardware and deterministic master-timeline architecture, but turns microtiming into a much more deliberate performance tool through Grooves, Custom Groove editing and live TAP recording. It also adds Pre-Count and configurable roles for the two conditioned digital inputs.

## Highlights

- **Grooves are now a first-class timing layer:** factory `SWING 54 / 58 / 62 / 66` and `POCKET A / B / C` patterns provide deterministic microtiming with Amount and Rotate controls. Humanize remains a separate stochastic layer rather than being folded into Groove timing.
- **Custom Grooves can be drawn or played in:** the 1-64-step graphical editor supports signed early/late timing and live preview, while `RECORD >` captures high-resolution TAP timing against the real Q32 engine position with independent Count-In, One-Shot/Endless policies and a live playhead.
- **Ten named Custom Groove slots are persistent:** save, load, overwrite, rename and delete all use the same CRC-protected store, and a recorded Groove can move directly into the editor before it is committed.
- **Pre-Count is built into transport:** `OFF / 1-64` beats can precede a fresh STOP-to-PLAY start while all gates remain LOW and musical pattern phase stays at zero; external Pre-Count follows the configured master-meter beat unit.
- **The two conditioned inputs are configurable:** the physical SYNC/PA8 and RST/PA9 paths can be assigned globally to `OFF / SYNC / RESET / RUN / START / STOP / RESTART / TAP`, with factory roles remaining SYNC and RESET and active-role uniqueness enforced.
- **The small-screen UI is easier to navigate:** Channel Mode selection uses a horizontal centered carousel, Settings use full-row selection, long read-only values can open a full-value popover, and hardware-local encoder/orientation options have their own Settings page.
- **Release qualification remains explicit:** the repository exposes 496 named Native tests with the 95/95/90 coverage gates intact; Groove and Recorder bench vectors are tracked in the HIL ledger but remain pending physical measurements under the advisory-through-1.4.x HIL policy.

## Firmware updates

Routine USB updates continue to use persistence-preserving DfuSe release images and leave the A/B persistence sectors untouched. Disconnect Eurorack power before connecting USB. ST-LINK/SWD remains the recommended first-install and recovery path.

## Compatibility

CLOCK 1.1.0 keeps the existing Rev-1 GPIO and electrical hardware contract. Persistence advances to schema v12 and explicitly migrates the supported older formats, including stable 1.0.x schema v8. The 8-KiB logical image remains fixed: ten 96-byte Custom Groove records use the previously free lower-image range at bytes 3136-4095 without moving legacy score data or the Top-100 region. Existing CURRENT state, named presets and score/leaderboard data remain inside the established persistence layout and are preserved by routine release DFU updates.
