<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# V1 Forward-Compatibility Audit

**Baseline:** `0.19.0-beta.1`  
**Purpose:** prove that the V1 feature freeze does not trap later 1.x work behind avoidable persistence or real-time architecture mistakes.

This audit does **not** implement post-1.0 features. It records the constraints that Swing/Groove 2.0, Sequencer 2.0, Euclid Auto-Fill, Ratchets, structured random, and channel interaction must respect when they are implemented later.

## Result

The current V1 runtime is suitable for release qualification, with one important persistence constraint: **post-1.0 per-step metadata must not be added by simply expanding the repeated ClockState record.** The 1.1 development line uses schema v10 for Pre-Count plus the small Stage-1 Groove assignments, but this does not change the constraint. There is a clean migration path, but the V1 logical layout does not contain enough repeated-record headroom for that approach.

No V1 feature must be added to solve this now. The correct action before 1.0 is to freeze and test the current layout, document the migration boundary, and require a new schema/layout design when the first storage-heavy 1.x feature is implemented.

## Persistent-image map

The V1 logical image is 8,192 bytes inside each power-loss-safe 16-KiB physical A/B Flash slot.

| Region | Offset | Current use |
| --- | ---: | --- |
| Clock state + presets | `0` | CURRENT + eight named presets |
| Legacy score compatibility | `3072` | historical 16-byte single-score records |
| Top-100 leaderboards | `4096` | four independent compact leaderboard records |
| End of V1 logical image | `8192` | current staging/commit image boundary |
| Architectural maximum image | `12288` | policy ceiling; not allocated by V1 |

The boundaries are now centralized in `src/hal/persistent_layout.h` and guarded by compile-time non-overlap assertions.

## Exact V1 state budget

Current development schema v9 uses:

- serialized `ClockState` payload: **256 bytes**;
- CURRENT record: **268 bytes**;
- one named preset record: **284 bytes**;
- CURRENT + eight presets: **2,540 bytes**.

The next fixed region begins at byte 3,072, so only **532 bytes** remain between the current preset area and the legacy-score compatibility region.

Every byte added naively to the serialized ClockState payload is repeated once in CURRENT and once in each of eight presets. It therefore consumes **9 logical-image bytes**. The maximum safe in-place payload growth before colliding with the next region is only:

```text
floor(289 / 9) = 32 bytes
```

This is now a tested V1 contract (`kMaximumInPlaceStatePayloadGrowthBytes == 59`). It corrects the misleading assumption that all unused bytes in the 8-KiB image are available to grow the state record.

## Consequence for post-1.0 Sequencer/Groove work

Eight channels × 64 Sequencer steps = **512 steps**. Even one extra byte of metadata per step would require 512 bytes for one state and 4,608 bytes when repeated across CURRENT + eight presets. Step Probability, gate length/tie, trigger conditions, and custom Groove data therefore require extension records or another explicit layout redesign - not incremental growth of the repeated schema-v9 ClockState payload.

Accepted future approaches include:

1. a redesigned compact fixed layout with explicitly packed, compiler-independent fields;
2. extension records referenced by the base state;
3. sparse/override storage where default step metadata consumes no record space;
4. a controlled logical-image increase up to the 12-KiB policy ceiling if target RAM/Flash measurements prove it safe.

The design must preserve migration from the supported pre-V1 formats, the v7 release-prep format, stable 1.0.x schema v8, and the current schema-v9 development format. Raw C++ structs and compiler bitfields remain prohibited for durable storage.

## 1.1 custom-groove slot target

The accepted 1.1 UX target is **up to 99 named Custom groove slots**, with rename, overwrite and delete support and explicit `YES / NO` confirmation before destructive overwrite/delete operations. This is a target for the Stage-2 custom editor, not permission to enlarge `ClockState`.

