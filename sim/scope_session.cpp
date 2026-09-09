/**
 * @file scope_session.cpp
 * @brief Transport-referenced oscilloscope session state for the native simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "scope_session.h"

#include <algorithm>

#include "scope_timeline.h"

namespace clockfw::sim::scope {

void Session::update(SimulatorRuntime& runtime) {
    const std::uint64_t nowUs = runtime.nowMicroseconds();
    if (!runtime.poweredOn() || (started_ && nowUs < epochSimulatorUs_)) {
        observedStartSequence_ = 0U;
        epochSimulatorUs_ = 0ULL;
        referenceUs_ = 0ULL;
        started_ = false;
        runtime.setTelemetryHistoryFrozen(false);
        if (!runtime.poweredOn()) {
            return;
        }
    }

    const TransportTelemetry transport = runtime.transportTelemetry();
    if (transport.startSequence != observedStartSequence_) {
        observedStartSequence_ = transport.startSequence;
        epochSimulatorUs_ = transport.lastStartUs;
        referenceUs_ = 0ULL;
        started_ = true;
    }

    if (!started_) {
        runtime.setTelemetryHistoryFrozen(false);
        return;
    }

    const bool freezeNow = freezeOnStop_ && transport.state == TransportState::Stopped;
    runtime.setTelemetryHistoryFrozen(freezeNow);

    const std::uint64_t referenceSimulatorUs = freezeNow && transport.lastStopUs >= epochSimulatorUs_
        ? transport.lastStopUs
        : runtime.nowMicroseconds();
    referenceUs_ = referenceSimulatorUs >= epochSimulatorUs_
        ? referenceSimulatorUs - epochSimulatorUs_
        : 0ULL;
}

void Session::setWindowUs(const std::uint64_t windowUs) {
    windowUs_ = std::clamp<std::uint64_t>(
        windowUs,
        kWindowOptionsUs.front(),
        kWindowOptionsUs.back());
}

void Session::setFreezeOnStop(const bool enabled) {
    freezeOnStop_ = enabled;
}

void Session::toggleFreezeOnStop() {
    freezeOnStop_ = !freezeOnStop_;
}

bool Session::freezeOnStop() const {
    return freezeOnStop_;
}

SessionView Session::view() const {
    return {windowUs_, referenceUs_, epochSimulatorUs_, started_, freezeOnStop_};
}

}  // namespace clockfw::sim::scope
