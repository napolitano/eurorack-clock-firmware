/**
 * @file virtual_input_signal.h
 * @brief Idealized analog-source and comparator model for simulator input jacks.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>

namespace clockfw::sim {

/** @brief Ideal source waveforms available ahead of the simulated comparator. */
enum class SignalWaveform : std::uint8_t { Square, Sine, Triangle };

/** @brief Returns the next selectable simulator source waveform. */
SignalWaveform nextSignalWaveform(SignalWaveform waveform);

/** @brief Returns a compact uppercase waveform label for developer instrumentation. */
const char* signalWaveformName(SignalWaveform waveform);

/**
 * @brief Samples an ideal bipolar source in the normalized range -1..+1.
 * @param waveform Source waveform.
 * @param nowUs Current virtual time in microseconds.
 * @param epochUs Generator phase epoch in microseconds.
 * @param periodUs Generator period in microseconds; zero is treated as one microsecond.
 * @return Normalized bipolar source value.
 */
double sampleIdealSignal(
    SignalWaveform waveform,
    std::uint64_t nowUs,
    std::uint64_t epochUs,
    std::uint64_t periodUs);

/**
 * @brief Applies the simulator's ideal zero-volt comparator threshold.
 * @param waveform Source waveform ahead of the comparator.
 * @param nowUs Current virtual time in microseconds.
 * @param epochUs Generator phase epoch in microseconds.
 * @param periodUs Generator period in microseconds.
 * @return True for comparator HI, false for comparator LO.
 *
 * This intentionally models only the digital conditioning boundary seen by the
 * firmware. LM393 common-mode limits, open-collector pull-up behavior, hysteresis,
 * noise, propagation delay, and the Eurorack protection network remain HIL concerns.
 */
bool comparatorHigh(
    SignalWaveform waveform,
    std::uint64_t nowUs,
    std::uint64_t epochUs,
    std::uint64_t periodUs);

}  // namespace clockfw::sim
