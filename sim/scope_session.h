/**
 * @file scope_session.h
 * @brief Transport-referenced oscilloscope session state for the native simulator.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

#include "simulator_runtime.h"

namespace clockfw::sim::scope {

/** @brief Snapshot consumed by the renderer for one transport-referenced scope frame. */
struct SessionView {
    std::uint64_t windowUs = 4000000ULL;
    std::uint64_t referenceUs = 0ULL;
    std::uint64_t epochSimulatorUs = 0ULL;
    bool started = false;
    bool freezeOnStop = true;
};

/** @brief Maintains the developer scope's transport-relative timebase and freeze policy. */
class Session final {
public:
    /** @brief Updates scope epoch/reference state from the real firmware transport state. */
    void update(SimulatorRuntime& runtime);

    /** @brief Changes the selected fixed visible time span. */
    void setWindowUs(std::uint64_t windowUs);

    /** @brief Enables or disables freezing the scope when the STOP transport state is entered. */
    void setFreezeOnStop(bool enabled);

    /** @brief Toggles STOP freezing. */
    void toggleFreezeOnStop();

    /** @brief Returns true when STOP freezes the developer scope. */
    bool freezeOnStop() const;

    /** @brief Returns current immutable renderer view. */
    SessionView view() const;

private:
    std::uint64_t windowUs_ = 4000000ULL;
    std::uint64_t referenceUs_ = 0ULL;
    std::uint64_t epochSimulatorUs_ = 0ULL;
    std::uint32_t observedStartSequence_ = 0U;
    bool started_ = false;
    bool freezeOnStop_ = true;
};

}  // namespace clockfw::sim::scope
