/**
 * @file png_writer.h
 * @brief Deterministic dependency-free PNG/RGBA8 writer for Storybook lossless frames.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#pragma once

#include <cstdint>
#include <filesystem>
#include <string>

#include "tutorial/tutorial_surface.h"

namespace clockfw::sim::tutorial {

/** @brief Writes one deterministic lossless PNG using RGBA8 colour and no external codec dependency. */
void writeTutorialPng(const std::filesystem::path& path, const TutorialSurface& surface);

/** @brief Returns a deterministic non-cryptographic FNV-1a digest of raw RGBA8 pixels. */
std::uint64_t tutorialRgbaFnv1a64(const TutorialSurface& surface);

/** @brief Formats one 64-bit digest as 16 lowercase hexadecimal digits. */
std::string formatDigest64(std::uint64_t digest);

}  // namespace clockfw::sim::tutorial
