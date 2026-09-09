/**
 * @file persistent_state_codec.cpp
 * @brief Stable field-by-field serialization, validation, and CRC for persisted clock state.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "services/persistent_state_service.h"
#include "services/persistent_state_validation.h"


#include <algorithm>
#include <cstring>

#include "config.h"

namespace clockfw::services {
namespace {

/** Four-byte CURRENT magic value "CUR6" stored little-endian. */
constexpr std::uint32_t kCurrentMagic = 0x36525543UL;

/** Four-byte preset magic value "PRE6" stored little-endian. */
constexpr std::uint32_t kPresetMagic = 0x36455250UL;

/** Reflected CRC-32 polynomial used by Ethernet/ZIP and many embedded formats. */
constexpr std::uint32_t kCrcPolynomial = 0xEDB88320UL;

/** CRC-32 initialization/final-XOR value. */
constexpr std::uint32_t kCrcInitialValue = 0xFFFFFFFFUL;

/** Fixed byte offset at which CURRENT payload begins. */
constexpr std::size_t kCurrentPayloadOffset = 8U;

/** Fixed byte offset at which one preset name begins. */
constexpr std::size_t kPresetNameOffset = 8U;

/** Fixed byte offset at which one preset state payload begins. */
constexpr std::size_t kPresetPayloadOffset = kPresetNameOffset +
    PersistentStateService::kPresetNameLength;

/** Returns true for characters supported by the on-device high-score name editor. */
bool isAllowedPresetCharacter(const char character) {
    return character == ' ' || character == '-' || character == '_' ||
        (character >= 'A' && character <= 'Z') ||
        (character >= '0' && character <= '9');
}

/** Writes one 16-bit little-endian integer into a byte array. */
void writeUint16Le(std::uint8_t* const destination, const std::uint16_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
}

/** Reads one 16-bit little-endian integer from a byte array. */
std::uint16_t readUint16Le(const std::uint8_t* const source) {
    return static_cast<std::uint16_t>(source[0]) |
        static_cast<std::uint16_t>(static_cast<std::uint16_t>(source[1]) << 8U);
}

/** Writes one 32-bit little-endian integer into a byte array. */
void writeUint32Le(std::uint8_t* const destination, const std::uint32_t value) {
    destination[0] = static_cast<std::uint8_t>(value & 0xFFU);
    destination[1] = static_cast<std::uint8_t>((value >> 8U) & 0xFFU);
    destination[2] = static_cast<std::uint8_t>((value >> 16U) & 0xFFU);
    destination[3] = static_cast<std::uint8_t>((value >> 24U) & 0xFFU);
}

/** Reads one 32-bit little-endian integer from a byte array. */
std::uint32_t readUint32Le(const std::uint8_t* const source) {
    return static_cast<std::uint32_t>(source[0]) |
        (static_cast<std::uint32_t>(source[1]) << 8U) |
        (static_cast<std::uint32_t>(source[2]) << 16U) |
        (static_cast<std::uint32_t>(source[3]) << 24U);
}

/** Writes one 64-bit little-endian integer into a byte array. */
void writeUint64Le(std::uint8_t* const destination, const std::uint64_t value) {
    for (std::uint8_t byteIndex = 0U; byteIndex < 8U; ++byteIndex) {
        destination[byteIndex] = static_cast<std::uint8_t>(value >> (8U * byteIndex));
    }
}

/** Reads one 64-bit little-endian integer from a byte array. */
std::uint64_t readUint64Le(const std::uint8_t* const source) {
    std::uint64_t value = 0U;
    for (std::uint8_t byteIndex = 0U; byteIndex < 8U; ++byteIndex) {
        value |= static_cast<std::uint64_t>(source[byteIndex]) << (8U * byteIndex);
    }
    return value;
}

/** Small cursor used to keep field-by-field serialization auditable. */
class ByteWriter final {
public:
    explicit ByteWriter(std::uint8_t* const destination) : destination_(destination) {}

    void write8(const std::uint8_t value) { destination_[position_++] = value; }
    void write16(const std::uint16_t value) {
        writeUint16Le(destination_ + position_, value);
        position_ += 2U;
    }
    void write64(const std::uint64_t value) {
        writeUint64Le(destination_ + position_, value);
        position_ += 8U;
    }
    std::size_t position() const { return position_; }

private:
    std::uint8_t* destination_;
    std::size_t position_ = 0U;
};

