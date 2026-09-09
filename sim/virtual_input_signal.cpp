/**
 * @file virtual_input_signal.cpp
 * @brief Idealized analog-source and comparator model for simulator input jacks.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "virtual_input_signal.h"

#include <cmath>

namespace clockfw::sim {
namespace {

constexpr double kTwoPi = 6.28318530717958647692;

}  // namespace

SignalWaveform nextSignalWaveform(const SignalWaveform waveform) {
    switch (waveform) {
        case SignalWaveform::Square: return SignalWaveform::Sine;
        case SignalWaveform::Sine: return SignalWaveform::Triangle;
        case SignalWaveform::Triangle: return SignalWaveform::Square;
    }
    return SignalWaveform::Square;
}

const char* signalWaveformName(const SignalWaveform waveform) {
    switch (waveform) {
        case SignalWaveform::Square: return "SQUARE";
        case SignalWaveform::Sine: return "SINE";
        case SignalWaveform::Triangle: return "TRIANGLE";
    }
    return "SQUARE";
}

double sampleIdealSignal(
    const SignalWaveform waveform,
    const std::uint64_t nowUs,
    const std::uint64_t epochUs,
    const std::uint64_t periodUs) {
    const std::uint64_t safePeriodUs = periodUs != 0ULL ? periodUs : 1ULL;
    const std::uint64_t elapsedUs = nowUs >= epochUs ? nowUs - epochUs : 0ULL;
    const double phase = static_cast<double>(elapsedUs % safePeriodUs) /
        static_cast<double>(safePeriodUs);

    switch (waveform) {
        case SignalWaveform::Square:
            return phase < 0.5 ? 1.0 : -1.0;
        case SignalWaveform::Sine:
            return std::sin(kTwoPi * phase);
        case SignalWaveform::Triangle:
            if (phase < 0.25) return phase * 4.0;
            if (phase < 0.75) return 2.0 - phase * 4.0;
            return phase * 4.0 - 4.0;
    }
    return 1.0;
}

bool comparatorHigh(
    const SignalWaveform waveform,
    const std::uint64_t nowUs,
    const std::uint64_t epochUs,
    const std::uint64_t periodUs) {
    return sampleIdealSignal(waveform, nowUs, epochUs, periodUs) >= 0.0;
}

}  // namespace clockfw::sim
