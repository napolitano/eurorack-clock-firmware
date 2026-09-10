<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Changelog

All notable prototype changes are documented here.

This project is still in active development. Until the first stable release, version numbers primarily identify development milestones rather than a compatibility promise.



## [0.19.0-beta.3] - 2026-09-10

### HIL release-gate staging

- Changed physical HIL qualification from a hard `1.0.0-rc.*`/`1.0.0` release-build gate to an advisory qualification signal through firmware `1.4.x`. Open `PENDING`/`BLOCKED` bench items remain visible but no longer prevent current or early 1.x release builds.
- Scheduled the hard all-PASS HIL release guarantee for `1.5.0`: release candidates and stable releases at or beyond `1.5.0` require every normative HIL item to be `PASS` with repository evidence. Alpha/beta development builds remain advisory so bench work cannot block development snapshots.
- Replaced the old `--require-pass-if-rc` workflow contract with `--require-pass-from 1.5.0` and added SemVer-aware enforcement tests across pre-1.5 stable/RC versions, 1.5+ release candidates/stable versions, and alpha/beta prereleases.
- Kept structural HIL validation active in every build. Missing normative tests, invalid statuses, fabricated PASS states, or missing PASS evidence still fail CI; only incomplete physical qualification is temporarily non-blocking.
- Updated README, roadmap, HIL plan, qualification contract and user documentation to distinguish visible/advisory HIL evidence from the future hard release gate.

### Compatibility

- No production clock-engine, persistence-schema, UI or musical-behavior changes in beta.3.

## [0.19.0-beta.2] - 2026-09-10

### V1 qualification infrastructure

- Converted the physical V1 HIL phase from a prose-only checklist into a machine-auditable qualification workflow. `docs/qualification/v1_qualification.json` now tracks every normative HIL test from `docs/HIL_TEST_PLAN.md` as PASS/PENDING/BLOCKED/FAIL and refuses PASS without repository evidence.
- Added `docs/qualification/V1_ACCEPTANCE.md`. Scheduler-derived V1 timing limits are now explicit instead of using undefined phrases such as "within release tolerance": ordinary gate-width error, simultaneous eight-output skew, phase error, and display-correlated added period deviation are each bounded to one 20-kHz scheduler quantum (50 us) where that limit follows from the firmware architecture.
- Kept genuinely hardware-dependent limits unresolved rather than inventing numbers. Absolute oscillator ppm, final comparator threshold tolerance/amplitude envelope, representative output load, and the measured EXTI capture-latency distribution remain BLOCKED/PENDING until the relevant hardware premise exists.
- Added `docs/qualification/V1_BENCH_MATRIX.md` with repeatable configurations for simultaneous-output skew, all seven gate widths, integer/rational rates, Swing, phase, mixed-mode endurance, display stress, External Sync, RST and persisted-transport boot safety.
- Added a vendor-neutral HIL edge-capture exchange format (`time_us,signal,level`) and `scripts/analyze_hil_capture.py` for pulse-width, period, eight-channel skew and idle-vs-display-stress checks. The analyzer uses worst observed error/deviation rather than allowing mean values to hide isolated excursions.
- Added an evidence-record template and minimum evidence counts: at least 100 complete events for ordinary timing checks and at least 1,000 for display-stress comparison.

### Release gating

- Added `scripts/check_hil_qualification.py` and integrated it into CI and release workflows. Beta builds validate ledger completeness and evidence semantics; `1.0.0-rc.*` and stable `1.0.0` automatically require every normative HIL item to be PASS.
- Added regression tests for malformed captures, objective pulse-width pass/fail, eight-output skew matching, display-stress event loss, added timing excursions, missing PASS evidence, and the intentional failure of the current incomplete ledger under the future RC gate.
- No musical behavior, persistence schema or V1 UI contract changes in beta.2. This release prepares real-hardware qualification rather than adding features.

## [0.19.0-beta.1] - 2026-09-10

### V1 feature freeze

- Declared the implemented alpha.63 musical feature set as the V1 release-qualification baseline. From beta.1 to 1.0, new musical features are deferred; changes are limited to defects, physical qualification, reproducibility/documentation work, and compatibility work needed to preserve later 1.x upgrades.
- Added `docs/ROADMAP.md` with the explicit V1 qualification path and the post-1.0 plan for Swing/Groove 2.0, Sequencer 2.0, Euclid Auto-Fill/conditions, Ratchets, structured random, channel interaction, scenes/phase work, later experiments, and a separate VCV Rack track.
- Documented the deliberate Hardware Rev 1 product boundary: CLOCK remains a digital timing/gate/trigger instrument with SYNC/RST, not a general CV/modulation platform. README, User Guide, architecture, configuration, identity, and publication manual now explain the quality/buildability rationale rather than presenting missing CV as an unresolved feature gap.

### Forward compatibility

- Added `docs/V1_FORWARD_COMPATIBILITY.md` and made the V1 Flash map an explicit architecture contract.
- Centralized logical persistence-region boundaries in `src/hal/persistent_layout.h` and replaced duplicated score/leaderboard offsets with those constants.
- Exposed and tested the exact schema-v6 state budget: 252-byte state payload, 264-byte CURRENT record, 280-byte preset record, 2,504 bytes for CURRENT + eight presets, 568 bytes before the legacy-score region, and only 63 additional state-payload bytes available for naive in-place schema growth.
- Recorded the resulting 1.x rule: storage-heavy per-step data must use a schema-v7-or-later layout/migration strategy rather than simply expanding `ChannelConfig`/schema v6. Any logical-image increase beyond 8 KiB also requires a real STM32 target memory measurement because the persistence staging image is static BSS.
- Kept the V1 real-time engine behavior unchanged. The audit explicitly reserves a later event-pipeline refactor for the first post-1.0 features rather than destabilizing the beta baseline.

### Documentation and release tooling

- Updated prerelease wording so physical HIL is a beta/RC qualification gate rather than incorrectly described as a prerequisite to entering beta.
- Made manual cover/back stamping phase-aware: alpha builds render as Prototype, beta builds as Beta, release candidates as Release candidate, and stable versions as Release.
- Added documentation/tooling regressions that keep the no-general-CV scope decision, roadmap, forward-compatibility analysis, persistence budget, and beta publication labels from silently drifting.

### Compatibility

- No new musical behavior is introduced by beta.1. Existing alpha.63 user configuration semantics and persistence schema v6 remain the V1 compatibility baseline.

## [0.19.0-alpha.63] - 2026-09-10

### Documentation

- Rebuilt the repository landing page around the actual product: a compact best-practice badge set, a prominent **Why another Eurorack Clock?** section, concise product rationale, the three output topologies, timing/sync behavior, simulator/test entry points, hardware target, sponsorship, and explicit source-available licensing language.
- Replaced the former front-panel callout graphic with a scalable numbered manual SVG generated from `sim/panel_layout.ini`. The new illustration follows the Quantizer/Drift documentation style, uses CLOCK cobalt blue, and keeps OLED, encoder, PLAY/TAP/STOP, SYNC/RST, eight outputs, and activity LEDs aligned with the simulator geometry.
- Reworked `docs/USER_GUIDE.md` into a more didactic operating reference: Quick Start, numbered front-panel tour, detailed Clock/One Clock/Divider Bank/Euclid/Sequencer sections, timing-control semantics, External SYNC/RST, presets, and current technical limits.
- Added descriptive context to embedded UI screenshots. Main operating states remain individually explained; screensavers and boot Easter eggs are presented as compact 2-/3-column galleries instead of consuming one full documentation block per image.
- Documented the intended future manual-authoring pipeline: Markdown as the maintainable content source, deterministic SVG/framebuffer assets, a controlled ODT template/postprocessor, and PDF visual-regression gates. Plain default Pandoc-to-ODT output is explicitly not accepted as a replacement until it reproduces the approved publication layout.
- Synchronized the Markdown documentation index and manual workspace rules with the generated front-panel asset and screenshot-description requirements.

### Release metadata

- Advanced firmware/documentation identity, `CITATION.cff`, and CodeMeta metadata to `0.19.0-alpha.63`.
- Stamped the maintained ODT manual to alpha.63 and added the version-matched frozen manual source under `docs/manual/releases/0.19.0-alpha.63/`.
- No production clock-engine or hardware-behavior change is introduced by alpha.63; the runtime audit/test baseline from alpha.62 remains the code baseline for this documentation-focused increment.

### Validation

- Architecture policy, documentation/link/SVG checks, and native-test inventory pass; Python release/manual/persistence tooling: **41/41 PASS**.
- Native inventory remains **89 named cases**: 44 Clock Core, 24 Realtime/SYNC/RST, and 21 complete Host Firmware/UI/HAL scenarios. Direct host execution passes with 215,299 Core assertions, 159/162 Realtime assertions, 3,171 I2C firmware assertions, 2,136 assertions for each SPI controller variant, and 3,169 fixed-I2C/reset assertions.
- Aggregate production coverage is **96.02% executable lines**, **98.34% functions**, and **90.16% non-throw decision branches**; all configured hard gates pass.
- ASan/UBSan passes for Core, Realtime, external-GPIO Realtime, complete I2C firmware, SPI SSD1306, SPI SSD1315, and fixed-I2C/reset firmware.
- Headless simulator builds and tests pass with both GCC and Clang: **2/2 PASS** for each compiler.
- The alpha.63 frozen ODT validates and converts locally to a **38-page PDF**. Local conversion uses font substitution because Ubuntu fonts are not installed in this environment; the release workflow remains responsible for the required Ubuntu-font publication check.
- PlatformIO and Doxygen executables are not installed in this validation environment. Their configuration is covered by repository tooling tests, but no local `pio` target build or real `doxygen Doxyfile` execution is claimed for this package.


## [0.19.0-alpha.62] - 2026-09-09

- Expose GitHub Sponsors and Patreon through `.github/FUNDING.yml` so GitHub can render the repository sponsorship sidebar.
- Make bare `doxygen Doxyfile` generation work from a clean checkout by using a creatable top-level output directory.
- Expand `pio test -e native` from the 44-case core-only filter to the complete 89-case native Core + Realtime + Host Firmware suite, with a regression gate on the published test inventory.
- Harden runtime memory and timing behavior: remove large persistence stack frames, avoid idle/control-path interrupt locks, narrow master-tempo updates, and make SYNC/RST overflow handling continuity-safe.
- Add permanent architecture gates that reject embedded heap allocation and production stack frames larger than 4 KiB.

### Fixed

- Reworked the publication-layout end-user manual into a complete user-oriented flow while preserving the existing visual design. The maintained ODT now matches the alpha.62 UI and behavior, including One Clock as the factory topology, the current encoder short/long-press grammar, RESET input Trigger/Gate modes, presets, screensavers, Easter eggs, and the SPI/I2C timing/HIL status.
- Fixed release-manual font validation so CI verifies the required Ubuntu Regular/Light/Bold faces with fontconfig before LibreOffice conversion, then verifies embedded/subset Ubuntu-family Unicode fonts in the generated PDF instead of depending on Poppler exposing the source face name as `Ubuntu-Light`.
- Updated the root documentation index and manual workspace documentation so the typeset manual is described as a maintained artifact rather than a future placeholder.
- Fixed Windows/MSVC `/W4 /WX` simulator builds by removing implicit narrowing through `std::pair` converting constructors in Formula 1 and screensaver geometry. Small coordinate tables now use explicitly typed aggregate point structures.
- Fixed the persistence migration erase-fill value so `std::fill_n` receives `std::uint8_t` directly instead of instantiating an `unsigned int` to `uint8_t` conversion.
- Removed an unused Moon Buggy geometry constant and an unused stored timer-instance field exposed by an additional Clang `-Werror` portability pass.

### Validation

- Native test inventory: **89 named cases** — 44 Clock Core, 24 Realtime/SYNC/RST and 21 complete Host Firmware/UI/HAL scenarios. The default host run executes more than **218,000 assertions**.
- Full firmware host coverage: **96.02% executable lines**, **98.34% functions**, **90.16% non-throw decision branches**; the 90% decision gate is restored and passing.
- Architecture, documentation and native-test inventory checks pass; Python tooling regression suite: **41/41 PASS**.
- Changed Realtime/SYNC/RST paths pass ASan/UBSan in both ordinary and external-GPIO configurations.
- Native headless simulator builds cleanly with GCC and Clang using `-Wconversion -Wsign-conversion -Werror`; both simulator test cases pass on both builds.
- Final comparator/PCB and oscilloscope-level timing validation remain hardware-in-the-loop requirements and are not inferred from host tests.

## [0.19.0-alpha.61] - 2026-09-09

### Changed

- Brought the primary README, architecture, configuration, developer, user, coverage, manual, and HIL documentation in line with the current interrupt-driven encoder/SYNC/RST architecture and dual OLED transport support.
- Kept **SPI SSD1306** as the PlatformIO reference/default display profile while adding explicit supported I2C SSD1306 and SSD1315 build profiles for procurement flexibility.
- Defined an explicit interrupt-priority contract: the 20 kHz TIM3 musical scheduler runs above GPIO EXTI capture, while I2C service remains lower priority.
- Reworked runtime I2C display refresh into a deferred service path: `present()` publishes the newest framebuffer without bus traffic and each foreground `service()` call performs at most one short transaction.
- Limited I2C data transactions to 24 framebuffer bytes plus the controller byte. A newer UI frame may replace an incomplete older frame; stale display frames are expendable, musical timing is not.
- Kept the SPI dirty-page path immediate and independent so SPI users do not pay for the I2C scheduling strategy.

