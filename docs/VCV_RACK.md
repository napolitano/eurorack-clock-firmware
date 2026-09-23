<!-- Author: Axel Napolitano | License: PolyForm-Noncommercial-1.0.0 -->

# CLOCK VCV Rack Trial-First Port

Status: **EXPERIMENTAL / post-1.1.0**.

The VCV Rack track exists so prospective builders can try CLOCK before committing to the hardware build. Its product requirement is therefore unusually strict: it should behave like CLOCK, not merely reproduce a similar feature list.

The initial implementation lives in [`../vcv/`](../vcv/). It builds the production CLOCK sources and uses the same host runtime boundary already exercised by the native simulator. The Rack-specific code is responsible for Rack sample-time adaptation, Rack ports/controls, the functional Rack panel, framebuffer presentation and Rack patch serialization. Musical timing, Groove/Euclid/Sequencer behavior, transport, settings and rendering stay in the production code.

## Current verified boundary

The Rack-independent adapter is built and exercised by normal CMake/CTest. It has been verified locally against the production source for:

- normal CLOCK boot path;
- PLAY-driven engine start;
- eight 0/+5-V logical gate outputs;
- the real CLOCK OLED framebuffer;
- raw CLOCK persistence-image export/import;
- direct conditioned external-input levels driven on Rack sample time.

This does **not** yet constitute a real Rack SDK build or in-Rack runtime test. The repository CI workflow `.github/workflows/vcv.yml` performs the first real SDK build using the pinned Rack SDK.

## Deliberate first-alpha limitation

The current host HAL and application callback thunks are process-global. The first VCV alpha therefore supports exactly one active CLOCK instance per Rack process. Additional instances are disabled rather than allowed to corrupt each other's timer/GPIO/input state.

The correct multi-instance solution is an instance-context host HAL shared by `sim/` and `vcv/`, followed by removal of the static single-active-application assumptions. That refactor is a separate architecture step.

## Deployment path

Development path:

```text
production CLOCK code
        -> Rack-independent VCV adapter tests
        -> Rack SDK plugin build
        -> .vcvplugin CI artifact
        -> manual Rack smoke test
        -> Windows/macOS/Linux package matrix
        -> tagged CLOCK release artifact under dist/vcv/
```

See [`../vcv/README.md`](../vcv/README.md) for build/install commands and the current smoke-test script.

<h6 align="center">From Munich with &#9829;</h6>
