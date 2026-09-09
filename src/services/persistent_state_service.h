/**
 * @file persistent_state_service.h
 * @brief CRC-protected persistence for the current configuration and named user presets.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#pragma once

#include <array>
#include <cstddef>
#include <cstdint>

#include "domain/clock_types.h"
#include "hal/persistent_storage.h"

namespace clockfw::services {

/**
 * @brief Owns the durable representation of all user-editable clock configuration.
 *
 * One automatically maintained CURRENT record stores the complete application
 * configuration. Eight additional named user slots can be saved and loaded
 * explicitly. Records are serialized field-by-field so Flash data never depends
 * on C++ structure padding or compiler ABI details.
 *
 * The transport value is persisted as historical user intent, but boot safety is
 * stronger than persistence: ClockApplication always starts the runtime transport
 * in STOP and never auto-starts from a stored PLAY value.
 */
class PersistentStateService final {
public:
    /** Number of explicit user preset slots. */
    static constexpr std::uint8_t kUserPresetSlotCount = 8U;

    /** Maximum number of user-visible characters in one preset name. */
    static constexpr std::size_t kPresetNameLength = 16U;

    /**
     * @brief Constructs the service around the non-volatile storage HAL.
     * @param storage Storage boundary used for validated record reads and writes.
     */
    explicit PersistentStateService(hal::PersistentStorage& storage);

    /** @brief Loads and validates CURRENT plus all preset-slot metadata. */
    void begin();

    /**
     * @brief Restores the complete CURRENT configuration when a valid record exists.
     * @param state Destination state receiving the persisted configuration.
     * @return True when a valid current-state record was restored.
     */
    bool restoreCurrentState(ClockState& state) const;

    /**
     * @brief Schedules a wear-coalesced save of the complete current configuration.
     * @param state Current mutable application configuration.
     * @param nowMs Current monotonic time in milliseconds.
     *
     * The runtime boot-forced STOP value does not overwrite the user's previously
     * persisted transport preference unless requestTransportState() is called.
     */
    void requestCurrentState(const ClockState& state, std::uint32_t nowMs);

    /**
     * @brief Updates the transport value that belongs in the durable CURRENT record.
     * @param transport New transport state explicitly selected by the user.
     * @param nowMs Current monotonic time in milliseconds.
     */
    void requestTransportState(TransportState transport, std::uint32_t nowMs);

    /**
     * @brief Commits pending durable records when Flash writes are timing-safe.
     * @param nowMs Current monotonic time in milliseconds.
     * @param allowFlashCommit False while transport is PLAYING on STM32 targets.
     *
     * STM32F4 stalls Flash reads during erase/program operations. The application
     * therefore defers every physical persistence commit while gates are running.
     * Explicit preset saves are staged in RAM and are flushed as soon as transport
     * is PAUSED or STOPPED.
     */
    void service(std::uint32_t nowMs, bool allowFlashCommit = true);

    /** @brief Returns true when a valid CURRENT record was loaded or written. */
    bool hasStoredCurrentState() const;

    /**
     * @return Persisted user transport preference, or STOP when CURRENT is invalid.
     * @brief This value is informational and must never be interpreted as an auto-start request.
     */
    TransportState storedTransportState() const;

    /** @brief Compatibility alias retained for tests and prerelease callers. */
    bool hasStoredTransportState() const;

    /**
     * @brief Saves the complete current configuration into one named user slot.
     * @param slotIndex Zero-based slot index in the range 0..7.
     * @param name User-visible preset name; invalid characters are replaced with spaces.
     * @param state Configuration snapshot to persist.
     * @return True when the record was validated and staged for durable storage.
     */
    bool savePreset(std::uint8_t slotIndex, const char* name, const ClockState& state);

    /**
     * @brief Loads a complete user preset while preserving live transport state.
     * @param slotIndex Zero-based slot index in the range 0..7.
     * @param state Mutable application state receiving the stored configuration.
     * @return True when the requested preset exists and validates successfully.
     */
    bool loadPreset(std::uint8_t slotIndex, ClockState& state) const;

    /**
     * @brief Renames an existing user preset without changing its configuration.
     * @return True when the slot existed and the updated record was written.
     */
    bool renamePreset(std::uint8_t slotIndex, const char* name);

