/**
 * @file native_smoke_main.cpp
 * @brief Minimal host entry point used only by ordinary PlatformIO native builds.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#if defined(CLOCK_NATIVE_SMOKE_BUILD) && !defined(PIO_UNIT_TESTING)

/**
 * Provides a successful host executable for `pio run -e native`.
 *
 * The native test environment builds the project sources with `test_build_src = yes`.
 * `PIO_UNIT_TESTING` therefore suppresses this smoke entry point while each Unity
 * suite provides its own main().
 *
 * @return Always zero because this target only verifies build configuration.
 */
int main() {
    return 0;
}

#endif
