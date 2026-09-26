/**
 * @file sequencer_step_expression.inc
 * @brief Sequencer 2.0 per-step expression runtime contracts.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

std::size_t countGateWrites(const std::uint8_t value) {
    std::size_t count = 0U;
    for (const auto& write : fakefw::writes) {
        if (write.pin == pinmap::kGateChannelPins[0] && write.value == value) {
            ++count;
        }
    }
    return count;
}

void configureSequencerFixture(Fixture& fixture, const SequencerPatternV2& pattern) {
    fixture.state.source = ClockSource::Internal;
    fixture.state.bpm = 120U;
    fixture.state.channels[0].common.mode = ChannelMode::Sequencer;
    fixture.state.channels[0].common.probabilityPercent = 100U;
    fixture.state.channels[0].common.gateLengthMs = 10U;
    fixture.begin();
    fixture.engine.updateSequencerPattern(0U, pattern, true);
    fakefw::writes.clear();
}

void testSequencerStepProbabilityOverrideReplacesChannelProbability() {
    Fixture fixture;
    SequencerPatternV2 pattern{};
    pattern.length = 1U;
    pattern.gates = {{1ULL, 0ULL}};
    configureSequencerFixture(fixture, pattern);
    fixture.state.channels[0].common.probabilityPercent = 0U;
    fixture.engine.updateChannel(0U, fixture.state.channels[0], true);

    std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
    SequencerStepMetadata step{};
    step.probabilityPercent = 100U;
    metadata[0U] = packSequencerStepMetadata(step);
    fixture.engine.updateSequencerStepMetadata(0U, metadata);
    runTicks(fixture.engine, 3000U);
    CHECK(countGateWrites(HIGH) >= 1U);

    fixture.engine.stop();
    fakefw::writes.clear();
    metadata.fill(0U);
    fixture.engine.updateSequencerStepMetadata(0U, metadata);
    fixture.engine.play();
    runTicks(fixture.engine, 3000U);
    CHECK_EQ(countGateWrites(HIGH), 0U);
}

void testSequencerEvenRatchetEmitsBoundedDistinctSubEvents() {
    Fixture fixture;
    SequencerPatternV2 pattern{};
    pattern.length = 2U;
    pattern.gates = {{1ULL, 0ULL}};
    configureSequencerFixture(fixture, pattern);

    std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
    SequencerStepMetadata step{};
    step.gateProfile = SequencerGateProfile::Trigger1Ms;
    step.ratchetCount = 3U;
    metadata[0U] = packSequencerStepMetadata(step);
    fixture.engine.updateSequencerStepMetadata(0U, metadata);

    runTicks(fixture.engine, 2400U);
    CHECK_EQ(countGateWrites(HIGH), 3U);
    CHECK_EQ(countGateWrites(LOW), 3U);
    runTicks(fixture.engine, 200U);
    CHECK_EQ(countGateWrites(HIGH), 3U);
}

void testSequencerTieKeepsAdjacentGateContinuousThenReleases() {
    Fixture fixture;
    SequencerPatternV2 pattern{};
    pattern.length = 2U;
    pattern.gates = {{3ULL, 0ULL}};
    configureSequencerFixture(fixture, pattern);

    std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
    SequencerStepMetadata first{};
    first.tie = true;
    metadata[0U] = packSequencerStepMetadata(first);
    fixture.engine.updateSequencerStepMetadata(0U, metadata);

    runTicks(fixture.engine, 2600U);
    CHECK_EQ(countGateWrites(HIGH), 1U);
    CHECK(fixture.gates.channelStateHigh(0U));
    runTicks(fixture.engine, 300U);
    CHECK_EQ(countGateWrites(HIGH), 1U);
    CHECK_EQ(countGateWrites(LOW), 1U);
    CHECK(!fixture.gates.channelStateHigh(0U));
}

void testSequencerTieDoesNotCrossRestRandomOrOnceEnd() {
    {
        Fixture fixture;
        SequencerPatternV2 pattern{};
        pattern.length = 2U;
        pattern.gates = {{1ULL, 0ULL}};
        configureSequencerFixture(fixture, pattern);
        std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
        SequencerStepMetadata step{};
        step.tie = true;
        metadata[0U] = packSequencerStepMetadata(step);
        fixture.engine.updateSequencerStepMetadata(0U, metadata);
        runTicks(fixture.engine, 300U);
        CHECK(!fixture.gates.channelStateHigh(0U));
    }
    {
        Fixture fixture;
        SequencerPatternV2 pattern{};
        pattern.length = 1U;
        pattern.loopMode = SequencerLoopMode::Once;
        pattern.gates = {{1ULL, 0ULL}};
        configureSequencerFixture(fixture, pattern);
        std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
        SequencerStepMetadata step{};
        step.tie = true;
        metadata[0U] = packSequencerStepMetadata(step);
        fixture.engine.updateSequencerStepMetadata(0U, metadata);
        runTicks(fixture.engine, 300U);
        CHECK(!fixture.gates.channelStateHigh(0U));
        const std::size_t highs = countGateWrites(HIGH);
        runTicks(fixture.engine, 3000U);
        CHECK_EQ(countGateWrites(HIGH), highs);
    }
    {
        Fixture fixture;
        SequencerPatternV2 pattern{};
        pattern.length = 2U;
        pattern.direction = SequencerPlayDirection::Random;
        pattern.gates = {{3ULL, 0ULL}};
        configureSequencerFixture(fixture, pattern);
        std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
        SequencerStepMetadata step{};
        step.tie = true;
        metadata[0U] = packSequencerStepMetadata(step);
        metadata[1U] = packSequencerStepMetadata(step);
        fixture.engine.updateSequencerStepMetadata(0U, metadata);
        runTicks(fixture.engine, 2800U);
        CHECK(countGateWrites(HIGH) >= 2U);
        CHECK(countGateWrites(LOW) >= 2U);
    }
}


void testSequencerLiveEditReleasesActiveTieImmediately() {
    {
        Fixture fixture;
        SequencerPatternV2 pattern{};
        pattern.length = 2U;
        pattern.gates = {{3ULL, 0ULL}};
        configureSequencerFixture(fixture, pattern);

        std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
        SequencerStepMetadata first{};
        first.tie = true;
        metadata[0U] = packSequencerStepMetadata(first);
        fixture.engine.updateSequencerStepMetadata(0U, metadata);

        runTicks(fixture.engine, 100U);
        CHECK(fixture.gates.channelStateHigh(0U));
        CHECK_EQ(countGateWrites(HIGH), 1U);

        metadata[0U] = 0U;
        fixture.engine.updateSequencerStepMetadata(0U, metadata);
        CHECK(!fixture.gates.channelStateHigh(0U));
        CHECK_EQ(countGateWrites(LOW), 1U);
    }

    {
        Fixture fixture;
        SequencerPatternV2 pattern{};
        pattern.length = 2U;
        pattern.gates = {{3ULL, 0ULL}};
        configureSequencerFixture(fixture, pattern);

        std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
        SequencerStepMetadata first{};
        first.tie = true;
        metadata[0U] = packSequencerStepMetadata(first);
        fixture.engine.updateSequencerStepMetadata(0U, metadata);

        runTicks(fixture.engine, 100U);
        CHECK(fixture.gates.channelStateHigh(0U));
        CHECK_EQ(countGateWrites(HIGH), 1U);

        pattern.gates = {{1ULL, 0ULL}};
        fixture.engine.updateSequencerPattern(0U, pattern, false);
        CHECK(!fixture.gates.channelStateHigh(0U));
        CHECK_EQ(countGateWrites(LOW), 1U);
    }
}

void testSequencerRelativeDutyOverridesGateLength() {
    Fixture fixture;
    SequencerPatternV2 pattern{};
    pattern.length = 1U;
    pattern.gates = {{1ULL, 0ULL}};
    configureSequencerFixture(fixture, pattern);

    std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
    SequencerStepMetadata step{};
    step.gateProfile = SequencerGateProfile::Duty50;
    metadata[0U] = packSequencerStepMetadata(step);
    fixture.engine.updateSequencerStepMetadata(0U, metadata);

    runTicks(fixture.engine, 1000U);
    CHECK(fixture.gates.channelStateHigh(0U));
    runTicks(fixture.engine, 400U);
    CHECK(!fixture.gates.channelStateHigh(0U));
}
