<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Timing Contract

This document defines the firmware timing boundary. It distinguishes hard real-time musical behavior from foreground work such as display rendering, persistence, and menu handling.

## Hard real-time layer

CLOCK currently uses TIM3 as a **20 kHz scheduler interrupt**. One scheduler quantum is therefore **50 µs**. Gate scheduling, pulse termination, phase progression, external-sync consumption, and external-reset semantics execute from this scheduler path rather than from the foreground application loop.

The target interrupt hierarchy is deliberately asymmetric:

| Layer | Target priority | Responsibility |
| --- | ---: | --- |
| TIM3 scheduler | 0 | gate edges, pulse termination, phase, SYNC/RST consumption |
| GPIO EXTI | 4 | conditioned SYNC/RST edge capture |
| Foreground | n/a | UI, rendering, display service, persistence scheduling |

STM32 NVIC priorities use lower numbers for higher preemption priority. The scheduler therefore remains able to preempt display and input-service activity.

The current scheduler is service-tick based rather than final output-compare scheduling. Consequently the software timing quantization is 50 µs. A future compare-event scheduler may tighten that bound without changing the application timing contract.

## Encoder and external inputs

The physical encoder and the external timing inputs deliberately use different capture paths.

The PEC11L A/B contacts are wired to PB6/PB7 and decoded by **TIM4 encoder mode**. The timer counts quadrature transitions in hardware; foreground control processing converts the wrapping transition count into mechanical detents and applies detent-phase resynchronization plus the user `NORMAL / REVERSED` semantic direction. The encoder therefore does not depend on GPIO EXTI delivery or display-loop latency.

Both conditioned comparator paths remain GPIO-interrupt driven. Their ISR work is limited to timestamping/queuing physical INPUT 1 / INPUT 2 levels; musical role interpretation happens in the 20 kHz scheduler. Factory roles remain SYNC/RESET, but either physical path may own a supported role. RESET processing retains precedence over SYNC when both assigned roles are pending at the same scheduler boundary; edge-controlled transport commands follow, with STOP winning simultaneous START/RESTART, and an assigned RUN level applied last as the authoritative transport state.

The current baseline timestamps conditioned comparator edges through GPIO EXTI; when one path owns the SYNC role, those timestamps feed the external-period estimator. Final hardware qualification must measure capture/output jitter under representative worst-case load. PA8 remains timer Input Capture capable, so hardware capture remains an escalation option if measured EXTI timing misses the requirement.

## Groove and TAP-record timing

Groove is a deterministic timing layer on the existing master timeline, not a second clock. Factory Groove presets and named Custom Grooves ultimately resolve to bounded per-step offsets. `AMOUNT` scales the stored offset and `ROTATE` changes which stored step is applied to the current event. In Independent mode the owning channel supplies the Groove; One Clock uses one shared Groove; Divider Bank bypasses Groove entirely. One Clock Humanize remains a separate deterministic per-output displacement and is not folded into stored Groove data.

Custom Groove offsets are signed and represented relative to the nominal step interval. The engine clamps effective event positions so timestamps remain monotonic and a marker cannot overtake the preceding or following event. Gate-off scheduling is still bounded against the resulting effective interval, so deep Groove cannot create a rising-edge/gate-off inversion. Straight timing, classic Swing and Groove therefore share the same scheduler invariants.

`GROOVE → RECORD` captures the front-panel TAP button against the live Q32 musical position. The physical button layer preserves the original press timestamp before debounce; after the press is validated, the recorder uses that high-resolution timestamp instead of the later debounced foreground time. TAP is consumed by the recorder and is not sent to Tap Tempo while the Record screen is active.

Recorder Count-In is independent from the global transport Pre-Count. It gates **capture readiness**, not the master transport configuration. `ONE SHOT` stops at the end of the selected 1–64-step pattern; `ENDLESS` wraps and replaces only steps receiving a new TAP. PLAY from the Record screen controls recorder start/stop, while PLAY+TURN is consumed as a zoom chord and must not issue a transport command. The visible playhead is derived from the same engine phase used for capture, so UI and stored offsets share one musical reference.

