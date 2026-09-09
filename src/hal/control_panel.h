/**
 * @file control_panel.h
 * @brief HAL driver for the rotary encoder and three front-panel buttons.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

namespace clockfw::hal {

/** @brief Edge event emitted by one debounced push button. */
enum class ButtonEdge : std::uint8_t { None, Pressed, Released };

/** @brief Debounced state and edge event for one physical button. */
struct ButtonSample {
    ButtonEdge edge = ButtonEdge::None;
    bool pressed = false;
};

/** @brief Complete control-panel sample produced once per application loop. */
struct ControlSample {
    std::int8_t encoderDelta = 0;
    ButtonSample encoderButton{};
    ButtonSample transportButton{};
    ButtonSample tapButton{};
    ButtonSample resetButton{};
};

/** @brief Reads, debounces, and decodes all physical front-panel controls. */
class ControlPanel final {
public:
    /** @brief Constructs the control panel with the board-specific GPIO assignment. */
    ControlPanel();

    /** @brief Configures GPIO inputs and captures the initial encoder/button state. */
    void begin();

    /**
     * @brief Samples all controls and emits debounced edge events.
     * @param nowMs Current monotonic time in milliseconds.
     * @return Current encoder delta and button states.
     */
    ControlSample sample(std::uint32_t nowMs);

private:
    /** @brief Small state machine used for one active-low button input. */
    class DebouncedButton final {
    public:
        /** @brief Associates the helper with a physical GPIO pin. */
        explicit DebouncedButton(std::uint32_t pin);

        /** @brief Initializes the active-low input with the MCU pull-up enabled. */
        void begin(std::uint32_t nowMs);

        /**
         * @brief Samples and debounces the physical button.
         * @param nowMs Current monotonic time in milliseconds.
         * @return Debounced edge and stable pressed state.
         */
        ButtonSample sample(std::uint32_t nowMs);

    private:
        std::uint32_t pin_;
        bool rawPressed_ = false;
        bool stablePressed_ = false;
        std::uint32_t rawChangedAtMs_ = 0U;
    };

    /** @brief Atomically drains complete detents accumulated by GPIO edge interrupts. */
    std::int8_t sampleEncoder();

    /** @brief Shared GPIO interrupt thunk for both encoder quadrature phases. */
    static void encoderInterruptThunk();

    /** @brief Decodes one A/B edge and accumulates completed detents. */
    void handleEncoderEdgeFromIsr();

    DebouncedButton encoderButton_;
    DebouncedButton transportButton_;
    DebouncedButton tapButton_;
    DebouncedButton resetButton_;
    static ControlPanel* activeInstance_;
    volatile std::uint8_t previousEncoderState_ = 0U;
    volatile std::int8_t encoderAccumulator_ = 0;
    volatile std::int16_t pendingEncoderDetents_ = 0;
};

}  // namespace clockfw::hal
