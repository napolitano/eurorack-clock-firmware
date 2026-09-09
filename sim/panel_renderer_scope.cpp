/**
 * @file panel_renderer_scope.cpp
 * @brief SDL developer oscilloscope rendering for the native clock simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "panel_renderer.h"

#include <algorithm>
#include <cstdio>

#include "scope_timeline.h"

namespace clockfw::sim {
namespace {

void setColor(
    SDL_Renderer* const renderer,
    const Uint8 red,
    const Uint8 green,
    const Uint8 blue,
    const Uint8 alpha = 255U) {
    (void)SDL_SetRenderDrawColor(renderer, red, green, blue, alpha);
}

void fillRect(SDL_Renderer* const renderer, const layout::Rect& rect) {
    const SDL_FRect sdlRect{rect.x, rect.y, rect.width, rect.height};
    (void)SDL_RenderFillRect(renderer, &sdlRect);
}

void strokeRect(SDL_Renderer* const renderer, const layout::Rect& rect) {
    const SDL_FRect sdlRect{rect.x, rect.y, rect.width, rect.height};
    (void)SDL_RenderRect(renderer, &sdlRect);
}

const char* transportName(const TransportState state) {
    switch (state) {
        case TransportState::Stopped: return "STOP";
        case TransportState::Paused: return "PAUSE";
        case TransportState::Playing: return "PLAY";
    }
    return "?";
}

const char* operatingModeName(const OperatingMode mode) {
    switch (mode) {
        case OperatingMode::Independent: return "INDEPENDENT";
        case OperatingMode::UnifiedClock: return "ONE CLOCK";
        case OperatingMode::DividerBank: return "DIVIDER";
    }
    return "?";
}

}  // namespace

void PanelRenderer::renderDeveloperPanel(
    SDL_Renderer* const renderer,
    const SimulatorRuntime& runtime,
    const double speedMultiplier,
    const scope::SessionView& scopeView) const {
    setColor(renderer, 29U, 30U, 33U);
    fillRect(renderer, panelLayout_.developerPanel);
    setColor(renderer, 82U, 84U, 87U);
    strokeRect(renderer, panelLayout_.developerPanel);

    const ClockState& state = runtime.state();
    setColor(renderer, 225U, 227U, 230U);
    drawText(renderer, 430.0F, 48.0F, "DEVELOPER TIMING VIEW");

    char status[128]{};
    std::snprintf(
        status,
        sizeof(status),
        "BPM %u  %s  %s  SPEED %.0fx  POWER %s  BUFFER %s",
        static_cast<unsigned>(state.bpm),
        operatingModeName(state.operatingMode),
        transportName(state.transport),
        speedMultiplier,
        runtime.poweredOn() ? "ON" : "OFF",
        runtime.outputStageEnabled() ? "ON" : "HIGH-Z");
    drawText(renderer, 430.0F, 72.0F, status);
    const SyncInputTelemetry sync = runtime.syncInputTelemetry();
    char syncStatus[160]{};
    std::snprintf(
        syncStatus,
        sizeof(syncStatus),
        "SYNC %s/%s %s %s  %.3f BPM  G%u/F%u -> %.3f  %s  pulses=%llu",
        sync.cableConnected ? "CABLE" : "OPEN",
        sync.generatorRunning ? "RUN" : "HOLD",
        signalWaveformName(sync.waveform),
        sync.signalHigh ? "HI" : "LO",
        static_cast<double>(sync.bpmMilli) / 1000.0,
        static_cast<unsigned>(sync.ppqn),
        static_cast<unsigned>(sync.firmwarePpqn),
        static_cast<double>(sync.engineBpmMilli) / 1000.0,
        sync.locked ? "LOCK" : "ACQ",
        static_cast<unsigned long long>(sync.pulseCount));
    drawText(renderer, 430.0F, 96.0F, syncStatus);

    const ResetInputTelemetry reset = runtime.resetInputTelemetry();
    char resetStatus[128]{};
    std::snprintf(
        resetStatus,
        sizeof(resetStatus),
        "RST  %s/%s %s %s  period=%ums  resets=%llu",
        reset.cableConnected ? "CABLE" : "OPEN",
        reset.generatorRunning ? "RUN" : "HOLD",
        signalWaveformName(reset.waveform),
        reset.signalHigh ? "HI" : "LO",
        static_cast<unsigned>(reset.periodMs),
        static_cast<unsigned long long>(reset.resetCount));
    drawText(renderer, 430.0F, 116.0F, resetStatus);
    drawText(renderer, 430.0F, 136.0F, "MOUSE: L jack=cable  R jack=run  M jack=waveform  wheel=tempo/period");
    drawText(renderer, 430.0F, 156.0F, "KEYS: A/D ENC  E push  P/SPACE play  T tap  S stop  R single reset");
    drawText(renderer, 430.0F, 176.0F, "SYNC C/G [/] PgUp/PgDn Q PPQN | W waveform | F2 POWER | F1 dev | F12 shot");

    const layout::Rect powerBox{
        panelLayout_.developerPanel.x + panelLayout_.developerPanel.width - 118.0F,
        panelLayout_.developerPanel.y + 26.0F, 92.0F, 20.0F};
    setColor(renderer, runtime.poweredOn() ? 92U : 58U, runtime.poweredOn() ? 132U : 60U, runtime.poweredOn() ? 96U : 62U);
    fillRect(renderer, powerBox);
    setColor(renderer, 195U, 198U, 202U);
    strokeRect(renderer, powerBox);
    drawText(renderer, powerBox.x + 10.0F, powerBox.y + 6.0F, runtime.poweredOn() ? "POWER ON" : "POWER OFF");

    const layout::Rect freezeBox{430.0F, 196.0F, 14.0F, 14.0F};
    setColor(renderer, 118U, 121U, 126U);
    strokeRect(renderer, freezeBox);
    if (scopeView.freezeOnStop) {
        setColor(renderer, 175U, 220U, 185U);
        (void)SDL_RenderLine(renderer, 433.0F, 203.0F, 436.0F, 207.0F);
        (void)SDL_RenderLine(renderer, 436.0F, 207.0F, 441.0F, 199.0F);
    }
    setColor(renderer, 185U, 188U, 193U);
    drawText(renderer, 452.0F, 198.0F, "FREEZE ON STOP");

    constexpr float graphLeft = 480.0F;
    constexpr float graphRight = 1138.0F;
    constexpr float graphWidth = graphRight - graphLeft;
    constexpr float firstRowY = 248.0F;
    constexpr float rowHeight = 73.0F;

    const std::uint64_t safeWindowUs = std::clamp<std::uint64_t>(
        scopeView.windowUs,
        scope::kWindowOptionsUs.front(),
        scope::kWindowOptionsUs.back());
    const SyncInputTelemetry syncGrid = runtime.syncInputTelemetry();
    const std::uint32_t referenceBpmMilli = scope::effectiveReferenceBpmMilli(
        state, syncGrid.locked, syncGrid.engineBpmMilli);
    const scope::MusicalGridSpec grid = scope::musicalGridSpec(
        state, referenceBpmMilli, safeWindowUs);
    const double referenceUs = static_cast<double>(scopeView.referenceUs);
    const double startUs = referenceUs - static_cast<double>(safeWindowUs);
    // Gate telemetry is timestamped in integer microseconds. Keep its clipping
    // path integer-only so the strict simulator build never relies on implicit
    // int64_t -> double conversions. The musical grid remains double-based
    // because rational BPM/rate intervals can land between integer microseconds.
    const std::int64_t gateReferenceUs = static_cast<std::int64_t>(scopeView.referenceUs);
    const std::int64_t gateStartUs = gateReferenceUs - static_cast<std::int64_t>(safeWindowUs);
    const float graphTop = firstRowY + 2.0F;
    const float graphBottom = firstRowY +
        static_cast<float>(kChannelCount - 1U) * rowHeight + 32.0F;

    if (scopeView.started) {
        // Musical references are generated from the unswung/unhumanized clock
        // lattice. PLAY from STOP defines serial 0 at t=0. Swing, phase and
        // Humanize therefore remain visible as displacement from the reference.
        const std::int64_t signedStartUs = static_cast<std::int64_t>(startUs);
        const std::uint64_t firstSerial = scope::firstMinorReferenceSerialAtOrAfter(
            signedStartUs, grid);
        const std::uint64_t serialStep = std::max<std::uint32_t>(grid.minorEvery, 1U);
        for (std::uint64_t serial = firstSerial;; serial += serialStep) {
            const double gridTimeUs = scope::referenceTimeUs(serial, grid);
            if (gridTimeUs > referenceUs) {
                break;
            }
            const float x = graphLeft + static_cast<float>(scope::normalizedPosition(
                gridTimeUs, startUs, safeWindowUs)) * graphWidth;
            const bool major = scope::isMajorReference(serial, grid);
            setColor(renderer, major ? 72U : 49U, major ? 74U : 51U, major ? 79U : 55U);
            (void)SDL_RenderLine(renderer, x, graphTop, x, graphBottom);
            if (major) {
                char referenceLabel[24]{};
                std::snprintf(
                    referenceLabel,
                    sizeof(referenceLabel),
                    "%.3fs",
                    gridTimeUs / 1000000.0);
                setColor(renderer, 122U, 124U, 129U);
                drawText(renderer, std::min(x + 3.0F, graphRight - 64.0F), 214.0F, referenceLabel);
            }
            if (serial > UINT64_MAX - serialStep) {
                break;
            }
        }

        setColor(renderer, 96U, 99U, 105U);
        (void)SDL_RenderLine(renderer, graphRight, graphTop, graphRight, graphBottom);
        drawText(renderer, graphRight - 24.0F, 214.0F, "NOW");
    } else {
        setColor(renderer, 122U, 124U, 129U);
        drawText(renderer, graphRight - 48.0F, 214.0F, "ARMED");
    }

    const char* const scopeState = !scopeView.started
        ? "ARMED"
        : (scopeView.freezeOnStop && state.transport == TransportState::Stopped
            ? "FROZEN"
            : "RUN");
    char scopeStatus[80]{};
    std::snprintf(
        scopeStatus,
        sizeof(scopeStatus),
        "SCOPE %.1fs  GRID %s %.2fms x%u  %s",
        static_cast<double>(safeWindowUs) / 1000000.0,
        grid.basis,
        grid.intervalUs / 1000.0,
        static_cast<unsigned>(grid.minorEvery),
        scopeState);
    setColor(renderer, 150U, 152U, 156U);
    drawText(renderer, 870.0F, 196.0F, scopeStatus);

    const auto& channels = runtime.telemetry();
    for (std::size_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        const float rowY = firstRowY + static_cast<float>(channelIndex) * rowHeight;
        char label[96]{};
        std::snprintf(
            label,
            sizeof(label),
            "OUT%u  edges=%llu  last=%.2fms",
            static_cast<unsigned>(channelIndex + 1U),
            static_cast<unsigned long long>(channels[channelIndex].risingEdges),
            static_cast<double>(channels[channelIndex].lastPulseWidthUs) / 1000.0);
        setColor(renderer, 214U, 216U, 219U);
        drawText(renderer, 430.0F, rowY - 8.0F, label);

        const float lowY = rowY + 28.0F;
        const float highY = rowY + 6.0F;
        setColor(renderer, 70U, 72U, 76U);
        (void)SDL_RenderLine(renderer, graphLeft, lowY, graphRight, lowY);

        bool high = false;
        float previousX = graphLeft;
        if (scopeView.started) {
            for (const GateTransition& transition : channels[channelIndex].transitions) {
                if (transition.timestampUs < scopeView.epochSimulatorUs) {
                    continue;
                }
                const std::int64_t transitionUs = static_cast<std::int64_t>(
                    transition.timestampUs - scopeView.epochSimulatorUs);
                if (transitionUs < gateStartUs) {
                    high = transition.high;
                    continue;
                }
                if (transitionUs > gateReferenceUs) {
                    break;
                }
                const float x = graphLeft + static_cast<float>(scope::normalizedPosition(
                    static_cast<double>(transitionUs),
                    static_cast<double>(gateStartUs),
                    safeWindowUs)) * graphWidth;
                setColor(renderer, 160U, 220U, 170U);
                (void)SDL_RenderLine(renderer, previousX, high ? highY : lowY, x, high ? highY : lowY);
                (void)SDL_RenderLine(renderer, x, high ? highY : lowY, x, transition.high ? highY : lowY);
                previousX = x;
                high = transition.high;
            }
            setColor(renderer, 160U, 220U, 170U);
            (void)SDL_RenderLine(renderer, previousX, high ? highY : lowY, graphRight, high ? highY : lowY);
        }
    }
}


}  // namespace clockfw::sim