### Added

- Added `docs/TIMING.md` as the timing contract for scheduler quantization, IRQ hierarchy, encoder/SYNC/RST capture, SPI/I2C display behavior, and the rationale for avoiding an RTOS on the single-core STM32F401.
- Expanded the HIL plan with SPI/I2C display-stress A/B measurements, faulted-I2C behavior, and explicit requirements that display activity must not alter gate timing or lose SYNC/RST/encoder events.
- Added host regressions for bounded I2C transactions, deferred refresh, latest-frame-wins replacement, and command/data NACK retry without advancing the modeled physical framebuffer.
- Added explicit coverage for reset-gate menu formatting and preset-name wrap/fallback helpers.

### Validation

- Repository host coverage: **95.97% executable lines**, **98.31% functions**, **90.01% non-throw decision branches**.
- Core, focused realtime, external-GPIO realtime, complete I2C firmware, and reference SPI SSD1306 firmware pass ASan/UBSan in the validation environment.
- Architecture, documentation, 35 Python tooling tests, and both headless simulator tests pass.
- Final physical SPI/I2C jitter equivalence remains a hardware-in-the-loop requirement; host tests do not claim oscilloscope-level timing.

## [0.19.0-alpha.60] - 2026-09-09

### Added

- Added root-level `CITATION.cff` (Citation File Format 1.2) with the canonical searchable/citation identity **South Signal Lab CLOCK**, author, SPDX license identifier, release version/date, abstract, and discovery keywords. GitHub can expose this through **Cite this repository**, and the same file is prepared as the future Zenodo metadata source.
- Added `codemeta.json` using CodeMeta 3.0 / `SoftwareSourceCode` JSON-LD with explicit author, maintainer, South Signal Lab publisher/producer identity, target platform, programming languages, development status, license URL, and aligned discovery vocabulary.
- Added `docs/PROJECT_IDENTITY.md` defining the distinction between product name `CLOCK`, canonical searchable identity `South Signal Lab CLOCK`, compact firmware identity `SSL CLOCK`, and the future DOI/SWHID roles.
- Added README citation and CodeMeta badges plus a dedicated persistent-identity/citation section.

### Changed

- Strengthened the repository landing-page identity to **CLOCK — Eurorack Clock by South Signal Lab** while retaining `CLOCK` as the product-facing name.
- Applied the canonical **South Signal Lab CLOCK** identity to primary documentation titles and Doxygen project metadata so generated/indexed documentation is no longer headed only by the generic word `CLOCK`.
- Changed the firmware Info product value to the compact `SSL CLOCK` identity.
- Changed the INFO → UPDATES QR payload from `http://github.com/napolitano` to the verified TLS URL `https://github.com/napolitano`; no repository-specific URL is fabricated while the final public repository slug is not yet verifiable.
- Updated the README test-coverage baseline to the current repository-wide 96.08% lines / 98.29% functions / 90.02% source decisions.

### Metadata policy

- No DOI is claimed before a real archive mints one. Zenodo/DOI fields and repository-code URLs remain absent rather than using placeholders.
- `CITATION.cff` remains the intended single citation metadata source; `.zenodo.json` is deliberately not introduced while no Zenodo-specific metadata is required.

## [0.19.0-alpha.59] - 2026-09-09

### Fixed

- Moved encoder quadrature capture from foreground polling to GPIO edge interrupts. Complete detents are accumulated independently of OLED rendering, preventing I2C/SPI frame time from dropping encoder transitions.
- Removed the fixed 1.5-second external-sync loss assumption. Lock timeout now scales with the measured/allowed pulse interval, so slow sources such as 20 BPM at 1 PPQN remain valid.
- Treat a SYNC queue overflow as a discontinuity instead of a long timing period; the controller re-anchors to the newest edge and waits for a clean subsequent period measurement.
- Preserve the newest RST input level on queue saturation so a dropped LOW cannot leave gate-reset latched indefinitely.
- Removed the simulator's duplicate fixed sync timeout. Firmware is now the single owner of external-lock loss and reacquisition semantics.

### Added

- Added interrupt-driven comparator input capture queues for external SYNC and RST. Input ISRs only timestamp/queue events; the 20 kHz scheduler consumes them deterministically rather than mutating engine state from EXTI context.
- Added configurable external **RST MODE** under Sync settings:
  - **TRIGGER** (factory default): one reset on the rising edge; a held HIGH does not retrigger.
  - **GATE**: HIGH holds the clock engine in reset with all outputs LOW; release restarts from phase zero.
- Added persistence schema v6 and lossless v3/v4/v5 migration. Existing configurations migrate to `TRIGGER`.
- Added a dedicated realtime host suite covering 20/120/999 BPM, 1/2/4/24 PPQN, jitter, timestamp wraparound, adaptive timeout, queue overflow/reacquisition, gate lengths, reset modes, and long-running phase/timing invariants.
- Added an external-GPIO realtime variant that exercises the full fake comparator-pin -> IRQ -> queue -> scheduler path.
- Added simulator regressions for slow external clocks and realistic quantized PPQN tempo measurement.
- Expanded persistence, leaderboard, arcade, and Breakout boundary tests until all repository coverage gates are met again.

### Timing architecture

- RST has deterministic priority over SYNC when both are consumed in the same scheduler quantum.
- External tempo is estimated in pulse-period space from edge timestamps; 32-bit timestamp wraparound is explicitly supported.
- GPIO IRQ timestamping remains the portable implementation/fallback. The final PCB should route SYNC to an STM32 timer input-capture-capable pin so the production hardware can later move edge timestamping to timer capture without changing higher layers.

### Validation

- Repository coverage gates restored: **96.08% executable lines**, **98.29% functions**, **90.02% non-throw decision branches**.
- Focused realtime and GPIO realtime suites pass, including ASan/UBSan variants.
- Full I2C/SPI/fixed-reset host firmware matrix and headless simulator are part of the release validation matrix.


## [0.19.0-alpha.57] - 2026-09-08

### Fixed

- Removed the 2.6-second automatic timeout from the shared Easter-egg intro state. Pixel Raid, Formula 1, Breakout, Egg Journey, and BEATKNECHT now remain on their animated retro intro indefinitely until the user explicitly starts with **TAP** or encoder **PUSH**.
- The transport button no longer dismisses an Easter-egg intro; the on-screen `TAP/PUSH START` instruction now exactly matches the accepted controls.

### Validation

- Added host regressions across all five Easter-egg titles that leave each intro running for 60 seconds and verify that it remains in the intro state.
- Added regressions across all five titles that verify a transport-button press does not start gameplay from the intro.


## [0.19.0-alpha.56] - 2026-09-08

### Changed

- Changed the factory operating mode to **ONE CLOCK** and reordered the six-function mode palette to **ONE CLOCK, DIVIDER, CLOCK, EUCLID, SEQUENCER, OFF** without changing the persisted enum values.
- Reorganized the main settings hierarchy into **GENERAL SETTINGS**, **CHANNEL SETTINGS**, **PRESETS**, **INFO**, and **RESET**. General Settings now owns Clock, Sync, and Screensaver; Info is split into Name, Version, Author, Licenses, and Updates.
- `ALL MASTER` now intentionally follows the One Clock factory topology, while Clock Tree, Dividers, Polyrhythm, Euclid Kit, and Hybrid explicitly select independent-channel operation so their per-channel configuration remains effective after the new factory default.

### Added

- Added a full-screen, camera-scannable QR code under **INFO → UPDATES**, currently encoding `http://github.com/napolitano`.
- Documented the gate-output electrical contract as nominal **0/+5 V** and the existing configurable gate lengths of **1/2/5/10/20/50/100 ms**, with **10 ms** as factory default.

### Hardware decision

- Retained the planned 74HCT244 +5 V output architecture. +10 V gate outputs are not a current requirement for the Clock/trigger/Euclid/sequencer use cases and would require a different higher-voltage output stage rather than a firmware-only change.

### Validation

- Added/updated host regressions for the One Clock factory state, reordered mode palette, reorganized settings navigation, factory-template operating modes, and QR Updates page.


## [0.19.0-alpha.55] - 2026-09-08

### Added

- Added a shared arcade presentation layer for Pixel Raid, Formula 1, Breakout, and Egg Journey. Every ranked game now opens with its own compact retro intro, project-owned pixel motif, and game-specific scrolling marquee before gameplay begins.
- Added independent CRC-protected **Top 100** leaderboards for all four ranked Easter eggs. A qualifying final score enters the shared three-letter initials editor; non-qualifying scores go directly to the ranking. The encoder scrolls the table and BACK starts a new run.
- Added a compact 8-byte leaderboard-entry format and four fixed 100-entry records in the upper half of the persistent image.
- Added score/life progression to Breakout so it participates in the same ranked arcade flow: bricks and completed boards award points, three missed balls end the run, and the result is routed to its own Top 100.
- Added individual retro intros for BEATKNECHT as well. BEATKNECHT remains a utility Easter egg and deliberately has no score or leaderboard.

### Changed

- Renamed the former Virtual Drummer implementation and source class to **BEATKNECHT** throughout the product-facing UI, documentation, simulator screenshots, and compile-time Easter-egg selection.
- Expanded the logical A/B persistent image from 4 KiB to **8 KiB** to host the four arcade Top-100 tables while retaining the established lower-4-KiB settings/preset/legacy-score layout.
- Earlier valid 4-KiB A/B generations are now accepted directly. Reads beyond their historical payload return erased bytes, and the first subsequent write promotes the old image to 8 KiB without discarding the existing payload.
- Historical one-entry Pixel Raid, Formula 1, Breakout, and Egg Journey score slots remain readable as migration fallbacks until the corresponding Top-100 record is first created.
- Final game-over retry behavior is centralized: ranked games finish in the Top-100 flow instead of each implementing a separate final retry screen. Intermediate life-loss retry behavior inside Egg Journey remains unchanged.

### Safety

- Ranked arcade intros, name entry, and leaderboard screens retain the disabled external gate-output policy. BEATKNECHT keeps all outputs LOW during its intro and enables the output stage only after the rhythm generator actually starts.

### Validation

- Added host regression coverage for 4-KiB-to-8-KiB A/B payload migration, leaderboard qualification at the 100-entry boundary, legacy single-score fallback, initials insertion, ranking scrolling, and BACK-to-new-game behavior.

## [0.19.0-alpha.54] - 2026-09-08

### Added

- Added `BLOX`, a stateful falling-body screensaver in which project-owned shapes made from triangular 8×8 cells accumulate until the OLED fills and the pile restarts.
- Added `MATRIX`, an original monochrome procedural digital-rain screensaver using project-owned numeric/symbol glyphs and independently moving columns; no film glyph artwork is copied.
- Added `CUBE COVER`, which fills the 16×8 tile grid with small cube motifs using changing forward, reverse, center-out, outside-in, snake, and ring/labyrinth-like traversal orders.
- Added `BEATKNECHT` as `CLOCK_EASTER_EGG=5`: eight 16-step gate patterns, TAP style selection, encoder BPM control, current-step display, and intentional OUT 1–8 gate generation. Curated styles cover EDM, House, Techno, Hip Hop, Trap, Pop, Rock, Blues, Funk, Reggae, Salsa, Samba, Bossa Nova, and Drum & Bass.

### Changed

- Renamed the user-facing EGG JOURNEY Easter egg to **Egg Journey** while retaining internal compatibility identifiers for this prerelease.
- Added progressive Egg Journey difficulty: scroll speed rises in stages, static crater density/radius increases with distance, and later stages shorten asteroid intervals and increase fall speed. The HUD now shows the current stage.
- Reworked `FIELD` for a slower, denser plasma-like motion: four smoothly moving sources, reduced animation phase rate, 11 contour levels, a finer sampling grid, and interpolated marching-squares edge crossings.
- Reordered the screensaver selector without renumbering persisted modes: `OFF` is now the first UI entry, followed by the animation list. Factory default remains `CLOCK`.

### Safety

- BEATKNECHT is the only boot Easter egg that intentionally enables the external gate-output stage. It sets every channel LOW before enabling, emits short gate pulses only while active, and returns all channels LOW before disabling the stage on exit. Arcade Easter eggs retain the disabled-output policy.

### Validation

- Extended host tests for the new screensavers, explicit OFF-first selector order, BEATKNECHT style/tempo/step/gate behavior, and the existing Egg Journey life/retry path.

## [0.19.0-alpha.53] - 2026-09-08

### Added

- Added the `FIELD` STOP-mode screensaver: a monochrome plasma-like scalar-field visualisation built from three independently moving attractors. Seven isocontour levels are extracted with a compact marching-squares renderer, allowing contours to merge, deform, pinch, and separate without grayscale.
- Added deterministic manual/host coverage for the new Field screensaver.

### Changed

- Reworked EGG JOURNEY into a continuously scrolling traversal game. The lunar ground now advances independently of player input; encoder movement shifts the egg forward/backward within the viewport rather than being the only source of world motion.
- Added three-layer parallax to EGG JOURNEY: slowly drifting stars, distant mountains, and a nearer ridge move at different fractions of the foreground terrain speed.
- EGG JOURNEY now starts with three lives. Crater and asteroid failures consume one life and retain the distinct broken/flattened egg states. Runs end only after the third failure.
- Fixed EGG JOURNEY `TAP RETRY`: retry is release-armed and requires a fresh TAP press, preventing a held jump button from consuming the next life. A retry safely advances past the fatal crater before respawning; GAME OVER retry starts a fresh three-life session.
- Added the remaining-life counter to the EGG JOURNEY HUD.

