/**
 * @file sequencer_step_metadata.h
 * @brief Compact Sequencer 2.0 per-step override model.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstdint>

namespace clockfw {

/** Packed real-time representation used by the scheduler: no compiler bitfields. */
using SequencerStepMetadataWord = std::uint16_t;

/** Gate-duration override applied to one Sequencer step. */
enum class SequencerGateProfile : std::uint8_t {
    Default = 0U,
    Trigger1Ms,
    Trigger2Ms,
    Trigger5Ms,
    Trigger10Ms,
    Duty25,
    Duty50,
    Duty75,
    Gate20Ms,
    Gate50Ms,
    Gate100Ms,
};

/**
 * @brief Optional expression attached to one zero-based Sequencer step.
 *
 * Probability 0 means inherit the channel value. Ratchet count 1 means no
 * ratchet. DEFAULT gate profile inherits the channel gate length. Tie and
 * ratchet are mutually exclusive when normalized for runtime use.
 */
struct SequencerStepMetadata final {
    std::uint8_t probabilityPercent = 0U;
    SequencerGateProfile gateProfile = SequencerGateProfile::Default;
    std::uint8_t ratchetCount = 1U;
    bool tie = false;
};

/** Maximum bounded EVEN ratchet count supported by the Sequencer step model. */
inline constexpr std::uint8_t kSequencerMaximumRatchetCount = 8U;

/** @brief Returns true when every encoded step-override value is valid. */
bool isSequencerStepMetadataValid(const SequencerStepMetadata& metadata);

/** @brief Returns true when a step inherits all channel defaults and has no expression override. */
bool isSequencerStepMetadataDefault(const SequencerStepMetadata& metadata);

/** @brief Packs one validated metadata record into the stable 16-bit runtime representation. */
SequencerStepMetadataWord packSequencerStepMetadata(const SequencerStepMetadata& metadata);

/** @brief Decodes one stable 16-bit runtime word; invalid/reserved words decode to defaults. */
SequencerStepMetadata unpackSequencerStepMetadata(SequencerStepMetadataWord word);

/** @brief Returns a relative duty percentage, or zero for DEFAULT/absolute trigger profiles. */
std::uint8_t sequencerGateProfileDutyPercent(SequencerGateProfile profile);

/** @brief Returns an absolute gate duration in ms, inheriting fallbackMs for DEFAULT/duty profiles. */
std::uint16_t sequencerGateProfileMilliseconds(
    SequencerGateProfile profile,
    std::uint16_t fallbackMs);

}  // namespace clockfw
