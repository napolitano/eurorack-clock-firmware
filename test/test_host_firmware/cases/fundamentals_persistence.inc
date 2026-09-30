/**
 * @file fundamentals_persistence.inc
 * @brief Included test cases split from test_host_firmware/test_main.cpp for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

void testLocalizationAndFont() {
    for (std::size_t index = 0U; index < text::kTextCount; ++index) {
        const auto id = static_cast<text::TextId>(index);
        CHECK(text::get(id, config::UiLanguage::EnglishUs) != nullptr);
        CHECK(text::get(id, config::UiLanguage::GermanDe) != nullptr);
    }
    CHECK(&text::catalogFor(static_cast<config::UiLanguage>(99)) == &text::kEnglishUs);
    CHECK(std::strcmp(channelModeShortLabel(ChannelMode::Clock), "C") == 0);
    CHECK(std::strcmp(channelModeShortLabel(ChannelMode::Euclid), "E") == 0);
    CHECK(std::strcmp(channelModeShortLabel(ChannelMode::Sequencer), "S") == 0);
    CHECK(std::strcmp(channelModeShortLabel(ChannelMode::Off), "O") == 0);
    CHECK(std::strcmp(channelModeShortLabel(static_cast<ChannelMode>(99)), "C") == 0);
    CHECK(std::strcmp(channelModeLongLabel(ChannelMode::Clock), "CLOCK") == 0);
    CHECK(std::strcmp(channelModeLongLabel(ChannelMode::Euclid), "EUCLID") == 0);
    CHECK(std::strcmp(channelModeLongLabel(ChannelMode::Sequencer), "SEQUENCER") == 0);
    CHECK(std::strcmp(channelModeLongLabel(ChannelMode::Off), "OFF") == 0);
    CHECK(std::strcmp(clockSourceLabel(ClockSource::Internal), "INT") == 0);
    CHECK(std::strcmp(clockSourceLabel(ClockSource::External), "EXT") == 0);
    CHECK(std::strcmp(clockSourceLabel(ClockSource::Auto), "AUTO") == 0);
    CHECK(std::strcmp(clockSourceLabel(static_cast<ClockSource>(99)), "INT") == 0);

    for (char c = 'A'; c <= 'Z'; ++c) CHECK(hal::font::glyphFor(c) != hal::font::kBlank);
    for (char c = '0'; c <= '9'; ++c) CHECK(hal::font::glyphFor(c) != hal::font::kBlank);
    CHECK(hal::font::glyphFor('a') == hal::font::glyphFor('A'));
    for (char c : std::string("/:%-+><.=_|*!?")) CHECK(hal::font::glyphFor(c) != hal::font::kBlank);
    CHECK(hal::font::glyphFor(' ') == hal::font::kBlank);
    CHECK(hal::font::glyphFor('@') == hal::font::kBlank);

    // The tempo font is a dedicated native-resolution numeral set, not scaled 5x7 text.
    for (char digit = '0'; digit <= '9'; ++digit) {
        const hal::font::TempoGlyph glyph = hal::font::tempoGlyphFor(digit);
        CHECK(glyph.width > 0U);
        CHECK(glyph.width <= hal::font::kTempoGlyphMaximumWidth);
        CHECK(std::any_of(glyph.rows.begin(), glyph.rows.end(), [](std::uint16_t row) { return row != 0U; }));
    }
    CHECK_EQ(hal::font::tempoGlyphFor('X').width, 0U);
}

void testDefaultsTemplatesAndServices() {
    ClockState state{};
    initializeFactoryDefaults(state);
    CHECK_EQ(state.bpm, defaults::kMasterBpm);
    CHECK_EQ(state.operatingMode, OperatingMode::UnifiedClock);
    CHECK_EQ(defaults::kClockSource, ClockSource::Auto);
    CHECK_EQ(state.source, ClockSource::Auto);
    CHECK_EQ(state.tempoRange.minimumBpm, 20U);
    CHECK_EQ(state.tempoRange.maximumBpm, 999U);
    CHECK_EQ(state.unifiedClock.humanizeUs, 0U);
    CHECK_EQ(state.masterMeter.beats, defaults::kMasterMeter.beats);
    CHECK_EQ(state.preCountSteps, 0U);
    CHECK_EQ(state.channels[0].common.mode, ChannelMode::Clock);
    CHECK_EQ(state.channels[0].common.gateLengthMs, 10U);
    CHECK_EQ(state.unifiedClock.gateLengthMs, 10U);
    CHECK_EQ(state.dividerBank.gateLengthMs, 10U);
    CHECK_EQ(kGateLengthOptionsMs, (std::array<std::uint16_t, 7U>{1U, 2U, 5U, 10U, 20U, 50U, 100U}));
    CHECK_EQ(state.channels[0].sequencer.pattern, defaults::kSequencerPatternA);
    CHECK_EQ(state.channels[1].sequencer.pattern, defaults::kSequencerPatternB);
    CHECK_EQ(state.channels[7].euclid.hits, static_cast<std::uint8_t>(defaults::kEuclidBaseHits + 3U));
    CHECK_EQ(state.display.screensaverMode, ScreensaverMode::Clock);
    CHECK_EQ(state.display.screensaverAfterMinutes, 2U);
    CHECK_EQ(state.display.dimAfterMinutes, 5U);
    CHECK_EQ(state.display.offAfterMinutes, 10U);

    CHECK(!state.device.encoderDirectionReversed);
    CHECK(!state.device.displayRotated180);
    CHECK_EQ(state.inputs.input1, InputFunction::Sync);
    CHECK_EQ(state.inputs.input2, InputFunction::Reset);

    for (std::size_t index = 0U; index < services::TemplateService::kTemplateCount; ++index) {
        ClockState templated{};
        CHECK(services::TemplateService::apply(index, templated));
        CHECK(std::strlen(services::TemplateService::name(index)) > 0U);
        CHECK_EQ(
            templated.operatingMode,
            index == 0U ? OperatingMode::UnifiedClock : OperatingMode::Independent);
        if (index == 1U) CHECK_EQ(templated.channels[1].common.rate.factor, 2U);
        if (index == 2U) CHECK_EQ(templated.channels[7].common.rate.factor, 16U);
        if (index == 3U) CHECK_EQ(templated.channels[6].common.rate.numerator, 5U);
        if (index == 4U) CHECK_EQ(templated.channels[0].common.mode, ChannelMode::Euclid);
        if (index == 5U) CHECK_EQ(templated.channels[7].common.mode, ChannelMode::Sequencer);
    }
    ClockState playingTemplate = makeDefaultState();
    playingTemplate.transport = TransportState::Playing;
    playingTemplate.device = {true, true};
    CHECK(services::TemplateService::apply(1U, playingTemplate));
    CHECK_EQ(playingTemplate.transport, TransportState::Playing);
    CHECK(playingTemplate.device.encoderDirectionReversed);
    CHECK(playingTemplate.device.displayRotated180);

    CHECK(!services::TemplateService::apply(99U, state));
    CHECK(std::strcmp(services::TemplateService::name(99U), "OFF") == 0);

    services::TapTempo tap;
    tap.reset();
    CHECK_EQ(tap.registerTap(0U, 20U, 300U), 0U);
    CHECK_EQ(tap.registerTap(500U, 20U, 300U), 120U); // timestamp zero is a valid first tap
    tap.reset();
    CHECK_EQ(tap.registerTap(1000U, 300U, 20U), 0U); // invalid clamp range cannot reach std::clamp UB
    CHECK_EQ(tap.registerTap(1000U, 20U, 300U), 0U);
    CHECK_EQ(tap.registerTap(1500U, 20U, 300U), 120U);
    CHECK_EQ(tap.registerTap(2000U, 20U, 300U), 120U);
    CHECK_EQ(tap.registerTap(2500U, 20U, 300U), 120U);
    CHECK_EQ(tap.registerTap(3000U, 20U, 300U), 120U);
    CHECK_EQ(tap.registerTap(3500U, 20U, 300U), 120U); // rolls the four-sample window
    CHECK_EQ(tap.registerTap(3550U, 20U, 300U), 0U);   // 50 ms: below supported ceiling
    tap.reset();
    CHECK_EQ(tap.registerTap(1000U, 1U, 300U), 0U);
    CHECK_EQ(tap.registerTap(61000U, 1U, 300U), 1U);   // 60 s interval = 1 BPM
    CHECK_EQ(tap.registerTap(130000U, 1U, 300U), 0U); // sequence timeout/reset
    tap.reset();
    CHECK_EQ(tap.registerTap(130000U, 130U, 140U), 0U);
    CHECK_EQ(tap.registerTap(130461U, 130U, 140U), 130U); // slowest interval still permitted by min BPM
    tap.reset();
    CHECK_EQ(tap.registerTap(130500U, 20U, 100U), 0U);
    CHECK_EQ(tap.registerTap(131000U, 20U, 100U), 100U);  // user maximum still clamps fast taps
    tap.reset();
    CHECK_EQ(tap.registerTap(1000U, 20U, 999U), 0U);
    CHECK_EQ(tap.registerTap(1060U, 20U, 999U), 999U);    // 60 ms ~= 1000 BPM, user max clamps to 999
}


void testPersistentLayoutV1ForwardCompatibilityContract() {
    CHECK_EQ(hal::persistent_layout::kV1ImageBytes, 8192U);
    CHECK_EQ(hal::persistent_layout::kMaximumImageBytes, 12288U);
    CHECK_EQ(hal::persistent_layout::kLegacyScoreRegionOffset, 3072U);
    CHECK_EQ(hal::persistent_layout::kLeaderboardRegionOffset, 4096U);
    CHECK_EQ(services::PersistentStateService::kCurrentStatePayloadBytes, 294U);
    CHECK_EQ(services::PersistentStateService::kCurrentRecordBytes, 306U);
    CHECK_EQ(services::PersistentStateService::kPresetRecordBytes, 322U);
    CHECK_EQ(services::PersistentStateService::kSettingsPresetFootprintBytes, 2882U);
    CHECK_EQ(services::PersistentStateService::kBytesBeforeLegacyScoreRegion, 190U);
    CHECK_EQ(services::PersistentStateService::kMaximumInPlaceStatePayloadGrowthBytes, 21U);
    CHECK_EQ(services::CustomGrooveStore::kStorageOffset, 3136U);
    CHECK_EQ(services::CustomGrooveStore::kStorageOffset +
        static_cast<std::size_t>(kCustomGrooveSlotCount) * services::CustomGrooveStore::kRecordBytes,
        hal::persistent_layout::kLeaderboardRegionOffset);
}


void testCustomGrooveStorePreservesRegionsAndRejectsCorruption() {
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::CustomGrooveStore store(storage);
    const std::uint8_t legacySentinel = 0x4CU;
    const std::uint8_t leaderboardSentinel = 0xA7U;
    CHECK(storage.writeBytes(services::CustomGrooveStore::kStorageOffset - 1U, &legacySentinel, 1U));
    CHECK(storage.writeBytes(hal::persistent_layout::kLeaderboardRegionOffset, &leaderboardSentinel, 1U));

    CustomGroovePattern source{};
    source.length = 64U;
    for (std::size_t index = 0U; index < source.length; ++index) {
        source.offsets256[index] = static_cast<std::int8_t>(
            static_cast<int>(index % 9U) * 20 - 80);
    }
    CHECK(store.save(0U, "DEEP POCKET", source));
    CHECK(store.save(kCustomGrooveSlotCount - 1U, "LAST SLOT", source));
    CHECK(store.service(true));
    CHECK(store.exists(0U));
    CHECK(!store.exists(kCustomGrooveSlotCount));

    CustomGroovePattern restored{};
    char name[services::CustomGrooveStore::kNameLength + 1U]{};
    CHECK(store.load(0U, restored, name, sizeof(name)));
    CHECK_EQ(restored.length, 64U);
    CHECK(std::strcmp(name, "DEEP POCKET") == 0);
    for (std::size_t index = 0U; index < source.length; ++index) {
        CHECK_EQ(restored.offsets256[index], source.offsets256[index]);
    }

    std::uint8_t sentinel = 0U;
    CHECK(storage.readBytes(services::CustomGrooveStore::kStorageOffset - 1U, &sentinel, 1U));
    CHECK_EQ(sentinel, legacySentinel);
    CHECK(storage.readBytes(hal::persistent_layout::kLeaderboardRegionOffset, &sentinel, 1U));
    CHECK_EQ(sentinel, leaderboardSentinel);

    std::array<std::uint8_t, services::CustomGrooveStore::kRecordBytes> raw{};
    CHECK(storage.readBytes(services::CustomGrooveStore::kStorageOffset, raw.data(), raw.size()));
    raw[32] ^= 0x01U;
    CHECK(storage.writeBytes(services::CustomGrooveStore::kStorageOffset, raw.data(), raw.size()));
    CHECK(!store.load(0U, restored));

    CHECK(!store.load(kCustomGrooveSlotCount, restored));
    CHECK(!store.save(kCustomGrooveSlotCount, "NOPE", source));
    CHECK(!store.clear(kCustomGrooveSlotCount));
    CustomGroovePattern invalid{};
    invalid.length = 0U;
    CHECK(!store.save(1U, "BAD", invalid));

    CustomGroovePattern shortPattern{};
    shortPattern.length = 4U;
    shortPattern.offsets256[1] = 32;
    CHECK(store.save(1U, nullptr, shortPattern));
    CHECK(store.service(true));
    char tinyName[3]{'X','X','\0'};
    CHECK(store.load(1U, restored, tinyName, sizeof(tinyName)));
    CHECK_EQ(tinyName[2], '\0');
    store.name(1U, nullptr, 0U);
    char emptyName[4]{'X','X','X','\0'};
    store.name(kCustomGrooveSlotCount, emptyName, sizeof(emptyName));
    CHECK_EQ(emptyName[0], '\0');

    CHECK(store.save(2U, "A\x01" "B", shortPattern));
    CHECK(store.service(true));
    char normalized[services::CustomGrooveStore::kNameLength + 1U]{};
    CHECK(store.load(2U, restored, normalized, sizeof(normalized)));
    CHECK(normalized[1] == ' ');

    CHECK(storage.readBytes(services::CustomGrooveStore::kStorageOffset + 2U * services::CustomGrooveStore::kRecordBytes,
                            raw.data(), raw.size()));
    raw[5] = 0U;
    writeTestUint32Le(raw.data() + raw.size() - 4U, testCrc32(raw.data(), raw.size() - 4U));
    CHECK(storage.writeBytes(services::CustomGrooveStore::kStorageOffset + 2U * services::CustomGrooveStore::kRecordBytes,
                             raw.data(), raw.size()));
    CHECK(!store.load(2U, restored));

    // Accepted mutations stay visible in RAM while the A/B commit is deferred.
    // A simulated power loss before COMMIT must still leave the previous durable
    // generation readable from a fresh store, and the pending mutation retryable.
    CHECK(store.save(3U, "OLD NAME", shortPattern));
    CHECK(store.service(true));
    CHECK(store.save(3U, "NEW NAME", shortPattern));
    hal::PersistentStorage::powerLossBeforeCommitForTest();
    CHECK(!store.service(true));
    char pendingName[services::CustomGrooveStore::kNameLength + 1U]{};
    store.name(3U, pendingName, sizeof(pendingName));
    CHECK(std::strcmp(pendingName, "NEW NAME") == 0);
    services::CustomGrooveStore durableAfterFailedRename(storage);
    char preservedName[services::CustomGrooveStore::kNameLength + 1U]{};
    durableAfterFailedRename.name(3U, preservedName, sizeof(preservedName));
    CHECK(std::strcmp(preservedName, "OLD NAME") == 0);
    CHECK(store.service(true));

    CHECK(store.clear(3U));
    hal::PersistentStorage::powerLossBeforeCommitForTest();
    CHECK(!store.service(true));
    services::CustomGrooveStore durableAfterFailedClear(storage);
    CHECK(durableAfterFailedClear.exists(3U));
    CHECK(store.service(true));
    CHECK(!store.exists(3U));

    CHECK(store.clear(1U));
    CHECK(!store.exists(1U));
}

void testCustomGrooveMutationsDeferPhysicalFlashCommit() {
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::CustomGrooveStore store(storage);

    CustomGroovePattern first{};
    first.length = 2U;
    first.offsets256[0] = -32;
    first.offsets256[1] = 32;
    CustomGroovePattern second = first;
    second.offsets256[0] = -64;
    second.offsets256[1] = 64;

    CHECK(store.save(0U, "FIRST", first));
    CHECK(store.save(1U, "SECOND", second));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);
    CHECK(store.service(false));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);

    // Pending data must remain immediately readable to the UI while transport plays.
    CustomGroovePattern pending{};
    char name[services::CustomGrooveStore::kNameLength + 1U]{};
    CHECK(store.load(1U, pending, name, sizeof(name)));
    CHECK_EQ(pending.offsets256[0], -64);
    CHECK(std::strcmp(name, "SECOND") == 0);

    CHECK(store.service(true));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    services::CustomGrooveStore reloaded(storage);
    CHECK(reloaded.exists(0U));
    CHECK(reloaded.exists(1U));
    CHECK(reloaded.load(0U, pending, name, sizeof(name)));
    CHECK(std::strcmp(name, "FIRST") == 0);

    CHECK(store.clear(0U));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);
    CHECK(store.service(false));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);
    CHECK(store.service(true));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 2U);
    services::CustomGrooveStore afterClear(storage);
    CHECK(!afterClear.exists(0U));
}

void testPersistentStorageTransactionalUpdate() {
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    const std::uint8_t first = 0x11U;
    const std::uint8_t second = 0x22U;
    std::uint8_t readback = 0U;

    // Transaction API must reject use outside an open update and malformed ranges.
    CHECK(!storage.stageBytes(0U, &first, 1U));
    CHECK(!storage.stageFill(0U, 1U, 0xFFU));
    CHECK(!storage.commitUpdate());

    hal::PersistentStorage::failNextReadForTest();
    CHECK(!storage.beginUpdate());

    CHECK(storage.beginUpdate());
    CHECK(!storage.stageBytes(0U, nullptr, 1U));
    CHECK(!storage.stageBytes(hal::PersistentStorage::kCapacityBytes, &first, 1U));
    CHECK(!storage.stageFill(hal::PersistentStorage::kCapacityBytes, 1U, 0x00U));

    // Staging erased bytes over erased storage is a no-op and must not wear Flash.
    CHECK(storage.stageFill(0U, 4U, 0xFFU));
    CHECK(storage.commitUpdate());
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);

    // A dirty transaction commits exactly once and preserves staged writes/fills together.
    CHECK(storage.beginUpdate());
    CHECK(storage.stageBytes(0U, &first, 1U));
    CHECK(storage.stageFill(1U, 2U, second));
    CHECK(storage.commitUpdate());
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);
    CHECK(storage.readBytes(0U, &readback, 1U));
    CHECK_EQ(readback, first);
    CHECK(storage.readBytes(2U, &readback, 1U));
    CHECK_EQ(readback, second);

    // Re-staging identical data remains clean; explicit cancellation discards staged changes.
    CHECK(storage.beginUpdate());
    CHECK(storage.stageBytes(0U, &first, 1U));
    CHECK(storage.stageFill(1U, 2U, second));
    CHECK(storage.commitUpdate());
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    CHECK(storage.beginUpdate());
    CHECK(storage.stageBytes(0U, &second, 1U));
    storage.cancelUpdate();
    CHECK(!storage.commitUpdate());
    CHECK(storage.readBytes(0U, &readback, 1U));
    CHECK_EQ(readback, first);

    // Failed physical commit closes the transaction and leaves the previous slot authoritative.
    CHECK(storage.beginUpdate());
    CHECK(storage.stageBytes(0U, &second, 1U));
    hal::PersistentStorage::failNextWriteForTest();
    CHECK(!storage.commitUpdate());
    CHECK(!storage.commitUpdate());
    CHECK(storage.readBytes(0U, &readback, 1U));
    CHECK_EQ(readback, first);
}


void testPersistentStorageAndStateService() {
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    std::uint8_t byte = 0U;
    const std::uint8_t value = 0x42U;

    CHECK(!storage.readBytes(0U, nullptr, 1U));
    CHECK(!storage.readBytes(hal::PersistentStorage::kCapacityBytes, &byte, 1U));
    CHECK(!storage.readBytes(hal::PersistentStorage::kCapacityBytes + 1U, &byte, 0U));
    CHECK(!storage.writeBytes(0U, nullptr, 1U));
    CHECK(!storage.writeBytes(hal::PersistentStorage::kCapacityBytes, &value, 1U));
    CHECK(!storage.writeBytes(hal::PersistentStorage::kCapacityBytes + 1U, &value, 0U));
    CHECK(storage.writeBytes(0U, &value, 1U));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);
    CHECK(storage.readBytes(0U, &byte, 1U));
    CHECK_EQ(byte, value);
    CHECK(storage.writeBytes(0U, &value, 1U));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U); // unchanged data: no wear
    CHECK_EQ(hal::PersistentStorage::kPhysicalSlotBytes, 16U * 1024U);
    CHECK_EQ(hal::PersistentStorage::kMaximumImageBytes, 12U * 1024U);

    // A/B commit safety: an interrupted write must leave the previous generation readable.
    const std::uint8_t newerValue = 0x43U;
    hal::PersistentStorage::powerLossBeforeCommitForTest();
    CHECK(!storage.writeBytes(0U, &newerValue, 1U));
    byte = 0U;
    CHECK(storage.readBytes(0U, &byte, 1U));
    CHECK_EQ(byte, value);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    // After a successful newer generation, corruption of that slot must fall back
    // to the previous independently committed sector.
    CHECK(storage.writeBytes(0U, &newerValue, 1U));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 2U);
    hal::PersistentStorage::corruptNewestPayloadByteForTest(0U);
    byte = 0U;
    CHECK(storage.readBytes(0U, &byte, 1U));
    CHECK_EQ(byte, value);

    // Exercise the mirror fallback as well: after two more alternating commits,
    // slot A is newest. Corrupting it must recover from valid slot B.
    const std::uint8_t thirdValue = 0x44U;
    const std::uint8_t fourthValue = 0x45U;
    CHECK(storage.writeBytes(0U, &thirdValue, 1U));
    CHECK(storage.writeBytes(0U, &fourthValue, 1U));
    hal::PersistentStorage::corruptNewestPayloadByteForTest(0U);
    byte = 0U;
    CHECK(storage.readBytes(0U, &byte, 1U));
    CHECK_EQ(byte, thirdValue);
    hal::PersistentStorage::corruptNewestPayloadByteForTest(hal::PersistentStorage::kCapacityBytes);
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage::corruptNewestPayloadByteForTest(0U); // no valid slot: no-op

    // Validator short-circuit branches must independently reject malformed slot metadata.
    // A single committed slot is used each time so rejection is observable as erased storage.
    const auto expectInvalidHeaderRejected = [&](const std::size_t wordIndex, const std::uint32_t replacement) {
        hal::PersistentStorage::resetForTest();
        CHECK(storage.writeBytes(0U, &value, 1U));
        hal::PersistentStorage::corruptNewestHeaderWordForTest(wordIndex, replacement);
        std::uint8_t rejected = 0U;
        CHECK(storage.readBytes(0U, &rejected, 1U));
        CHECK_EQ(rejected, 0xFFU);
    };
    expectInvalidHeaderRejected(1U, 2U); // format version
    expectInvalidHeaderRejected(3U, 0U); // payload size
    expectInvalidHeaderRejected(7U, 0U); // commit marker
    expectInvalidHeaderRejected(5U, 0U); // header CRC
    hal::PersistentStorage::corruptNewestHeaderWordForTest(8U, 0U); // bounds guard
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage::corruptNewestHeaderWordForTest(0U, 0U); // no active slot

    // Backward migration: a committed alpha.54-era 4-KiB image must remain readable,
    // bytes in the new upper half read erased, and the first new write must promote
    // the whole image to 8 KiB without losing the legacy payload.
    std::array<std::uint8_t, 4096U> legacyImage{};
    legacyImage.fill(0xFFU);
    legacyImage[123U] = 0x5AU;
    CHECK(hal::PersistentStorage::seedLegacyImageForTest(legacyImage.data(), legacyImage.size()));
    std::uint8_t legacyByte = 0U;
    CHECK(storage.readBytes(123U, &legacyByte, 1U));
    CHECK_EQ(legacyByte, 0x5AU);
    std::uint8_t newHalfByte = 0U;
    CHECK(storage.readBytes(5000U, &newHalfByte, 1U));
    CHECK_EQ(newHalfByte, 0xFFU);
    const std::uint8_t promotedValue = 0x6BU;
    CHECK(storage.writeBytes(5000U, &promotedValue, 1U));
    legacyByte = 0U;
    newHalfByte = 0U;
    CHECK(storage.readBytes(123U, &legacyByte, 1U));
    CHECK(storage.readBytes(5000U, &newHalfByte, 1U));
    CHECK_EQ(legacyByte, 0x5AU);
    CHECK_EQ(newHalfByte, promotedValue);

    services::PersistentStateService service(storage);
    hal::PersistentStorage::resetForTest();
    service.begin();
    CHECK(!service.hasStoredTransportState());
    CHECK_EQ(service.storedTransportState(), TransportState::Stopped);

    hal::PersistentStorage::failNextReadForTest();
    service.begin();
    CHECK(!service.hasStoredTransportState());

    service.requestTransportState(TransportState::Playing, 100U);
    // Repeating the same still-pending value must keep the original coalescing window.
    service.requestTransportState(TransportState::Playing, 101U);
    service.service(100U + config::kPersistenceCommitDelayMs - 1U);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);
    hal::PersistentStorage::failNextWriteForTest();
    service.service(100U + config::kPersistenceCommitDelayMs);
    CHECK(!service.hasStoredTransportState());
    service.service(101U + config::kPersistenceCommitDelayMs);
    CHECK(service.hasStoredTransportState());
    CHECK_EQ(service.storedTransportState(), TransportState::Playing);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    // Requesting the already durable value must not schedule another Flash erase.
    service.requestTransportState(TransportState::Playing, 5000U);
    service.service(5000U + config::kPersistenceCommitDelayMs + 1U);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    // A pending change can be cancelled by returning to the already stored state.
    service.requestTransportState(TransportState::Paused, 6000U);
    service.requestTransportState(TransportState::Playing, 6001U);
    service.service(6001U + config::kPersistenceCommitDelayMs + 1U);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    // Changing the pending value restarts the stabilization window.
    service.requestTransportState(TransportState::Paused, 7000U);
    service.requestTransportState(TransportState::Stopped, 7100U);
    service.service(7000U + config::kPersistenceCommitDelayMs);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);
    service.service(7100U + config::kPersistenceCommitDelayMs);
    CHECK_EQ(service.storedTransportState(), TransportState::Stopped);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 2U);

    // Load the serialized record through a fresh service instance.
    services::PersistentStateService reloaded(storage);
    reloaded.begin();
    CHECK(reloaded.hasStoredTransportState());
    CHECK_EQ(reloaded.storedTransportState(), TransportState::Stopped);

    constexpr std::size_t kTransportRecordSize = 12U;
    std::array<std::uint8_t, kTransportRecordSize> validRecord{};
    CHECK(storage.readBytes(0U, validRecord.data(), validRecord.size()));

    auto expectCorruptRecordRejected = [&](std::size_t byteIndex, std::uint8_t replacement) {
        auto corruptRecord = validRecord;
        corruptRecord[byteIndex] = replacement;
        CHECK(storage.writeBytes(0U, corruptRecord.data(), corruptRecord.size()));
        services::PersistentStateService corrupted(storage);
        corrupted.begin();
        CHECK(!corrupted.hasStoredTransportState());
        CHECK_EQ(corrupted.storedTransportState(), TransportState::Stopped);
        CHECK(storage.writeBytes(0U, validRecord.data(), validRecord.size()));
    };

    expectCorruptRecordRejected(0U, static_cast<std::uint8_t>(validRecord[0] ^ 0x01U)); // magic
    expectCorruptRecordRejected(4U, 0x7FU); // schema
    expectCorruptRecordRejected(5U, 0xFFU); // transport enum
    expectCorruptRecordRejected(8U, static_cast<std::uint8_t>(validRecord[8] ^ 0x80U)); // CRC

    // The alpha.6 schema persists the complete musical configuration, not only transport.
    ClockState complete = makeDefaultState();
    complete.bpm = 137U;
    complete.masterMeter = {7U, 8U};
    complete.transport = TransportState::Playing;
    complete.source = ClockSource::Auto;
    complete.preCountSteps = 64U;
    complete.externalSync = {24U, SyncEdge::Falling, SyncLossMode::Internal, ExternalResetMode::Gate, SyncSmoothing::Full, 750U, 2200U};
    complete.display = {ScreensaverMode::Orbit, 3U, 7U, 12U};
    complete.device = {true, true};
    for (std::uint8_t channelIndex = 0U; channelIndex < kChannelCount; ++channelIndex) {
        ChannelConfig& channel = complete.channels[channelIndex];
        channel.common.mode = static_cast<ChannelMode>(channelIndex % 4U);
        channel.common.rate = {ClockRatioMode::Multiply, static_cast<std::uint8_t>(channelIndex + 1U), 3U, 2U};
        channel.common.swingPercent = static_cast<std::uint8_t>(channelIndex * 3U);
        channel.common.probabilityPercent = static_cast<std::uint8_t>(100U - channelIndex);
        channel.common.gateLengthMs = static_cast<std::uint16_t>(5U + channelIndex);
        channel.common.phasePercent = static_cast<std::uint8_t>(channelIndex * 7U);
        channel.common.resetMode = (channelIndex & 1U) != 0U ? ResetMode::Free : ResetMode::Global;
        channel.common.muted = channelIndex == 7U;
        channel.euclid = {static_cast<std::uint8_t>(13U + channelIndex), static_cast<std::uint8_t>(5U + channelIndex), 2U};
        channel.sequencer.length = static_cast<std::uint8_t>(17U + channelIndex);
        channel.sequencer.rotation = channelIndex;
        channel.sequencer.pattern = 0xA55A000000000001ULL ^ channelIndex;
    }
    service.requestTransportState(TransportState::Playing, 10000U);
    service.requestCurrentState(complete, 10000U);
    service.service(10000U + config::kPersistenceCommitDelayMs);

    services::PersistentStateService fullReload(storage);
    fullReload.begin();
    ClockState restored{};
    CHECK(fullReload.restoreCurrentState(restored));
    CHECK_EQ(restored.bpm, 137U);
    CHECK_EQ(restored.masterMeter.beats, 7U);
    CHECK_EQ(restored.source, ClockSource::Auto);
    CHECK_EQ(restored.preCountSteps, 64U);
    CHECK_EQ(restored.externalSync.pulsesPerQuarterNote, 24U);
    CHECK_EQ(restored.display.screensaverMode, ScreensaverMode::Orbit);
    CHECK_EQ(restored.display.screensaverAfterMinutes, 3U);
    CHECK_EQ(restored.display.dimAfterMinutes, 7U);
    CHECK_EQ(restored.display.offAfterMinutes, 12U);
    CHECK(restored.device.encoderDirectionReversed);
    CHECK(restored.device.displayRotated180);
    CHECK_EQ(restored.channels[6].common.mode, ChannelMode::Sequencer);
    CHECK_EQ(restored.channels[5].euclid.steps, 18U);
    CHECK_EQ(restored.channels[7].sequencer.length, 24U);
    CHECK_EQ(restored.channels[7].sequencer.pattern, 0xA55A000000000006ULL);

    ClockState invalidDisplay = complete;
    invalidDisplay.display.screensaverMode = static_cast<ScreensaverMode>(0U);
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));
    invalidDisplay = complete;
    invalidDisplay.display.screensaverAfterMinutes = 0U;
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));
    invalidDisplay = complete;
    invalidDisplay.display.screensaverAfterMinutes = 8U;
    invalidDisplay.display.dimAfterMinutes = 7U;
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));
    ClockState invalidPreCount = complete;
    invalidPreCount.preCountSteps = 65U;
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidPreCount));
    ClockState invalidInputs = complete;
    invalidInputs.inputs = {InputFunction::Sync, InputFunction::Sync};
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidInputs));
    invalidInputs.inputs = {InputFunction::Fill, InputFunction::Reset};
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidInputs));
    invalidInputs.inputs = {InputFunction::Off, InputFunction::Off};
    CHECK(fullReload.savePreset(7U, "OFF-OFF", invalidInputs));
    CHECK(fullReload.clearPreset(7U));
    invalidDisplay = complete;
    invalidDisplay.display.dimAfterMinutes = 13U;
    invalidDisplay.display.offAfterMinutes = 12U;
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));
    invalidDisplay = complete;
    invalidDisplay.display.offAfterMinutes = static_cast<std::uint8_t>(config::kMaximumScreensaverMinutes + 1U);
    CHECK(!fullReload.savePreset(7U, "INVALID", invalidDisplay));

    // Public preset API boundaries and null-output guards.
    CHECK(!fullReload.savePreset(services::PersistentStateService::kUserPresetSlotCount, "OOB", complete));
    CHECK(!fullReload.savePreset(1U, nullptr, complete));
    ClockState boundaryLoad = makeDefaultState();
    CHECK(!fullReload.loadPreset(services::PersistentStateService::kUserPresetSlotCount, boundaryLoad));
    CHECK(!fullReload.renamePreset(services::PersistentStateService::kUserPresetSlotCount, "OOB"));
    CHECK(!fullReload.renamePreset(1U, "EMPTY"));
    CHECK(!fullReload.clearPreset(services::PersistentStateService::kUserPresetSlotCount));
    CHECK(!fullReload.presetExists(services::PersistentStateService::kUserPresetSlotCount));
    fullReload.presetName(0U, nullptr, 17U);
    char zeroSizeName[1]{'X'};
    fullReload.presetName(0U, zeroSizeName, 0U);
    CHECK_EQ(zeroSizeName[0], 'X');

    CHECK(fullReload.savePreset(0U, "groove_1", complete));
    CHECK(fullReload.presetExists(0U));
    char presetName[17]{}; fullReload.presetName(0U, presetName, sizeof(presetName));
    CHECK(std::strcmp(presetName, "GROOVE_1") == 0);
    ClockState loadedPreset = makeDefaultState(); loadedPreset.transport = TransportState::Paused;
    loadedPreset.device = {false, false};
    CHECK(fullReload.loadPreset(0U, loadedPreset));
    CHECK_EQ(loadedPreset.bpm, 137U);
    CHECK_EQ(loadedPreset.transport, TransportState::Paused);
    CHECK_EQ(loadedPreset.channels[7].sequencer.pattern, complete.channels[7].sequencer.pattern);
    CHECK(!loadedPreset.device.encoderDirectionReversed);
    CHECK(!loadedPreset.device.displayRotated180);
    CHECK(!fullReload.renamePreset(0U, nullptr));
    CHECK(fullReload.renamePreset(0U, "ALT-01"));
    fullReload.presetName(0U, presetName, sizeof(presetName));
    CHECK(std::strcmp(presetName, "ALT-01") == 0);
    CHECK(fullReload.clearPreset(0U));
    CHECK(!fullReload.presetExists(0U));
    CHECK(!fullReload.loadPreset(0U, loadedPreset));

    // Pending and durable preset-name paths both trim right-padding. The durable
    // record also gives us a target for read-failure and corrupt-record tests.
    CHECK(fullReload.savePreset(2U, "PAD   ", complete));
    fullReload.presetName(2U, presetName, sizeof(presetName));
    CHECK(std::strcmp(presetName, "PAD") == 0);
    fullReload.service(30000U, true);
    fullReload.presetName(2U, presetName, sizeof(presetName));
    CHECK(std::strcmp(presetName, "PAD") == 0);
    hal::PersistentStorage::failNextReadForTest();
    CHECK(!fullReload.loadPreset(2U, loadedPreset));
    hal::PersistentStorage::failNextReadForTest();
    CHECK(!fullReload.renamePreset(2U, "READFAIL"));

    constexpr std::size_t kCurrentRecordSizeForTest = services::PersistentStateService::kCurrentRecordBytes;
    constexpr std::size_t kPresetRecordSizeForTest = services::PersistentStateService::kPresetRecordBytes;
    constexpr std::size_t kPreset2OffsetForTest =
        kCurrentRecordSizeForTest + 2U * kPresetRecordSizeForTest;
    std::uint8_t presetMagicByte = 0U;
    CHECK(storage.readBytes(kPreset2OffsetForTest, &presetMagicByte, 1U));
    const std::uint8_t corruptPresetMagic = static_cast<std::uint8_t>(presetMagicByte ^ 0x01U);
    CHECK(storage.writeBytes(kPreset2OffsetForTest, &corruptPresetMagic, 1U));
    CHECK(!fullReload.loadPreset(2U, loadedPreset));
    CHECK(!fullReload.renamePreset(2U, "CORRUPT"));
    CHECK(storage.writeBytes(kPreset2OffsetForTest, &presetMagicByte, 1U));

    // STM32F4 Flash erase/program stalls instruction fetches. Pending CURRENT and
    // preset writes therefore remain RAM-only while PLAY is active and flush only
    // after the application explicitly permits a timing-safe commit.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage deferredStorage;
    services::PersistentStateService deferred(deferredStorage);
    deferred.begin();
    ClockState deferredState = makeDefaultState();
    deferredState.bpm = 149U;
    deferred.requestCurrentState(deferredState, 20000U);
    CHECK(deferred.savePreset(1U, "LIVE", deferredState));
    CHECK(deferred.presetExists(1U));
    ClockState stagedLoad = makeDefaultState();
    CHECK(deferred.loadPreset(1U, stagedLoad));
    CHECK_EQ(stagedLoad.bpm, 149U);
    hal::PersistentStorage::failNextReadForTest();
    deferred.service(20000U + config::kPersistenceCommitDelayMs + 1U, true);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);
    deferred.service(20000U + config::kPersistenceCommitDelayMs + 1U, false);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);
    deferred.service(20000U + config::kPersistenceCommitDelayMs + 1U, true);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);
    services::PersistentStateService deferredReload(deferredStorage);
    deferredReload.begin();
    CHECK(deferredReload.presetExists(1U));
    ClockState durableLoad = makeDefaultState();
    CHECK(deferredReload.loadPreset(1U, durableLoad));
    CHECK_EQ(durableLoad.bpm, 149U);
}


void testMultipleDeferredPresetMutationsSurviveUntilCommit() {
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage storage;
    services::PersistentStateService service(storage);
    service.begin();

    ClockState first = makeDefaultState();
    first.bpm = 121U;
    ClockState second = makeDefaultState();
    second.bpm = 137U;

    CHECK(service.savePreset(0U, "FIRST", first));
    service.service(1000U, false);
    CHECK(service.savePreset(1U, "SECOND", second));
    CHECK(service.clearPreset(2U));
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 0U);

    ClockState pending = makeDefaultState();
    CHECK(service.loadPreset(0U, pending));
    CHECK_EQ(pending.bpm, 121U);
    CHECK(service.loadPreset(1U, pending));
    CHECK_EQ(pending.bpm, 137U);

    service.service(1001U, true);
    CHECK_EQ(hal::PersistentStorage::writeCommitCountForTest(), 1U);

    services::PersistentStateService reload(storage);
    reload.begin();
    CHECK(reload.presetExists(0U));
    CHECK(reload.presetExists(1U));
    CHECK(!reload.presetExists(2U));
    char name[services::PersistentStateService::kPresetNameLength + 1U]{};
    reload.presetName(0U, name, sizeof(name));
    CHECK(std::strcmp(name, "FIRST") == 0);
    reload.presetName(1U, name, sizeof(name));
    CHECK(std::strcmp(name, "SECOND") == 0);
}

void testPersistentV3Migration() {
    constexpr std::size_t kV12CurrentSize = services::PersistentStateService::kCurrentRecordBytes;
    constexpr std::size_t kV12PresetSize = services::PersistentStateService::kPresetRecordBytes;
    constexpr std::size_t kV11CurrentSize = 297U;
    constexpr std::size_t kV11PresetSize = 313U;
    constexpr std::size_t kV10CurrentSize = 295U;
    constexpr std::size_t kV10PresetSize = 311U;
    constexpr std::size_t kV9CurrentSize = 268U;
    constexpr std::size_t kV9PresetSize = 284U;
    constexpr std::size_t kV8CurrentSize = 267U;
    constexpr std::size_t kV8PresetSize = 283U;
    constexpr std::size_t kV7CurrentSize = 266U;
    constexpr std::size_t kV7PresetSize = 282U;
    constexpr std::size_t kV6CurrentSize = 264U;
    constexpr std::size_t kV6PresetSize = 280U;
    constexpr std::size_t kV5CurrentSize = 263U;
    constexpr std::size_t kV5PresetSize = 279U;
    constexpr std::size_t kV4CurrentSize = 257U;
    constexpr std::size_t kV4PresetSize = 273U;
    constexpr std::size_t kV3CurrentSize = 253U;
    constexpr std::size_t kV3PresetSize = 269U;
    constexpr std::size_t kV10PresetOffset = kV10CurrentSize;
    constexpr std::size_t kV9PresetOffset = kV9CurrentSize;
    constexpr std::size_t kV8PresetOffset = kV8CurrentSize;
    constexpr std::size_t kV7PresetOffset = kV7CurrentSize;
    constexpr std::size_t kV6PresetOffset = kV6CurrentSize;
    constexpr std::size_t kV5PresetOffset = kV5CurrentSize;
    constexpr std::size_t kV4PresetOffset = kV4CurrentSize;
    constexpr std::size_t kV3PresetOffset = kV3CurrentSize;
    constexpr std::size_t kScoreOffset = 3072U;

    // Seed a v10 record using the production serializer, strip the appended
    // Stage-1 Groove fields to obtain v9, then derive each older schema in turn.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage seedStorage;
    services::PersistentStateService seed(seedStorage);
    seed.begin();
    ClockState legacyState = makeDefaultState();
    legacyState.bpm = 143U;
    legacyState.masterMeter = {7U, 8U};
    legacyState.transport = TransportState::Playing;
    legacyState.externalSync.resetMode = ExternalResetMode::Gate;
    legacyState.channels[2].common.mode = ChannelMode::Euclid;
    legacyState.channels[2].euclid = {13U, 5U, 3U};
    seed.requestTransportState(TransportState::Playing, 10U);
    seed.requestCurrentState(legacyState, 10U);
    seed.service(10U + config::kPersistenceCommitDelayMs);
    CHECK(seed.savePreset(0U, "MIGRATE", legacyState));
    seed.service(5000U);

    std::array<std::uint8_t, kV12CurrentSize> v12Current{};
    std::array<std::uint8_t, kV12PresetSize> v12Preset{};
    CHECK(seedStorage.readBytes(0U, v12Current.data(), v12Current.size()));
    CHECK(seedStorage.readBytes(kV12CurrentSize, v12Preset.data(), v12Preset.size()));
    const auto v11Current = makeLegacyV11Record<kV12CurrentSize, kV11CurrentSize, 8U>(v12Current);
    const auto v11Preset = makeLegacyV11Record<kV12PresetSize, kV11PresetSize, 24U>(v12Preset);
    const auto v10Current = makeLegacyV10Record<kV11CurrentSize, kV10CurrentSize, 8U>(v11Current);
    const auto v10Preset = makeLegacyV10Record<kV11PresetSize, kV10PresetSize, 24U>(v11Preset);
    const auto v9Current = makeLegacyV9Record<kV10CurrentSize, kV9CurrentSize, 8U>(v10Current);
    const auto v9Preset = makeLegacyV9Record<kV10PresetSize, kV9PresetSize, 24U>(v10Preset);
    const auto v8Current = makeLegacyV8Record<kV9CurrentSize, kV8CurrentSize, 8U>(v9Current);
    const auto v8Preset = makeLegacyV8Record<kV9PresetSize, kV8PresetSize, 24U>(v9Preset);
    const auto v7Current = makeLegacyV7Record<kV8CurrentSize, kV7CurrentSize, 8U>(v8Current);
    const auto v7Preset = makeLegacyV7Record<kV8PresetSize, kV7PresetSize, 24U>(v8Preset);
    const auto v6Current = makeLegacyV6Record<kV7CurrentSize, kV6CurrentSize, 8U>(v7Current);
    const auto v6Preset = makeLegacyV6Record<kV7PresetSize, kV6PresetSize, 24U>(v7Preset);
    const auto v5Current = makeLegacyV5Record<kV6CurrentSize, kV5CurrentSize, 8U>(v6Current);
    const auto v5Preset = makeLegacyV5Record<kV6PresetSize, kV5PresetSize, 24U>(v6Preset);
    const auto v4Current = makeLegacyV4Record<kV5CurrentSize, kV4CurrentSize, 8U>(v5Current);
    const auto v4Preset = makeLegacyV4Record<kV5PresetSize, kV4PresetSize, 24U>(v5Preset);
    const auto v3Current = makeLegacyV3Record<kV4CurrentSize, kV3CurrentSize, 8U>(v4Current);
    const auto v3Preset = makeLegacyV3Record<kV4PresetSize, kV3PresetSize, 24U>(v4Preset);

    const std::array<std::uint8_t, 16U> scoreMarker{
        0x52U, 0x41U, 0x49U, 0x44U, 1U, 2U, 3U, 4U,
        5U, 6U, 7U, 8U, 9U, 10U, 11U, 12U};

    // v11 -> v12: configurable inputs survive and the new Custom Groove slot
    // references default to slot 0 without changing the selected factory groove.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV11Storage;
    CHECK(legacyV11Storage.writeBytes(0U, v11Current.data(), v11Current.size()));
    CHECK(legacyV11Storage.writeBytes(kV11CurrentSize, v11Preset.data(), v11Preset.size()));
    services::PersistentStateService migratedV11(legacyV11Storage);
    migratedV11.begin();
    ClockState restoredV11 = makeDefaultState();
    CHECK(migratedV11.restoreCurrentState(restoredV11));
    CHECK_EQ(restoredV11.bpm, 143U);
    CHECK_EQ(restoredV11.inputs.input1, InputFunction::Sync);
    CHECK_EQ(restoredV11.inputs.input2, InputFunction::Reset);
    CHECK_EQ(restoredV11.unifiedClock.groove.customSlot, 0U);
    CHECK_EQ(restoredV11.channels[2].common.groove.customSlot, 0U);
    CHECK(migratedV11.presetExists(0U));
    std::array<std::uint8_t, kV12CurrentSize> migratedV11Current{};
    std::array<std::uint8_t, kV12PresetSize> migratedV11Preset{};
    CHECK(legacyV11Storage.readBytes(0U, migratedV11Current.data(), migratedV11Current.size()));
    CHECK(legacyV11Storage.readBytes(kV12CurrentSize, migratedV11Preset.data(), migratedV11Preset.size()));
    CHECK_EQ(migratedV11Current[4], 12U);
    CHECK_EQ(migratedV11Preset[4], 12U);

    // Every legacy record parser has a fail-closed header/CRC contract. Exercise
    // every short-circuit guard independently so migration regressions cannot be
    // hidden by the normal happy-path rewrite. Preset names also cover each
    // allowed character class before the explicit invalid-character case.
    const auto rejectLegacyCurrent = [&](const auto& baseRecord, const auto& mutate) {
        auto record = baseRecord;
        mutate(record);
        hal::PersistentStorage::resetForTest();
        hal::PersistentStorage storage;
        CHECK(storage.writeBytes(0U, record.data(), record.size()));
        services::PersistentStateService service(storage);
        service.begin();
        CHECK(!service.hasStoredCurrentState());
    };
    const auto rejectLegacyPreset = [&](
                                        const auto& baseRecord,
                                        const std::size_t presetOffset,
                                        const auto& mutate) {
        auto record = baseRecord;
        mutate(record);
        hal::PersistentStorage::resetForTest();
        hal::PersistentStorage storage;
        CHECK(storage.writeBytes(presetOffset, record.data(), record.size()));
        services::PersistentStateService service(storage);
        service.begin();
        CHECK(!service.presetExists(0U));
    };
    const auto acceptLegacyPresetCharacterClasses = [&](
                                                         const auto& baseRecord,
                                                         const std::size_t presetOffset) {
        auto record = baseRecord;
        const char testName[] = "A-1_TEST";
        for (std::size_t index = 0U; index < services::PersistentStateService::kPresetNameLength; ++index) {
            record[8U + index] = static_cast<std::uint8_t>(' ');
        }
        for (std::size_t index = 0U; index < sizeof(testName) - 1U; ++index) {
            record[8U + index] = static_cast<std::uint8_t>(testName[index]);
        }
        writeTestUint32Le(
            record.data() + record.size() - 4U,
            testCrc32(record.data(), record.size() - 4U));
        hal::PersistentStorage::resetForTest();
        hal::PersistentStorage storage;
        CHECK(storage.writeBytes(presetOffset, record.data(), record.size()));
        services::PersistentStateService service(storage);
        service.begin();
        CHECK(service.presetExists(0U));
    };
    const auto exerciseLegacyRecordGuards = [&](
                                                 const auto& currentRecord,
                                                 const auto& presetRecord,
                                                 const std::size_t presetOffset) {
        rejectLegacyCurrent(currentRecord, [](auto& record) { record[0U] ^= 0x01U; });
        rejectLegacyCurrent(currentRecord, [](auto& record) { record[4U] ^= 0x7FU; });
        rejectLegacyCurrent(currentRecord, [](auto& record) { record[5U] = 1U; });
        rejectLegacyCurrent(currentRecord, [](auto& record) { record[6U] ^= 0x01U; });
        rejectLegacyCurrent(currentRecord, [](auto& record) { record.back() ^= 0x80U; });

        rejectLegacyPreset(presetRecord, presetOffset, [](auto& record) { record[0U] ^= 0x01U; });
        rejectLegacyPreset(presetRecord, presetOffset, [](auto& record) { record[4U] ^= 0x7FU; });
        rejectLegacyPreset(presetRecord, presetOffset, [](auto& record) { record[5U] = 0U; });
        rejectLegacyPreset(presetRecord, presetOffset, [](auto& record) { record[6U] ^= 0x01U; });
        rejectLegacyPreset(presetRecord, presetOffset, [](auto& record) { record.back() ^= 0x80U; });
        rejectLegacyPreset(presetRecord, presetOffset, [](auto& record) {
            record[8U] = static_cast<std::uint8_t>('?');
            writeTestUint32Le(
                record.data() + record.size() - 4U,
                testCrc32(record.data(), record.size() - 4U));
        });
        acceptLegacyPresetCharacterClasses(presetRecord, presetOffset);
    };

    exerciseLegacyRecordGuards(v11Current, v11Preset, kV11CurrentSize);
    exerciseLegacyRecordGuards(v10Current, v10Preset, kV10PresetOffset);
    exerciseLegacyRecordGuards(v9Current, v9Preset, kV9PresetOffset);
    exerciseLegacyRecordGuards(v8Current, v8Preset, kV8PresetOffset);
    exerciseLegacyRecordGuards(v7Current, v7Preset, kV7PresetOffset);
    exerciseLegacyRecordGuards(v6Current, v6Preset, kV6PresetOffset);
    exerciseLegacyRecordGuards(v5Current, v5Preset, kV5PresetOffset);
    exerciseLegacyRecordGuards(v4Current, v4Preset, kV4PresetOffset);
    exerciseLegacyRecordGuards(v3Current, v3Preset, kV3PresetOffset);

    // v10 -> v11: Stage-1 Groove survives and configurable inputs receive factory roles.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV10Storage;
    CHECK(legacyV10Storage.writeBytes(0U, v10Current.data(), v10Current.size()));
    CHECK(legacyV10Storage.writeBytes(kV10PresetOffset, v10Preset.data(), v10Preset.size()));
    services::PersistentStateService migratedV10(legacyV10Storage);
    migratedV10.begin();
    ClockState restoredV10 = makeDefaultState();
    CHECK(migratedV10.restoreCurrentState(restoredV10));
    CHECK_EQ(restoredV10.inputs.input1, InputFunction::Sync);
    CHECK_EQ(restoredV10.inputs.input2, InputFunction::Reset);
    CHECK(migratedV10.presetExists(0U));
    std::array<std::uint8_t, kV12CurrentSize> rewrittenV12Current{};
    std::array<std::uint8_t, kV12PresetSize> rewrittenV12Preset{};
    CHECK(legacyV10Storage.readBytes(0U, rewrittenV12Current.data(), rewrittenV12Current.size()));
    CHECK(legacyV10Storage.readBytes(kV12CurrentSize, rewrittenV12Preset.data(), rewrittenV12Preset.size()));
    CHECK_EQ(rewrittenV12Current[4], 12U);
    CHECK_EQ(rewrittenV12Preset[4], 12U);

    // v9 -> v11: early 1.1 Pre-Count state survives and Groove defaults to OFF.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV9Storage;
    CHECK(legacyV9Storage.writeBytes(0U, v9Current.data(), v9Current.size()));
    CHECK(legacyV9Storage.writeBytes(kV9PresetOffset, v9Preset.data(), v9Preset.size()));
    services::PersistentStateService migratedV9(legacyV9Storage);
    migratedV9.begin();
    ClockState restoredV9 = makeDefaultState();
    CHECK(migratedV9.restoreCurrentState(restoredV9));
    CHECK_EQ(restoredV9.bpm, 143U);
    CHECK_EQ(restoredV9.unifiedClock.groove.preset, GroovePreset::Off);
    CHECK_EQ(restoredV9.unifiedClock.groove.amountPercent, 100U);
    CHECK_EQ(restoredV9.channels[2].common.groove.preset, GroovePreset::Off);
    CHECK_EQ(restoredV9.inputs.input1, InputFunction::Sync);
    CHECK_EQ(restoredV9.inputs.input2, InputFunction::Reset);
    CHECK(migratedV9.presetExists(0U));

    // v8 -> v10: stable 1.0.x state survives unchanged; Pre-Count and Groove default
    // to OFF so upgrading firmware never delays an existing user's next PLAY.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV8Storage;
    CHECK(legacyV8Storage.writeBytes(0U, v8Current.data(), v8Current.size()));
    CHECK(legacyV8Storage.writeBytes(kV8PresetOffset, v8Preset.data(), v8Preset.size()));
    services::PersistentStateService migratedV8(legacyV8Storage);
    migratedV8.begin();
    ClockState restoredV8 = makeDefaultState();
    CHECK(migratedV8.restoreCurrentState(restoredV8));
    CHECK_EQ(restoredV8.bpm, 143U);
    CHECK_EQ(restoredV8.externalSync.smoothing, legacyState.externalSync.smoothing);
    CHECK_EQ(restoredV8.preCountSteps, 0U);
    CHECK_EQ(restoredV8.inputs.input1, InputFunction::Sync);
    CHECK_EQ(restoredV8.inputs.input2, InputFunction::Reset);
    CHECK(migratedV8.presetExists(0U));
    std::array<std::uint8_t, kV11CurrentSize> migratedV8ToV9Current{};
    std::array<std::uint8_t, kV11PresetSize> migratedV8ToV9Preset{};
    CHECK(legacyV8Storage.readBytes(0U, migratedV8ToV9Current.data(), migratedV8ToV9Current.size()));
    CHECK(legacyV8Storage.readBytes(kV12CurrentSize, migratedV8ToV9Preset.data(), migratedV8ToV9Preset.size()));
    CHECK_EQ(migratedV8ToV9Current[4], 12U);
    CHECK_EQ(migratedV8ToV9Preset[4], 12U);

    // v7 -> v10: all prior musical/device state survives, SYNC smoothing receives
    // the release-safe LOW default, and Pre-Count receives OFF before rewrite.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV7Storage;
    CHECK(legacyV7Storage.writeBytes(0U, v7Current.data(), v7Current.size()));
    CHECK(legacyV7Storage.writeBytes(kV7PresetOffset, v7Preset.data(), v7Preset.size()));
    services::PersistentStateService migratedV7(legacyV7Storage);
    migratedV7.begin();
    ClockState restoredV7 = makeDefaultState();
    CHECK(migratedV7.restoreCurrentState(restoredV7));
    CHECK_EQ(restoredV7.bpm, 143U);
    CHECK_EQ(restoredV7.externalSync.resetMode, ExternalResetMode::Gate);
    CHECK_EQ(restoredV7.externalSync.smoothing, SyncSmoothing::Low);
    CHECK(migratedV7.presetExists(0U));
    CHECK_EQ(restoredV7.preCountSteps, 0U);
    std::array<std::uint8_t, kV12CurrentSize> migratedV7ToV9Current{};
    std::array<std::uint8_t, kV12PresetSize> migratedV7ToV9Preset{};
    CHECK(legacyV7Storage.readBytes(0U, migratedV7ToV9Current.data(), migratedV7ToV9Current.size()));
    CHECK(legacyV7Storage.readBytes(kV12CurrentSize, migratedV7ToV9Preset.data(), migratedV7ToV9Preset.size()));
    CHECK_EQ(migratedV7ToV9Current[4], 12U);
    CHECK_EQ(migratedV7ToV9Preset[4], 12U);

    // A CRC-valid current record with a semantically invalid payload must be rejected.
    // This exercises the post-decode semantic-validation path independently of CRC checks.
    auto semanticallyInvalidV6 = v6Current;
    semanticallyInvalidV6[8U] = 0U;  // BPM = 0, below every supported tempo range.
    semanticallyInvalidV6[9U] = 0U;
    writeTestUint32Le(
        semanticallyInvalidV6.data() + semanticallyInvalidV6.size() - 4U,
        testCrc32(semanticallyInvalidV6.data(), semanticallyInvalidV6.size() - 4U));
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage invalidV6Storage;
    CHECK(invalidV6Storage.writeBytes(0U, semanticallyInvalidV6.data(), semanticallyInvalidV6.size()));
    services::PersistentStateService invalidV6(invalidV6Storage);
    invalidV6.begin();
    CHECK(!invalidV6.hasStoredCurrentState());

    // v6 -> v10: musical state is preserved and new device preferences default safely.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV6Storage;
    CHECK(legacyV6Storage.writeBytes(0U, v6Current.data(), v6Current.size()));
    CHECK(legacyV6Storage.writeBytes(kV6PresetOffset, v6Preset.data(), v6Preset.size()));
    services::PersistentStateService migratedV6(legacyV6Storage);
    migratedV6.begin();
    ClockState restoredV6 = makeDefaultState();
    CHECK(migratedV6.restoreCurrentState(restoredV6));
    CHECK_EQ(restoredV6.bpm, 143U);
    CHECK_EQ(restoredV6.externalSync.resetMode, ExternalResetMode::Gate);
    CHECK(!restoredV6.device.encoderDirectionReversed);
    CHECK(!restoredV6.device.displayRotated180);
    CHECK_EQ(restoredV6.externalSync.smoothing, SyncSmoothing::Low);
    CHECK_EQ(restoredV6.preCountSteps, 0U);
    CHECK(migratedV6.presetExists(0U));
    std::array<std::uint8_t, kV12CurrentSize> migratedV6ToV9Current{};
    std::array<std::uint8_t, kV12PresetSize> migratedV6ToV9Preset{};
    CHECK(legacyV6Storage.readBytes(0U, migratedV6ToV9Current.data(), migratedV6ToV9Current.size()));
    CHECK(legacyV6Storage.readBytes(kV12CurrentSize, migratedV6ToV9Preset.data(), migratedV6ToV9Preset.size()));
    CHECK_EQ(migratedV6ToV9Current[4], 12U);
    CHECK_EQ(migratedV6ToV9Preset[4], 12U);

    // v5 -> v10: user state remains intact; newer fields receive safe defaults.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV5Storage;
    CHECK(legacyV5Storage.writeBytes(0U, v5Current.data(), v5Current.size()));
    CHECK(legacyV5Storage.writeBytes(kV5PresetOffset, v5Preset.data(), v5Preset.size()));
    CHECK(legacyV5Storage.writeBytes(kScoreOffset, scoreMarker.data(), scoreMarker.size()));
    services::PersistentStateService migratedV5(legacyV5Storage);
    migratedV5.begin();
    ClockState restoredV5 = makeDefaultState();
    CHECK(migratedV5.restoreCurrentState(restoredV5));
    CHECK_EQ(restoredV5.bpm, 143U);
    CHECK_EQ(restoredV5.externalSync.resetMode, defaults::kExternalResetMode);
    CHECK(!restoredV5.device.encoderDirectionReversed);
    CHECK(!restoredV5.device.displayRotated180);
    CHECK_EQ(restoredV5.externalSync.smoothing, SyncSmoothing::Low);
    CHECK_EQ(restoredV5.preCountSteps, 0U);
    CHECK(migratedV5.presetExists(0U));
    std::array<std::uint8_t, kV12CurrentSize> migratedV9Current{};
    std::array<std::uint8_t, kV12PresetSize> migratedV9Preset{};
    CHECK(legacyV5Storage.readBytes(0U, migratedV9Current.data(), migratedV9Current.size()));
    CHECK(legacyV5Storage.readBytes(kV12CurrentSize, migratedV9Preset.data(), migratedV9Preset.size()));
    CHECK_EQ(migratedV9Current[4], 12U);
    CHECK_EQ(migratedV9Preset[4], 12U);

    // v4 -> v10: user BPM remains intact and newer preferences receive factory defaults.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyV4Storage;
    CHECK(legacyV4Storage.writeBytes(0U, v4Current.data(), v4Current.size()));
    CHECK(legacyV4Storage.writeBytes(kV4PresetOffset, v4Preset.data(), v4Preset.size()));
    CHECK(legacyV4Storage.writeBytes(kScoreOffset, scoreMarker.data(), scoreMarker.size()));
    services::PersistentStateService migratedV4(legacyV4Storage);
    migratedV4.begin();
    ClockState restoredV4 = makeDefaultState();
    CHECK(migratedV4.restoreCurrentState(restoredV4));
    CHECK_EQ(restoredV4.bpm, 143U);
    CHECK_EQ(restoredV4.tempoRange.minimumBpm, defaults::kMinimumBpm);
    CHECK_EQ(restoredV4.tempoRange.maximumBpm, defaults::kMaximumBpm);
    CHECK_EQ(restoredV4.unifiedClock.humanizeUs, defaults::kUnifiedClockHumanizeUs);
    CHECK_EQ(restoredV4.externalSync.resetMode, defaults::kExternalResetMode);
    CHECK_EQ(restoredV4.externalSync.smoothing, SyncSmoothing::Low);
    CHECK_EQ(restoredV4.preCountSteps, 0U);
    CHECK(migratedV4.presetExists(0U));
    CHECK(legacyV4Storage.readBytes(0U, migratedV9Current.data(), migratedV9Current.size()));
    CHECK(legacyV4Storage.readBytes(kV12CurrentSize, migratedV9Preset.data(), migratedV9Preset.size()));
    CHECK_EQ(migratedV9Current[4], 12U);
    CHECK_EQ(migratedV9Preset[4], 12U);

    // v3 -> v10 through all intermediate layouts, preserving the independent high-score area.
    hal::PersistentStorage::resetForTest();
    hal::PersistentStorage legacyStorage;
    CHECK(legacyStorage.writeBytes(0U, v3Current.data(), v3Current.size()));
    CHECK(legacyStorage.writeBytes(kV3PresetOffset, v3Preset.data(), v3Preset.size()));
    CHECK(legacyStorage.writeBytes(kScoreOffset, scoreMarker.data(), scoreMarker.size()));

    services::PersistentStateService migrated(legacyStorage);
    migrated.begin();
    ClockState restored = makeDefaultState();
    CHECK(migrated.restoreCurrentState(restored));
    CHECK_EQ(restored.bpm, 143U);
    CHECK_EQ(restored.masterMeter.unit, 8U);
    CHECK_EQ(restored.transport, TransportState::Playing);
    CHECK_EQ(restored.channels[2].euclid.steps, 13U);
    CHECK_EQ(restored.display.screensaverMode, defaults::kScreensaverMode);
    CHECK_EQ(restored.display.screensaverAfterMinutes, defaults::kScreensaverAfterMinutes);
    CHECK_EQ(restored.display.dimAfterMinutes, defaults::kScreensaverDimAfterMinutes);
    CHECK_EQ(restored.display.offAfterMinutes, defaults::kScreensaverOffAfterMinutes);
    CHECK_EQ(restored.tempoRange.minimumBpm, defaults::kMinimumBpm);
    CHECK_EQ(restored.tempoRange.maximumBpm, defaults::kMaximumBpm);
    CHECK_EQ(restored.unifiedClock.humanizeUs, 0U);
    CHECK_EQ(restored.externalSync.resetMode, defaults::kExternalResetMode);
    CHECK(!restored.device.encoderDirectionReversed);
    CHECK(!restored.device.displayRotated180);
    CHECK_EQ(restored.externalSync.smoothing, SyncSmoothing::Low);
    CHECK_EQ(restored.preCountSteps, 0U);
    CHECK(migrated.presetExists(0U));
    char presetName[services::PersistentStateService::kPresetNameLength + 1U]{};
    migrated.presetName(0U, presetName, sizeof(presetName));
    CHECK(std::strncmp(presetName, "MIGRATE", 7U) == 0);
    ClockState loadedPreset = makeDefaultState();
    loadedPreset.transport = TransportState::Paused;
    CHECK(migrated.loadPreset(0U, loadedPreset));
    CHECK_EQ(loadedPreset.bpm, 143U);
    CHECK_EQ(loadedPreset.transport, TransportState::Paused);
    CHECK_EQ(loadedPreset.display.screensaverMode, defaults::kScreensaverMode);
    CHECK_EQ(loadedPreset.externalSync.resetMode, defaults::kExternalResetMode);
    CHECK_EQ(loadedPreset.externalSync.smoothing, SyncSmoothing::Low);
    CHECK_EQ(loadedPreset.preCountSteps, 0U);

    std::array<std::uint8_t, 16U> scoreAfter{};
    CHECK(legacyStorage.readBytes(kScoreOffset, scoreAfter.data(), scoreAfter.size()));
    CHECK(scoreAfter == scoreMarker);
    CHECK(legacyStorage.readBytes(0U, migratedV9Current.data(), migratedV9Current.size()));
    CHECK(legacyStorage.readBytes(kV12CurrentSize, migratedV9Preset.data(), migratedV9Preset.size()));
    CHECK_EQ(migratedV9Current[4], 12U);
    CHECK_EQ(migratedV9Preset[4], 12U);

    // A damaged v3 CURRENT must not prevent a valid v3 preset from being recovered.
    auto corruptCurrent = v3Current;
    corruptCurrent[40U] ^= 0x80U;
    hal::PersistentStorage::resetForTest();
    CHECK(legacyStorage.writeBytes(0U, corruptCurrent.data(), corruptCurrent.size()));
    CHECK(legacyStorage.writeBytes(kV3PresetOffset, v3Preset.data(), v3Preset.size()));
    services::PersistentStateService presetOnlyMigration(legacyStorage);
    presetOnlyMigration.begin();
    CHECK(!presetOnlyMigration.hasStoredCurrentState());
    CHECK(presetOnlyMigration.presetExists(0U));

    // A CRC-valid legacy preset with unsupported name characters is rejected.
    auto badNamePreset = v3Preset;
    badNamePreset[8U] = static_cast<std::uint8_t>('?');
    writeTestUint32Le(
        badNamePreset.data() + badNamePreset.size() - 4U,
        testCrc32(badNamePreset.data(), badNamePreset.size() - 4U));
    hal::PersistentStorage::resetForTest();
    CHECK(legacyStorage.writeBytes(kV3PresetOffset, badNamePreset.data(), badNamePreset.size()));
    services::PersistentStateService invalidNameMigration(legacyStorage);
    invalidNameMigration.begin();
    CHECK(!invalidNameMigration.presetExists(0U));

    // Legacy tempo below the new factory minimum remains valid after migration.
    // The migration lowers that user's personal minimum rather than rewriting BPM.
    auto lowTempoV4 = v4Current;
    lowTempoV4[8U] = 12U;
    lowTempoV4[9U] = 0U;
    writeTestUint32Le(
        lowTempoV4.data() + lowTempoV4.size() - 4U,
        testCrc32(lowTempoV4.data(), lowTempoV4.size() - 4U));
    hal::PersistentStorage::resetForTest();
    CHECK(legacyStorage.writeBytes(0U, lowTempoV4.data(), lowTempoV4.size()));
    services::PersistentStateService lowTempoMigration(legacyStorage);
    lowTempoMigration.begin();
    ClockState lowTempoRestored = makeDefaultState();
    CHECK(lowTempoMigration.restoreCurrentState(lowTempoRestored));
    CHECK_EQ(lowTempoRestored.bpm, 12U);
    CHECK_EQ(lowTempoRestored.tempoRange.minimumBpm, 12U);
    CHECK_EQ(lowTempoRestored.tempoRange.maximumBpm, 999U);
    CHECK_EQ(lowTempoRestored.externalSync.resetMode, defaults::kExternalResetMode);
}


void testPersistentStateValidationBoundaries() {
    ClockState state = makeDefaultState();
    CHECK(services::isPersistentStateValid(state));

    const auto invalid = [](const auto& mutate) {
        ClockState candidate = makeDefaultState();
        mutate(candidate);
        CHECK(!services::isPersistentStateValid(candidate));
    };

    invalid([](ClockState& s) { s.tempoRange.minimumBpm = 0U; });
    invalid([](ClockState& s) { s.tempoRange.maximumBpm = 1000U; });
    invalid([](ClockState& s) { s.tempoRange.minimumBpm = 200U; s.tempoRange.maximumBpm = 100U; });
    invalid([](ClockState& s) { s.bpm = 19U; });
    invalid([](ClockState& s) { s.bpm = 1000U; });
    invalid([](ClockState& s) { s.masterMeter.beats = 0U; });
    invalid([](ClockState& s) { s.masterMeter.beats = 17U; });
    invalid([](ClockState& s) { s.masterMeter.unit = 3U; });
    // Exercise every valid meter denominator so the validator's short-circuit
    // chain is covered on each accepted exit path, not only the factory 4/4.
    for (const std::uint8_t unit : std::array<std::uint8_t, 5U>{1U, 2U, 4U, 8U, 16U}) {
        ClockState candidate = makeDefaultState();
        candidate.masterMeter.unit = unit;
        CHECK(services::isPersistentStateValid(candidate));
    }
    invalid([](ClockState& s) { s.transport = static_cast<TransportState>(99U); });
    invalid([](ClockState& s) { s.source = static_cast<ClockSource>(99U); });
    invalid([](ClockState& s) { s.operatingMode = static_cast<OperatingMode>(99U); });
    invalid([](ClockState& s) { s.externalSync.pulsesPerQuarterNote = 0U; });
    invalid([](ClockState& s) { s.externalSync.edge = static_cast<SyncEdge>(99U); });
    invalid([](ClockState& s) { s.externalSync.lossMode = static_cast<SyncLossMode>(99U); });
    invalid([](ClockState& s) { s.externalSync.resetMode = static_cast<ExternalResetMode>(99U); });
    invalid([](ClockState& s) { s.externalSync.smoothing = static_cast<SyncSmoothing>(99U); });
    invalid([](ClockState& s) { s.preCountSteps = 65U; });
    invalid([](ClockState& s) { s.inputs.input1 = static_cast<InputFunction>(99U); });
    invalid([](ClockState& s) { s.inputs.input2 = static_cast<InputFunction>(99U); });
    invalid([](ClockState& s) { s.inputs.input1 = InputFunction::Tap; s.inputs.input2 = InputFunction::Tap; });
    {
        ClockState candidate = makeDefaultState();
        candidate.inputs.input1 = InputFunction::Off;
        candidate.inputs.input2 = InputFunction::Off;
        CHECK(services::isPersistentStateValid(candidate));
    }

    invalid([](ClockState& s) { s.unifiedClock.rate.mode = static_cast<ClockRatioMode>(99U); });
    invalid([](ClockState& s) { s.unifiedClock.rate.factor = 0U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.factor = 33U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.numerator = 0U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.numerator = 17U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.denominator = 0U; });
    invalid([](ClockState& s) { s.unifiedClock.rate.denominator = 17U; });
    invalid([](ClockState& s) { s.unifiedClock.swingPercent = 51U; });
    invalid([](ClockState& s) { s.unifiedClock.groove.preset = static_cast<GroovePreset>(99U); });
    invalid([](ClockState& s) { s.unifiedClock.groove.amountPercent = 101U; });
    invalid([](ClockState& s) { s.unifiedClock.groove.customSlot = kCustomGrooveSlotCount; });
    invalid([](ClockState& s) { s.unifiedClock.groove = {GroovePreset::Custom, 100U, static_cast<std::uint8_t>(kCustomGrooveMaximumSteps), 0U}; });
    invalid([](ClockState& s) { s.unifiedClock.groove = {GroovePreset::Swing54, 100U, 2U, 0U}; });
    invalid([](ClockState& s) { s.unifiedClock.phasePercent = 100U; });
    invalid([](ClockState& s) { s.unifiedClock.humanizeUs = static_cast<std::uint16_t>(config::kMaximumHumanizeUs + 1U); });
    invalid([](ClockState& s) { s.dividerBank.bank = static_cast<DividerBank>(99U); });
    invalid([](ClockState& s) { s.display.screensaverMode = static_cast<ScreensaverMode>(0U); });
    invalid([](ClockState& s) { s.display.screensaverMode = static_cast<ScreensaverMode>(17U); });
    invalid([](ClockState& s) { s.display.screensaverAfterMinutes = 0U; });
    invalid([](ClockState& s) { s.display.screensaverAfterMinutes = 6U; s.display.dimAfterMinutes = 5U; });
    invalid([](ClockState& s) { s.display.dimAfterMinutes = 11U; s.display.offAfterMinutes = 10U; });
    invalid([](ClockState& s) { s.display.offAfterMinutes = static_cast<std::uint8_t>(config::kMaximumScreensaverMinutes + 1U); });

    invalid([](ClockState& s) { s.channels[0].common.mode = static_cast<ChannelMode>(99U); });
    invalid([](ClockState& s) { s.channels[0].common.rate.mode = static_cast<ClockRatioMode>(99U); });
    invalid([](ClockState& s) { s.channels[0].common.rate.factor = 0U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.factor = 33U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.numerator = 0U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.numerator = 17U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.denominator = 0U; });
    invalid([](ClockState& s) { s.channels[0].common.rate.denominator = 17U; });
    invalid([](ClockState& s) { s.channels[0].common.swingPercent = 51U; });
    invalid([](ClockState& s) { s.channels[0].common.groove.preset = static_cast<GroovePreset>(99U); });
    invalid([](ClockState& s) { s.channels[0].common.groove.amountPercent = 101U; });
    invalid([](ClockState& s) { s.channels[0].common.groove.customSlot = kCustomGrooveSlotCount; });
    invalid([](ClockState& s) { s.channels[0].common.groove = {GroovePreset::Custom, 100U, static_cast<std::uint8_t>(kCustomGrooveMaximumSteps), 0U}; });
    invalid([](ClockState& s) { s.channels[0].common.groove = {GroovePreset::PocketA, 100U, 8U, 0U}; });
    invalid([](ClockState& s) { s.channels[0].common.probabilityPercent = 101U; });
    invalid([](ClockState& s) { s.channels[0].common.phasePercent = 100U; });
    invalid([](ClockState& s) { s.channels[0].common.resetMode = static_cast<ResetMode>(99U); });
    invalid([](ClockState& s) { s.channels[0].clock.meter.beats = 0U; });
    invalid([](ClockState& s) { s.channels[0].clock.meter.beats = 17U; });
    invalid([](ClockState& s) { s.channels[0].clock.meter.unit = 3U; });
    for (const std::uint8_t unit : std::array<std::uint8_t, 5U>{1U, 2U, 4U, 8U, 16U}) {
        ClockState candidate = makeDefaultState();
        candidate.channels[0].clock.meter.unit = unit;
        CHECK(services::isPersistentStateValid(candidate));
    }
    invalid([](ClockState& s) { s.channels[0].euclid.steps = 0U; });
    invalid([](ClockState& s) { s.channels[0].euclid.steps = 65U; });
    invalid([](ClockState& s) { s.channels[0].euclid.steps = 8U; s.channels[0].euclid.hits = 9U; });
    invalid([](ClockState& s) { s.channels[0].euclid.steps = 8U; s.channels[0].euclid.rotation = 8U; });
    invalid([](ClockState& s) { s.channels[0].sequencer.length = 0U; });
    invalid([](ClockState& s) { s.channels[0].sequencer.length = 65U; });
    invalid([](ClockState& s) { s.channels[0].sequencer.length = 8U; s.channels[0].sequencer.rotation = 8U; });
}
