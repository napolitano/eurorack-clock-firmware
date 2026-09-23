/**
 * @file plugin.cpp
 * @brief VCV Rack plugin entry point for South Signal Lab CLOCK.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "plugin.hpp"

Plugin* pluginInstance = nullptr;

void init(Plugin* plugin) {
    pluginInstance = plugin;
    plugin->addModel(modelClock);
}
