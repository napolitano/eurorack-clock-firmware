/**
 * @file sync_frontend_model.h
 * @brief Host-only nominal electrical model of the documented CLOCK LM393 SYNC frontend.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

namespace clockfw::testsupport {

/**
 * @brief Idealized Schmitt-comparator model for the documented Rev-1 SYNC input network.
 *
 * This model intentionally covers only the nominal resistor network around the LM393:
 * 100 kOhm input series resistor, 10 kOhm pull-down, 1 MOhm positive feedback,
 * and the 5-V -> 300 kOhm / 10 kOhm reference divider. It does not model resistor
 * tolerance, BAT85 clamp behavior, LM393 input/common-mode limits, propagation delay,
 * noise, output saturation, or PCB parasitics. Those remain physical HIL concerns.
 */
class SyncFrontendModel final {
public:
    static constexpr double kInputSeriesOhm = 100000.0;
    static constexpr double kInputPulldownOhm = 10000.0;
    static constexpr double kFeedbackOhm = 1000000.0;
    static constexpr double kReferenceTopOhm = 300000.0;
    static constexpr double kReferenceBottomOhm = 10000.0;
    static constexpr double kReferenceSupplyV = 5.0;
    static constexpr double kComparatorPullupV = 3.3;
    static constexpr double kComparatorLowV = 0.0;

    static constexpr double referenceVoltageV() {
        return kReferenceSupplyV * kReferenceBottomOhm /
            (kReferenceTopOhm + kReferenceBottomOhm);
    }

    static constexpr double inputThresholdForOutputV(const double outputV) {
        const double conductanceSum =
            (1.0 / kInputSeriesOhm) +
            (1.0 / kInputPulldownOhm) +
            (1.0 / kFeedbackOhm);
        return kInputSeriesOhm *
            (referenceVoltageV() * conductanceSum - outputV / kFeedbackOhm);
    }

    static constexpr double risingThresholdV() {
        return inputThresholdForOutputV(kComparatorLowV);
    }

    static constexpr double fallingThresholdV() {
        return inputThresholdForOutputV(kComparatorPullupV);
    }

    bool sample(const double inputV) {
        if (outputHigh_) {
            if (inputV < fallingThresholdV()) {
                outputHigh_ = false;
            }
        } else if (inputV > risingThresholdV()) {
            outputHigh_ = true;
        }
        return outputHigh_;
    }

    bool outputHigh() const {
        return outputHigh_;
    }

    void setOutputHighForTest(const bool high) {
        outputHigh_ = high;
    }

private:
    bool outputHigh_ = false;
};

}  // namespace clockfw::testsupport
