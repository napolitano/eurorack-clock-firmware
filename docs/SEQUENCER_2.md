<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# Sequencer 2.0 — development contract

This document separates the implemented post-1.1 development state from later Sequencer 2.0 work. The 1.1 user guide remains authoritative for released 1.1.0 behavior.

## CURRENT development state (r46)

- Maximum pattern length: **128 steps**. The OLED editor is an eight-step viewport, not a length limit.
- **8 pattern slots per channel × 8 physical channels = 64 pattern slots**. The active slot is persisted per channel.
- Binary gates use two explicit 64-bit words; no heap allocation and no compiler bitfields.
- Pattern-local traversal: `FORWARD`, `REVERSE`, `PINGPONG`, `RANDOM`. PINGPONG does not repeat the end points. RANDOM uses a deterministic, seedable, history-free mapping from event serial to step so rescheduling is reproducible.
- Pattern-local end behavior: `LOOP` or `ONCE`. RANDOM+ONCE emits exactly LENGTH traversal events; PINGPONG+ONCE emits one complete no-duplicate-endpoint traversal cycle.
- The production ClockEngine consumes the active SequencerPatternV2 snapshot for each channel. Legacy 1.1 Sequencer state remains only as the migration/fallback source when no V2 pattern has been activated.
- The Sequencer settings page exposes `PATTERN`, `LENGTH`, `ROTATE`, `MODE` (play direction), `LOOP`, and `EDITOR`.
- `EDITOR` opens the eight-step viewport directly. BACK returns to Sequencer parameters. Encoder long-press opens pattern operations; BACK from pattern operations returns to the editor.
- The editor scrolls across 16 eight-step pages for a 128-step pattern and renders the actual active P1–P8 pattern, including steps above the old 64-step boundary.
- Persistent pattern bank: fixed 2-KiB extension at logical offset 8192, explicit records + CRC, global rather than duplicated in CURRENT/eight presets.
- Legacy migration: each channel's released 1.1 64-bit Sequencer state becomes P1; P2–P8 keep clean defaults.
- Flash writes are coalesced and are not committed while transport is PLAYING.

## PLANNED 1.2 step expression

Each step may later override Probability, Gate Length / relative Duty, short Trigger length, Tie and bounded Ratchet behavior. `DEFAULT` means inheritance from the channel/pattern defaults rather than duplicated metadata. These overrides require a sparse extension format; they must not be copied into every repeated `ClockState`.

## PROPOSED Song Mode

Song Mode is a global topology peer to Independent, One Clock and Divider Bank, not a submenu of one channel. The arrangement grid has eight channel columns. Each row cell chooses `-` or P1–P8 for that channel. Row duration is defined on the master timeline rather than by waiting for all channel patterns to finish.

Polymeter therefore comes from different pattern lengths; polyrhythm comes from the existing rational per-channel rates. Pattern changes must remain anchored to the shared musical timeline. Exact CONTINUE/RESTART transition semantics still require a final contract before implementation.

## OPEN after r46

- sparse per-step Probability/Gate/Tie/Ratchet records and UI;
- Song Mode topology and arrangement persistence;
- USB full-device backup/restore transport and guarded device-side USB DATA mode;
- target STM32F401 ELF/RAM proof for the 12-KiB logical image.

## Persistence qualification boundary

The 12-KiB logical-image source configuration consumes 4 KiB more static staging RAM than released 1.1.0. Host migration and power-loss tests do not prove the target RAM budget. A real STM32F401 ELF must pass the repository memory gate before this layout is release-approved.

<h6 align="center">From Munich with &#9829;</h6>
