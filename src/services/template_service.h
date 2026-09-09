/**
 * @file template_service.h
 * @brief Factory template catalog and deterministic template application logic.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>

#include "domain/clock_types.h"

namespace clockfw::services {

/** @brief Applies factory rhythm templates to the complete clock state. */
class TemplateService final {
public:
    /** Number of built-in factory templates. */
    static constexpr std::size_t kTemplateCount = 6U;

    /**
     * @brief Returns the compact display name for a built-in template.
     * @param templateIndex Zero-based template index.
     * @return Static template name, or an empty string for an invalid index.
     */
    static const char* name(std::size_t templateIndex);

    /**
     * @brief Applies one built-in template to the supplied state.
     * @param templateIndex Zero-based template index.
     * @param state State to overwrite with the selected starting configuration.
     * @return True when the index was valid and the template was applied.
     */
    static bool apply(std::size_t templateIndex, ClockState& state);
};

}  // namespace clockfw::services
