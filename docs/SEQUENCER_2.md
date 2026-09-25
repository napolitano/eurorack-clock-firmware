<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Sequencer 2.0 — development contract

This document separates the implemented post-1.1 foundation from the still-planned user-visible Sequencer 2.0 feature. The current 1.1 user guide remains authoritative for released behavior.

## CURRENT development foundation (r45)

- Maximum pattern model length: **128 steps**. The planned eight-step OLED view is a viewport, not a length limit.
- **8 pattern slots per channel × 8 physical channels = 64 pattern slots**.
- Binary gates use two explicit 64-bit words; no heap allocation and no compiler bitfields.
- Pattern-local traversal: `FORWARD`, `REVERSE`, `PINGPONG`, `RANDOM`. PINGPONG does not repeat the end points. RANDOM uses a deterministic, seedable, history-free mapping from event serial to step so a reschedule does not change past/future selection merely because a helper was called a different number of times.
- Pattern-local end behavior: `LOOP` or `ONCE`. RANDOM+ONCE emits exactly LENGTH traversal events; PINGPONG+ONCE emits one complete no-duplicate-endpoint traversal cycle.
- Persistent pattern bank: fixed 2-KiB extension at logical offset 8192, explicit records + CRC, global rather than duplicated in CURRENT/eight presets.
- Legacy migration: each channel's released 1.1 64-bit Sequencer state becomes P1; P2–P8 keep clean defaults.
- Flash writes are coalesced and are not committed while transport is PLAYING.

The engine and UI do **not yet consume this bank**. Released Sequencer execution remains active until the next integration slice.

## PROPOSED 1.2 user model

Each pattern will own Length, Rotate, Play Direction, Loop Mode and its binary/metadata steps. Each step may later override gate state, Probability, Gate Length, Tie and bounded Ratchet behavior; `DEFAULT` means inheritance rather than duplicated metadata.

The editor will show eight steps at a time and scroll across the active 1–128-step pattern. Symbols may summarize overrides, while detailed values remain one level deeper.

## PROPOSED Song Mode

Song Mode is a global topology peer to Independent, One Clock and Divider Bank, not a submenu of one channel. The arrangement grid has eight channel columns. Each row cell chooses `-` or P1–P8 for that channel. Row duration is defined on the master timeline rather than by waiting for all channel patterns to finish.

Polymeter therefore comes from different pattern lengths; polyrhythm comes from the existing rational per-channel rates. Pattern changes must remain anchored to the shared musical timeline. Exact CONTINUE/RESTART transition semantics still require a final contract before implementation.

## OPEN / not implemented in r45

- active-pattern selection and Engine integration;
- eight-step OLED viewport and pattern operations;
- sparse per-step Probability/Gate/Tie/Ratchet records;
- Song Mode topology and arrangement persistence;
- USB full-device backup/restore transport;
- target STM32F401 memory proof for the 12-KiB logical image.

## Persistence qualification boundary

The 12-KiB logical-image source configuration consumes 4 KiB more static staging RAM than released 1.1.0. Host migration and power-loss tests do not prove the target RAM budget. A real STM32F401 ELF must pass the repository memory gate before this layout is release-approved.