### Validation

- Extended host regression coverage for automatic world scrolling, encoder-relative positioning, three-life progression, release-armed retry, final game-over restart, and Field animation.

## [0.19.0-alpha.52] - 2026-09-08

### Added

- Added a fourth compile-time boot Easter egg, **EGG JOURNEY** (`CLOCK_EASTER_EGG=4`), with a deliberately original monochrome lunar-traversal presentation for the 128x64 OLED.
- The player character is an egg with eyes. Encoder motion drives forward/backward with inertia and drag; TAP launches a gravity-driven jump.
- Added deterministic pseudo-random lunar craters plus telegraphed falling asteroids. Asteroid impacts create new persistent-in-session craters, award an avoidance bonus when missed, and remain visible as impact bursts briefly after landing.
- Added two distinct failure states: entering a crater while grounded visibly breaks the egg shell, while a direct asteroid strike flattens the egg. TAP restarts after either game-over state.
- Added a dedicated CRC-protected EGG JOURNEY high-score slot with the same three-letter arcade initials workflow used by Pixel Raid and Formula 1. Existing score slots remain independent and unchanged.
- Added deterministic manual screenshots for the active EGG JOURNEY scene, broken-egg crater crash, and flattened-egg asteroid hit.

### Changed

- Extended `CLOCK_EASTER_EGG` validation and application dispatch from three to four boot games.
- Expanded the Easter-egg persistence allocation from three to four 16-byte score slots inside the existing 4 KiB logical persistent image.

### Validation

- Added host coverage for EGG JOURNEY movement, jump physics, asteroid activation/direct hits, both failure render states, restart behavior, gate-buffer safety, and score-slot independence.

## [0.19.0-alpha.51] - 2026-09-08

### Added

- Added the `SPECTRUM` STOP-mode screensaver: a deliberately synthetic 16-band, 1-bit spectrum-analyser animation inspired by classic desktop media-player visualisers. It uses independently evolving segmented bars plus peak-hold and decay markers and does not require or sample an audio input.
- Added deterministic host coverage and a generated manual screenshot for the Spectrum animation.

### Compatibility

- Existing screensaver persistence values are preserved. `SPECTRUM` is appended as value 8; the factory default remains `CLOCK`.

## [0.19.0-alpha.50] - 2026-09-08

### Added

- Added the `ACID` STOP-mode screensaver: a monochrome acid-style smiley follows fixed-point ball physics with gravity, wall/corner reflections, damping/re-energising, and collision-coupled angular velocity so the face visibly rotates as it bounces.
- Added a dedicated persistent Formula 1 high-score slot, independent of Pixel Raid, using the same CRC-protected score/three-letter-initials contract.
- Added Breakout falling modifiers with original minimal glyphs: temporary smaller paddle, larger paddle, and faster paddle movement.

### Changed

- Formula 1 world motion is faster than alpha.49 but remains well below the unplayable alpha.48 scale. Automatic target speed, traffic advance, and spawn cadence were rebalanced together.
- Formula 1 collisions now trigger a visible crash burst and recovery pause instead of merely deleting the traffic car and reducing speed. Three crashes end the run; TAP starts a fresh run.
- Formula 1 HUD now includes the durable high score alongside current score, while keeping the live mini-track.
- Breakout now has explicit top/left/right playfield borders and seven paddle impact zones producing multiple horizontal/vertical rebound-angle combinations.

### Persistence

- Pixel Raid retains its existing score record at the original NVM offset. Formula 1 uses the next dedicated 16-byte score slot, so existing Pixel Raid highscores remain compatible and the games cannot overwrite one another.

## [0.19.0-alpha.49] - 2026-09-08

### Fixed

- Slowed Formula 1 world/traffic motion substantially so steering and overtaking remain playable on the encoder. Displayed speed is now decoupled from the coarse OLED-world pixel advance.
- Clipped center-lane dashes strictly below the horizon so scrolling markers can no longer enter the sky/scenery region.

### Added

- Added real road bends using a deterministic track curvature profile. Road edges, lane centers, traffic placement, and steering bounds now follow the current curve.
- Added a compact live mini track map to the Formula 1 HUD with a moving position marker.

### Changed

- Rebalanced automatic acceleration/braking, spawn cadence, collision slowdown, scoring, and lap distance around the slower world scale.
- Formula 1 TIME now reflects elapsed game time instead of deriving time-like numbers from distance.

## [0.19.0-alpha.48] - 2026-09-08

### Changed

- Reworked the Formula 1 boot Easter egg to better match the requested classic pseudo-3D racer direction while staying fully monochrome for the 128x64 OLED.
- The racer now renders a horizon scene with clouds and mountain silhouettes, a wider perspective road with shoulder lines and scrolling center dashes, more recognisable formula-style player/opponent cars, a compact retro HUD, and a steering-wheel motif at the bottom of the screen.
- Traffic placement now follows lane-centered perspective rendering instead of flat top-down rectangles, while the underlying automatic speed/brake behaviour and encoder steering remain unchanged.
- Regenerated the simulator/manual screenshot for the Formula 1 Easter egg to reflect the new presentation.

### Validation

- Host firmware and simulator screenshot generation remain green after the Formula 1 renderer refresh.

## [0.19.0-alpha.47] - 2026-09-08

### Changed

- Reworked encoder navigation: short push from Performance opens the channel overview; turning only moves the highlight; short push in the overview commits the highlighted channel and returns to Performance. Channel/global settings now open only through a 650 ms encoder long push from Performance or the overview.
- Channel settings now show full mode names instead of single-letter abbreviations.
- Renamed the Settings root/page entry from DISPLAY to SCREENSAVER.
- The current Joy-IT SBC-OLED01V2 prototype is now represented semantically as SSD1306 in the default PlatformIO/VS Code build profile while retaining independent SSD1306/SSD1315 controller selection and fully configurable display wiring.

### Added

- Added `CLOCK`, `PLUG`, and `HEARTBEAT` screensavers. CLOCK renders eight independently phased oscilloscope-style clock traces and is the new factory default; PLUG renders a damped plucked string; HEARTBEAT renders a pulsating heart. Existing FRACTAL, ORBIT, and OFF choices remain available.
- Added compile-time `CLOCK_EASTER_EGG` selection: `1=Pixel Raid`, `2=Formula 1`, `3=Breakout`. Formula 1 uses encoder steering with automatic acceleration/braking and generated traffic; Breakout uses encoder paddle control and TAP to launch the waiting ball.
- Added deterministic manual screenshots for all three new screensavers and all three boot Easter eggs.

### Safety

- Every boot Easter egg still runs before the real-time scheduler starts and while the external gate-output stage is disabled.

## [0.19.0-alpha.46] - 2026-09-08

### Fixed

- Fixed a production-only SPI pin-mapping bug introduced by the dedicated `SPIClass` object. The firmware passed STM32 `PinName` values (`PA_7`, `PA_6`, `PA_5`) through a `uint32_t` cast into the `SPIClass` constructor, whose integer overload interprets them as Arduino digital-pin numbers and converts them again. Host tests did not reproduce that conversion, so the invalid production routing could pass regression tests while the physical OLED remained dark.
- The dedicated SPI object now receives actual configurable Arduino digital pins (`PA7`, `PA6`, `PA5` by default), matching STM32duino's documented constructor contract.

### Added

- Added explicit compile-time OLED controller selection for both **SSD1306** and **SSD1315**. The 128x64 command profile remains shared because these two controllers use the compatible command set required by the current driver; the controller identity is nevertheless explicit so future controller-specific settings can diverge without changing UI or transport code.
- Made every OLED wiring signal configurable from build flags: I2C SDA/SCL, SPI SCK/MOSI/MISO/CS/D-C, and RESET, plus SPI/I2C bus frequency and normal/dimmed contrast.
- Added separate PlatformIO environments and VS Code tasks for SSD1315 and SSD1306 SPI builds. The current physical prototype defaults to the SSD1315 profile, while `blackpill_f401cc_spi` remains as a backward-compatible alias.
- Expanded the host matrix with SSD1306, SSD1315, and non-default custom-wiring SPI variants.

### Hardware profile

- Current default SPI wiring remains `SCK=PA5`, `MOSI/DIN=PA7`, `MISO=PA6` (MCU-side only, unconnected at the OLED), `CS=PA4`, `D/C=PB9`, `RESET=PB15`.

## [0.19.0-alpha.45] - 2026-09-08

### Fixed

- Replaced mutation of the global STM32duino `SPI` object with a dedicated OLED SPI1 instance using explicit PA7=MOSI, PA6=MISO, PA5=SCK and no hardware NSS. The OLED remains wired with software CS on PA4, D/C on PB9 and active-low RESET on PB15.
- SPI OLED transfers are now explicitly transmit-only; the display has no MISO connection and received bytes are irrelevant.
- Control GPIO initialization now occurs after SPI1 initialization, ensuring PA4 and PB9 are left in the exact GPIO modes required by the OLED before reset and command traffic begin.

### Development

- Restored the complete project `.vscode/settings.json`, including the requested Peacock/workbench color customizations.
- Normalized VS Code task labels to `CLOCK:` and added `CLOCK: Clean + upload SPI firmware (DFU)` to force a clean SPI rebuild before flashing during hardware bring-up.

### Validation

- Host SPI regression covers the existing reset/CS/D-C/SCK/MOSI contract with the dedicated transport path represented by the host SPI shim. Hardware verification remains required for the physical OLED.

## [0.19.0-alpha.44] - 2026-09-08

### Fixed

- Hardened physical SPI OLED bring-up after a real prototype remained dark even though the host SPI regression tests passed. SPI CS and D/C are now configured before the hardware RESET pulse, so CS is guaranteed HIGH while the SSD1306 resets.
- Added explicit 20 ms settling periods before RESET assertion and after RESET release. The SSD1306 datasheet only requires a microsecond-scale reset pulse, but the additional delay provides margin for the complete breakout/module power-up path.
- Reduced the prototype OLED SPI clock from 8 MHz to 1 MHz. 8 MHz is inside the SSD1306 serial-interface specification, but 1 MHz is intentionally conservative for breadboard/point-to-point prototype wiring and removes signal-integrity margin from the immediate bring-up diagnosis.
- Strengthened host regressions to verify CS-HIGH precedes RESET-LOW, RESET completes before the first CS-LOW command transaction, and the configured SPI transfer frequency is actually used.

### Validation

- The alpha.43 to alpha.44 functional change is isolated to display bring-up timing/order and SPI prototype frequency; application timing and clock-engine behavior are unchanged.

## [0.19.0-alpha.43] - 2026-09-08

### Added

- Added the first maintained CLOCK end-user manual source under `docs/manual/clock-user-manual.odt`, following the same release-publication model used by the Quantizer and Drift projects.
- Added manual release tooling that stamps the firmware version into ODT text/metadata and the rasterized front/back covers, freezes a version-specific source under `docs/manual/releases/<version>/`, converts the frozen ODT to PDF with LibreOffice, and validates the resulting artifacts.
- Added a dedicated manual-publication smoke workflow.

### Changed

- The tag release workflow now requires a version-matched frozen user manual and publishes both `clock-user-manual.<version>.odt` and `clock-user-manual.<version>.pdf` as GitHub Release assets.
- Release preparation now has an explicit manual-version gate so a tag cannot silently ship an ODT/PDF that still identifies an older firmware build.

### Validation

- Manual ODT container/version contract, cover/back version stamping, LibreOffice PDF conversion path, release-artifact naming, documentation policy, architecture policy, and release tooling tests are covered by automated checks.

## [0.19.0-alpha.42] - 2026-09-07

### Changed

- Replaced the runtime CLOCK vector reconstruction with a generated, packed 1-bit boot-logo bitmap. The canonical artwork remains `docs/manual/assets/clock-logo.svg`; it is rasterized at the exact OLED target size and checked in as `clock-logo-1bit.png` plus `src/ui/clock_logo_bitmap.h`.
- The boot renderer now draws the approved bitmap directly into the production framebuffer. No Bézier sampling, line-thickening heuristic, or runtime logo geometry remains, eliminating the glyph/spacing differences previously visible between the SVG and OLED output.
- Kept the centered 5x7 firmware-version line and the bottom boot progress bar.
- Regenerated the manual screenshot gallery and 1:1 boot-screen export from the real firmware framebuffer.

### Validation

- Documentation/architecture/tooling gates pass; the headless simulator renders the checked-in boot bitmap deterministically.

## [0.19.0-alpha.41] - 2026-09-07

### Changed

- Corrected the CLOCK wordmark once more: the L remains tightened, the O and second C now have explicit visual separation to remove the residual overlap, and the K spacing was pushed right for a cleaner balance.
- Regenerated the standalone logo assets and boot-screen screenshots to reflect the corrected lettering.

### Validation

- Headless simulator rendering remains deterministic after the final spacing correction.

## [0.19.0-alpha.40] - 2026-09-07

### Changed

- Corrected the CLOCK wordmark geometry again: the L was tightened, the O and second C were separated to remove overlap, and the K was redrawn with cleaner, symmetric diagonals.
- Updated the standalone SVG/PNG logo assets and regenerated the boot-screen screenshots so the revised lettering is reflected consistently across firmware and documentation.

