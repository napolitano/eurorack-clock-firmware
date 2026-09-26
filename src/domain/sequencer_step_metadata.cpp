/**
 * @file sequencer_step_metadata.cpp
 * @brief Sequencer 2.0 step-metadata validation and stable packing.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "domain/sequencer_step_metadata.h"

namespace clockfw {
namespace {
constexpr std::uint16_t kProbabilityMask = 0x007FU;
constexpr std::uint16_t kGateProfileMask = 0x0780U;
constexpr std::uint16_t kRatchetMask = 0x3800U;
constexpr std::uint16_t kTieMask = 0x4000U;
constexpr std::uint16_t kReservedMask = 0x8000U;
constexpr unsigned kGateProfileShift = 7U;
constexpr unsigned kRatchetShift = 11U;

bool validGateProfile(const SequencerGateProfile profile) {
    return profile >= SequencerGateProfile::Default &&
        profile <= SequencerGateProfile::Gate100Ms;
}
}  // namespace

bool isSequencerStepMetadataValid(const SequencerStepMetadata& metadata) {
    return metadata.probabilityPercent <= 100U &&
        validGateProfile(metadata.gateProfile) &&
        metadata.ratchetCount >= 1U && metadata.ratchetCount <= kSequencerMaximumRatchetCount;
}

bool isSequencerStepMetadataDefault(const SequencerStepMetadata& metadata) {
    return metadata.probabilityPercent == 0U &&
        metadata.gateProfile == SequencerGateProfile::Default &&
        metadata.ratchetCount == 1U && !metadata.tie;
}

SequencerStepMetadataWord packSequencerStepMetadata(const SequencerStepMetadata& metadata) {
    if (!isSequencerStepMetadataValid(metadata)) {
        return 0U;
    }
    const std::uint16_t probability = metadata.probabilityPercent;
    const std::uint16_t gate = static_cast<std::uint16_t>(metadata.gateProfile);
    const std::uint16_t ratchet = static_cast<std::uint16_t>(metadata.ratchetCount - 1U);
    return static_cast<std::uint16_t>(
        probability |
        static_cast<std::uint16_t>(gate << kGateProfileShift) |
        static_cast<std::uint16_t>(ratchet << kRatchetShift) |
        (metadata.tie ? kTieMask : 0U));
}

SequencerStepMetadata unpackSequencerStepMetadata(const SequencerStepMetadataWord word) {
    SequencerStepMetadata metadata{};
    if ((word & kReservedMask) != 0U) {
        return metadata;
    }
    metadata.probabilityPercent = static_cast<std::uint8_t>(word & kProbabilityMask);
    metadata.gateProfile = static_cast<SequencerGateProfile>(
        (word & kGateProfileMask) >> kGateProfileShift);
    metadata.ratchetCount = static_cast<std::uint8_t>(
        ((word & kRatchetMask) >> kRatchetShift) + 1U);
    metadata.tie = (word & kTieMask) != 0U;
    if (!isSequencerStepMetadataValid(metadata)) {
        return SequencerStepMetadata{};
    }
    return metadata;
}

std::uint8_t sequencerGateProfileDutyPercent(const SequencerGateProfile profile) {
    switch (profile) {
        case SequencerGateProfile::Duty25: return 25U;
        case SequencerGateProfile::Duty50: return 50U;
        case SequencerGateProfile::Duty75: return 75U;
        default: return 0U;
    }
}

std::uint16_t sequencerGateProfileMilliseconds(
    const SequencerGateProfile profile,
    const std::uint16_t fallbackMs) {
    switch (profile) {
        case SequencerGateProfile::Trigger1Ms: return 1U;
        case SequencerGateProfile::Trigger2Ms: return 2U;
        case SequencerGateProfile::Trigger5Ms: return 5U;
        case SequencerGateProfile::Trigger10Ms: return 10U;
        case SequencerGateProfile::Gate20Ms: return 20U;
        case SequencerGateProfile::Gate50Ms: return 50U;
        case SequencerGateProfile::Gate100Ms: return 100U;
        default: return fallbackMs;
    }
}

}  // namespace clockfw
