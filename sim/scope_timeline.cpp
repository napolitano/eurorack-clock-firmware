/**
 * @file scope_timeline.cpp
 * @brief Time-axis and musical reference-grid model for the native simulator oscilloscope.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "scope_timeline.h"

#include <algorithm>
#include <cmath>

#include "clock_core.h"

namespace clockfw::sim::scope {
namespace {

constexpr double kMicrosPerMinuteMilliBpm = 60000000000.0;

/** Returns one physical master-beat duration for the configured meter unit. */
double masterBeatIntervalUs(const ClockState& state, const std::uint32_t bpmMilli) {
    const std::uint8_t beatUnit = state.masterMeter.unit != 0U ? state.masterMeter.unit : 4U;
    const double divisor = static_cast<double>(std::max<std::uint32_t>(bpmMilli, 1U)) *
        static_cast<double>(beatUnit);
    return (kMicrosPerMinuteMilliBpm * 4.0) / divisor;
}

/** Resolves one integer/rational rate without duplicating ClockEngine scheduling policy. */
void resolveRate(
    const RateSettings& rate,
    std::uint32_t& numerator,
    std::uint32_t& denominator) {
    CommonChannelSettings settings{};
    settings.rate = rate;
    core::calculateEffectiveRate(settings, numerator, denominator);
}

/** Chooses a power-of-two serial decimation while keeping enough visual references. */
std::uint32_t readableMinorStride(const double intervalUs, const std::uint64_t windowUs) {
    if (intervalUs <= 0.0) {
        return 1U;
    }
    const double rawReferences = static_cast<double>(windowUs) / intervalUs;
    std::uint32_t stride = 1U;
    while (rawReferences / static_cast<double>(stride) >
               static_cast<double>(kMaximumVisibleMinorReferences) &&
           stride <= (UINT32_MAX / 2U)) {
        stride *= 2U;
    }
    return stride;
}

}  // namespace

std::size_t zoomInIndex(const std::size_t index) {
    return index > 0U ? index - 1U : 0U;
}

std::size_t zoomOutIndex(const std::size_t index) {
    const std::size_t last = kWindowOptionsUs.size() - 1U;
    return index < last ? index + 1U : last;
}

std::uint32_t effectiveReferenceBpmMilli(
    const ClockState& state,
    const bool externalLocked,
    const std::uint32_t externalBpmMilli) {
    std::uint32_t bpmMilli = static_cast<std::uint32_t>(state.bpm) * 1000U;
    if (state.source == ClockSource::External) {
        if ((externalLocked || state.externalSync.lossMode == SyncLossMode::Freewheel) &&
            externalBpmMilli != 0U) {
            bpmMilli = externalBpmMilli;
        }
    } else if (state.source == ClockSource::Auto && externalLocked && externalBpmMilli != 0U) {
        bpmMilli = externalBpmMilli;
    }
    return std::max<std::uint32_t>(bpmMilli, 1U);
}

MusicalGridSpec musicalGridSpec(
    const ClockState& state,
    const std::uint32_t effectiveBpmMilli,
    const std::uint64_t windowUs) {
    MusicalGridSpec grid{};
    const double beatUs = masterBeatIntervalUs(state, effectiveBpmMilli);

    if (state.operatingMode == OperatingMode::UnifiedClock) {
        std::uint32_t numerator = 1U;
        std::uint32_t denominator = 1U;
        resolveRate(state.unifiedClock.rate, numerator, denominator);
        grid.intervalUs = beatUs * static_cast<double>(denominator) /
            static_cast<double>(std::max<std::uint32_t>(numerator, 1U));
        grid.basis = "ONE CLOCK";
    } else if (state.operatingMode == OperatingMode::DividerBank) {
        // OUT1 of every divider family is the undivided master beat.
        grid.intervalUs = beatUs;
        grid.basis = "BEAT";
    } else {
        // Pattern x1 is exactly a sixteenth note. CLOCK note values 1/1..1/16
        // also lie on this lattice, making it the useful common reference in
        // mixed CLOCK/EUCLID/SEQ views.
        grid.intervalUs = kMicrosPerMinuteMilliBpm /
            (4.0 * static_cast<double>(std::max<std::uint32_t>(effectiveBpmMilli, 1U)));
        grid.basis = "1/16";
    }

    grid.intervalUs = std::max(grid.intervalUs, 1.0);
    grid.minorEvery = readableMinorStride(grid.intervalUs, windowUs);
    grid.majorEvery = grid.minorEvery <= UINT32_MAX / 4U
        ? grid.minorEvery * 4U
        : grid.minorEvery;
    return grid;
}

std::uint64_t firstMinorReferenceSerialAtOrAfter(
    const std::int64_t startUs,
    const MusicalGridSpec& grid) {
    const double safeIntervalUs = std::max(grid.intervalUs, 1.0);
    const std::uint64_t stride = std::max<std::uint32_t>(grid.minorEvery, 1U);
    if (startUs <= 0LL) {
        return 0ULL;
    }
    const double rawSerial = std::ceil(static_cast<double>(startUs) / safeIntervalUs);
    const std::uint64_t serial = rawSerial > 0.0
        ? static_cast<std::uint64_t>(rawSerial)
        : 0ULL;
    const std::uint64_t remainder = serial % stride;
    return remainder == 0ULL ? serial : serial + (stride - remainder);
}

double referenceTimeUs(const std::uint64_t serial, const MusicalGridSpec& grid) {
    return static_cast<double>(serial) * std::max(grid.intervalUs, 1.0);
}

double normalizedPosition(
    const double timestampUs,
    const double startUs,
    const std::uint64_t windowUs) {
    if (windowUs == 0ULL) {
        return 0.0;
    }
    const double position = (timestampUs - startUs) / static_cast<double>(windowUs);
    return std::clamp(position, 0.0, 1.0);
}

bool isMajorReference(const std::uint64_t serial, const MusicalGridSpec& grid) {
    const std::uint64_t majorEvery = std::max<std::uint32_t>(grid.majorEvery, 1U);
    return (serial % majorEvery) == 0ULL;
}

}  // namespace clockfw::sim::scope
