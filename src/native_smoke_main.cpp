/**
 * @file native_smoke_main.cpp
 * @brief Minimal host entry point used only by ordinary PlatformIO native builds.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#if defined(CLOCK_NATIVE_SMOKE_BUILD)

/**
 * Provides a successful host executable for `pio run -e native`.
 *
 * PlatformIO unit tests do not build the project src directory unless
 * test_build_src is enabled, so the Unity test executable keeps its own main().
 *
 * @return Always zero because this target only verifies build configuration.
 */
int main() {
    return 0;
}

#endif