The current persistence policy leaves at most **4,096 additional logical bytes** between the V1 8-KiB image and the 12-KiB architectural ceiling. If all 99 Custom slots were stored entirely in that extension, the absolute mathematical ceiling would be about **41 bytes per slot before any shared directory/header overhead** (`4096 / 99`). A realistic fixed-slot design therefore needs a record at or below roughly 40 bytes, or a compact variable/sparse representation. The final feasibility depends on the still-open Custom-groove grid length, marker resolution, name encoding and record-integrity metadata.

Accordingly, 99 slots are **architecturally plausible but not yet release-proven**. Before Stage 2 is approved, the implementation must:

- define the bounded Custom-groove representation and exact serialized byte count;
- keep groove payloads outside the repeated CURRENT/eight-preset `ClockState` records;
- preserve migration from schema v9 and supported 1.0.x records;
- prove the enlarged logical-image/BSS cost with a real STM32F401 ELF and the existing memory gate;
- keep overwrite/delete power-loss-safe under the A/B commit contract;
- add explicit persistence tests for create/load/rename/overwrite/delete, `NO` cancellation and interrupted commits.

If the final groove representation cannot satisfy that budget safely, the slot count must be reduced or the storage architecture changed deliberately; it must not be achieved by weakening the persistence or memory gates.

## RAM consequence of increasing the logical image

`PersistentStorage` deliberately owns one complete staging image in BSS so Flash commits do not create multi-kilobyte stack frames. V1 therefore reserves **8 KiB static RAM** for persistence staging.

Raising the logical image directly from 8 KiB to the 12-KiB policy ceiling with the same implementation would reserve another **4 KiB of BSS**. On the STM32F401CCU6's 64-KiB SRAM this is material. No image-size increase is approved until a real target ELF is built and the existing 90% memory gate plus explicit headroom review pass.

## Real-time architecture boundary

The current host ABI reports `sizeof(ChannelConfig) == 40` bytes. `ClockEngine::updateChannel()` copies one complete `ChannelConfig` while interrupts are masked. This is acceptable for V1 but becomes the wrong mechanism if hundreds of bytes of per-step metadata are inserted into `ChannelConfig` later.

Post-1.0 storage-heavy rhythm data must therefore remain outside the small hot-path channel configuration and use bounded targeted updates or a prepared/double-buffered snapshot strategy. The scheduler remains allocation-free and bounded.

`ClockEngine::fireChannelEvent()` is also intentionally **not** refactored during the V1 freeze. Before Ratchets, Conditions, per-step Probability/Gate metadata, or cross-channel event processing are added, the implementation should introduce a staged event-decision pipeline rather than accumulating feature-specific branches in that function.

## Public-contract decisions frozen for V1

- Hardware Rev 1 remains gate/trigger focused; no analog CV subsystem is required for 1.0.
- Persistent schema v9 is the current development write format. Stable 1.0.x schema v8, schema v7, and the earlier supported formats remain explicit migration sources for upgrades.
- Boot always enters STOP regardless of stored transport history.
- CURRENT and eight named presets remain the user-facing persistence model.
- No future 1.x feature may silently invalidate V1 presets; migration or explicit compatibility handling is required.
- Hardware, simulator, and any future VCV Rack port should consume the same musical core rather than fork timing semantics.

## V1 guardrails enforced by this audit

- one centralized persistent logical-layout header;
- compile-time region-order and non-overlap checks;
- explicit tested constants for V1 state/preset footprint and in-place growth ceiling;
- documentation that distinguishes physical Flash headroom from actually usable schema-v10 repeated-record headroom;
- a release roadmap that forbids new musical features during V1 qualification.

## Open qualification work - not architecture blockers

The following remain physical qualification work and are not inferred from host tests:

- final PCB and comparator validation;
- oscilloscope/logic-analyzer timing and gate-width measurements;
- worst-case eight-channel timing under display activity;
- SPI-vs-I2C HIL comparison;
- SYNC/RST behavior with representative real Eurorack signal sources;
- real target ELF Flash/RAM measurement before any persistence-capacity change;
- decision on timer Input Capture only if HIL demonstrates that the existing EXTI path does not meet the V1 timing requirement.
