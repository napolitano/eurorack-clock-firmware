/**
 * @file story_contract.cpp
 * @brief Implements the fixed Phase-1 CLOCK Storybook semantic constants and schema spellings.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_contract.h"

namespace clockfw::sim::tutorial {

std::uint32_t currentSchemaVersion() {
    return 1U;
}

OutputProfile defaultOutputProfile() {
    return OutputProfile{};
}

bool phase1SupportsTransition(const TransitionKind transition) {
    return transition == TransitionKind::Cut;
}

bool presentationScenesAdvanceFirmwareTime() {
    return false;
}

PublicationFormat defaultPublicationFormat() {
    return PublicationFormat::Mp4H264;
}

std::string_view moduleControlName(const ModuleControl control) {
    switch (control) {
        case ModuleControl::Encoder:
            return "encoder";
        case ModuleControl::EncoderPush:
            return "encoder_push";
        case ModuleControl::Play:
            return "PLAY";
        case ModuleControl::Tap:
            return "TAP";
        case ModuleControl::StopBack:
            return "STOP_BACK";
        case ModuleControl::Power:
            return "power";
    }
    return {};
}

std::string_view patchActionName(const PatchAction action) {
    switch (action) {
        case PatchAction::SyncCable:
            return "sync_cable";
        case PatchAction::ResetCable:
            return "rst_cable";
    }
    return {};
}

std::string_view externalStimulusName(const ExternalStimulus stimulus) {
    switch (stimulus) {
        case ExternalStimulus::SyncGenerator:
            return "sync_generator";
        case ExternalStimulus::SyncSource:
            return "sync_source";
        case ExternalStimulus::ResetGenerator:
            return "rst_generator";
        case ExternalStimulus::ResetPulse:
            return "rst_pulse";
    }
    return {};
}

}  // namespace clockfw::sim::tutorial
