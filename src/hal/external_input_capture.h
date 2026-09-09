/**
 * @file external_input_capture.h
 * @brief Interrupt-driven capture boundary for conditioned SYNC and RST comparator signals.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

namespace clockfw::hal {

/** @brief One comparator transition with a wrapping microsecond timestamp. */
struct ExternalInputEdge {
    std::uint32_t timestampUs = 0U;
    bool high = false;
    bool continuityLost = false;
};

/**
 * @brief Captures external comparator transitions without involving the foreground loop.
 *
 * The current prototype backend uses GPIO CHANGE interrupts. The public edge queue is
 * deliberately timer-agnostic so the final PCB can replace SYNC timestamp production
 * with STM32 timer input capture without changing the real-time synchronization service.
 */
class ExternalInputCapture final {
public:
    /** @brief Configures assigned comparator GPIOs and attaches edge interrupts. */
    void begin();

    /** @brief Pops the oldest captured SYNC edge; false means the queue is empty. */
    bool popSyncEdge(ExternalInputEdge& edge);

    /** @brief Pops the oldest captured RST edge; false means the queue is empty. */
    bool popResetEdge(ExternalInputEdge& edge);

    /** @brief Returns the most recently observed RST comparator level. */
    bool resetLevelHigh() const;

    /** @brief Returns the number of SYNC edges dropped because the queue was saturated. */
    std::uint32_t droppedSyncEdges() const;

    /** @brief Returns the number of RST edges collapsed because the queue was saturated. */
    std::uint32_t collapsedResetEdges() const;

#ifdef CLOCK_HOST_TEST
    /** @brief Injects a captured SYNC transition for deterministic host tests. */
    void injectSyncEdgeForTest(std::uint32_t timestampUs, bool high);

    /** @brief Injects a captured RST transition for deterministic host tests. */
    void injectResetEdgeForTest(std::uint32_t timestampUs, bool high);
#endif

private:
    static constexpr std::uint8_t kQueueCapacity = 16U;

    struct EdgeQueue {
        std::array<ExternalInputEdge, kQueueCapacity> edges{};
        volatile std::uint8_t head = 0U;
        volatile std::uint8_t tail = 0U;
        volatile bool overflowPending = false;
        volatile std::uint32_t overflowTimestampUs = 0U;
        volatile bool overflowLevelHigh = false;
        volatile std::uint32_t overflowCount = 0U;
    };

    /** @brief Shared EXTI thunk for the SYNC comparator GPIO. */
    static void syncInterruptThunk();

    /** @brief Shared EXTI thunk for the RST comparator GPIO. */
    static void resetInterruptThunk();

    /** @brief Captures the current SYNC GPIO level and timestamp from interrupt context. */
    void captureSyncFromIsr();

    /** @brief Captures the current RST GPIO level and timestamp from interrupt context. */
    void captureResetFromIsr();

    /** @brief Pushes one edge into a single-producer/single-consumer ISR queue. */
    static void pushFromIsr(EdgeQueue& queue, ExternalInputEdge edge);

    /** @brief Pops one edge, including the latest collapsed overflow state when necessary. */
    static bool pop(EdgeQueue& queue, ExternalInputEdge& edge);

    static ExternalInputCapture* activeInstance_;
    EdgeQueue syncQueue_{};
    EdgeQueue resetQueue_{};
    volatile bool resetLevelHigh_ = false;
};

}  // namespace clockfw::hal