    /**
     * @brief Erases one logical preset slot by writing an invalid/empty record.
     * @return True when the slot index was valid and the clear operation succeeded.
     */
    bool clearPreset(std::uint8_t slotIndex);

    /** @brief Returns true when one zero-based user slot contains a valid preset. */
    bool presetExists(std::uint8_t slotIndex) const;

    /**
     * @brief Copies one preset name into a caller-owned null-terminated buffer.
     * @param slotIndex Zero-based preset slot.
     * @param destination Output text buffer.
     * @param destinationSize Output buffer size including terminator.
     */
    void presetName(
        std::uint8_t slotIndex,
        char* destination,
        std::size_t destinationSize) const;

private:
    /** Serialized field count for one complete ClockState payload. */
    static constexpr std::size_t kStatePayloadSize = 252U;

    /** CURRENT record size: header + state payload + CRC-32. */
    static constexpr std::size_t kCurrentRecordSize = 264U;

    /** Preset record size: header + 16-byte name + state payload + CRC-32. */
    static constexpr std::size_t kPresetRecordSize = 280U;

    /** Stable CURRENT record schema version. */
    static constexpr std::uint8_t kSchemaVersion = 6U;

    /** v5 schema used by alpha.57 before configurable external reset semantics. */
    static constexpr std::uint8_t kV5SchemaVersion = 5U;
    static constexpr std::size_t kV5StatePayloadSize = 251U;
    static constexpr std::size_t kV5CurrentRecordSize = 263U;
    static constexpr std::size_t kV5PresetRecordSize = 279U;

    /** Earlier v4 schema, before tempo limits and ONE CLOCK humanize were persisted. */
    static constexpr std::uint8_t kPreviousSchemaVersion = 4U;

    /** v4 payload and record sizes. */
    static constexpr std::size_t kPreviousStatePayloadSize = 245U;
    static constexpr std::size_t kPreviousCurrentRecordSize = 257U;
    static constexpr std::size_t kPreviousPresetRecordSize = 273U;

    /** Older v3 schema, before display-protection preferences were persisted. */
    static constexpr std::uint8_t kLegacySchemaVersion = 3U;

    /** v3 payload and record sizes. */
    static constexpr std::size_t kLegacyStatePayloadSize = 241U;
    static constexpr std::size_t kLegacyCurrentRecordSize = 253U;
    static constexpr std::size_t kLegacyPresetRecordSize = 269U;

    /** First byte offset of the automatically maintained CURRENT record. */
    static constexpr std::size_t kCurrentRecordOffset = 0U;

    /** First byte offset of the explicit user-preset area. */
    static constexpr std::size_t kPresetAreaOffset = kCurrentRecordSize;

    /** @brief Returns the byte offset for one preset slot. */
    static constexpr std::size_t presetOffset(const std::uint8_t slotIndex) {
        return kPresetAreaOffset + static_cast<std::size_t>(slotIndex) * kPresetRecordSize;
    }

    /** @brief Returns the byte offset used by one v5 preset slot. */
    static constexpr std::size_t v5PresetOffset(const std::uint8_t slotIndex) {
        return kV5CurrentRecordSize +
            static_cast<std::size_t>(slotIndex) * kV5PresetRecordSize;
    }

    /** @brief Returns the byte offset used by one v4 preset slot. */
    static constexpr std::size_t previousPresetOffset(const std::uint8_t slotIndex) {
        return kPreviousCurrentRecordSize +
            static_cast<std::size_t>(slotIndex) * kPreviousPresetRecordSize;
    }

    /** @brief Returns the byte offset used by one v3 preset slot. */
    static constexpr std::size_t legacyPresetOffset(const std::uint8_t slotIndex) {
        return kLegacyCurrentRecordSize +
            static_cast<std::size_t>(slotIndex) * kLegacyPresetRecordSize;
    }

    /** @brief Serializes ClockState into a stable field-by-field byte payload. */
    static std::array<std::uint8_t, kStatePayloadSize> serializeState(const ClockState& state);

    /** @brief Validates and deserializes one state payload. */
    static bool deserializeState(
        const std::array<std::uint8_t, kStatePayloadSize>& payload,
        ClockState& state);

    /** @brief Serializes the CURRENT record including metadata and CRC. */
    static std::array<std::uint8_t, kCurrentRecordSize> serializeCurrentRecord(
        const ClockState& state);