### Validation

- Headless simulator rendering remains deterministic after the lettering correction.

## [0.19.0-alpha.39] - 2026-09-07

### Changed

- Refined the CLOCK wordmark again, specifically reworking the C glyphs and the K geometry. The Cs now use smoother, more circular Bézier curves with cleaner terminal direction, and the K is narrower and more balanced relative to the other letters.
- Increased the internal Bézier sampling density used by the OLED boot renderer so curved glyphs are reproduced more faithfully on the 128x64 display.
- Regenerated the logo assets, manual screenshots, and 1:1 bootscreen export to reflect the revised lettering.

### Validation

- Headless simulator rendering remains deterministic after the updated wordmark geometry.

## [0.19.0-alpha.38] - 2026-09-07

### Changed

- Kept the boot-screen version line in the existing 5x7 small OLED font, but moved it lower to create more visual separation beneath the CLOCK wordmark.
- Regenerated the manual boot screenshots and the 1:1 boot-screen export so the documented startup screen reflects the revised spacing.

### Validation

- Headless simulator rendering remains deterministic after the boot-screen spacing adjustment.

## [0.19.0-alpha.37] - 2026-09-07

### Changed

- Refined the CLOCK boot wordmark to a higher-quality rounded monoline rendering. The two C glyphs were redrawn with smoother cubic Bézier geometry, and the O now uses a fully rounded Bézier outline to fit the same visual language more cleanly.
- Removed the decorative horizontal lines above and below the boot logo.
- Removed the `BY SOUTH SIGNAL LAB` byline from the boot screen. The boot layout now shows only the CLOCK wordmark, the centered firmware version beneath it, and the bottom progress bar.
- Regenerated the manual screenshot gallery and the 1:1 boot-screen asset so the documentation reflects the refined boot rendering.

### Validation

- Headless simulator rendering remains deterministic and the updated bootscreen continues to be generated from the real production framebuffer.

## [0.19.0-alpha.36] - 2026-09-07

### Changed

- Refined the OLED boot screen layout: the large CLOCK vector wordmark now sits above two small 5x7 text lines, with `BY SOUTH SIGNAL LAB` centered beneath the logo and the firmware version centered on its own line below the byline.
- Kept the existing monoline/vector wordmark style and the bottom progress bar, but improved the visual hierarchy so branding, maker attribution, and version information read more cleanly on the 128x64 SSD1306.
- Regenerated the manual screenshot gallery so the documented boot frames reflect the updated CLOCK boot screen layout.
- Added an exact 1:1 PNG export of the completed OLED boot screen under `docs/manual/assets/boot-1000-1x.png`.

### Validation

- Headless simulator boot rendering remains deterministic and produces updated boot screenshots from the real production framebuffer.

## [0.19.0-alpha.35] - 2026-09-07

### Changed

- Renamed the product identity to **CLOCK** throughout firmware UI text, boot UI, simulator, documentation, Doxygen metadata, notices, CI/release metadata, developer tasks, and release packaging. Author and copyright metadata remain unchanged.
- Replaced the previous four-letter product wordmark with the five-letter **CLOCK** monoline wordmark. Both `C` glyphs are defined as smooth cubic Bézier curves; `L`, `O`, and `K` retain the established straight-line geometric style.
- Updated the one-second boot animation to construct the new CLOCK wordmark progressively from the same vector primitives used by the logo design, including sampled cubic Bézier segments for both `C` glyphs.
- Renamed product-specific tooling identifiers to the `clock-*` namespace, including the manual screenshot generator, temporary-generation prefixes, release artifact stem, VS Code task label, and GitHub release title.
- Renamed the reusable manual logo assets to `docs/manual/assets/clock-logo.svg` and `clock-logo.png` and removed the previous product-name logo filenames.

### Validation

- Documentation policy PASS; architecture policy PASS; release/tooling tests 26/26 PASS.
- Headless simulator PASS 2/2 in the default build and PASS 2/2 with `CLOCK_DISPLAY_USE_SPI=1`.
- Headless simulator PASS 2/2 under ASan/UBSan.
- Manual screenshot generator regenerated all 53 curated states; a second run produced byte-identical PNG hashes.
- Core host suite reached 44/44 and the I2C firmware host variant 1595 checks before the environment timed out compiling the full serial coverage matrix; no new full-matrix coverage percentage is claimed for alpha.35.

## [0.19.0-alpha.34] - 2026-09-07

### Added

- Added reusable standalone SVG and transparent PNG wordmark assets under `docs/manual/assets/` for documentation and publication workflows.

### Changed

- Integrated the wordmark assets into `docs/manual/README.md` alongside the existing explanatory diagrams and screenshot assets.

### Validation

- Documentation policy included the new SVG asset and remained green.

## [0.19.0-alpha.33] - 2026-09-07

### Added

- Added the VS Code task **`CLOCK: Generate manual screenshots`** and `scripts/generate_manual_screenshots.py` for deterministic regeneration of the complete OLED documentation gallery.
- Added the SDL-free CMake target `clock-manual-screenshot-generator`, which renders the production 128x64 firmware framebuffer for 53 curated user-visible states.
- Added screenshot coverage for boot progression, power-off, transport states, all generator/output topologies, Internal/External/Auto clock roles, navigation/dialog states, every Settings page, presets, both screensavers, and Pixel Raid.
- Added `docs/manual/assets/manual-screenshots.tsv` as the generated catalog/manifest.

### Changed

- Consolidated generated OLED screenshots under `docs/manual/assets/` beside the explanatory SVG artwork; the User Guide now references that single manual asset source.
- Screenshot publication uses dependency-free PNG encoding with integer-only 4x nearest-neighbour scaling. Raw image content still comes exclusively from the production framebuffer renderer.
- Removed the obsolete one-off `test/tools/render_ui_previews.cpp` path and the duplicated `docs/assets/` screenshot directory.
- Fixed the boot-screen version line discovered by the gallery: prerelease versions such as `0.19.0-alpha.33` are now rendered in full instead of being truncated by the old `FW %s` format/buffer.

### Validation

- The generator builds through the headless simulator preset with the repository's strict warning policy and regenerates all 53 PNG files deterministically without SDL or external Python imaging packages.
- Functional production firmware is unchanged from alpha.32; its existing host coverage baseline remains applicable.

## [0.19.0-alpha.32] - 2026-09-07

### Changed

- Reworked the boot screen so the product name **CLOCK** is now drawn as a large vector wordmark built progressively from line segments over the one-second startup interval instead of rendering the small text label directly.
- Kept the boot progress bar and centered firmware-version readout, but moved product branding responsibility entirely to the new line-drawn CLOCK wordmark.
- Simplified the simulator's built-in front-panel branding so the product name is shown directly as **CLOCK**, with the module descriptor moved to the secondary line.

### Validation

- Headless simulator boot tests remain green and continue to verify that power-on exposes a visible boot interval before normal runtime begins.
- Existing host/simulator coverage is functionally unchanged by this branding/UI update; the change is isolated to rendering.

## [0.19.0-alpha.31] - 2026-09-07

### Fixed

- Fixed the current SPI-prototype OLED RESET mapping: PB15 is now the default active-low display reset GPIO instead of leaving RESET unassigned. The SSD1306 startup sequence therefore drives an explicit high -> low -> high reset pulse before bus initialization.
- Fixed the local developer workflow where VS Code exposed an SPI build task but only an I2C upload task. A dedicated SPI DFU upload task is now present, and the SPI firmware target is the PlatformIO project default for the current physical prototype.
- Added host regression checks for the SPI bus pins, RESET final state/pulse, chip-select idle/toggling, D/C configuration, and bus activity so an SPI build can no longer pass while silently omitting the physical reset line.

### Hardware mapping

- Current SPI OLED: PA5=SCK, PA7=MOSI, PA4=CS, PB9=D/C, PB15=RESET.
- The legacy I2C target remains buildable for regression; the physical prototype no longer defaults to it.

## [0.19.0-alpha.30] - 2026-09-07

### Added

- Added a simulator **POWER** control (`F2` or the developer-panel toggle). Power-off disables the gate buffer, forces MCU-side gates low, clears the OLED, destroys volatile application state, resets the virtual timer/MCU boundary, and preserves durable simulator Flash state. Power-on reconstructs `ClockApplication` and executes the real timed boot path again.
- Added a configurable **RST IN** jack beside SYNC IN with cable/run state, a continuous generator, single-reset injection (`R`), period adjustment by mouse wheel, reset counter, and visible comparator `HI`/`LO` state.
- Added ideal SQUARE / SINE / TRIANGLE source models ahead of virtual SYNC/RST comparators. The simulator models the firmware-facing conditioned digital boundary only; LM393 electrical behavior remains a HIL responsibility.
- Added an ISR-safe `ClockEngine::resetGlobalPhaseFromIsr()` entry point for the eventual external-reset capture path. Global reset restarts shared musical phase without stopping transport; channels configured `RESET=FREE` keep their independent runtime phase.
- Added explicit unassigned SYNC/RST comparator and jack-detect pin placeholders in `pin_map.h` rather than inventing STM32 routing before the PCB/pin assignment is frozen.

### Fixed

- Fixed Pixel Raid projectiles being invisible or only sporadically visible. Projectile physics now advances on the 40 ms game/render cadence instead of moving two pixels on every 1 ms control poll. Controls remain responsive at the foreground-poll rate.
- Refactored the simulator boot/game lifecycle so the one-second boot screen and encoder-held Pixel Raid boot chord are observable and testable without blocking the SDL event loop.
- Reset the developer oscilloscope session on POWER OFF / virtual MCU time rollback, preventing a stale pre-reboot transport epoch from surviving a power cycle.

### Changed

- SYNC IN now uses the same ideal source/comparator abstraction as RST IN and exposes selected waveform plus conditioned `HI`/`LO` state in the developer view.
- Simulator runtime ownership changed from a permanently resident application object to reconstructible volatile application state, matching MCU power-cycle semantics more closely.
- Static simulator layout/timing contracts were split from runtime integration tests to keep implementation units inside the repository's maintainability limit.

### Validation

- Headless simulator tests cover visible boot animation state, POWER OFF/ON reconstruction, the real Pixel Raid boot chord, framebuffer-visible projectiles, SYNC waveform conditioning, continuous and one-shot RST injection, RST layout hit testing, and scope re-arming after power cycles.
- Final host validation: 44/44 core tests; 1595 I2C / 1593 SPI / 1593 fixed-reset firmware checks; 4310/4435 executable lines (97.18%); 349/349 functions (100.00%); 2472/2743 source decision branches (90.12%).
- Core, I2C, SPI, fixed-reset, and headless simulator tests pass under ASan/UBSan; headless simulator CTest passes 2/2; architecture/documentation policy passes; release/persistence tooling passes 26/26.
- Physical LM393 thresholds, hysteresis, common-mode behavior, pull-up/open-collector timing, input protection, propagation delay, and final STM32 timer-capture routing remain explicitly HIL-only and are not claimed by the simulator.

## [0.19.0-alpha.29] - 2026-09-07

### Added

- Adopted **CLOCK** as the module and firmware product name across the boot UI, simulator, documentation, Doxygen metadata, release metadata, notices, and current project prose.
- Rebuilt the root README as the GitHub project landing page with capability/status badges, GitHub admonitions, Mermaid architecture/flow diagrams, current feature and limitation summaries, build/test entry points, and audience-oriented documentation links.
- Added a documentation index and explicit repository documentation style guide covering US-English terminology, GitHub Markdown, Mermaid, SVG artwork, version-sensitive claims, units, Doxygen API comments, and the shared README footer convention.
- Added `CONTRIBUTING.md` and a repository security policy describing timing/safety invariants, validation expectations, reporting scope, and release hygiene.
- Added reusable plain-SVG manual artwork under `docs/manual/assets/` for front-panel anatomy, operating modes, timing/Swing/phase, External Sync, A/B persistence, and the simulator oscilloscope.
- Expanded source API documentation with explicit Doxygen `@brief` descriptions; architecture policy now requires `@file` and `@brief` metadata in every project-owned C++ file.

### Changed

- Synchronized the User Guide, Simulator Guide, Architecture, Development, Configuration, coverage, licensing, dependency, and HIL documentation with the alpha.28 implementation.
- Updated all README-style documents to end with a centered level-six `From Munich with ♥` footer using a text-color heart (`&#9829;`).
- Prepared the documentation tree for a later publication-quality manual while keeping planned hardware clearly separated from implemented and HIL-verified behavior.

### Validation

- Documentation/repository policy checks validate local Markdown links, README footers, fenced blocks, and all manual SVG assets.
- Functional timing behavior is unchanged from alpha.28; the existing host, simulator, sanitizer, architecture, and release-tooling baseline remains applicable.

## [0.19.0-alpha.28] - 2026-09-07

### Fixed

- Fixed the strict Windows SDL simulator build in `panel_renderer_scope.cpp`: gate-transition clipping now compares integer microsecond timestamps against integer scope-window bounds instead of implicitly converting `int64_t` timestamps to the `double` values used by the musical reference-grid renderer. This removes the `-Wconversion -Werror` failures reported by GCC/UCRT64.
- Kept floating-point arithmetic confined to projection of rational musical grid positions; gate telemetry remains integer-timestamped. The alpha.27 musical-reference behavior is otherwise unchanged.

