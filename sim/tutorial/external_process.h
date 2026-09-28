/**
 * @file external_process.h
 * @brief Host-only executable discovery and argv-safe child-process execution for Storybook publication tools.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <filesystem>
#include <optional>
#include <string>
#include <vector>

namespace clockfw::sim::tutorial {

/** @brief Locates an executable on PATH without introducing a shell dependency. */
std::optional<std::filesystem::path> findHostExecutable(const std::string& name);

/** @brief Executes argv[0] with the supplied argument vector and returns its process exit code. */
int runHostProcess(const std::vector<std::string>& arguments);

}  // namespace clockfw::sim::tutorial
