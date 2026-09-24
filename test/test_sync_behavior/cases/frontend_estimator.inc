/**
 * @file frontend_estimator.inc
 * @brief Included SYNC behavior cases split for readability.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

void testSyncFrontendReferenceDividerIsNominal161mV() {
    CHECK_NEAR(testsupport::SyncFrontendModel::referenceVoltageV(), 0.16129, 0.00002);
}

void testSyncFrontendNominalRisingThresholdIsAbout1p79V() {
    CHECK_NEAR(testsupport::SyncFrontendModel::risingThresholdV(), 1.7903, 0.001);
}

void testSyncFrontendNominalFallingThresholdIsAbout1p46V() {
    CHECK_NEAR(testsupport::SyncFrontendModel::fallingThresholdV(), 1.4603, 0.001);
}

void testSyncFrontendNominalHysteresisIsAbout330mV() {
    const double width = testsupport::SyncFrontendModel::risingThresholdV() -
        testsupport::SyncFrontendModel::fallingThresholdV();
    CHECK_NEAR(width, 0.3300, 0.001);
}

void testSyncFrontendZeroVoltsRemainsLow() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(0.0));
}

void testSyncFrontendOneVoltRemainsLow() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(1.0));
}

void testSyncFrontend1400mVRemainsLowFromLowState() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(1.4));
}

void testSyncFrontend1500mVRetainsLowStateInsideHysteresis() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(1.5));
}

void testSyncFrontend1500mVRetainsHighStateInsideHysteresis() {
    testsupport::SyncFrontendModel model;
    model.setOutputHighForTest(true);
    CHECK(model.sample(1.5));
}

void testSyncFrontend1600mVRetainsLowStateInsideHysteresis() {
    testsupport::SyncFrontendModel model;
    CHECK(!model.sample(1.6));
}

void testSyncFrontend1600mVRetainsHighStateInsideHysteresis() {
    testsupport::SyncFrontendModel model;
    model.setOutputHighForTest(true);
    CHECK(model.sample(1.6));
}

void testSyncFrontend1900mVRisesFromLowState() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(1.9));
}

void testSyncFrontend1300mVFallsFromHighState() {
    testsupport::SyncFrontendModel model;
    model.setOutputHighForTest(true);
    CHECK(!model.sample(1.3));
}

void testSyncFrontend2500mVIsNominallyHigh() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(2.5));
}

void testSyncFrontend3000mVIsNominallyHigh() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(3.0));
}

void testSyncFrontend4000mVIsNominallyHigh() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(4.0));
}

void testSyncFrontend5000mVIsNominallyHigh() {
    testsupport::SyncFrontendModel model;
    CHECK(model.sample(5.0));
}

// End-to-end nominal voltage -> comparator model -> digital capture -> estimator.
void testAnalog2500mVAt20BpmLocksEngine() { assertAnalogClockLocks(2.5, 20U); }
void testAnalog2500mVAt120BpmLocksEngine() { assertAnalogClockLocks(2.5, 120U); }
void testAnalog2500mVAt999BpmLocksEngine() { assertAnalogClockLocks(2.5, 999U); }
void testAnalog3000mVAt20BpmLocksEngine() { assertAnalogClockLocks(3.0, 20U); }
void testAnalog3000mVAt120BpmLocksEngine() { assertAnalogClockLocks(3.0, 120U); }
void testAnalog3000mVAt999BpmLocksEngine() { assertAnalogClockLocks(3.0, 999U); }
void testAnalog4000mVAt20BpmLocksEngine() { assertAnalogClockLocks(4.0, 20U); }
void testAnalog4000mVAt120BpmLocksEngine() { assertAnalogClockLocks(4.0, 120U); }
void testAnalog4000mVAt999BpmLocksEngine() { assertAnalogClockLocks(4.0, 999U); }

// -------------------------------------------------------------------------
// Atomic external-tempo and PPQN behavior.
// -------------------------------------------------------------------------

void testExternal1BpmBoundaryIsAccepted() { assertExactTempo(1U); }
void testExternal20BpmIsAccepted() { assertExactTempo(20U); }
void testExternal30BpmIsAccepted() { assertExactTempo(30U); }
void testExternal60BpmIsAccepted() { assertExactTempo(60U); }
void testExternal120BpmIsAccepted() { assertExactTempo(120U); }
void testExternal240BpmIsAccepted() { assertExactTempo(240U); }
void testExternal300BpmIsAccepted() { assertExactTempo(300U); }
void testExternal600BpmIsAccepted() { assertExactTempo(600U); }
void testExternal900BpmIsAccepted() { assertExactTempo(900U); }
void testExternal999BpmBoundaryIsAccepted() { assertExactTempo(999U); }

void testExternal120BpmAt2PpqnIsAccepted() { assertPpqnTempo(2U); }
void testExternal120BpmAt4PpqnIsAccepted() { assertPpqnTempo(4U); }
void testExternal120BpmAt24PpqnIsAccepted() { assertPpqnTempo(24U); }

void testFallingEdgeClockLocksAt120Bpm() {
    Fixture fixture;
    fixture.state.externalSync.edge = SyncEdge::Falling;
    fixture.begin();
    injectFallingPulse(fixture, 2000U);
    injectFallingPulse(fixture, 502000U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 120000U);
}

void testWrongPolarityTransitionsDoNotEstablishPeriod() {
    Fixture fixture;
    fixture.state.externalSync.edge = SyncEdge::Rising;
    fixture.begin();
    fixture.inputs.injectSyncEdgeForTest(1000U, false);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(501000U, false);
    fixture.sync.processSchedulerTick(501000U);
    CHECK(!fixture.engine.snapshot().externalLocked);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 0U);
}

// -------------------------------------------------------------------------
// Glitch rejection and malformed edge streams.
// -------------------------------------------------------------------------

void testDuplicateTimestampDoesNotChangeFilteredTempo() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(501000U, true);
    fixture.sync.processSchedulerTick(501000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void testTenMicrosecondGlitchIsRejected() {
    Fixture fixture;
    fixture.state.externalSync.glitchFilterUs = 1000U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(501010U, true);
    fixture.sync.processSchedulerTick(501010U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void test999MicrosecondGlitchIsRejectedBy1000usFilter() {
    Fixture fixture;
    fixture.state.externalSync.glitchFilterUs = 1000U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(501999U, true);
    fixture.sync.processSchedulerTick(501999U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void test1000MicrosecondBoundaryIsRejectedByTechnicalMaxFloor() {
    Fixture fixture;
    fixture.state.externalSync.pulsesPerQuarterNote = 24U;
    fixture.state.externalSync.glitchFilterUs = 1000U;
    fixture.begin();
    acquireRising(fixture, 20833U);
    const std::uint32_t before = fixture.sync.filteredBpmMilli();
    fixture.inputs.injectSyncEdgeForTest(21833U + 1000U, true);
    fixture.sync.processSchedulerTick(21833U + 1000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), before);
}

void testConfiguredGlitchFloorBoundaryIsAcceptedWhenTechnicallyValid() {
    Fixture fixture;
    fixture.state.externalSync.pulsesPerQuarterNote = 24U;
    fixture.state.externalSync.glitchFilterUs = 5000U;
    fixture.begin();
    fixture.inputs.injectSyncEdgeForTest(1000U, true);
    fixture.sync.processSchedulerTick(1000U);
    fixture.inputs.injectSyncEdgeForTest(6000U, true);
    fixture.sync.processSchedulerTick(6000U);
    CHECK_EQ(fixture.sync.filteredBpmMilli(), 500000U);
}

void testGlitchStormCannotDragStable120BpmEstimator() {
    Fixture fixture;
    fixture.state.externalSync.glitchFilterUs = 1000U;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t stable = fixture.sync.filteredBpmMilli();
    for (std::uint32_t offset = 10U; offset < 1000U; offset += 10U) {
        fixture.inputs.injectSyncEdgeForTest(501000U + offset, true);
        fixture.sync.processSchedulerTick(501000U + offset);
    }
    CHECK_EQ(fixture.sync.filteredBpmMilli(), stable);
    CHECK(fixture.engine.snapshot().externalLocked);
}

void testOppositePolarityNoiseCannotChangeRisingEdgePeriod() {
    Fixture fixture;
    fixture.begin();
    acquireRising(fixture, 500000U);
    const std::uint32_t stable = fixture.sync.filteredBpmMilli();
    for (std::uint32_t index = 0U; index < 20U; ++index) {
        fixture.inputs.injectSyncEdgeForTest(510000U + index * 1000U, false);
        fixture.sync.processSchedulerTick(510000U + index * 1000U);
    }
    CHECK_EQ(fixture.sync.filteredBpmMilli(), stable);
}

// -------------------------------------------------------------------------
// Jitter and deliberately unstable clocks. The contract is bounded, stable
// behavior, not pretending that a wildly varying source has one exact tempo.
// -------------------------------------------------------------------------

void testPlusMinus50usInputJitterRemainsTightlyBounded() {
    assertJitterBand({{-50, 50, -25, 25, -10, 10, 0, 0}}, 119900U, 120100U);
}

void testPlusMinus500usInputJitterRemainsBounded() {
    assertJitterBand({{-500, 500, -250, 250, -100, 100, 0, 0}}, 119000U, 121000U);
}

void testPlusMinus2msInputJitterRemainsBounded() {
    assertJitterBand({{-2000, 2000, -1500, 1500, -750, 750, 0, 0}}, 118000U, 122000U);
}

void testPlusMinus10msInputJitterRemainsLocked() {
    assertJitterBand({{-10000, 10000, -8000, 8000, -4000, 4000, 0, 0}}, 114000U, 126000U);
}

void testPlusMinus20PercentInputJitterRemainsLocked() {
    assertJitterBand({{-100000, 100000, -75000, 75000, -50000, 50000, -25000, 25000}}, 95000U, 155000U);
}

void testAlternating400And600msPeriodsRemainLocked() {
    Fixture fixture;
    fixture.begin();
    feedAlternatingPeriods(fixture, 400000U, 600000U, 128U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK(fixture.sync.filteredBpmMilli() >= 100000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 150000U);
}

void testAlternating250And750msPeriodsRemainLocked() {
    Fixture fixture;
    fixture.begin();
    feedAlternatingPeriods(fixture, 250000U, 750000U, 128U);
    CHECK(fixture.engine.snapshot().externalLocked);
    CHECK(fixture.sync.filteredBpmMilli() >= 80000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 240000U);
}

void testChaoticValidPeriodSequenceNeverProducesOutOfRangeTempo() {
    Fixture fixture;
    fixture.begin();
    constexpr std::array<std::uint32_t, 16U> periods{{
        300000U, 690000U, 410000U, 555000U, 350000U, 620000U, 480000U, 700000U,
        325000U, 590000U, 450000U, 675000U, 375000U, 530000U, 495000U, 640000U}};
    std::uint32_t timestamp = 1000U;
    injectRisingPulse(fixture, timestamp);
    for (std::uint32_t cycle = 0U; cycle < 32U; ++cycle) {
        for (const std::uint32_t period : periods) {
            timestamp += period;
            injectRisingPulse(fixture, timestamp);
            CHECK(fixture.engine.snapshot().externalLocked);
            CHECK(fixture.sync.filteredBpmMilli() >= 1000U);
            CHECK(fixture.sync.filteredBpmMilli() <= 999000U);
        }
    }
}

void testAbrupt60To180BpmChangeConvergesWithoutUnlock() {
    Fixture fixture;
    fixture.begin();
    feedPeriods(fixture, {1000000U, 1000000U, 1000000U, 1000000U});
    std::uint32_t timestamp = 4001000U;
    for (std::uint32_t i = 0U; i < 28U; ++i) {
        timestamp += 333333U;
        injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
    }
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), 180000U, 1000U);
}

void testAbrupt180To60BpmChangeConvergesWithoutUnlock() {
    Fixture fixture;
    fixture.begin();
    feedPeriods(fixture, {333333U, 333333U, 333333U, 333333U});
    std::uint32_t timestamp = 1334332U;
    for (std::uint32_t i = 0U; i < 28U; ++i) {
        timestamp += 1000000U;
        injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
    }
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), 60000U, 1000U);
}


void testLinearAccelerationFrom60To180BpmStaysLocked() {
    Fixture fixture; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    std::uint32_t previousFiltered = 0U;
    for (std::uint32_t i = 0U; i < 80U; ++i) {
        const std::uint32_t bpm = 60U + (120U * i) / 79U;
        timestamp += 60000000U / bpm;
        injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
        const std::uint32_t filtered = fixture.sync.filteredBpmMilli();
        CHECK(filtered >= 1000U && filtered <= 999000U);
        if (i > 12U) CHECK(filtered + 5000U >= previousFiltered);
        previousFiltered = filtered;
    }
    CHECK(fixture.sync.filteredBpmMilli() >= 165000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 185000U);
}

void testLinearDecelerationFrom180To60BpmStaysLocked() {
    Fixture fixture; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    std::uint32_t previousFiltered = 1000000U;
    for (std::uint32_t i = 0U; i < 80U; ++i) {
        const std::uint32_t bpm = 180U - (120U * i) / 79U;
        timestamp += 60000000U / bpm;
        injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
        const std::uint32_t filtered = fixture.sync.filteredBpmMilli();
        CHECK(filtered >= 1000U && filtered <= 999000U);
        if (i > 12U) CHECK(filtered <= previousFiltered + 5000U);
        previousFiltered = filtered;
    }
    CHECK(fixture.sync.filteredBpmMilli() >= 55000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 70000U);
}

void testSlowTempoDriftAround120BpmRemainsLockedAndBounded() {
    Fixture fixture; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    for (std::uint32_t i = 0U; i < 200U; ++i) {
        const std::int32_t offset = static_cast<std::int32_t>(i % 41U) - 20;
        const std::uint32_t bpm = static_cast<std::uint32_t>(120 + offset / 4);
        timestamp += 60000000U / bpm; injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
        CHECK(fixture.sync.filteredBpmMilli() >= 110000U);
        CHECK(fixture.sync.filteredBpmMilli() <= 130000U);
    }
}

void testRepeatedTempoStepsDoNotLoseLock() {
    Fixture fixture; fixture.begin();
    constexpr std::array<std::uint32_t, 8U> bpms{{120U, 90U, 150U, 60U, 180U, 75U, 200U, 120U}};
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    for (const std::uint32_t bpm : bpms) {
        for (std::uint32_t n = 0U; n < 16U; ++n) {
            timestamp += 60000000U / bpm; injectRisingPulse(fixture, timestamp);
            CHECK(fixture.engine.snapshot().externalLocked);
            CHECK(fixture.sync.filteredBpmMilli() >= 1000U);
            CHECK(fixture.sync.filteredBpmMilli() <= 999000U);
        }
    }
    CHECK_NEAR(fixture.sync.filteredBpmMilli(), 120000U, 5000U);
}

void testAccelerationWithAlternatingJitterRemainsLocked() {
    Fixture fixture; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    for (std::uint32_t i = 0U; i < 96U; ++i) {
        const std::uint32_t bpm = 80U + (80U * i) / 95U;
        const std::uint32_t nominal = 60000000U / bpm;
        const std::int32_t jitter = (i & 1U) == 0U ? -2000 : 2000;
        const std::int32_t interval = static_cast<std::int32_t>(nominal) + jitter;
        CHECK(interval > 0); timestamp += static_cast<std::uint32_t>(interval);
        injectRisingPulse(fixture, timestamp); CHECK(fixture.engine.snapshot().externalLocked);
    }
    CHECK(fixture.sync.filteredBpmMilli() >= 145000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 170000U);
}

void testTwentyFourPpqnTempoRampRemainsLocked() {
    Fixture fixture; fixture.state.externalSync.pulsesPerQuarterNote = 24U; fixture.begin();
    std::uint32_t timestamp = 1000U; injectRisingPulse(fixture, timestamp);
    for (std::uint32_t i = 0U; i < 120U; ++i) {
        const std::uint32_t bpm = 90U + (60U * i) / 119U;
        timestamp += 60000000U / (bpm * 24U); injectRisingPulse(fixture, timestamp);
        CHECK(fixture.engine.snapshot().externalLocked);
        CHECK(fixture.sync.filteredBpmMilli() >= 1000U);
        CHECK(fixture.sync.filteredBpmMilli() <= 999000U);
    }
    CHECK(fixture.sync.filteredBpmMilli() >= 135000U);
    CHECK(fixture.sync.filteredBpmMilli() <= 160000U);
}