/** Matching cursor used by the stable serialized state schema. */
class ByteReader final {
public:
    explicit ByteReader(const std::uint8_t* const source) : source_(source) {}

    std::uint8_t read8() { return source_[position_++]; }
    std::uint16_t read16() {
        const std::uint16_t value = readUint16Le(source_ + position_);
        position_ += 2U;
        return value;
    }
    std::uint64_t read64() {
        const std::uint64_t value = readUint64Le(source_ + position_);
        position_ += 8U;
        return value;
    }
    std::size_t position() const { return position_; }

private:
    const std::uint8_t* source_;
    std::size_t position_ = 0U;
};

}  // namespace

std::array<std::uint8_t, PersistentStateService::kStatePayloadSize>
PersistentStateService::serializeState(const ClockState& state) {
    std::array<std::uint8_t, kStatePayloadSize> payload{};
    ByteWriter writer(payload.data());

    writer.write16(state.bpm);
    writer.write16(state.tempoRange.minimumBpm);
    writer.write16(state.tempoRange.maximumBpm);
    writer.write8(state.masterMeter.beats);
    writer.write8(state.masterMeter.unit);
    writer.write8(static_cast<std::uint8_t>(state.transport));
    writer.write8(static_cast<std::uint8_t>(state.source));
    writer.write8(static_cast<std::uint8_t>(state.operatingMode));
    writer.write8(state.externalSync.pulsesPerQuarterNote);
    writer.write8(static_cast<std::uint8_t>(state.externalSync.edge));
    writer.write8(static_cast<std::uint8_t>(state.externalSync.lossMode));
    writer.write8(static_cast<std::uint8_t>(state.externalSync.resetMode));
    writer.write16(state.externalSync.glitchFilterUs);
    writer.write16(state.externalSync.timeoutMs);

    writer.write8(static_cast<std::uint8_t>(state.unifiedClock.rate.mode));
    writer.write8(state.unifiedClock.rate.factor);
    writer.write8(state.unifiedClock.rate.numerator);
    writer.write8(state.unifiedClock.rate.denominator);
    writer.write8(state.unifiedClock.swingPercent);
    writer.write16(state.unifiedClock.gateLengthMs);
    writer.write8(state.unifiedClock.phasePercent);
    writer.write16(state.unifiedClock.humanizeUs);
    writer.write8(static_cast<std::uint8_t>(state.dividerBank.bank));
    writer.write16(state.dividerBank.gateLengthMs);
    writer.write8(static_cast<std::uint8_t>(state.display.screensaverMode));
    writer.write8(state.display.screensaverAfterMinutes);
    writer.write8(state.display.dimAfterMinutes);
    writer.write8(state.display.offAfterMinutes);

    for (const ChannelConfig& channel : state.channels) {
        writer.write8(static_cast<std::uint8_t>(channel.common.mode));
        writer.write8(static_cast<std::uint8_t>(channel.common.rate.mode));
        writer.write8(channel.common.rate.factor);
        writer.write8(channel.common.rate.numerator);
        writer.write8(channel.common.rate.denominator);
        writer.write8(channel.common.swingPercent);
        writer.write8(channel.common.probabilityPercent);
        writer.write16(channel.common.gateLengthMs);
        writer.write8(channel.common.phasePercent);
        writer.write8(static_cast<std::uint8_t>(channel.common.resetMode));
        writer.write8(channel.common.muted ? 1U : 0U);
        writer.write8(channel.clock.meter.beats);
        writer.write8(channel.clock.meter.unit);
        writer.write8(channel.euclid.steps);
        writer.write8(channel.euclid.hits);
        writer.write8(channel.euclid.rotation);
        writer.write8(channel.sequencer.length);
        writer.write8(channel.sequencer.rotation);
        writer.write64(channel.sequencer.pattern);
    }

    // A schema-size mismatch is a programmer error and should fail at compile/test time.
    (void)writer.position();
    return payload;
}

