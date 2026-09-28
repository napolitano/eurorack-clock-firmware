/**
 * @file storybook_contract_tests.cpp
 * @brief Verifies the frozen host-only CLOCK Storybook Phase-1 semantic contract.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <cstdlib>
#include <iostream>

#include "tutorial/story_contract.h"

namespace {

using clockfw::sim::tutorial::ExternalStimulus;
using clockfw::sim::tutorial::ModuleControl;
using clockfw::sim::tutorial::PatchAction;
using clockfw::sim::tutorial::PublicationFormat;
using clockfw::sim::tutorial::TransitionKind;

bool require(const bool condition, const char* const message) {
    if (!condition) {
        std::cerr << "Storybook contract failure: " << message << '\n';
        return false;
    }
    return true;
}

}  // namespace

int main() {
    using namespace clockfw::sim::tutorial;

    bool ok = true;
    ok &= require(currentSchemaVersion() == 1U, "schema version must be 1");

    const OutputProfile output = defaultOutputProfile();
    ok &= require(output.width == 1920U, "default width must be 1920");
    ok &= require(output.height == 1080U, "default height must be 1080");
    ok &= require(output.framesPerSecond == 30U, "default frame rate must be 30 fps");

    ok &= require(phase1SupportsTransition(TransitionKind::Cut), "CUT must be supported in Phase 1");
    ok &= require(!phase1SupportsTransition(TransitionKind::Fade), "FADE must be rejected in Phase 1");
    ok &= require(!presentationScenesAdvanceFirmwareTime(), "presentation-only scenes must freeze firmware time");
    ok &= require(defaultPublicationFormat() == PublicationFormat::Mp4H264, "MP4/H.264 must be the default publication format");

    ok &= require(moduleControlName(ModuleControl::Encoder) == "encoder", "encoder schema spelling changed");
    ok &= require(moduleControlName(ModuleControl::EncoderPush) == "encoder_push", "encoder-push schema spelling changed");
    ok &= require(moduleControlName(ModuleControl::Play) == "PLAY", "PLAY schema spelling changed");
    ok &= require(moduleControlName(ModuleControl::Tap) == "TAP", "TAP schema spelling changed");
    ok &= require(moduleControlName(ModuleControl::StopBack) == "STOP_BACK", "STOP/BACK schema spelling changed");
    ok &= require(moduleControlName(ModuleControl::Power) == "power", "POWER schema spelling changed");

    ok &= require(patchActionName(PatchAction::SyncCable) == "sync_cable", "SYNC cable schema spelling changed");
    ok &= require(patchActionName(PatchAction::ResetCable) == "rst_cable", "RST cable schema spelling changed");

    ok &= require(externalStimulusName(ExternalStimulus::SyncGenerator) == "sync_generator", "SYNC generator schema spelling changed");
    ok &= require(externalStimulusName(ExternalStimulus::SyncSource) == "sync_source", "SYNC source schema spelling changed");
    ok &= require(externalStimulusName(ExternalStimulus::ResetGenerator) == "rst_generator", "RST generator schema spelling changed");
    ok &= require(externalStimulusName(ExternalStimulus::ResetPulse) == "rst_pulse", "RST pulse schema spelling changed");

    if (!ok) {
        return EXIT_FAILURE;
    }

    std::cout << "CLOCK Storybook Phase-1 contract: PASS\n";
    return EXIT_SUCCESS;
}