Host tests verify timestamp preservation, step assignment, One-Shot/Endless behavior and monotonic scheduling. Physical switch latency, contact behavior and end-to-end TAP-to-gate jitter remain HIL measurements (`HIL-GRV-004`).

## Display transports

Final hardware uses SPI. The legacy I2C transport is retained only as a host regression path and is not a production Blackpill profile.

### SPI — reference transport

SPI is the default PlatformIO/reference profile. `present()` transfers dirty pages immediately. The dedicated bus is fast enough that this remains the simplest path and it does not inherit the additional scheduling machinery required by I2C.

### Legacy I2C transport — regression/reference only

I2C is no longer a supported production wiring option because PB6/PB7 are reserved for the encoder. The implementation remains regression-tested so older transport logic does not silently rot; blocking STM32Cube I2C master transmissions can keep the foreground waiting for transfer completion, so CLOCK never sends a complete 1 KiB framebuffer from one runtime `present()` call.

Instead:

1. `present()` publishes the newest 1 KiB framebuffer as the current target and performs **no runtime I2C transfer**.
2. `OledDisplay::service()` performs at most **one I2C transaction per foreground iteration**.
3. Display data transactions contain at most **24 framebuffer bytes** plus the SSD13xx control byte.
4. Dirty pages are transferred incrementally.
5. If a newer frame is published before the old one finishes, the newer frame becomes the target immediately. UI frames may therefore be skipped; musical events may not.

At 400 kHz, a 24-byte payload transaction occupies the bus for well below one millisecond before software overhead. This is a bounded foreground cost rather than a multi-page framebuffer stall. The exact physical timing remains a HIL measurement, not a host-test claim.

Controller initialization may still flush the initial blank framebuffer synchronously because this happens before the scheduler and gate-output stage are enabled.

Small OLED commands such as contrast/power changes remain synchronous single-command transactions. They are short and remain preemptible by the scheduler.

## Why no RTOS

The STM32F401 is single-core. An RTOS would provide scheduling abstraction, not simultaneous execution. The hard timing work already lives in hardware interrupts, while display work is explicitly bounded and discardable. Introducing an RTOS would add stack/RAM usage, priority interactions, and synchronization paths without solving a problem that requires a second execution core.

If physical HIL shows unacceptable I2C interference despite the bounded service path, the next escalation is a dedicated **HAL interrupt/DMA display transport**, not an RTOS. That change must preserve the same application-level `present()`/`service()` contract.

## Timing invariants

The following invariants are release requirements:

- display transport must not change generated clock frequency;
- no gate edge may be omitted because a display transfer is active;
- configured gate lengths remain scheduler-quantized but transport-independent;
- TIM4 encoder transitions must continue accumulating while foreground display work is active, and foreground detent conversion must not lose a complete mechanical detent;
- conditioned SYNC/RST edges must remain capturable while OLED traffic is active;
- legacy I2C regression builds may lower visual frame rate or skip stale frames under load;
- persistence writes remain prohibited while PLAYING because STM32F4 Flash programming can stall instruction/data fetches;
- Groove/Swing offsets must preserve monotonic event order and cannot move a rising event beyond its neighboring event boundary;
- Groove Record must preserve the physical TAP press timestamp through debounce and must never route a recorder TAP into Tap Tempo.

## Verification layers

Host tests prove the state-machine and scheduling contract: fixed-point timing, gate lengths, SYNC/RST semantics, TIM4-style wrapping encoder accumulation/detent recovery, I2C transaction bounds, latest-frame-wins behavior, and SPI/I2C build equivalence. Physical encoder contact behavior remains a HIL concern.

Only HIL can establish actual edge jitter, IRQ latency, SPI electrical behavior, comparator thresholds, and the final timing distribution at the jacks. The required bench matrix is defined in [`HIL_TEST_PLAN.md`](HIL_TEST_PLAN.md).

<h6 align="center">From Munich with &#9829;</h6>