### Validation

- `panel_renderer_scope.cpp` passes a strict C++17 syntax compile with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror` against the SDL3 renderer API surface.
- Headless simulator tests pass 2/2; architecture policy passes; release/persistence tooling passes 26/26. Production firmware is unchanged from alpha.27.

## [0.19.0-alpha.27] - 2026-09-07

### Fixed

- Replaced the developer oscilloscope's fixed millisecond ruler with a transport-anchored musical reference lattice. The previous 250 ms minor grid at the 4 s zoom stage visibly drifted against, for example, a 130 BPM x2 CLOCK (~230.769 ms) even with Swing at 0%.
- ONE CLOCK now derives every scope reference from its exact unswung shared output rate. DIVIDER uses the undivided master beat; INDEPENDENT uses the neutral 1/16 lattice shared by x1 EUCLID/SEQ and all supported CLOCK note values.
- External/Auto sync reference spacing follows the engine's interpreted external tempo and is deliberately independent of manual MIN/MAX BPM settings.
- High-rate/long-window views reduce line density only by whole power-of-two reference serial strides, so every visible ruler line remains on the same musical lattice instead of introducing a rounded timebase.

### Validation

- Added simulator regressions for the exact 130 BPM case that exposed the bug: the independent 1/16 reference is 115.384615 ms and a CLOCK x2 boundary is therefore 230.769231 ms, not 250 ms.
- Added ONE CLOCK x2, effective external-tempo, high-rate decimation, transport-origin, and scrolling-reference tests. Headless simulator tests pass 2/2 and also pass under ASan/UBSan.
- Production firmware is unchanged from alpha.26 and revalidates at 44 core tests, 1592 I2C / 1590 SPI / 1590 fixed-reset host checks, 97.16% lines, 100.00% functions, and 90.04% decision branches.

## [0.19.0-alpha.26] - 2026-09-07

### Changed

- Rebased the simulator developer oscilloscope on the real transport epoch. Before the first PLAY the scope remains armed at `t=0`; PLAY from STOP starts a new reference timeline, while PAUSE/resume keeps the current session.
- Added `FREEZE ON STOP` to the developer panel, enabled by default. When checked, STOP freezes both the scope reference time and retained waveform for inspection; when unchecked the already-started timeline keeps scrolling.
- Added 16 s and 32 s oscilloscope zoom-out stages in addition to 0.5/1/2/4/8 s. The new stages use 1/4 s and 2/8 s minor/major ruler spacing respectively.
- Reorganized the top-level Settings tree into `CLOCK`, `SYNC`, the active mode/channel page, `PRESETS`, `DISPLAY`, `SYSTEM`, and `RESET`. Factory templates now live under PRESETS; screensaver/dim/off controls live directly under DISPLAY.

### Validation

- Added simulator regressions proving that the scope does not advance before PLAY, uses the exact STOP-to-PLAY transition as its epoch, freezes on STOP when requested, and resumes scrolling when STOP freezing is disabled.
- Host firmware matrix passes with 1592 I2C checks and 1590 checks in both SPI and fixed-reset variants. Coverage remains 97.16% lines, 100.00% functions, and 90.04% decision branches.
- Core, I2C, SPI, and fixed-reset host variants pass under ASan/UBSan; headless simulator tests pass 2/2; architecture policy passes.

## [0.19.0-alpha.25] - 2026-09-07

### Fixed

- Fixed the Windows SDL simulator build after the developer oscilloscope renderer was split into `panel_renderer_scope.cpp`; rectangle drawing helpers are now available in that translation unit, eliminating the `fillRect` / `strokeRect` compile failure under `-Werror`.

### Changed

- Set the checked-in Peacock workspace color to Azure (`#007fff`).

### Validation

- `panel_renderer_scope.cpp` passes a strict C++17 syntax build with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror` against the SDL3 API surface used by the renderer.
- Headless simulator tests pass 2/2; architecture policy passes; release/persistence tooling passes 26/26. Firmware production code is unchanged from alpha.24.

## [0.19.0-alpha.24] - 2026-09-07

### Changed

- Reworked the simulator developer oscilloscope into a real scrolling time axis. Vertical ruler lines are anchored to simulator-time reference timestamps rather than fixed screen divisions, so waveform edges and their time references move together and Swing/Humanize offsets remain visually measurable. The right edge is an explicit `NOW` cursor.
- Added fixed oscilloscope spans of 0.5 / 1 / 2 / 4 / 8 seconds. `-` reduces the visible span (zoom in); `+`/`=` increases it (zoom out). Four seconds remains the default. Zoom hotkeys use SDL keycodes so the `+`/`-` meaning follows the active keyboard layout instead of assuming US physical scancodes.
- Ruler density now follows the selected span: 25/100 ms minor/major at 0.5 s, 50/200 ms at 1 s, 100/500 ms at 2 s, 250 ms/1 s at 4 s, and 500 ms/2 s at 8 s. Major reference lines are labeled with their simulator timestamp.
- Extended simulator gate-transition retention to eight seconds so zooming out never invents or stretches missing history.

### Validation

- Added SDL-independent scope-axis regression tests proving fixed zoom stages, zoom-dependent grid spacing, correct alignment for positive and pre-epoch timestamps, and that a given reference timestamp moves left as `now` advances while the right edge remains the current time.
- Headless simulator smoke/runtime tests pass 2/2 with the new scope timeline model.

## [0.19.0-alpha.23] - 2026-09-07

### Added

- Added user-configurable `MIN BPM` and `MAX BPM` master settings. Factory defaults are 20–999 BPM while the technical manual range remains 1–999 BPM. Manual encoder changes and Tap Tempo respect the user limits; External Sync deliberately does not.
- Added `HUMANIZE` exclusively to **One Clock**. Curated values of 0/250/500/1000/2000 us apply deterministic per-output scheduler-quantized offsets while preserving the shared restart/downbeat and safe gate spacing.
- Added a compact human pictogram to the Performance header whenever One Clock Humanize is active.
- Added vertical 250 ms divisions to the simulator's four-second timing scope, with major 1 s lines, so Swing/Humanize displacement is directly visible.

### Changed

- Renamed the user-facing shared-clock topology from `ALL CLOCK`/`UNIFIED` to **ONE CLOCK** consistently; internal enum/class names remain unchanged.
- Euclid and Sequencer Performance views now place their compact pattern counts on the right side of the BPM band instead of lower in the screen.
- Raised the technical master-tempo ceiling from 300 to 999 BPM and reduced the Tap Tempo minimum accepted interval to 60 ms. Existing scheduler-resolution protection still caps physically impossible derived output rates.
- Persistence schema is now v5 so user BPM limits and One Clock Humanize are stored. Existing v3/v4 CURRENT and preset records migrate automatically; legacy BPM values below the new 20 BPM factory minimum retain a matching personal minimum.

### Validation

- Added regression coverage proving that Humanize affects the eight outputs only in One Clock, keeps the common first downbeat exact, and has zero timing effect in Independent mode even when a Humanize value is stored.
- Added coverage for 20/999 factory limits, 1/999 technical limits, 999 BPM Tap Tempo clamping, and External Sync operating outside the user's manual tempo range.
- Extended the core accumulator property sweep from 1–300 BPM to 1–999 BPM.
- Final release validation passes with 44 core tests; 1565 I2C / 1563 SPI / 1563 fixed-reset host checks; 97.16% lines, 100.00% functions, and 90.04% decision branches; all four host variants clean under ASan/UBSan; headless simulator 2/2; release/persistence tooling 26/26; architecture policy PASS.

## [0.19.0-alpha.22] - 2026-09-07

### Changed

- FRACTAL now chooses among six curated Barnsley-fern viewports at each growth-cycle boundary instead of always rendering the same crown crop.
- View selection uses a tiny deterministic PRNG, avoids immediate repeats, and keeps its sequence state in the renderer so waking and re-entering the screensaver does not always return to the first composition.
- Arbitrary random crop coordinates are deliberately avoided: every selectable viewport is precomposed to contain useful leaflet structure on the 128x64 monochrome OLED.

### Validation

- Full host firmware matrix passes with 1465 I2C checks and 1463 checks in both SPI and fixed-reset variants.
- Coverage remains above project gates at 96.81% lines, 100.00% functions, and 90.08% decision branches.
- Core, I2C, SPI, and fixed-reset host variants are clean under ASan/UBSan.
- Regression coverage verifies stable repeated rendering within one frame, progressive growth inside one crop, crop changes at cycle boundaries, and a different crop after screensaver re-entry.

## [0.19.0-alpha.21] - 2026-09-07

### Changed

- Screensaver mode values are now shown as `FRACTAL`, `ORBIT`, and `OFF` instead of numeric enum values.
- Replaced the moving full-screen Julia-set screensaver with a deterministic cropped Barnsley fern. The leafy crown is progressively revealed over a roughly 19-second cycle so the fractal visibly forms instead of reading as random monochrome noise.

### Fixed

- Fixed the interactive SDL3 simulator failing to compile under the project's mandatory `-Wshadow -Werror` policy because the output-jack label path redeclared a local `jack` reference in an enclosing scope.

### Validation

- Full host firmware matrix passes with 1461 I2C checks and corresponding SPI/fixed-reset variants; coverage remains above project gates at 96.84% lines, 100.00% functions, and 90.06% decision branches.
- Core and I2C firmware paths are clean under ASan/UBSan for the changed common UI code.
- SDL-free simulator smoke/runtime tests pass 2/2.
- `sim/panel_renderer.cpp` passes an SDL3-compatible syntax compile with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion -Werror`; this directly covers the alpha.20 Windows compile failure.

## [0.19.0-alpha.20] - 2026-09-07

### Changed

- Reworked native-simulator front-panel geometry around one uniform `pixels_per_mm` scale: all panel positions and visible mechanical sizes are now specified in millimetres and preserve physical proportions.
- C&K/Littelfuse D6R controls now model the documented 9.0 mm round visible actuator and 12.0 mm behind-panel body separately; only the actuator is rendered on the front. PLAY, TAP, and STOP have independent configurable `#RRGGBB` colors, defaulting to red, grey, and black.
- Added distinct TS/TRS Thonkiconn profiles with configurable front hardware. Defaults model a 7.85 mm QingPu knurled nut, 6.0 mm bushing, and approximately 3.6 mm opening; PJ366ST/TRS remains a separate type despite sharing the same front geometry.
- Output activity LEDs are explicitly 3.0 mm by default and their on/off colors are configurable.
- Encoder knob outside diameter is now an explicit `knob_diameter_mm` layout value. The committed diameter remains provisional because the final physical knob SKU has not yet been frozen.

### Fixed

- Removed the visually incorrect 12 mm D6R body disc from the front-panel rendering; the body is behind the panel and is retained only as mechanical/hit-area data.
- Fixed Thonkiconn rendering using only the 6 mm threaded bushing as its visible outside diameter; the installed front nut is now represented separately at its actual default diameter.

### Validation

- SDL-free simulator smoke/runtime integration tests pass (2/2), including mm-to-pixel geometry, custom D6R dimensions/colors, TS/TRS selection, jack front hardware dimensions, and LED diameter overrides.
- Architecture guardrails pass across 108 C++ files.
- Production host matrix remains unchanged and green: 44 core tests; 1450 I2C / 1448 SPI / 1448 fixed-I2C-reset checks; 96.83% lines, 100.00% functions, 90.10% decision branches.
- ASan/UBSan remain clean for core, I2C, SPI, and fixed-I2C/reset host configurations.
- The interactive SDL3 frontend cannot be linked locally in the current build environment because `github.com` is unavailable for CMake FetchContent; the SDL-free simulator path is fully built and tested.

## [0.19.0-alpha.19] - 2026-09-07

### Changed

- Native-simulator output LEDs now use a 75 ms perceptual activity hold at 1× virtual time, scaled with the selected simulator speed. The jack `HI`/`LO`/`HZ` annotation and timing waveform remain tied to the instantaneous gate level.
- The developer timing view now reports the most recently completed physical gate width per output in milliseconds, making the factory-default 10 ms trigger explicit instead of relying on a four-second waveform scale.
- Encoder rotation keys now accept SDL key-repeat, natural/flipped mouse-wheel direction is normalized, wheel rotation works across the module panel except the dedicated SYNC IN target, and the virtual encoder indicator visibly follows requested detents.

### Fixed

- Fixed short gate pulses appearing only sporadically on the simulator LEDs when a complete HIGH interval occurred between two rendered desktop frames.
- Fixed encoder interaction feeling inert when a rotation key was held, because the frontend globally discarded SDL repeat events and the knob graphic never moved.

### Validation

- Production host matrix remains green: 44 core tests; 1450 I2C / 1448 SPI / 1448 fixed-I2C-reset firmware checks; 96.83% lines, 100.00% functions, 90.10% decision branches.
- SDL-free simulator smoke/runtime integration tests pass (2/2), including encoder visual-position telemetry and measured default 10 ms gate-width telemetry.

## [0.19.0-alpha.18] - 2026-09-07

### Added