    /** @brief Validates and extracts one CURRENT record. */
    static bool deserializeCurrentRecord(
        const std::array<std::uint8_t, kCurrentRecordSize>& record,
        ClockState& state);

    /** @brief Serializes one named user preset record including metadata and CRC. */
    static std::array<std::uint8_t, kPresetRecordSize> serializePresetRecord(
        const char* name,
        const ClockState& state);

    /** @brief Validates and extracts one named user preset record. */
    static bool deserializePresetRecord(
        const std::array<std::uint8_t, kPresetRecordSize>& record,
        char* name,
        ClockState& state);

    /** @brief Upgrades a v5 state payload by injecting the factory RST interpretation. */
    static bool deserializeV5State(
        const std::array<std::uint8_t, kV5StatePayloadSize>& payload,
        ClockState& state);

    /** @brief Validates and upgrades one v5 CURRENT record. */
    static bool deserializeV5CurrentRecord(
        const std::array<std::uint8_t, kV5CurrentRecordSize>& record,
        ClockState& state);

    /** @brief Validates and upgrades one named v5 preset record. */
    static bool deserializeV5PresetRecord(
        const std::array<std::uint8_t, kV5PresetRecordSize>& record,
        char* name,
        ClockState& state);

    /** @brief Validates and upgrades one v4 CURRENT record to the current in-memory state. */
    static bool deserializePreviousCurrentRecord(
        const std::array<std::uint8_t, kPreviousCurrentRecordSize>& record,
        ClockState& state);

    /** @brief Validates and upgrades one named v4 preset record. */
    static bool deserializePreviousPresetRecord(
        const std::array<std::uint8_t, kPreviousPresetRecordSize>& record,
        char* name,
        ClockState& state);

    /** @brief Upgrades a v4 state payload by injecting tempo-range and humanize defaults. */
    static bool deserializePreviousState(
        const std::array<std::uint8_t, kPreviousStatePayloadSize>& payload,
        ClockState& state);

    /** @brief Validates and upgrades one v3 CURRENT record to the current in-memory state. */
    static bool deserializeLegacyCurrentRecord(
        const std::array<std::uint8_t, kLegacyCurrentRecordSize>& record,
        ClockState& state);

    /** @brief Validates and upgrades one named v3 preset record. */
    static bool deserializeLegacyPresetRecord(
        const std::array<std::uint8_t, kLegacyPresetRecordSize>& record,
        char* name,
        ClockState& state);

    /** @brief Upgrades a v3 state payload by injecting current display-protection defaults. */
    static bool deserializeLegacyState(
        const std::array<std::uint8_t, kLegacyStatePayloadSize>& payload,
        ClockState& state);

    /** @brief Calculates standard reflected CRC-32 over a bounded byte sequence. */
    static std::uint32_t calculateCrc32(const std::uint8_t* data, std::size_t size);

    /** @brief Returns true when all serialized enum/range values form a valid ClockState. */
    static bool isStateValid(const ClockState& state);

    /** @brief Converts a user name into the fixed 16-character storage representation. */
    static void normalizePresetName(const char* source, char* destination);

    /** @brief Compares two complete states without relying on structure padding. */
    static bool statesEqual(const ClockState& first, const ClockState& second);

    hal::PersistentStorage& storage_;
    ClockState storedCurrentState_{};
    ClockState pendingCurrentState_{};
    std::array<std::array<char, kPresetNameLength + 1U>, kUserPresetSlotCount> presetNames_{};
    std::array<bool, kUserPresetSlotCount> presetValid_{};
    TransportState persistedTransportPreference_ = TransportState::Stopped;
    std::uint32_t pendingSinceMs_ = 0U;
    bool hasStoredCurrentState_ = false;
    bool writePending_ = false;
    std::array<std::uint8_t, kPresetRecordSize> pendingPresetRecord_{};
    std::array<char, kPresetNameLength + 1U> pendingPresetName_{};
    std::uint8_t pendingPresetSlot_ = 0U;
    bool pendingPresetWrite_ = false;
    bool pendingPresetWillExist_ = false;
};

static_assert(
    263U + PersistentStateService::kUserPresetSlotCount * 279U <=
        hal::PersistentStorage::kCapacityBytes,
    "Persistent storage capacity is too small for CURRENT plus all user presets.");

}  // namespace clockfw::services
