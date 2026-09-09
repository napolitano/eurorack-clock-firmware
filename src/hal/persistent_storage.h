/**
 * @file persistent_storage.h
 * @brief Power-loss-safe A/B Flash persistence boundary for small firmware settings.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <cstddef>
#include <cstdint>

namespace clockfw::hal {

/**
 * @brief Provides a small byte-addressed persistence API backed by two independent
 * STM32F401 16-KiB Flash erase sectors.
 *
 * Every physical commit writes a complete logical image to the inactive slot,
 * verifies it, and programs the commit marker last. The previously committed
 * slot is therefore never erased until a newer complete image is durable.
 */
class PersistentStorage final {
public:
    /** Current logical image size. Kept deliberately small to avoid needless RAM use. */
    static constexpr std::size_t kCapacityBytes = 8192U;

    /** Hard architectural ceiling for future persistent images inside one 16-KiB slot. */
    static constexpr std::size_t kMaximumImageBytes = 12U * 1024U;

    /** Physical size of each independently erasable STM32F401 slot sector. */
    static constexpr std::size_t kPhysicalSlotBytes = 16U * 1024U;

    static_assert(kCapacityBytes <= kMaximumImageBytes, "Persistent image exceeds 12-KiB policy");

    /**
     * @brief Reads one range from the newest valid committed logical image.
     * @param offset Byte offset inside the logical image.
     * @param destination Destination buffer.
     * @param size Number of bytes to read.
     * @return True when the requested range was valid and readable.
     */
    bool readBytes(std::size_t offset, std::uint8_t* destination, std::size_t size) const;
    /**
     * @brief Updates one logical range and commits a complete new A/B generation.
     * @param offset Byte offset inside the logical image.
     * @param source Source buffer.
     * @param size Number of bytes to copy.
     * @return True only after the new generation has been committed and verified.
     */
    bool writeBytes(std::size_t offset, const std::uint8_t* source, std::size_t size);

#ifdef CLOCK_HOST_TEST
    /** @brief Resets both simulated Flash sectors to the erased value 0xFF. */
    static void resetForTest();

    /** @brief Returns number of successfully committed A/B slot generations. */
    static std::uint32_t writeCommitCountForTest();

    /** @brief Makes the next logical read fail without touching storage. */
    static void failNextReadForTest();

    /** @brief Makes the next write fail before touching the inactive slot. */
    static void failNextWriteForTest();

    /** @brief Simulates power loss after erase/payload programming but before COMMIT. */
    static void powerLossBeforeCommitForTest();

    /** @brief Corrupts one byte in the newest committed slot to exercise CRC fallback. */
    static void corruptNewestPayloadByteForTest(std::size_t offset);

    /** @brief Replaces one raw header word in the newest committed slot for validator tests. */
    static void corruptNewestHeaderWordForTest(std::size_t wordIndex, std::uint32_t value);

    /** @brief Seeds one valid legacy 4-KiB committed slot for migration regression tests. */
    static bool seedLegacyImageForTest(const std::uint8_t* image, std::size_t size);
#endif

private:
    /** @brief Returns whether one byte range fits completely inside the logical image. */
    static bool isRangeValid(std::size_t offset, std::size_t size);
};

}  // namespace clockfw::hal