- Added power-loss-safe A/B persistence using STM32F401 Flash sectors 1 and 2 (16 KiB each), with a 12 KiB hard maximum logical-image policy and no external NVM.
- Added a persistence-preserving DFU uploader that flashes sector 0 and sectors 3..5 separately, plus one-time migration of the former sector-5 STM32duino EEPROM image when no valid A/B generation exists.
- Added host regressions for interrupted commits, both A/B fallback directions, malformed slot metadata, CRC fallback, zero-length defensive pattern cycles, and persistence upload-region invariants.
- Added a deterministic cross-mode synchronization matrix covering CLOCK/EUCLID/SEQUENCER equivalence across multiple master beat units, rational rates, swing, phase, and live per-channel rescheduling.

### Changed

- Replaced local remainder/swing reschedule state with an epoch-bound `eventSerial`; exact cumulative rational offsets, swing parity, and GLOBAL pattern position are reconstructed from that shared serial.
- GLOBAL CLOCK, EUCLID, and SEQUENCER channels now derive both edge timing and pattern step from the same schedule epoch instead of maintaining independently drifting local counters.
- Added a custom STM32F401 linker layout: sector 0 remains the vector/boot region, sectors 1/2 are persistence A/B, and sectors 3..5 provide the main application region. Firmware retains 224 KiB total Flash.
- `PersistentStorage` now obtains the selected generation header directly from A/B slot arbitration instead of revalidating the active slot a second time.

### Fixed

- Fixed `PHASE=0` scheduling one full interval later than a small positive phase; the first zero-phase event now lies on the shared downbeat.
- Fixed live CLOCK→EUCLID/SEQ mode changes and reschedules preserving edge timing while evaluating the wrong pattern step.
- Fixed live per-channel rescheduling restarting swing with local parity and permanently offsetting an otherwise equivalent GLOBAL channel.
- Fixed live Euclid `STEPS` and Sequencer `LENGTH` changes leaving the runtime step in the old cycle until a later wrap.
- Fixed rational-rate rescheduling estimating event position from `elapsed / floor(interval)`, which could skip an event because fractional remainder history was not reconstructible.

### Validation

- Core tests: 44 tests, 0 failures. Full firmware: 1450 I2C / 1448 SPI / 1448 fixed-I2C-reset checks, 0 failures.
- Coverage: 96.83% executable lines, 100.00% functions, and 90.10% non-throw decision branches.
- ASan/UBSan are clean for core, I2C, SPI, and fixed-I2C/reset configurations.
- SDL-free simulator smoke and runtime integration tests pass (2/2).
- Architecture guardrails pass; release/persistence tooling tests pass (26/26).

## [0.19.0-alpha.17] - 2026-09-07

### Added

- Added dedicated regression coverage for external-sync gate release, monotonic master time, external phase alignment, ISR-safe pulse injection, immediate full-state mute/OFF handling, and the fastest legal scheduler configuration.
- Added backward-compatible persistence migration from schema v3 (`CUR3` / `PRE3`) to schema v4, including CURRENT and all eight user-preset slots while preserving unrelated NVM such as the Pixel Raid high score.
- Added an ISR-safe `acceptExternalPulseFromIsr()` engine entry point for the future STM32 timer Input Capture boundary.
- Added simulator telemetry and regression coverage that distinguishes generator PPQN from the firmware-configured external-sync PPQN.

### Changed

- Split clock timing and external-sync implementation into dedicated translation units so the engine remains within the repository file-size guardrail.
- External-sync phase correction now uses a separate musical-position reference instead of moving the monotonic master scheduler position backwards.
- Output scheduling clamps intervals and swing to the 20 kHz scheduler's representable timing quantum instead of silently collapsing distinct legal trigger events.
- Changing the master beat unit now reschedules channel events immediately.
- Factory templates preserve the live transport state instead of overwriting the application state with STOP while the engine may still be running.
- Simulator pulse injection now interprets the generated pulse train through the firmware PPQN setting rather than bypassing that configuration.

### Fixed

- Fixed External Sync gate-length limiting using the stored internal BPM instead of the effective external tempo, which could keep a fast retriggered output permanently HIGH.
- Fixed external pulse alignment corrupting beat/bar phase after acquisition and allowing `masterPositionQ32_` to move backwards on late pulses.
- Fixed full-state configuration updates leaving a newly muted or OFF channel HIGH until a later scheduler event.
- Fixed legal high-rate/swing combinations losing trigger edges when their short interval fell below one scheduler tick.
- Fixed factory-template application desynchronizing `ClockState::transport` from the actual engine transport and thereby allowing persistence commits while gates were still running.

### Validation

- Core tests: 44 tests, 0 failures. Full firmware: 1187 I2C / 1185 SPI / 1185 fixed-I2C-reset checks, 0 failures.
- Coverage: 96.87% executable lines, 100.00% functions, and 90.17% non-throw decision branches.
- ASan/UBSan are clean for core, I2C, SPI, and fixed-I2C/reset configurations.
- SDL-free simulator smoke and runtime integration tests pass, including firmware/generator PPQN mismatch behavior.
- Architecture guardrails pass with all production implementation files within the configured size limit.

## [0.19.0-alpha.16] - 2026-09-06

### Added

- Added ergonomic simulator keyboard aliases for encoder, PLAY, TAP, STOP/BACK, virtual-time speed, developer view, screenshots, and virtual SYNC IN control.
- Added mouse-specific behavior: wheel-over-encoder rotates, PLAY/TAP/STOP remain clickable, left-clicking SYNC IN inserts/removes a virtual cable, right-clicking SYNC IN runs/holds the generator, and wheel-over-SYNC adjusts its BPM.
- Added a virtual SYNC IN pulse generator with 1..300 BPM, firmware-supported 1/2/4/24 PPQN, acquisition/lock/loss state, pulse counters, and injection into the real clock engine after the not-yet-final analogue/input-capture boundary.
- Added per-output `HI` / `LO` annotations at all eight simulated output jacks, with `HZ` shown whenever the HCT244 output stage is disabled. The existing eight activity LEDs remain separate and are positioned directly above their matching jacks.
- Added full-panel windowless BMP screenshot rendering via `--screenshot`, optional virtual-time advancement via `--screenshot-after-ms`, and interactive F12 capture.
- Added `--display-scale N` and strict integer-only OLED scaling from 1× through 8×.

### Changed

- The SDL logical presentation now uses integer scaling instead of fractional letterbox scaling, and each 128×64 OLED pixel is rendered as an exact integer-sized rectangle with no antialiasing.
- External source timing can retain milli-BPM precision. External/Auto source selection now follows the simulator's filtered external tempo, and simulator pulse edges establish/refresh a shared musical phase reference without duplicating channel timing logic.
- Simulator integration tests now assert LED-above-jack geometry, integer display scaling, SYNC IN hit testing, generated sync acquisition, PPQN changes, and loss timeout behavior.
- Added an SDL-enabled CTest that creates a full-panel screenshot through the off-screen software renderer without opening a desktop window.

### Validation

- SDL-free full-firmware simulator build and both CTest runtime integrations pass with strict warnings enabled.
- SDL frontend sources compile cleanly against the SDL3 API surface used for integer presentation, off-screen software rendering, render readback, BMP output, mouse-wheel coordinates, and image loading.
- Core tests: 44 tests, 0 failures. Full firmware: 1139 I2C / 1137 SPI / 1137 fixed-I2C-reset checks, 0 failures.
- Coverage: 96.92% executable lines, 100.00% functions, and 90.46% non-throw decision branches.
- ASan/UBSan are clean for core, I2C, SPI, and fixed-I2C/reset configurations.

## [0.19.0-alpha.15] - 2026-09-06

### Added

- Added `sim/panel_layout.ini` as a strict, human-editable front-panel layout definition using physical millimetre coordinates for the OLED, encoder, PLAY/TAP/STOP buttons, SYNC IN, eight output jacks, eight LEDs, and mounting screws.
- Added optional simulator front-panel artwork loading. SDL3 3.4+ loads BMP, PNG, or JPEG directly; no SDL_image dependency is introduced.
- Added `--layout FILE` and `--panel-image IMAGE` simulator command-line overrides. Relative image paths in the INI file resolve relative to the layout file.
- Added regression tests for physical-to-logical coordinate mapping, configurable hit testing, background-image path resolution, and command-line image override behavior.

### Changed

- The simulator renderer and mouse hit testing now consume the same runtime `PanelLayout` object instead of compile-time coordinates, preventing visual/control drift when the panel is rearranged.
- `scripts/run_simulator.py` automatically supplies the committed `sim/panel_layout.ini` unless the user explicitly provides another layout.
- OLED pixels scale into the configured display rectangle, and jack/LED geometry scales with the configured physical panel dimensions.
- Built-in simulator labels and mounting screws can be disabled independently when a production front-panel image already contains its own graphics.

### Validation

- The complete firmware still builds and runs through the SDL-free simulator headless target with strict warnings enabled.
- Simulator runtime/layout CTests pass, including custom millimetre geometry and persistence integration.
- SDL frontend source compile-checks cleanly against the SDL3 3.4 API surface used for image loading and texture creation.

## [0.19.0-alpha.14] - 2026-09-06

### Added

- Added a native Windows/macOS/Linux simulator that executes the real firmware above a dedicated simulator hardware boundary instead of reimplementing the UI.
- Added an SDL3 front-panel application for the intended 10 HP / 3U module layout, including the exact 128×64 firmware framebuffer, encoder, PLAY/TAP/STOP buttons, SYNC IN, eight output jacks, and eight activity LEDs.
- Added a developer timing view with four seconds of gate history, per-output rising-edge counters, BPM, transport, operating topology, virtual-time speed, and output-buffer state.
- Added 1× / 4× / 16× virtual-time acceleration for long-duration clock, screensaver, persistence, and phase-drift investigation.
- Added file-backed simulator persistence while retaining the firmware's real persistence service and delayed-write policy.
- Added SDL-free headless simulator smoke/integration tests that boot the complete firmware, drive real debounced controls, advance the real scheduler, verify gate edges, verify OLED output, and exercise persistence.
- Added portable CMake presets, VSCodium simulator tasks, and native simulator documentation for Windows, macOS, and Linux.
- Added SDL3's zlib license and third-party notice; SDL is simulator-only and is never linked into the STM32 firmware target.

### Changed

- Extended architecture guardrails to cover simulator implementation files and colocated simulator headers.
- Simulator CMake warning policy is compiler-aware: MSVC uses strict `/W4 /WX`, while GCC/Clang retain the existing strict warning set with `-Werror`.
- CI now builds/tests the headless simulator on Windows, macOS, and Linux and compile-checks the SDL3 frontend separately.

### Validation

- The complete firmware builds natively against the simulator HAL with strict warnings enabled.
- Headless simulator smoke and runtime integration tests pass without SDL.
- Interactive SDL frontend source is compile-checked against the SDL3 API; real SDL3 compilation is a dedicated CI gate.

## [0.19.0-alpha.13] - 2026-09-06

### Added

- Added configurable STOP-mode OLED protection under `PREFERENCES > SCREENSAVER`.
- Added screensaver **MODE 1** (animated fixed-point fractal), **MODE 2** (sparse orbital animation), and **MODE 3** (no animation; DIM/OFF protection still applies).
- Added editable inactivity thresholds for screensaver start, OLED dimming, and OLED power-off.
- Added native SSD1306 contrast and display-power controls while preserving display RAM for immediate wake-up.

### Changed

- Factory display defaults are now: MODE 1, screensaver after **2 min**, dim after **5 min**, OLED off after **10 min**.
- Any encoder or button activity wakes the OLED immediately, restores normal contrast, cancels the idle animation, and preserves the current UI/navigation context.
- Screensaver animation runs only while transport is `STOP`; PLAY and PAUSE keep the ordinary clock UI active.
- Persistent configuration schema advanced to v4 so CURRENT and all eight named preset slots include the complete display-protection preferences.

### Validation

- Deterministic core: 44 tests, 0 failures.
- Complete host firmware: screensaver renderer/settings/persistence and STOP→DIM→OFF→wake behavior are covered in I2C, SPI, and fixed-I2C/reset-pin configurations.

## [0.19.0-alpha.12] - 2026-09-06

### Added

- Added explicit **mode-change confirmation** with a safe-default `NO` before changing channel generators or switching into/out of either global operating topology.
- Added highlighted mode names to the top bar of the 2×3 pictographic mode palette; Unified Clock and Divider Bank additionally show short explanatory text while highlighted.
- Added global-mode overview screens: in Unified Clock or Divider Bank, the overview no longer pretends that eight independent channels are selectable and instead shows the active global-mode symbol and full mode name.
- Added an eight-slot Divider Bank performance grid below the centered tempo. Each output shows its effective multiply/divide factor using graphical multiply/divide notation.
- Added a fail-closed LGPL binary-packaging guard. `scripts/package_release.py` now requires an explicit static-link compliance acknowledgement before it will package BIN/ELF files.

### Changed

- Channel overview tiles now show only channel number `1…8` plus the current mode pictogram; the redundant `CH` prefix is removed from the overview only.
- Confirmed mode changes route immediately to the most relevant working screen: OFF/CLOCK → Performance, EUCLID → algorithm settings, SEQUENCER → editor, Unified Clock → shared-clock settings, Divider Bank → divider-family setting.
- Divider Bank uses a compact 2×4 output-factor display on Performance so all eight derived clocks are visible simultaneously.
- GitHub releases are now **source-only** while the target statically links LGPL-covered STM32duino code. CI still builds I2C and SPI targets but deliberately does not upload or attach BIN/ELF artifacts.
- Documented direct STM32Cube HAL/LL + CMSIS migration as the preferred long-term resolution of the LGPL static-link distribution burden.

