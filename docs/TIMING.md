<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# South Signal Lab CLOCK Timing Contract

This document defines the firmware timing boundary. It distinguishes hard real-time musical behavior from foreground work such as display rendering, persistence, and menu handling.

## Hard real-time layer

CLOCK currently uses TIM3 as a **20 kHz scheduler interrupt**. One scheduler quantum is therefore **50 µs**. Gate scheduling, pulse termination, phase progression, external-sync consumption, and external-reset semantics execute from this scheduler path rather than from the Arduino foreground loop.

The target interrupt hierarchy is deliberately asymmetric:

| Layer | Target priority | Responsibility |
| --- | ---: | --- |
| TIM3 scheduler | 0 | gate edges, pulse termination, phase, SYNC/RST consumption |
| GPIO EXTI | 4 | encoder A/B, conditioned SYNC/RST edge capture |
| I2C peripheral | 8 | OLED transport only |
| Foreground | n/a | UI, rendering, display service, persistence scheduling |

STM32 NVIC priorities use lower numbers for higher preemption priority. The scheduler therefore remains able to preempt display and input-service activity.

The current scheduler is service-tick based rather than final output-compare scheduling. Consequently the software timing quantization is 50 µs. A future compare-event scheduler may tighten that bound without changing the application timing contract.

## Encoder and external inputs

Encoder phase A/B and conditioned SYNC/RST inputs are captured by GPIO interrupts. They are not polled from the foreground loop.

Encoder ISR work is intentionally limited to Gray-code decoding and accumulation of complete detents. The foreground consumes accumulated detents later, so OLED traffic cannot make a valid quadrature transition disappear merely because `loop()` is busy.

SYNC/RST ISR work is limited to timestamping/queuing conditioned levels. The 20 kHz scheduler consumes those queues. RST has precedence over SYNC when both are pending at the same scheduler service boundary.

The V1 baseline timestamps conditioned SYNC edges through GPIO EXTI. Final hardware qualification must measure the resulting capture and output jitter under representative worst-case load. Routing to a timer Input Capture capable pin remains a useful design option because hardware capture removes software IRQ-entry latency from the period measurement, but adopting it is required only if the measured EXTI path fails the V1 timing target.

## Display transports

SPI and I2C remain fully supported build-time options. They intentionally have different refresh strategies.

### SPI — reference transport

SPI is the default PlatformIO/reference profile. `present()` transfers dirty pages immediately. The dedicated bus is fast enough that this remains the simplest path and it does not inherit the additional scheduling machinery required by I2C.

### I2C — procurement-compatible transport

I2C is treated as a supported production option, not a legacy test path. STM32duino master transmissions can keep the foreground waiting for transfer completion, so CLOCK never sends a complete 1 KiB framebuffer from one runtime `present()` call.

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
- encoder detents must accumulate while foreground display work is active;
- conditioned SYNC/RST edges must remain capturable while OLED traffic is active;
- I2C is allowed to lower visual frame rate or skip stale frames under load;
- persistence writes remain prohibited while PLAYING because STM32F4 Flash programming can stall instruction/data fetches.

## Verification layers

Host tests prove the state-machine and scheduling contract: fixed-point timing, gate lengths, SYNC/RST semantics, encoder accumulation, I2C transaction bounds, latest-frame-wins behavior, and SPI/I2C build equivalence.

Only HIL can establish actual edge jitter, IRQ latency, I2C/SPI electrical behavior, comparator thresholds, and the final timing distribution at the jacks. The required bench matrix is defined in [`HIL_TEST_PLAN.md`](HIL_TEST_PLAN.md).

<h6 align="center">From Munich with &#9829;</h6>
