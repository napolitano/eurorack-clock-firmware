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
    /** Raw physical transition timestamp associated with this validated edge. */
    std::uint32_t edgeTimestampUs = 0U;
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

    /** @brief Reverses the semantic sign of completed encoder detents without touching quadrature decoding. */
    void setEncoderDirectionReversed(bool reversed);

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
        std::uint32_t rawChangedAtUs_ = 0U;
    };

    /** @brief Converts the wrapping quadrature transition counter into complete detents. */
    std::int8_t sampleEncoder();

    /**
     * @brief Re-anchors quadrature bookkeeping after the encoder push switch settles.
     *
     * The PEC11L detent lies at a contact transition, so axial push/release can
     * leave a harmless partial quadrature residue even though the shaft is again
     * sitting in a mechanical detent. Re-anchoring after a debounced release
     * prevents that residue from consuming the next detent or direction reversal.
     */
    void resynchronizeEncoderAtRest();

    DebouncedButton encoderButton_;
    DebouncedButton transportButton_;
    DebouncedButton tapButton_;
    DebouncedButton resetButton_;
    std::uint32_t encoderLastTransitionCount_ = 0U;
    std::uint8_t encoderDetentPhase_ = 0U;
    std::int8_t encoderTransitionRemainder_ = 0;
    std::int16_t pendingEncoderDetents_ = 0;
    bool encoderDirectionReversed_ = false;
};

}  // namespace clockfw::hal