### Fixed

- Fixed the ARM GCC compile error in `PixelRaidGame::captureHighScoreName()` caused by an extra initializer-brace level around the four-character local name buffer.

## [0.19.0-alpha.11] - 2026-09-06

### Added

- Added full **1–300 BPM** master-tempo support, including a 60-second Tap Tempo interval for 1 BPM and a 65-second tap-sequence reset window.
- Added **PIXEL RAID**, an original hidden fixed-shooter entered by holding the encoder push switch through the complete boot screen. Encoder moves the cannon, TAP fires, a game-only encoder long-press opens an EXIT YES/NO dialog, and new high scores use three-letter initials.
- Added CRC-protected persistent game high score storage in a dedicated NVM region.
- Added game-only life-loss LED flashes while the external 74HCT244 output stage remains disabled.
- Added `docs/LICENSING.md` documenting why LGPL-2.1, BSD-3-Clause, and Apache-2.0 notices are all relevant to the current STM32duino build and identifying static-linked LGPL binary distribution as a pre-stable compliance item.

### Changed

- Non-zero Swing is rendered flush-left as only its percentage. Zero Swing remains hidden.
- Non-`x1` Clock rate is rendered flush-right; `x1` remains hidden. The tempo remains mathematically centered independently of both side annotations.
- Extended exhaustive master-accumulator verification to every integer BPM from **1 through 300**.
- Extended Tap Tempo timing policy to accept intervals from 200 ms through 60 s.
- Updated prerelease documentation and low-tempo UI stress rendering for the new 1 BPM lower bound.

### Safety

- Pixel Raid runs before the real-time scheduler starts and keeps gate-buffer `/OE` disabled throughout. LED effects occur only on the MCU side of the buffer; physical gate outputs remain high-impedance rather than being intentionally driven HIGH. Normal firmware still starts in STOP after leaving the game.

### Licensing

- Kept all three bundled third-party license texts because each is currently relevant: LGPL-2.1 for Arduino/STM32duino-derived portions, BSD-3-Clause for STM32F4 HAL/LL, and Apache-2.0 for CMSIS plus Roboto Condensed-derived tempo raster data.
- Documented that the PolyForm Noncommercial project license coexists with those component licenses but does not supersede them, and that final statically linked binary distribution needs an explicit LGPL compliance strategy.

### Validation

- Deterministic core: 44 tests, 0 failures.
- Complete host firmware: 1063 checks in I2C and 1061 checks in SPI/fixed-I2C-reset configurations, all with 0 failures.
- Project-owned coverage: 3346/3443 executable lines (97.18%), 279/279 functions (100.00%), 1903/2114 source decision branches (90.02%).
- ASan/UBSan: deterministic core plus I2C, SPI, and fixed-I2C/reset-pin complete-firmware variants clean.

## [0.19.0-alpha.10] - 2026-09-06

### Added

- Added two top-level eight-output operating functions to the graphical mode palette: **Unified Clock** drives all eight outputs from one shared clock configuration, while **Divider Bank** exposes fixed powers-of-two, integer, or prime divider families across the eight outputs.
- Added `OperatingMode`, unified-clock settings, divider-bank settings, physical-output resolution, persistence schema v3 fields, and dedicated settings pages for the new global functions.
- Added direct resolver, persistence, editor, renderer, and mode-palette regression coverage for all six graphical functions and all three divider families.

### Changed

- Reworked the graphical mode selector into a pure-symbol **2×3 palette**: OFF, Clock, Euclid, Sequencer, Unified Clock, and Divider Bank. No abbreviations are drawn inside the tiles.
- Simplified the channel overview to eight quiet 2×4 tiles containing only `CH1`…`CH8` and the current mode pictogram. The selected channel is fully inverted rather than double-outlined.
- Changed the live workflow: encoder push from Performance opens the channel overview; encoder rotation selects a channel immediately; encoder push on the overview opens the relevant settings; **TAP + turn** on the overview opens the six-function mode palette and releasing TAP commits the selection.
- `TAP + encoder push` from Performance remains the explicit shortcut to the combined Settings tree. Encoder long-press and long-TAP shortcuts are removed.
- Unified Clock and Divider Bank are rendered as global clock contexts while preserving the centered master tempo and common deterministic scheduler.
- Factory templates explicitly return to independent per-channel operation.

### Fixed

- Preserved the shared musical epoch when resolving Unified Clock and Divider Bank outputs, so switching output-function topology does not introduce a new per-channel timing origin.

## [0.19.0-alpha.9] - 2026-09-06

### Added

- Added a shared musical scheduling epoch for GLOBAL channels and a regression test proving Clock, Euclid, and Sequencer rising edges remain phase-coherent after live channel rescheduling.
- Added a compact 2×4 `SELECT CHANNEL` overview showing only channel number and `O/C/E/S`, with a double outline for the selected channel.
- Added `TAP + encoder push` as the explicit gesture for the combined Settings tree while plain encoder push opens channel selection; encoder long-press remains removed.
- Restored mode-aware long-TAP shortcuts from Performance directly to Euclid or Sequencer parameters without generating a tap-tempo event.
- Added scroll-band preset name entry showing the surrounding character repertoire and an explicit safe-default overwrite confirmation for occupied slots.
- Expanded INFO with author **Axel Napolitano**, project license, STM32duino/Core/HAL/CMSIS license identifiers, and the tempo-font attribution/license.
- Added `THIRD_PARTY_NOTICES.md` plus bundled LGPL-2.1, BSD-3-Clause, and Apache-2.0 license texts; release packaging now includes and checksums these files.
- Replaced the tempo numeral artwork with native-resolution 1-bit raster glyphs derived from Roboto Condensed Bold (Apache-2.0); the font file itself is not distributed.

### Changed

- Channel Settings now expose only parameters meaningful to the active channel mode. OFF exposes only MODE; active modes expose their common parameters plus exactly their own Clock, Euclid, or Sequencer configuration page.
- Simplified the performance display: non-zero Swing is rendered as only `nn%`, zero Swing is hidden, and Clock `x1` is omitted. Clock-mode BPM uses a larger, fuller sans-serif role with revised vertical balance.
- Euclid performance visualization now follows the same compact step grammar as Sequencer: filled event cells, outlined rests, and a two-pixel current-step underline.
- Releasing a Press-Turn channel selection still commits immediately; a plain push in the overview commits the highlighted channel as well.
- Documentation and release metadata now distinguish the PolyForm Noncommercial project license from third-party component licenses.

### Fixed

- Fixed a persistent phase offset that could appear between Clock, Euclid, and Sequencer channels after a live mode/rate/configuration update. GLOBAL channels now align the next event to the shared musical epoch instead of scheduling from `now + interval`.
- Preserved the previously added live-stall protections: Flash commits remain deferred during PLAY and SSD1306 transfers remain dirty-page only.

### Validation

- Deterministic core: 44 tests, 0 failures.
- Complete host firmware: 746 checks in I2C, 744 in SPI, and 744 in fixed-I2C/reset-pin configuration, all with 0 failures.
- Project-owned coverage: 2843/2907 executable lines (97.80%), 244/244 functions (100.00%), 1639/1793 source decision branches (91.41%).
- ASan/UBSan: deterministic core plus I2C, SPI, and fixed-I2C/reset-pin complete-firmware variants clean.

## [0.19.0-alpha.8] - 2026-09-06

### Added

- Added an explicit per-channel `OFF` mode. OFF channels are removed from event scheduling, keep their logical gate LOW, use mode marker `O`, and replace the centered tempo readout with `---`.
- Added a larger native Clock-mode tempo font and revised both tempo numeral sets toward fuller rounded sans-serif shapes.
- Added matched Euclid/Sequencer 16-step performance strips with a two-pixel current-step underline and 16-step block indicators for patterns longer than one block.
- Added regression tests for OFF scheduling/persistence/UI behavior, deferred Flash commits, dirty-page OLED refresh, unchanged-frame suppression, and Press-Turn edge cases.

### Changed

- Reworked the performance header to `M/S [lock] CHn O/C/E/S` on the left, centered master meter, and right-aligned transport. `M` is filled and `S` is outlined.
- Moved non-zero Swing to the left of the mathematically centered BPM display as `SW nn%`. Clock rate is shown on the right without shifting the centered tempo.
- Clock-only presentation uses the larger BPM role because it does not need a bottom pattern strip.
- Press-Turn is now the sole fast channel-selection gesture: the horizontal selector uses outlined channel cells with side margins, and releasing the encoder commits the candidate. The old 2x4 overview and encoder long-press gesture were removed.
- A normal encoder short press from Performance now opens Global Settings. Long-TAP remains the direct mode-aware parameter shortcut.
- The graphical mode selector now presents `O / C / E / S`.
- Physical persistent-storage writes are deferred while transport is PLAYING. Complete state and preset changes remain staged in RAM and flush after PAUSE/STOP.
- SSD1306 refresh now compares framebuffer pages and sends only dirty page runs; unchanged frames generate no display-bus transfer.
- Split OLED bus transport into a focused companion translation unit while retaining the implementation-size architecture guard.

### Fixed

- Removed two identified foreground stall sources: full-frame OLED writes for small visual changes and STM32 Flash commits during active playback. Hardware endurance verification remains part of the HIL plan.
- Corrected UI/control tests and navigation semantics after removal of the obsolete channel-overview and encoder-long-press paths.

### Validation

- Deterministic core: 44 tests, 0 failures.
- Complete host firmware: 701 checks in I2C, 699 in SPI, and 699 in fixed-I2C/reset-pin configuration, all with 0 failures.
- Project-owned coverage: 2677/2748 executable lines (97.42%), 241/241 functions (100.00%), 1524/1676 source decision branches (90.93%).

## [0.19.0-alpha.7] - 2026-09-06

### Fixed

- Worked around an ARM GCC 12.3 internal compiler error in `PersistentStateService::begin()` by replacing assignment from anonymous `ClockState{}` aggregate temporaries with a named default-initialized state object. Runtime behavior and the persistence schema are unchanged.

## [0.19.0-alpha.6] - 2026-09-06

### Added

- Added complete CRC-32/schema-protected persistence for the full working configuration, including all eight channel modes, Euclid settings and 64-step Sequencer patterns.
- Added one automatically maintained `CURRENT` configuration plus eight explicitly named user preset slots.
- Added a 16-character encoder-driven high-score-style preset name editor and `PREFERENCES` load/save workflow.
- Added the Press-Turn `SELECT CHANNEL` strip: hold the encoder, turn to select channel 1–8, release to commit.
- Added VSCodium/Open VSX extension recommendations, portable folder-icon/workspace-color settings, and repository guidance for avoiding local-path leakage.
- Added a release `NOTICE.txt` carrying the PolyForm Required Notice and included it in packaged/checksummed release artifacts.
- Added regression coverage for the PlatformIO/SCons post-build memory hook being invoked with keyword arguments.

### Changed

- Euclid and Sequencer `x1` now use a sixteenth-note pattern grid. In 4/4, a 16-step pattern spans exactly one bar instead of four.
- Autosave now persists the entire validated working state rather than transport metadata alone. Boot still always forces runtime transport to STOP.
- User preset loads preserve the current live transport state while replacing the complete musical/sync configuration.
- `.gitignore` now excludes generated compile databases, clang/index caches, local VSCodium metadata, environment/secrets, credentials and private-key/certificate material.
- CI coverage floors are set to 95% executable lines, 95% functions and 90% source decision branches; ASan/UBSan and strict host warnings remain hard gates.
- UI controller and persistence implementations are split into focused companion translation units to retain the repository's implementation-size guard.

### Fixed

- Fixed `scripts/platformio_memory_gate.py` so its SCons post-action accepts `target`, `source` and `env` keyword arguments, resolving `TypeError: check_memory() got an unexpected keyword argument 'env'`.
- Fixed a Press-Turn edge case where an encoder detent arriving in the same sample as the press edge could be misclassified as a normal short press.
- Corrected the pattern scheduler base rate that made Sequencer and Euclid tracks appear substantially too slow.

## [0.19.0-alpha.5] - 2026-09-06

### Added
- Added dedicated native-resolution tempo numerals for the BPM display; the performance tempo no longer scales the small 5x7 UI font.
- Added Euclidean pattern visualization on the performance screen with a live current-step underline.
- Added Sequencer playback visualization with a live current-step underline and 2-pixel 16-step block indicators for sequences longer than 16 steps.
- Added compact performance status badges: filled `M` for master, outlined `S` for slave, optional lock icon, active channel number, and one-character `C/E/S` mode status.
- Added long-TAP shortcuts from the performance screen directly to Euclid parameters or Sequencer parameters.
- Added CRC-protected, wear-coalesced persistence for the last user-selected transport state.
- Added a boot regression test proving that a persisted PLAY state never causes automatic gate output after power-up.
- Added STM32 Flash/RAM post-link memory gates and reserved the final F401 Flash sector for persistent storage.
- Added `docs/DEVELOPER_README.md` with a VSCodium-oriented setup workflow for Windows, macOS, and Linux.
- Added VSCodium/VS Code tasks for common build, upload, test, architecture, and coverage commands.