bool PersistentStateService::deserializeState(
    const std::array<std::uint8_t, kStatePayloadSize>& payload,
    ClockState& state) {
    ByteReader reader(payload.data());
    ClockState candidate{};

    candidate.bpm = reader.read16();
    candidate.tempoRange.minimumBpm = reader.read16();
    candidate.tempoRange.maximumBpm = reader.read16();
    candidate.masterMeter.beats = reader.read8();
    candidate.masterMeter.unit = reader.read8();
    candidate.transport = static_cast<TransportState>(reader.read8());
    candidate.source = static_cast<ClockSource>(reader.read8());
    candidate.operatingMode = static_cast<OperatingMode>(reader.read8());
    candidate.externalSync.pulsesPerQuarterNote = reader.read8();
    candidate.externalSync.edge = static_cast<SyncEdge>(reader.read8());
    candidate.externalSync.lossMode = static_cast<SyncLossMode>(reader.read8());
    candidate.externalSync.resetMode = static_cast<ExternalResetMode>(reader.read8());
    candidate.externalSync.glitchFilterUs = reader.read16();
    candidate.externalSync.timeoutMs = reader.read16();

    candidate.unifiedClock.rate.mode = static_cast<ClockRatioMode>(reader.read8());
    candidate.unifiedClock.rate.factor = reader.read8();
    candidate.unifiedClock.rate.numerator = reader.read8();
    candidate.unifiedClock.rate.denominator = reader.read8();
    candidate.unifiedClock.swingPercent = reader.read8();
    candidate.unifiedClock.gateLengthMs = reader.read16();
    candidate.unifiedClock.phasePercent = reader.read8();
    candidate.unifiedClock.humanizeUs = reader.read16();
    candidate.dividerBank.bank = static_cast<DividerBank>(reader.read8());
    candidate.dividerBank.gateLengthMs = reader.read16();
    candidate.display.screensaverMode = static_cast<ScreensaverMode>(reader.read8());
    candidate.display.screensaverAfterMinutes = reader.read8();
    candidate.display.dimAfterMinutes = reader.read8();
    candidate.display.offAfterMinutes = reader.read8();

    for (ChannelConfig& channel : candidate.channels) {
        channel.common.mode = static_cast<ChannelMode>(reader.read8());
        channel.common.rate.mode = static_cast<ClockRatioMode>(reader.read8());
        channel.common.rate.factor = reader.read8();
        channel.common.rate.numerator = reader.read8();
        channel.common.rate.denominator = reader.read8();
        channel.common.swingPercent = reader.read8();
        channel.common.probabilityPercent = reader.read8();
        channel.common.gateLengthMs = reader.read16();
        channel.common.phasePercent = reader.read8();
        channel.common.resetMode = static_cast<ResetMode>(reader.read8());
        channel.common.muted = reader.read8() != 0U;
        channel.clock.meter.beats = reader.read8();
        channel.clock.meter.unit = reader.read8();
        channel.euclid.steps = reader.read8();
        channel.euclid.hits = reader.read8();
        channel.euclid.rotation = reader.read8();
        channel.sequencer.length = reader.read8();
        channel.sequencer.rotation = reader.read8();
        channel.sequencer.pattern = reader.read64();
    }

    if (reader.position() != kStatePayloadSize || !isStateValid(candidate)) {
        return false;
    }
    state = candidate;
    return true;
}

std::array<std::uint8_t, PersistentStateService::kCurrentRecordSize>
PersistentStateService::serializeCurrentRecord(const ClockState& state) {
    std::array<std::uint8_t, kCurrentRecordSize> record{};
    writeUint32Le(record.data(), kCurrentMagic);
    record[4] = kSchemaVersion;
    record[5] = 0U;
    writeUint16Le(record.data() + 6U, static_cast<std::uint16_t>(kStatePayloadSize));

    const auto payload = serializeState(state);
    std::copy(payload.begin(), payload.end(), record.begin() + kCurrentPayloadOffset);
    writeUint32Le(
        record.data() + kCurrentRecordSize - 4U,
        calculateCrc32(record.data(), kCurrentRecordSize - 4U));
    return record;
}

