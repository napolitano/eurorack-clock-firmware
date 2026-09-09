/**
 * @file external_input_capture.cpp
 * @brief Interrupt-driven capture boundary for conditioned SYNC and RST comparator signals.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "hal/external_input_capture.h"

#include <Arduino.h>

#include "hal/system_clock.h"
#include "pin_map.h"

namespace clockfw::hal {

ExternalInputCapture* ExternalInputCapture::activeInstance_ = nullptr;

void ExternalInputCapture::begin() {
    activeInstance_ = this;
    if (pinmap::kExternalSyncSignalPin != pinmap::kUnassignedDigitalPin) {
        pinMode(pinmap::kExternalSyncSignalPin, INPUT_PULLUP);
        attachInterrupt(
            digitalPinToInterrupt(pinmap::kExternalSyncSignalPin),
            syncInterruptThunk,
            CHANGE);
    }
    if (pinmap::kExternalResetSignalPin != pinmap::kUnassignedDigitalPin) {
        pinMode(pinmap::kExternalResetSignalPin, INPUT_PULLUP);
        resetLevelHigh_ = digitalRead(pinmap::kExternalResetSignalPin) == HIGH;
        attachInterrupt(
            digitalPinToInterrupt(pinmap::kExternalResetSignalPin),
            resetInterruptThunk,
            CHANGE);
    }
}

bool ExternalInputCapture::popSyncEdge(ExternalInputEdge& edge) {
    return pop(syncQueue_, edge);
}

bool ExternalInputCapture::popResetEdge(ExternalInputEdge& edge) {
    return pop(resetQueue_, edge);
}

bool ExternalInputCapture::resetLevelHigh() const {
    return resetLevelHigh_;
}

std::uint32_t ExternalInputCapture::droppedSyncEdges() const {
    return syncQueue_.overflowCount;
}

std::uint32_t ExternalInputCapture::collapsedResetEdges() const {
    return resetQueue_.overflowCount;
}

void ExternalInputCapture::syncInterruptThunk() {
    if (activeInstance_ != nullptr) {
        activeInstance_->captureSyncFromIsr();
    }
}

void ExternalInputCapture::resetInterruptThunk() {
    if (activeInstance_ != nullptr) {
        activeInstance_->captureResetFromIsr();
    }
}

void ExternalInputCapture::captureSyncFromIsr() {
    pushFromIsr(syncQueue_, {
        SystemClock::microseconds(),
        digitalRead(pinmap::kExternalSyncSignalPin) == HIGH});
}

void ExternalInputCapture::captureResetFromIsr() {
    const bool high = digitalRead(pinmap::kExternalResetSignalPin) == HIGH;
    resetLevelHigh_ = high;
    pushFromIsr(resetQueue_, {SystemClock::microseconds(), high});
}

void ExternalInputCapture::pushFromIsr(EdgeQueue& queue, const ExternalInputEdge edge) {
    const std::uint8_t next = static_cast<std::uint8_t>((queue.head + 1U) % kQueueCapacity);
    if (next == queue.tail) {
        queue.overflowPending = true;
        queue.overflowTimestampUs = edge.timestampUs;
        queue.overflowLevelHigh = edge.high;
        ++queue.overflowCount;
        return;
    }
    queue.edges[queue.head] = edge;
    queue.head = next;
}

bool ExternalInputCapture::pop(EdgeQueue& queue, ExternalInputEdge& edge) {
    if (queue.tail != queue.head) {
        edge = queue.edges[queue.tail];
        queue.tail = static_cast<std::uint8_t>((queue.tail + 1U) % kQueueCapacity);
        return true;
    }
    if (!queue.overflowPending) {
        return false;
    }
    edge.timestampUs = queue.overflowTimestampUs;
    edge.high = queue.overflowLevelHigh;
    edge.continuityLost = true;
    queue.overflowPending = false;
    return true;
}

#ifdef CLOCK_HOST_TEST
void ExternalInputCapture::injectSyncEdgeForTest(
    const std::uint32_t timestampUs,
    const bool high) {
    pushFromIsr(syncQueue_, {timestampUs, high});
}

void ExternalInputCapture::injectResetEdgeForTest(
    const std::uint32_t timestampUs,
    const bool high) {
    resetLevelHigh_ = high;
    pushFromIsr(resetQueue_, {timestampUs, high});
}
#endif

}  // namespace clockfw::hal