### Changed
- Power-up now always enters STOP, regardless of the persisted transport metadata. The persisted value remains available for future UI/state-recovery behavior but is never auto-resumed.
- Removed the blinking playback square from the performance screen.
- Moved mode-specific configuration access higher in the channel menu; Sequencer long-TAP lands directly on `LENGTH`.
- Relaxed CI coverage thresholds to a maintainable policy of 98% executable lines, 100% functions, and 95% source decision branches. The current baseline still reaches 100% for all three metrics.
- Limited the application Flash region to 128 KiB because STM32duino EEPROM emulation owns F401 sector 5; the project additionally enforces a 90% application-Flash and 90% static-RAM budget.

### Fixed
- Fixed the BPM font regression that made the main tempo look like an enlarged small bitmap font.

## [0.19.0-alpha.4] - 2026-09-06

### Added

- Added complete decision-branch coverage for project-owned production firmware. The coverage gate now distinguishes source-level decision branches from GCC-generated exception/throw edges and requires 100% of the former.
- Added AddressSanitizer and UndefinedBehaviorSanitizer host matrices for the deterministic core plus I2C, SPI, and fixed-I2C/reset-pin firmware variants.
- Added fail-on-warning (`-Werror`) to host coverage and sanitizer builds.
- Added additional UI no-op and boundary tests for refresh throttling, non-context button presses, repeated long-press states, CLOCK/EUCLID routing, and settings-back navigation.
- Added exhaustive master-accumulator verification across every supported BPM from 20 through 300 and broader Swing interval/property checks.
- Expanded release-tooling tests with version-agnostic expectations, architecture-policy execution, missing-version and missing-license failure cases.

### Changed

- CI and release validation now gate on 100% executable-line, function, and non-throw decision-branch coverage. Raw GCC compiler-branch coverage remains in the report as a transparent informational metric.
- Coverage and ASan/UBSan execution can run as separate commands so CI reports failures independently.
- Narrowing-sensitive HAL/UI arithmetic now uses explicit conversions and passes the strict warning set without diagnostics.

### Validation

- Deterministic core: 43 tests, 0 failures.
- Complete host firmware: 493 checks in I2C, 490 in SPI, and 491 in fixed-I2C/reset-pin configuration, all with 0 failures.
- Project-owned executable coverage: 1840/1840 lines (100%), 162/162 functions (100%), 1076/1076 decision branches (100%).
- Raw GCC compiler branches: 1077/1261 (85.41%), reported separately because throw/exception edges are compiler-generated rather than source decisions.
- ASan/UBSan: core plus all three complete-firmware host configurations pass with no sanitizer findings.

## [0.19.0-alpha.3] - 2026-09-06

### Added

- Added complete host-side firmware tests covering Application, Engine, Services, UI, HAL, display transport, controls, and framework entry points in addition to the existing deterministic core suite.
- Added deterministic Arduino/Wire/SPI/HardwareTimer host fakes used only by tests.
- Added `scripts/run_host_tests.py`, which compiles and executes I2C, SPI, fixed-I2C/reset-pin, deterministic-core, and native-smoke variants and aggregates GCC JSON coverage.
- Added CI/release gates requiring **100% executable-line coverage** and **100% function coverage** across project-owned firmware code.
- Added an aggregate branch-coverage regression gate; current full-firmware baseline is **81.8%**.

### Changed

- Moved the last executable header-only helpers (UI text lookup, pixel-font glyph lookup, and beat-indicator derivation) into colocated `.cpp` files so every function is visible to dynamic coverage.
- Architecture policy now rejects new executable header-only functions in production code.
- Full host coverage now exercises both OLED transports and the optional OLED reset/fixed-address configuration branches.
- Replaced the old core-only gcovr gate in GitHub Actions with the complete firmware host-coverage gate.

### Fixed

- Added missing channel-index validation in `SettingsEditor`; invalid channel indices can no longer index beyond the 8-channel state array.
- Removed mathematically unreachable Swing defensive code and other unreachable UI-only coverage branches instead of hiding them from coverage.

### Validation

- Deterministic core: 40 tests, 0 failures.
- Full host firmware: 482 checks in the I2C variant, 479 in SPI, 480 in fixed-I2C/reset-pin, all with 0 failures.
- Aggregate project-owned coverage: 1835/1835 executable lines (100%), 161/161 functions (100%), 1038/1269 branches (81.8%).

## [0.19.0-alpha.2] - 2026-09-06

### Fixed

- Restored the Arduino framework linkage contract for global `setup()` and `loop()` by including `Arduino.h` in the framework entry-point translation unit.
- Added a native smoke entry point and native `build_src_filter` so PlatformIO `Build All` no longer treats test-only native environments as broken applications.
- Set the default PlatformIO build environment to the current I2C prototype firmware; SPI and native variants remain explicitly buildable.
- Updated the architecture policy so `src/main.cpp` may include the Arduino framework declaration while direct hardware APIs remain restricted to HAL/pin mapping.

### Validation

- ArduinoCore-API declares `setup()` and `loop()` inside `extern "C"`; the entry-point source now intentionally inherits that linkage.
- Native unit tests remain isolated from `src/` because PlatformIO `test_build_src` defaults to `no`.

## [0.19.0-alpha.1] - 2026-09-06

### Changed

- Marked the firmware explicitly as a prerelease (`0.19.0-alpha.1`).
- Moved global compile-time behavior to `src/config.h`.
- Moved factory user-setting defaults to editable `src/defaults.h`.
- Moved human-readable hardware wiring to `src/pin_map.h`.
- Colocated project headers with their implementation files under `src/`.
- Centralized all static user-visible firmware text in `src/ui_text.h` with a localization-ready `TextId` catalog.
- Replaced Adafruit GFX and Adafruit SSD1306 with a small project-owned SSD1306 framebuffer/primitive driver.
- Added compile-time I2C/SPI display transport selection and an SPI CI compile-check.
- Removed all third-party PlatformIO `lib_deps`.
- Added architecture checks for header colocation, UI-text centralization, and external-library policy.

### Hardware configuration

- Current I2C display mapping remains PB6/PB7.
- Planned SPI1 display mapping is PA5 SCK, PA7 MOSI, PA4 CS, PB9 D/C; reset remains optional.
- External-sync signal and jack-detect pins remain deliberately unassigned until final input routing is frozen.

## [0.18.0] - 2026-09-06

### Added

- Explicit HAL, Domain, Engine, Services, UI, and Application layers.
- Dedicated `PerformanceRenderer`, `ChannelNavigationRenderer`, and `SettingsRenderer` components instead of one large screen renderer.
- Dedicated `SettingsEditor` so navigation and validated configuration mutation have separate responsibilities.
- Repository architecture policy check (`scripts/check_architecture.py`) enforced by CI and release workflows.
- `docs/ARCHITECTURE.md` dependency and ownership documentation.
- Repository `Doxyfile` for generated API documentation.

### Changed

- Reduced `src/main.cpp` to framework delegation only; all subsystem ownership now lives in `ClockApplication`.
- Moved all Arduino, Wire, Adafruit, GPIO, wall-clock, interrupt, and `HardwareTimer` dependencies behind HAL components.
- Renamed timing, pattern, channel, and configuration APIs for descriptive US-English naming.
- Added comprehensive Doxygen documentation and standard author/license headers to project-owned source files.
- Enforced GNU C++17 explicitly for the STM32 build.
- Refined boot ordering so the timing engine starts only after gate outputs have been placed in a defined safe state.

### Quality

- Refactored firmware compiles cleanly against the STM32/Arduino API stubs with `-Wall -Wextra -Wpedantic -Wshadow -Wconversion -Wsign-conversion`.
- Existing 40-test deterministic core suite remains green after the structural refactor.
- No intended musical feature change; this release establishes a maintainable architecture for subsequent hardware and External Sync work.

## [0.17.6] - 2026-09-06

### Added

- PolyForm Noncommercial License 1.0.0 (`PolyForm-Noncommercial-1.0.0`) as the project software license.
- 40 native production-core test cases plus exhaustive/property checks.
- Deterministic RNG/Probability tests, Gate Length limiting tests, sequencer pattern-mask/invert/fill tests and broader timing edge cases.
- Exhaustive Euclidean hit-count validation across every supported Steps/Hits combination from 1 to 64.
- GCC/gcovr coverage reporting with CI gates for line and branch coverage.
- Python tests for version/tag validation, release-note extraction, packaging, checksums and license presence.
- License file in build/release packages and SHA-256 manifests.

### Changed

- Extracted additional production helpers into `clock_core` so Probability, gate-width limiting, step wrapping and pattern operations are directly unit-tested.
- Sequencer invert now masks the result to the configured pattern length, preventing hidden high bits from becoming gates after a later length increase.
- Alternating Fill now uses the shared tested pattern helper.

## [0.17.5] - 2026-09-05

### Added

- GitHub Actions CI for native unit tests and STM32F401 firmware builds.
- Tag-driven GitHub Release workflow with version validation, firmware packaging and SHA-256 checksums.
- Hardware-independent `clock_core` library shared by firmware and native tests.
- Initial automated tests for rational rate math, fractional accumulation, swing pair duration, phase, Euclidean patterns, sequencer rotation and reset policy.
- Reproducible release artifact naming and changelog-derived release notes.

### Changed

- Firmware version metadata moved to `include/version.h` as the release source of truth.
- Core timing/pattern helpers were extracted from `main.cpp` so CI tests production logic rather than duplicated test implementations.

## [0.17.4] - 2026-09-05

### Changed

- Reworked the 1000 ms boot animation to use a continuous 2-pixel progress bar along the bottom edge.
- Boot text uses the native 1:1 bitmap font.
- Removed the boot LED chase because LEDs tied to gate outputs would create unwanted output pulses.
- Gate outputs remain LOW and the output stage remains disabled for the complete boot interval.

## [0.17.3] - 2026-09-05

### Added

- Initial 1000 ms boot screen with module name and firmware version.
- Initial output-enable handling for the planned 74HCT244 gate stage.

### Changed

- Timing engine starts only after the boot sequence has completed.
- All channel logic pins are forced LOW before the output stage is enabled.

## [0.17.2] - 2026-09-05

### Added

- Firmware `INFO` page under Global Settings.
- Firmware version, MCU, timing-engine frequency, compiler build date and build time shown on-device.
- Repository changelog.
- Expanded end-user documentation with embedded pixel-accurate OLED screen renders.

### Changed

- Global Settings now contains `INFO >` before `RESET`.
- Documentation consistently uses the neutral project name **8-Channel Precision Clock**.

## [0.17.1] - 2026-09-05

### Added

- Pixel-accurate UI assets embedded directly into the GitHub Markdown user guide.
- Detailed documentation of Main, Channel Overview, Mode Select, Channel Settings and Sync screens.
- UI stress cases for dense values and long labels.

### Changed

- UI terminology shortened for 128x64 OLED use (`CLK`, `EUC`, `SEQ`, `INT`, `EXT`, `ACQ`, `FREE`).

## [0.17.0] - 2026-09-05

### Changed

- Reduced main tempo font size for better visual balance.
- Introduced graphical 2x4 channel overview as the primary channel-selection screen.
- Introduced graphical `CLK / EUC / SEQ` mode selection.
- Channel settings use five visible rows, `>` selection and value-only inversion while editing.
- Separated global settings from per-channel settings.

## [0.16.0] - 2026-09-05

### Added

- Eight independent channel configurations.
- Per-channel `CLK`, `EUC` and `SEQ` modes.
- Shared hardware-timer-driven engine for all three modes.
- 64-step sequencer representation.
- 64-step Euclidean pattern support.
- Shared per-channel Rate, Swing, Probability, Gate Length, Phase, Reset Mode and Mute.
- Rational clock relationships and retained fractional remainder for long-term timing accuracy.
- `GLOBAL` and `FREE` channel reset behaviour.

### Removed

- Global operating mode.
- MIDI source and MIDI-related UI.

## [0.15.0] - 2026-09-05

### Added

- STM32 hardware-timer-based clock prototype.
- Factory clock templates including All Master, Clock Tree, Dividers, Multipliers and Polyrhythm examples.

### Fixed

- Output/LED timing no longer depends on which OLED screen is open.
- All eight outputs default to the master clock at 1:1.

## [0.14.0] - 2026-09-05

### Added

- Eight channel activity LEDs in firmware.
- Per-channel Clock settings including meter, swing, ratio and rational polyrhythm values.
- External-sync configuration UI prototype.
- Eight-channel overview concept.

## [0.13.0] - 2026-09-05

### Added

- Hierarchical settings with scrollbars.
- Eight-track Euclidean configuration prototype.
- 8x16 gate sequencer editor prototype.

### Removed

- Turing mode, because the design has no CV output and the mode did not fit the product concept.

## [0.12.0] - 2026-09-05

### Changed

- Transport states use text instead of animated pictograms.
- `PLAY` is inverted, `PAUSE` outlined, `STOP` plain text.

## [0.10.0] - 2026-09-05

### Changed

- Removed redundant `BPM` unit from the main display.
- Introduced conditional Swing indication.

## [0.8.0] - 2026-09-05

### Fixed

- Tempo changes no longer restart or stall the running clock phase.

## [0.4.0] - 2026-09-05

### Added

- Graphical mode menu prototype.
- Mode-specific settings concept.

## [0.3.0] - 2026-09-05

### Added

- Encoder and three-button interaction.
- Tap tempo.
- Initial per-output ratio editing.

## [0.1.0] - 2026-09-05

### Added

- Initial STM32F401CCU6 PlatformIO prototype.
- SSD1306 I2C display support.
- Central tempo display and basic clock UI.
