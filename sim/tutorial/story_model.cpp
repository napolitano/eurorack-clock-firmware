/**
 * @file story_model.cpp
 * @brief Implements stable schema spellings for the typed CLOCK Storybook model.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_model.h"

namespace clockfw::sim::tutorial {

std::string_view interactionProfileName(const InteractionProfile profile) {
    switch (profile) {
        case InteractionProfile::HumanSlow:
            return "HUMAN_SLOW";
        case InteractionProfile::HumanNormal:
            return "HUMAN_NORMAL";
        case InteractionProfile::HumanFast:
            return "HUMAN_FAST";
    }
    return {};
}

}  // namespace clockfw::sim::tutorial
