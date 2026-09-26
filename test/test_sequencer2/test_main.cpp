/**
 * @file test_main.cpp
 * @brief Focused contracts for the Sequencer 2.0 pattern model and persistent bank.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include <array>
#include <cstdint>
#include <unity.h>

#include "config.h"
#include "domain/sequencer_pattern.h"
#include "domain/sequencer_step_metadata.h"
#include "hal/persistent_layout.h"
#include "hal/persistent_storage.h"
#include "services/sequencer_pattern_store.h"
#include "services/sequencer_step_store.h"

using namespace clockfw;

void setUp() { hal::PersistentStorage::resetForTest(); }
void tearDown() {}

namespace {

std::uint32_t testCrc32(const std::uint8_t* data, std::size_t size) {
    std::uint32_t crc = 0xFFFFFFFFUL;
    for (std::size_t index = 0U; index < size; ++index) {
        crc ^= data[index];
        for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
            const bool low = (crc & 1U) != 0U;
            crc >>= 1U;
            if (low) crc ^= 0xEDB88320UL;
        }
    }
    return crc ^ 0xFFFFFFFFUL;
}

void writeTest32(std::uint8_t* destination, std::uint32_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    destination[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    destination[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

void testDefaultPatternAnd128StepBoundary() {
    SequencerPatternV2 pattern{};
    TEST_ASSERT_TRUE(isSequencerPatternValid(pattern));
    TEST_ASSERT_EQUAL_UINT8(16U, pattern.length);
    TEST_ASSERT_TRUE(sequencerPatternGate(pattern, 0U));
    TEST_ASSERT_TRUE(sequencerPatternGate(pattern, 4U));
    TEST_ASSERT_FALSE(sequencerPatternGate(pattern, 1U));

    pattern.length = 128U;
    pattern.rotation = 127U;
    TEST_ASSERT_TRUE(setSequencerPatternGate(pattern, 127U, true));
    TEST_ASSERT_TRUE(sequencerPatternGate(pattern, 127U));
    TEST_ASSERT_FALSE(setSequencerPatternGate(pattern, 128U, true));
    TEST_ASSERT_FALSE(sequencerPatternGate(pattern, 128U));
    TEST_ASSERT_TRUE(isSequencerPatternValid(pattern));
}

void testClampPatternClearsBitsOutsideLength() {
    SequencerPatternV2 pattern{};
    pattern.length = 65U;
    pattern.rotation = 64U;
    pattern.gates = {{UINT64_MAX, UINT64_MAX}};
    clampSequencerPattern(pattern);
    TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, pattern.gates[0]);
    TEST_ASSERT_EQUAL_UINT64(1ULL, pattern.gates[1]);
    TEST_ASSERT_EQUAL_UINT8(64U, pattern.rotation);

    pattern.length = 4U;
    pattern.rotation = 9U;
    clampSequencerPattern(pattern);
    TEST_ASSERT_EQUAL_UINT64(0xFULL, pattern.gates[0]);
    TEST_ASSERT_EQUAL_UINT64(0ULL, pattern.gates[1]);
    TEST_ASSERT_EQUAL_UINT8(0U, pattern.rotation);
}

void testForwardReverseAndPingPongTraversal() {
    SequencerPatternV2 pattern{};
    pattern.length = 5U;
    const std::array<std::uint8_t, 8U> forward{{0U, 1U, 2U, 3U, 4U, 0U, 1U, 2U}};
    for (std::size_t index = 0U; index < forward.size(); ++index) {
        const auto result = resolveSequencerTraversal(index, pattern, 0x1234U);
        TEST_ASSERT_TRUE(result.active);
        TEST_ASSERT_EQUAL_UINT8(forward[index], result.step);
    }

    pattern.direction = SequencerPlayDirection::Reverse;
    const std::array<std::uint8_t, 8U> reverse{{4U, 3U, 2U, 1U, 0U, 4U, 3U, 2U}};
    for (std::size_t index = 0U; index < reverse.size(); ++index) {
        TEST_ASSERT_EQUAL_UINT8(
            reverse[index], resolveSequencerTraversal(index, pattern, 0x1234U).step);
    }

    pattern.direction = SequencerPlayDirection::PingPong;
    TEST_ASSERT_EQUAL_UINT32(8U, sequencerTraversalCycleLength(pattern));
    const std::array<std::uint8_t, 12U> pingPong{{0U,1U,2U,3U,4U,3U,2U,1U,0U,1U,2U,3U}};
    for (std::size_t index = 0U; index < pingPong.size(); ++index) {
        TEST_ASSERT_EQUAL_UINT8(
            pingPong[index], resolveSequencerTraversal(index, pattern, 0x1234U).step);
    }
}

void testOnceStopsAfterOneTraversalCycle() {
    SequencerPatternV2 pattern{};
    pattern.length = 5U;
    pattern.loopMode = SequencerLoopMode::Once;
    for (std::uint64_t event = 0U; event < 5U; ++event) {
        TEST_ASSERT_TRUE(resolveSequencerTraversal(event, pattern, 0U).active);
    }
    TEST_ASSERT_FALSE(resolveSequencerTraversal(5U, pattern, 0U).active);

    pattern.direction = SequencerPlayDirection::PingPong;
    for (std::uint64_t event = 0U; event < 8U; ++event) {
        TEST_ASSERT_TRUE(resolveSequencerTraversal(event, pattern, 0U).active);
    }
    TEST_ASSERT_FALSE(resolveSequencerTraversal(8U, pattern, 0U).active);

    pattern.direction = SequencerPlayDirection::Random;
    for (std::uint64_t event = 0U; event < 5U; ++event) {
        TEST_ASSERT_TRUE(resolveSequencerTraversal(event, pattern, 0xCAFEU).active);
    }
    TEST_ASSERT_FALSE(resolveSequencerTraversal(5U, pattern, 0xCAFEU).active);
}

void testRandomTraversalIsDeterministicHistoryFreeAndBounded() {
    SequencerPatternV2 pattern{};
    pattern.length = 13U;
    pattern.direction = SequencerPlayDirection::Random;

    bool anyDifferentSeed = false;
    for (std::uint64_t event = 0U; event < 128U; ++event) {
        const auto first = resolveSequencerTraversal(event, pattern, 0x12345678U);
        const auto repeated = resolveSequencerTraversal(event, pattern, 0x12345678U);
        const auto otherSeed = resolveSequencerTraversal(event, pattern, 0x87654321U);
        TEST_ASSERT_TRUE(first.active);
        TEST_ASSERT_EQUAL_UINT8(first.step, repeated.step);
        TEST_ASSERT_TRUE(first.step < pattern.length);
        anyDifferentSeed = anyDifferentSeed || first.step != otherSeed.step;
    }
    TEST_ASSERT_TRUE(anyDifferentSeed);
}

void testRotationIsAppliedAfterTraversal() {
    SequencerPatternV2 pattern{};
    pattern.length = 8U;
    pattern.rotation = 3U;
    pattern.gates = {{0ULL, 0ULL}};
    TEST_ASSERT_TRUE(setSequencerPatternGate(pattern, 3U, true));
    TEST_ASSERT_TRUE(sequencerPatternHitForEvent(0U, pattern, 0U));
    TEST_ASSERT_FALSE(sequencerPatternHitForEvent(1U, pattern, 0U));

    pattern.direction = SequencerPlayDirection::Reverse;
    pattern.gates = {{0ULL, 0ULL}};
    TEST_ASSERT_TRUE(setSequencerPatternGate(pattern, 2U, true));
    TEST_ASSERT_TRUE(sequencerPatternHitForEvent(0U, pattern, 0U));
}



void testStepMetadataPackingProfilesAndInvalidWords() {
    SequencerStepMetadata metadata{};
    TEST_ASSERT_TRUE(isSequencerStepMetadataValid(metadata));
    TEST_ASSERT_TRUE(isSequencerStepMetadataDefault(metadata));
    TEST_ASSERT_EQUAL_UINT32(0U, packSequencerStepMetadata(metadata));

    metadata.probabilityPercent = 73U;
    metadata.gateProfile = SequencerGateProfile::Duty50;
    metadata.ratchetCount = 4U;
    metadata.tie = true;
    TEST_ASSERT_TRUE(isSequencerStepMetadataValid(metadata));
    TEST_ASSERT_FALSE(isSequencerStepMetadataDefault(metadata));
    const SequencerStepMetadataWord packed = packSequencerStepMetadata(metadata);
    const SequencerStepMetadata restored = unpackSequencerStepMetadata(packed);
    TEST_ASSERT_EQUAL_UINT8(73U, restored.probabilityPercent);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<std::uint8_t>(SequencerGateProfile::Duty50),
        static_cast<std::uint8_t>(restored.gateProfile));
    TEST_ASSERT_EQUAL_UINT8(4U, restored.ratchetCount);
    TEST_ASSERT_TRUE(restored.tie);

    TEST_ASSERT_EQUAL_UINT8(25U, sequencerGateProfileDutyPercent(SequencerGateProfile::Duty25));
    TEST_ASSERT_EQUAL_UINT8(50U, sequencerGateProfileDutyPercent(SequencerGateProfile::Duty50));
    TEST_ASSERT_EQUAL_UINT8(75U, sequencerGateProfileDutyPercent(SequencerGateProfile::Duty75));
    TEST_ASSERT_EQUAL_UINT8(0U, sequencerGateProfileDutyPercent(SequencerGateProfile::Trigger5Ms));
    TEST_ASSERT_EQUAL_UINT32(1U, sequencerGateProfileMilliseconds(SequencerGateProfile::Trigger1Ms, 17U));
    TEST_ASSERT_EQUAL_UINT32(2U, sequencerGateProfileMilliseconds(SequencerGateProfile::Trigger2Ms, 17U));
    TEST_ASSERT_EQUAL_UINT32(5U, sequencerGateProfileMilliseconds(SequencerGateProfile::Trigger5Ms, 17U));
    TEST_ASSERT_EQUAL_UINT32(10U, sequencerGateProfileMilliseconds(SequencerGateProfile::Trigger10Ms, 17U));
    TEST_ASSERT_EQUAL_UINT32(20U, sequencerGateProfileMilliseconds(SequencerGateProfile::Gate20Ms, 17U));
    TEST_ASSERT_EQUAL_UINT32(50U, sequencerGateProfileMilliseconds(SequencerGateProfile::Gate50Ms, 17U));
    TEST_ASSERT_EQUAL_UINT32(100U, sequencerGateProfileMilliseconds(SequencerGateProfile::Gate100Ms, 17U));
    TEST_ASSERT_EQUAL_UINT32(17U, sequencerGateProfileMilliseconds(SequencerGateProfile::Duty75, 17U));

    metadata.probabilityPercent = 101U;
    TEST_ASSERT_FALSE(isSequencerStepMetadataValid(metadata));
    TEST_ASSERT_EQUAL_UINT32(0U, packSequencerStepMetadata(metadata));
    metadata = SequencerStepMetadata{};
    metadata.ratchetCount = 0U;
    TEST_ASSERT_FALSE(isSequencerStepMetadataValid(metadata));
    metadata.ratchetCount = static_cast<std::uint8_t>(kSequencerMaximumRatchetCount + 1U);
    TEST_ASSERT_FALSE(isSequencerStepMetadataValid(metadata));
    const SequencerStepMetadata reserved = unpackSequencerStepMetadata(0x8000U);
    TEST_ASSERT_TRUE(isSequencerStepMetadataDefault(reserved));
}

void testSparseStepStoreRoundTripRemovalAndPatternCopy() {
    hal::PersistentStorage storage;
    services::SequencerStepStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<std::uint32_t>(store.overrideCount()));

    SequencerStepMetadata first{};
    first.probabilityPercent = 42U;
    first.gateProfile = SequencerGateProfile::Duty25;
    first.ratchetCount = 3U;
    TEST_ASSERT_TRUE(store.updateMetadata(2U, 5U, 127U, first, 10U));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<std::uint32_t>(store.overrideCount()));
    TEST_ASSERT_TRUE(store.dirty());
    TEST_ASSERT_EQUAL_UINT8(42U, store.metadata(2U, 5U, 127U).probabilityPercent);

    std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> words{};
    store.loadPatternWords(2U, 5U, words);
    TEST_ASSERT_EQUAL_UINT32(packSequencerStepMetadata(first), words[127U]);
    TEST_ASSERT_EQUAL_UINT32(0U, words[126U]);

    SequencerStepMetadata second{};
    second.gateProfile = SequencerGateProfile::Trigger2Ms;
    second.tie = true;
    words[3U] = packSequencerStepMetadata(second);
    TEST_ASSERT_TRUE(store.replacePatternWords(1U, 1U, words, 11U));
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<std::uint32_t>(store.overrideCount()));
    TEST_ASSERT_EQUAL_UINT8(42U, store.metadata(1U, 1U, 127U).probabilityPercent);
    TEST_ASSERT_TRUE(store.metadata(1U, 1U, 3U).tie);

    TEST_ASSERT_TRUE(store.flush());
    services::SequencerStepStore restored(storage);
    TEST_ASSERT_TRUE(restored.begin());
    TEST_ASSERT_EQUAL_UINT32(3U, static_cast<std::uint32_t>(restored.overrideCount()));
    TEST_ASSERT_TRUE(restored.metadata(1U, 1U, 3U).tie);
    TEST_ASSERT_EQUAL_UINT8(42U, restored.metadata(1U, 1U, 127U).probabilityPercent);

    TEST_ASSERT_TRUE(restored.updateMetadata(1U, 1U, 3U, SequencerStepMetadata{}, 100U));
    TEST_ASSERT_EQUAL_UINT32(2U, static_cast<std::uint32_t>(restored.overrideCount()));
    TEST_ASSERT_TRUE(restored.clearPattern(1U, 1U, 101U));
    TEST_ASSERT_EQUAL_UINT32(1U, static_cast<std::uint32_t>(restored.overrideCount()));
    TEST_ASSERT_TRUE(restored.clearPattern(1U, 1U, 102U));
}

void testSparseStepStoreCapacityValidationAndReservedSongRegion() {
    hal::PersistentStorage storage;
    std::array<std::uint8_t, hal::persistent_layout::kSequencerSongRegionBytes> songSentinel{};
    for (std::size_t index = 0U; index < songSentinel.size(); ++index) {
        songSentinel[index] = static_cast<std::uint8_t>((index * 19U + 11U) & 0xFFU);
    }
    TEST_ASSERT_TRUE(storage.writeBytes(
        hal::persistent_layout::kSequencerSongRegionOffset,
        songSentinel.data(),
        songSentinel.size()));

    services::SequencerStepStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    SequencerStepMetadata metadata{};
    metadata.probabilityPercent = 100U;
    for (std::size_t index = 0U; index < services::SequencerStepStore::kMaximumOverrides; ++index) {
        const std::uint8_t step = static_cast<std::uint8_t>(index % kSequencerMaximumSteps);
        const std::size_t patternIndex = index / kSequencerMaximumSteps;
        const std::uint8_t channel = static_cast<std::uint8_t>(patternIndex % kChannelCount);
        const std::uint8_t slot = static_cast<std::uint8_t>(patternIndex / kChannelCount);
        TEST_ASSERT_TRUE(store.updateMetadata(channel, slot, step, metadata, 20U));
    }
    TEST_ASSERT_EQUAL_UINT32(
        static_cast<std::uint32_t>(services::SequencerStepStore::kMaximumOverrides),
        static_cast<std::uint32_t>(store.overrideCount()));
    TEST_ASSERT_FALSE(store.updateMetadata(2U, 7U, 124U, metadata, 21U));
    TEST_ASSERT_TRUE(store.flush());

    std::array<std::uint8_t, hal::persistent_layout::kSequencerSongRegionBytes> after{};
    TEST_ASSERT_TRUE(storage.readBytes(
        hal::persistent_layout::kSequencerSongRegionOffset, after.data(), after.size()));
    for (std::size_t index = 0U; index < after.size(); ++index) {
        TEST_ASSERT_EQUAL_UINT8(songSentinel[index], after[index]);
    }

    TEST_ASSERT_FALSE(store.updateMetadata(kChannelCount, 0U, 0U, metadata, 0U));
    TEST_ASSERT_FALSE(store.updateMetadata(0U, kSequencerPatternSlotsPerChannel, 0U, metadata, 0U));
    TEST_ASSERT_FALSE(store.updateMetadata(0U, 0U, kSequencerMaximumSteps, metadata, 0U));
    metadata.ratchetCount = 0U;
    TEST_ASSERT_FALSE(store.updateMetadata(0U, 0U, 0U, metadata, 0U));
    TEST_ASSERT_TRUE(isSequencerStepMetadataDefault(store.metadata(kChannelCount, 0U, 0U)));
    TEST_ASSERT_TRUE(isSequencerStepMetadataDefault(store.metadata(0U, 0U, kSequencerMaximumSteps)));
    TEST_ASSERT_FALSE(store.clearPattern(kChannelCount, 0U, 0U));
    TEST_ASSERT_FALSE(store.clearPattern(0U, kSequencerPatternSlotsPerChannel, 0U));

    std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> invalidWords{};
    invalidWords[0U] = 0x8000U;
    TEST_ASSERT_FALSE(store.replacePatternWords(0U, 0U, invalidWords, 0U));
    TEST_ASSERT_FALSE(store.replacePatternWords(kChannelCount, 0U, {}, 0U));
}

void testSparseStepStoreDelayedCommitAndReadWriteFailures() {
    hal::PersistentStorage storage;
    services::SequencerStepStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    SequencerStepMetadata metadata{};
    metadata.gateProfile = SequencerGateProfile::Gate20Ms;
    TEST_ASSERT_TRUE(store.updateMetadata(0U, 0U, 4U, metadata, 100U));
    TEST_ASSERT_TRUE(store.service(100U + config::kPersistenceCommitDelayMs - 1U, true));
    TEST_ASSERT_TRUE(store.dirty());
    TEST_ASSERT_TRUE(store.service(100U + config::kPersistenceCommitDelayMs, false));
    TEST_ASSERT_TRUE(store.dirty());
    hal::PersistentStorage::failNextWriteForTest();
    TEST_ASSERT_FALSE(store.service(100U + config::kPersistenceCommitDelayMs, true));
    TEST_ASSERT_TRUE(store.dirty());
    TEST_ASSERT_TRUE(store.flush());
    TEST_ASSERT_FALSE(store.dirty());

    hal::PersistentStorage::failNextReadForTest();
    services::SequencerStepStore unreadable(storage);
    TEST_ASSERT_FALSE(unreadable.begin());
    TEST_ASSERT_EQUAL_UINT32(0U, static_cast<std::uint32_t>(unreadable.overrideCount()));
}

void testLegacyChannelSequencesSeedPatternOneOnly() {
    hal::PersistentStorage storage;
    services::SequencerPatternStore store(storage);
    TEST_ASSERT_TRUE(store.begin());

    ClockState state{};
    for (std::uint8_t channel = 0U; channel < kChannelCount; ++channel) {
        state.channels[channel].sequencer.length = static_cast<std::uint8_t>(9U + channel);
        state.channels[channel].sequencer.rotation = static_cast<std::uint8_t>(channel % 3U);
        state.channels[channel].sequencer.pattern = 1ULL << channel;
    }
    // Defensive migration contracts: old/corrupt zero length becomes one step,
    // values above the released 64-step ceiling are capped, and rotations that
    // no longer fit the migrated length are re-anchored to zero.
    state.channels[0].sequencer.length = 0U;
    state.channels[0].sequencer.rotation = 7U;
    state.channels[1].sequencer.length = 65U;
    state.channels[1].sequencer.rotation = 64U;

    TEST_ASSERT_TRUE(store.seedLegacyPatternOnes(state, 123U));
    TEST_ASSERT_FALSE(store.seedLegacyPatternOnes(state, 124U));
    for (std::uint8_t channel = 0U; channel < kChannelCount; ++channel) {
        const auto& migrated = store.pattern(channel, 0U);
        const std::uint8_t expectedLength = channel == 0U
            ? 1U
            : (channel == 1U ? 64U : static_cast<std::uint8_t>(9U + channel));
        const std::uint8_t expectedRotation = channel < 2U
            ? 0U
            : static_cast<std::uint8_t>(channel % 3U);
        TEST_ASSERT_EQUAL_UINT8(expectedLength, migrated.length);
        TEST_ASSERT_EQUAL_UINT8(expectedRotation, migrated.rotation);
        TEST_ASSERT_EQUAL_UINT8(
            static_cast<std::uint8_t>(SequencerPlayDirection::Forward),
            static_cast<std::uint8_t>(migrated.direction));
        TEST_ASSERT_EQUAL_UINT8(
            static_cast<std::uint8_t>(SequencerLoopMode::Loop),
            static_cast<std::uint8_t>(migrated.loopMode));
        TEST_ASSERT_TRUE(sequencerPatternGate(migrated, channel));
        TEST_ASSERT_EQUAL_UINT8(16U, store.pattern(channel, 1U).length);
    }
    TEST_ASSERT_TRUE(store.flush());

    services::SequencerPatternStore restored(storage);
    TEST_ASSERT_TRUE(restored.begin());
    TEST_ASSERT_TRUE(restored.hasDurableBank());
    TEST_ASSERT_FALSE(restored.seedLegacyPatternOnes(state, 999U));
}

void testPersistentBankRoundTripAndDelayedCommit() {
    hal::PersistentStorage storage;
    services::SequencerPatternStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_FALSE(store.hasDurableBank());
    TEST_ASSERT_FALSE(store.dirty());

    SequencerPatternV2 pattern{};
    pattern.length = 128U;
    pattern.rotation = 127U;
    pattern.direction = SequencerPlayDirection::Random;
    pattern.loopMode = SequencerLoopMode::Once;
    pattern.gates = {{0ULL, 0ULL}};
    TEST_ASSERT_TRUE(setSequencerPatternGate(pattern, 127U, true));
    TEST_ASSERT_TRUE(store.updatePattern(7U, 7U, pattern, 100U));
    TEST_ASSERT_TRUE(store.dirty());
    TEST_ASSERT_EQUAL_UINT32(0U, hal::PersistentStorage::writeCommitCountForTest());

    TEST_ASSERT_TRUE(store.service(100U + config::kPersistenceCommitDelayMs - 1U, true));
    TEST_ASSERT_EQUAL_UINT32(0U, hal::PersistentStorage::writeCommitCountForTest());
    TEST_ASSERT_TRUE(store.service(100U + config::kPersistenceCommitDelayMs, true));
    TEST_ASSERT_EQUAL_UINT32(1U, hal::PersistentStorage::writeCommitCountForTest());
    TEST_ASSERT_FALSE(store.dirty());

    services::SequencerPatternStore restored(storage);
    TEST_ASSERT_TRUE(restored.begin());
    TEST_ASSERT_TRUE(restored.hasDurableBank());
    const auto& loaded = restored.pattern(7U, 7U);
    TEST_ASSERT_EQUAL_UINT8(128U, loaded.length);
    TEST_ASSERT_EQUAL_UINT8(127U, loaded.rotation);
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<std::uint8_t>(SequencerPlayDirection::Random),
        static_cast<std::uint8_t>(loaded.direction));
    TEST_ASSERT_EQUAL_UINT8(
        static_cast<std::uint8_t>(SequencerLoopMode::Once),
        static_cast<std::uint8_t>(loaded.loopMode));
    TEST_ASSERT_TRUE(sequencerPatternGate(loaded, 127U));
}

void testV1EightKiBImageMigratesWithoutTouchingHistoricalBytes() {
    std::array<std::uint8_t, hal::persistent_layout::kV1ImageBytes> oldImage{};
    for (std::size_t index = 0U; index < oldImage.size(); ++index) {
        oldImage[index] = static_cast<std::uint8_t>((index * 17U + 3U) & 0xFFU);
    }
    TEST_ASSERT_TRUE(hal::PersistentStorage::seedV1ImageForTest(oldImage.data(), oldImage.size()));

    hal::PersistentStorage storage;
    std::array<std::uint8_t, 32U> extensionProbe{};
    TEST_ASSERT_TRUE(storage.readBytes(
        hal::persistent_layout::kSequencer2RegionOffset,
        extensionProbe.data(),
        extensionProbe.size()));
    for (const std::uint8_t value : extensionProbe) {
        TEST_ASSERT_EQUAL_UINT8(0xFFU, value);
    }

    services::SequencerPatternStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    SequencerPatternV2 pattern{};
    pattern.length = 31U;
    pattern.direction = SequencerPlayDirection::PingPong;
    TEST_ASSERT_TRUE(store.updatePattern(2U, 5U, pattern, 0U));
    TEST_ASSERT_TRUE(store.flush());

    std::array<std::uint8_t, hal::persistent_layout::kV1ImageBytes> migratedPrefix{};
    TEST_ASSERT_TRUE(storage.readBytes(0U, migratedPrefix.data(), migratedPrefix.size()));
    for (std::size_t index = 0U; index < oldImage.size(); ++index) {
        TEST_ASSERT_EQUAL_UINT8(oldImage[index], migratedPrefix[index]);
    }
}


void testPatternValidationAndDegenerateTraversalBranches() {
    SequencerPatternV2 pattern{};
    pattern.length = 0U;
    TEST_ASSERT_FALSE(isSequencerPatternValid(pattern));
    clampSequencerPattern(pattern);
    TEST_ASSERT_EQUAL_UINT64(0ULL, pattern.gates[0]);
    TEST_ASSERT_EQUAL_UINT64(0ULL, pattern.gates[1]);
    TEST_ASSERT_EQUAL_UINT8(0U, pattern.rotation);
    TEST_ASSERT_FALSE(resolveSequencerTraversal(0U, pattern, 0U).active);
    TEST_ASSERT_EQUAL_UINT8(0U, rotateSequencerStep(0U, pattern));

    pattern = SequencerPatternV2{};
    pattern.length = 129U;
    TEST_ASSERT_FALSE(isSequencerPatternValid(pattern));
    pattern.length = 4U;
    pattern.rotation = 4U;
    TEST_ASSERT_FALSE(isSequencerPatternValid(pattern));
    pattern.rotation = 0U;
    pattern.direction = static_cast<SequencerPlayDirection>(99U);
    TEST_ASSERT_FALSE(isSequencerPatternValid(pattern));
    pattern.direction = SequencerPlayDirection::Forward;
    pattern.loopMode = static_cast<SequencerLoopMode>(99U);
    TEST_ASSERT_FALSE(isSequencerPatternValid(pattern));

    pattern = SequencerPatternV2{};
    pattern.length = 1U;
    pattern.direction = SequencerPlayDirection::PingPong;
    TEST_ASSERT_EQUAL_UINT32(1U, sequencerTraversalCycleLength(pattern));
    TEST_ASSERT_EQUAL_UINT8(0U, resolveSequencerTraversal(17U, pattern, 0U).step);

    pattern.length = 64U;
    pattern.gates = {{UINT64_MAX, UINT64_MAX}};
    clampSequencerPattern(pattern);
    TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, pattern.gates[0]);
    TEST_ASSERT_EQUAL_UINT64(0ULL, pattern.gates[1]);

    pattern.length = 128U;
    pattern.gates = {{UINT64_MAX, UINT64_MAX}};
    clampSequencerPattern(pattern);
    TEST_ASSERT_EQUAL_UINT64(UINT64_MAX, pattern.gates[1]);
}

void testStoreRejectsInvalidRequestsAndDefersWhenWritesAreBlocked() {
    hal::PersistentStorage storage;
    services::SequencerPatternStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    TEST_ASSERT_TRUE(store.flush());

    TEST_ASSERT_EQUAL_UINT8(0U, store.activeSlot(kChannelCount));
    TEST_ASSERT_FALSE(store.setActiveSlot(kChannelCount, 0U, 1U));
    TEST_ASSERT_FALSE(store.setActiveSlot(0U, kSequencerPatternSlotsPerChannel, 1U));
    TEST_ASSERT_TRUE(store.setActiveSlot(0U, 0U, 1U));
    TEST_ASSERT_TRUE(store.setActiveSlot(0U, 1U, 2U));
    TEST_ASSERT_TRUE(store.dirty());
    TEST_ASSERT_TRUE(store.setActiveSlot(0U, 2U, 3U));
    TEST_ASSERT_EQUAL_UINT8(2U, store.activeSlot(0U));
    TEST_ASSERT_TRUE(store.flush());

    SequencerPatternV2 invalid{};
    invalid.length = 0U;
    TEST_ASSERT_FALSE(store.updatePattern(0U, 0U, invalid, 0U));
    TEST_ASSERT_FALSE(store.updatePattern(kChannelCount, 0U, SequencerPatternV2{}, 0U));
    TEST_ASSERT_FALSE(store.updatePattern(0U, kSequencerPatternSlotsPerChannel, SequencerPatternV2{}, 0U));
    TEST_ASSERT_FALSE(store.setGate(kChannelCount, 0U, 0U, true, 0U));
    TEST_ASSERT_FALSE(store.setGate(0U, kSequencerPatternSlotsPerChannel, 0U, true, 0U));
    TEST_ASSERT_FALSE(store.setGate(0U, 0U, 128U, true, 0U));

    TEST_ASSERT_EQUAL_UINT8(16U, store.pattern(kChannelCount, 0U).length);
    TEST_ASSERT_EQUAL_UINT8(16U, store.pattern(0U, kSequencerPatternSlotsPerChannel).length);

    TEST_ASSERT_TRUE(store.setGate(0U, 0U, 127U, true, 10U));
    TEST_ASSERT_TRUE(store.dirty());
    TEST_ASSERT_TRUE(store.service(10U + config::kPersistenceCommitDelayMs + 1U, false));
    TEST_ASSERT_TRUE(store.dirty());
    TEST_ASSERT_TRUE(store.service(10U, true));
    TEST_ASSERT_TRUE(store.dirty());
    TEST_ASSERT_TRUE(store.service(10U + config::kPersistenceCommitDelayMs, true));
    TEST_ASSERT_FALSE(store.dirty());
}

void testStoreReadFailureAndCorruptHeadersFallBackToDefaults() {
    hal::PersistentStorage storage;
    services::SequencerPatternStore failedRead(storage);
    hal::PersistentStorage::failNextReadForTest();
    TEST_ASSERT_FALSE(failedRead.begin());
    TEST_ASSERT_FALSE(failedRead.hasDurableBank());

    services::SequencerPatternStore writer(storage);
    TEST_ASSERT_TRUE(writer.begin());
    SequencerPatternV2 pattern{};
    pattern.length = 23U;
    TEST_ASSERT_TRUE(writer.updatePattern(0U, 0U, pattern, 0U));
    TEST_ASSERT_TRUE(writer.flush());

    std::array<std::uint8_t, services::SequencerPatternStore::kStorageBytes> bank{};
    TEST_ASSERT_TRUE(storage.readBytes(services::SequencerPatternStore::kStorageOffset, bank.data(), bank.size()));
    const std::array<std::size_t, 6U> corruptOffsets{{0U, 4U, 5U, 6U, 7U, 8U}};
    for (const std::size_t offset : corruptOffsets) {
        auto corrupted = bank;
        corrupted[offset] ^= 0x01U;
        TEST_ASSERT_TRUE(storage.writeBytes(services::SequencerPatternStore::kStorageOffset, corrupted.data(), corrupted.size()));
        services::SequencerPatternStore reader(storage);
        TEST_ASSERT_TRUE(reader.begin());
        TEST_ASSERT_FALSE(reader.hasDurableBank());
        TEST_ASSERT_EQUAL_UINT8(16U, reader.pattern(0U, 0U).length);
        TEST_ASSERT_TRUE(storage.writeBytes(services::SequencerPatternStore::kStorageOffset, bank.data(), bank.size()));
    }
}

void testStoreRejectsInvalidPatternRecordWithValidBankCrc() {
    hal::PersistentStorage storage;
    services::SequencerPatternStore writer(storage);
    TEST_ASSERT_TRUE(writer.begin());
    TEST_ASSERT_TRUE(writer.updatePattern(0U, 0U, SequencerPatternV2{}, 0U));
    TEST_ASSERT_TRUE(writer.flush());

    std::array<std::uint8_t, services::SequencerPatternStore::kStorageBytes> bank{};
    TEST_ASSERT_TRUE(storage.readBytes(services::SequencerPatternStore::kStorageOffset, bank.data(), bank.size()));
    bank[services::SequencerPatternStore::kHeaderBytes] = 0U;
    const std::uint32_t crc = testCrc32(
        bank.data() + services::SequencerPatternStore::kHeaderBytes,
        bank.size() - services::SequencerPatternStore::kHeaderBytes);
    writeTest32(bank.data() + 8U, crc);
    TEST_ASSERT_TRUE(storage.writeBytes(services::SequencerPatternStore::kStorageOffset, bank.data(), bank.size()));

    services::SequencerPatternStore reader(storage);
    TEST_ASSERT_TRUE(reader.begin());
    TEST_ASSERT_FALSE(reader.hasDurableBank());
    TEST_ASSERT_EQUAL_UINT8(16U, reader.pattern(0U, 0U).length);
}

void testFailedCommitKeepsDirtyBankAndPreviousGenerationReadable() {
    hal::PersistentStorage storage;
    services::SequencerPatternStore store(storage);
    TEST_ASSERT_TRUE(store.begin());
    SequencerPatternV2 first{};
    first.length = 17U;
    TEST_ASSERT_TRUE(store.updatePattern(0U, 0U, first, 0U));
    TEST_ASSERT_TRUE(store.flush());

    SequencerPatternV2 second = first;
    second.length = 33U;
    TEST_ASSERT_TRUE(store.updatePattern(0U, 0U, second, 100U));
    hal::PersistentStorage::powerLossBeforeCommitForTest();
    TEST_ASSERT_FALSE(store.flush());
    TEST_ASSERT_TRUE(store.dirty());

    services::SequencerPatternStore recovered(storage);
    TEST_ASSERT_TRUE(recovered.begin());
    TEST_ASSERT_TRUE(recovered.hasDurableBank());
    TEST_ASSERT_EQUAL_UINT8(17U, recovered.pattern(0U, 0U).length);
}

}  // namespace

int main(int, char**) {
    UNITY_BEGIN();
    RUN_TEST(testDefaultPatternAnd128StepBoundary);
    RUN_TEST(testPatternValidationAndDegenerateTraversalBranches);
    RUN_TEST(testClampPatternClearsBitsOutsideLength);
    RUN_TEST(testForwardReverseAndPingPongTraversal);
    RUN_TEST(testOnceStopsAfterOneTraversalCycle);
    RUN_TEST(testRandomTraversalIsDeterministicHistoryFreeAndBounded);
    RUN_TEST(testRotationIsAppliedAfterTraversal);
    RUN_TEST(testStepMetadataPackingProfilesAndInvalidWords);
    RUN_TEST(testSparseStepStoreRoundTripRemovalAndPatternCopy);
    RUN_TEST(testSparseStepStoreCapacityValidationAndReservedSongRegion);
    RUN_TEST(testSparseStepStoreDelayedCommitAndReadWriteFailures);
    RUN_TEST(testLegacyChannelSequencesSeedPatternOneOnly);
    RUN_TEST(testPersistentBankRoundTripAndDelayedCommit);
    RUN_TEST(testStoreRejectsInvalidRequestsAndDefersWhenWritesAreBlocked);
    RUN_TEST(testStoreReadFailureAndCorruptHeadersFallBackToDefaults);
    RUN_TEST(testStoreRejectsInvalidPatternRecordWithValidBankCrc);
    RUN_TEST(testV1EightKiBImageMigratesWithoutTouchingHistoricalBytes);
    RUN_TEST(testFailedCommitKeepsDirtyBankAndPreviousGenerationReadable);
    return UNITY_END();
}