bool PersistentStateService::deserializeCurrentRecord(
    const std::array<std::uint8_t, kCurrentRecordSize>& record,
    ClockState& state) {
    if (readUint32Le(record.data()) != kCurrentMagic ||
        record[4] != kSchemaVersion ||
        readUint16Le(record.data() + 6U) != kStatePayloadSize ||
        readUint32Le(record.data() + kCurrentRecordSize - 4U) !=
            calculateCrc32(record.data(), kCurrentRecordSize - 4U)) {
        return false;
    }

    std::array<std::uint8_t, kStatePayloadSize> payload{};
    std::copy_n(record.begin() + kCurrentPayloadOffset, kStatePayloadSize, payload.begin());
    return deserializeState(payload, state);
}

std::array<std::uint8_t, PersistentStateService::kPresetRecordSize>
PersistentStateService::serializePresetRecord(
    const char* const name,
    const ClockState& state) {
    std::array<std::uint8_t, kPresetRecordSize> record{};
    writeUint32Le(record.data(), kPresetMagic);
    record[4] = kSchemaVersion;
    record[5] = 1U;
    writeUint16Le(record.data() + 6U, static_cast<std::uint16_t>(kStatePayloadSize));

    char normalizedName[kPresetNameLength + 1U]{};
    normalizePresetName(name, normalizedName);
    std::copy_n(
        reinterpret_cast<const std::uint8_t*>(normalizedName),
        kPresetNameLength,
        record.begin() + kPresetNameOffset);

    const auto payload = serializeState(state);
    std::copy(payload.begin(), payload.end(), record.begin() + kPresetPayloadOffset);
    writeUint32Le(
        record.data() + kPresetRecordSize - 4U,
        calculateCrc32(record.data(), kPresetRecordSize - 4U));
    return record;
}

bool PersistentStateService::deserializePresetRecord(
    const std::array<std::uint8_t, kPresetRecordSize>& record,
    char* const name,
    ClockState& state) {
    if (readUint32Le(record.data()) != kPresetMagic ||
        record[4] != kSchemaVersion ||
        record[5] != 1U ||
        readUint16Le(record.data() + 6U) != kStatePayloadSize ||
        readUint32Le(record.data() + kPresetRecordSize - 4U) !=
            calculateCrc32(record.data(), kPresetRecordSize - 4U)) {
        return false;
    }

    for (std::size_t index = 0U; index < kPresetNameLength; ++index) {
        const char character = static_cast<char>(record[kPresetNameOffset + index]);
        if (!isAllowedPresetCharacter(character)) {
            return false;
        }
        name[index] = character;
    }
    name[kPresetNameLength] = '\0';

    std::array<std::uint8_t, kStatePayloadSize> payload{};
    std::copy_n(record.begin() + kPresetPayloadOffset, kStatePayloadSize, payload.begin());
    return deserializeState(payload, state);
}


std::uint32_t PersistentStateService::calculateCrc32(
    const std::uint8_t* const data,
    const std::size_t size) {
    std::uint32_t crc = kCrcInitialValue;
    for (std::size_t index = 0U; index < size; ++index) {
        crc ^= data[index];
        for (std::uint8_t bit = 0U; bit < 8U; ++bit) {
            const bool leastSignificantBitSet = (crc & 1U) != 0U;
            crc >>= 1U;
            if (leastSignificantBitSet) {
                crc ^= kCrcPolynomial;
            }
        }
    }
    return crc ^ kCrcInitialValue;
}


bool PersistentStateService::isStateValid(const ClockState& state) {
    return isPersistentStateValid(state);
}

void PersistentStateService::normalizePresetName(
    const char* const source,
    char* const destination) {
    for (std::size_t index = 0U; index < kPresetNameLength; ++index) {
        const char sourceCharacter = source != nullptr ? source[index] : '\0';
        if (sourceCharacter == '\0') {
            std::fill(destination + index, destination + kPresetNameLength, ' ');
            break;
        }

        char normalizedCharacter = sourceCharacter;
        if (normalizedCharacter >= 'a' && normalizedCharacter <= 'z') {
            normalizedCharacter = static_cast<char>(normalizedCharacter - 'a' + 'A');
        }
        destination[index] = isAllowedPresetCharacter(normalizedCharacter)
            ? normalizedCharacter
            : ' ';
    }
    destination[kPresetNameLength] = '\0';
}

bool PersistentStateService::statesEqual(
    const ClockState& first,
    const ClockState& second) {
    return serializeState(first) == serializeState(second);
}


}  // namespace clockfw::services
