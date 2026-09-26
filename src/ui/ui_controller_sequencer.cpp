/**
 * @file ui_controller_sequencer.cpp
 * @brief Sequencer 2.0 pattern selection, editing, and context operations.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */

#include "ui/ui_controller.h"

#include <algorithm>

namespace clockfw::ui {

const SequencerPatternV2* UiController::activeSequencerPattern() const {
    if (sequencerPatternStore_ == nullptr || navigation_.selectedChannel >= kChannelCount) {
        return nullptr;
    }
    const std::uint8_t slot = sequencerPatternStore_->activeSlot(navigation_.selectedChannel);
    return &sequencerPatternStore_->pattern(navigation_.selectedChannel, slot);
}

void UiController::synchronizeActiveSequencerPattern(const bool rescheduleChannel) {
    const SequencerPatternV2* const pattern = activeSequencerPattern();
    if (pattern == nullptr) {
        return;
    }
    engine_.updateSequencerPattern(navigation_.selectedChannel, *pattern, rescheduleChannel);
    synchronizeActiveSequencerStepMetadata();
}

void UiController::synchronizeActiveSequencerStepMetadata() {
    if (sequencerStepStore_ == nullptr || sequencerPatternStore_ == nullptr ||
        navigation_.selectedChannel >= kChannelCount) {
        return;
    }
    const std::uint8_t channel = navigation_.selectedChannel;
    const std::uint8_t slot = sequencerPatternStore_->activeSlot(channel);
    std::array<SequencerStepMetadataWord, kSequencerMaximumSteps> metadata{};
    sequencerStepStore_->loadPatternWords(channel, slot, metadata);
    engine_.updateSequencerStepMetadata(channel, metadata);
}

void UiController::openSequencerStepEditor() {
    if (sequencerPatternStore_ == nullptr || sequencerStepStore_ == nullptr ||
        navigation_.selectedChannel >= kChannelCount) {
        return;
    }
    const SequencerPatternV2* const pattern = activeSequencerPattern();
    const std::uint8_t absoluteStep = static_cast<std::uint8_t>(
        navigation_.sequencerPage * 8U + navigation_.sequencerCursor);
    if (pattern == nullptr || absoluteStep >= pattern->length) {
        return;
    }
    navigation_.screen = Screen::SequencerStepEditor;
    navigation_.cursor = 0U;
    navigation_.editing = false;
    invalidate();
}

SequencerStepMetadata UiController::activeSequencerStepMetadata() const {
    if (sequencerPatternStore_ == nullptr || sequencerStepStore_ == nullptr ||
        navigation_.selectedChannel >= kChannelCount) {
        return SequencerStepMetadata{};
    }
    const std::uint8_t channel = navigation_.selectedChannel;
    const std::uint8_t slot = sequencerPatternStore_->activeSlot(channel);
    const std::uint8_t absoluteStep = static_cast<std::uint8_t>(
        navigation_.sequencerPage * 8U + navigation_.sequencerCursor);
    return sequencerStepStore_->metadata(channel, slot, absoluteStep);
}

void UiController::adjustSequencerStepMetadata(
    const std::int8_t delta,
    const std::uint32_t nowMs) {
    if (delta == 0 || sequencerPatternStore_ == nullptr || sequencerStepStore_ == nullptr ||
        navigation_.selectedChannel >= kChannelCount || navigation_.cursor == 0U) {
        return;
    }
    const std::uint8_t channel = navigation_.selectedChannel;
    const std::uint8_t slot = sequencerPatternStore_->activeSlot(channel);
    const SequencerPatternV2& pattern = sequencerPatternStore_->pattern(channel, slot);
    const std::uint8_t absoluteStep = static_cast<std::uint8_t>(
        navigation_.sequencerPage * 8U + navigation_.sequencerCursor);
    if (absoluteStep >= pattern.length) {
        return;
    }

    SequencerStepMetadata metadata = sequencerStepStore_->metadata(channel, slot, absoluteStep);
    if (navigation_.cursor == 1U) {
        metadata.probabilityPercent = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(metadata.probabilityPercent) + delta, 0, 100));
    } else if (navigation_.cursor == 2U) {
        const int next = clampInt(
            static_cast<int>(metadata.gateProfile) + delta,
            static_cast<int>(SequencerGateProfile::Default),
            static_cast<int>(SequencerGateProfile::Gate100Ms));
        metadata.gateProfile = static_cast<SequencerGateProfile>(next);
    } else if (navigation_.cursor == 3U) {
        metadata.ratchetCount = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(metadata.ratchetCount) + delta, 1,
            static_cast<int>(kSequencerMaximumRatchetCount)));
        if (metadata.ratchetCount > 1U) {
            metadata.tie = false;
        }
    } else if (navigation_.cursor == 4U) {
        if (pattern.direction == SequencerPlayDirection::Random) {
            return;
        }
        const int next = clampInt(static_cast<int>(metadata.tie ? 1 : 0) + delta, 0, 1);
        metadata.tie = next != 0;
        if (metadata.tie) {
            metadata.ratchetCount = 1U;
        }
    } else {
        return;
    }

    if (sequencerStepStore_->updateMetadata(channel, slot, absoluteStep, metadata, nowMs)) {
        synchronizeActiveSequencerStepMetadata();
        invalidate();
    }
}

void UiController::toggleSequencerStepGateFromDetail(const std::uint32_t nowMs) {
    toggleSequencerStepV2(nowMs);
}

void UiController::adjustSequencerV2(
    const std::int8_t delta,
    const std::uint32_t nowMs) {
    if (sequencerPatternStore_ == nullptr || delta == 0 ||
        navigation_.selectedChannel >= kChannelCount) {
        return;
    }

    const std::uint8_t channel = navigation_.selectedChannel;
    std::uint8_t slot = sequencerPatternStore_->activeSlot(channel);
    if (navigation_.cursor == 0U) {
        const int nextSlot = clampInt(
            static_cast<int>(slot) + delta,
            0,
            static_cast<int>(kSequencerPatternSlotsPerChannel - 1U));
        slot = static_cast<std::uint8_t>(nextSlot);
        if (sequencerPatternStore_->setActiveSlot(channel, slot, nowMs)) {
            synchronizeActiveSequencerPattern(true);
            navigation_.sequencerPage = 0U;
            navigation_.sequencerCursor = 0U;
            invalidate();
        }
        return;
    }

    SequencerPatternV2 pattern = sequencerPatternStore_->pattern(channel, slot);
    bool changed = false;
    if (navigation_.cursor == 1U) {
        pattern.length = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(pattern.length) + delta, 1,
            static_cast<int>(kSequencerMaximumSteps)));
        if (pattern.rotation >= pattern.length) {
            pattern.rotation = 0U;
        }
        clampSequencerPattern(pattern);
        changed = true;
    } else if (navigation_.cursor == 2U) {
        pattern.rotation = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(pattern.rotation) + delta,
            0,
            static_cast<int>(pattern.length - 1U)));
        changed = true;
    } else if (navigation_.cursor == 3U) {
        const int next = clampInt(
            static_cast<int>(pattern.direction) + delta,
            static_cast<int>(SequencerPlayDirection::Forward),
            static_cast<int>(SequencerPlayDirection::Random));
        pattern.direction = static_cast<SequencerPlayDirection>(next);
        changed = true;
    } else if (navigation_.cursor == 4U) {
        const int next = clampInt(
            static_cast<int>(pattern.loopMode) + delta,
            static_cast<int>(SequencerLoopMode::Loop),
            static_cast<int>(SequencerLoopMode::Once));
        pattern.loopMode = static_cast<SequencerLoopMode>(next);
        changed = true;
    }

    if (changed && sequencerPatternStore_->updatePattern(channel, slot, pattern, nowMs)) {
        engine_.updateSequencerPattern(channel, pattern, true);
        invalidate();
    }
}

void UiController::toggleSequencerStepV2(const std::uint32_t nowMs) {
    if (sequencerPatternStore_ == nullptr || navigation_.selectedChannel >= kChannelCount) {
        return;
    }
    const std::uint8_t channel = navigation_.selectedChannel;
    const std::uint8_t slot = sequencerPatternStore_->activeSlot(channel);
    const SequencerPatternV2& pattern = sequencerPatternStore_->pattern(channel, slot);
    const std::uint8_t absoluteStep = static_cast<std::uint8_t>(
        navigation_.sequencerPage * 8U + navigation_.sequencerCursor);
    if (absoluteStep >= pattern.length) {
        return;
    }
    const bool enabled = !sequencerPatternGate(pattern, absoluteStep);
    if (sequencerPatternStore_->setGate(channel, slot, absoluteStep, enabled, nowMs)) {
        synchronizeActiveSequencerPattern(false);
        invalidate();
    }
}

bool UiController::executeSequencerPatternCommandV2(
    const std::uint8_t rowIndex,
    const std::uint32_t nowMs) {
    if (sequencerPatternStore_ == nullptr || navigation_.selectedChannel >= kChannelCount) {
        return false;
    }
    const std::uint8_t channel = navigation_.selectedChannel;
    const std::uint8_t slot = sequencerPatternStore_->activeSlot(channel);
    SequencerPatternV2 pattern = sequencerPatternStore_->pattern(channel, slot);

    if (rowIndex == 3U) {
        sequencerPatternClipboard_ = pattern;
        sequencerPatternClipboardValid_ = true;
        if (sequencerStepStore_ != nullptr) {
            sequencerStepStore_->loadPatternWords(channel, slot, sequencerStepClipboard_);
            sequencerStepClipboardValid_ = true;
        } else {
            sequencerStepClipboard_.fill(0U);
            sequencerStepClipboardValid_ = false;
        }
        return true;
    }
    if (rowIndex == 4U) {
        if (!sequencerPatternClipboardValid_) {
            return false;
        }
        if (sequencerStepStore_ != nullptr && sequencerStepClipboardValid_ &&
            !sequencerStepStore_->replacePatternWords(
                channel, slot, sequencerStepClipboard_, nowMs)) {
            return false;
        }
        pattern = sequencerPatternClipboard_;
    } else if (rowIndex <= 2U) {
        for (std::uint8_t step = 0U; step < pattern.length; ++step) {
            bool enabled = false;
            if (rowIndex == 0U) {
                enabled = !sequencerPatternGate(pattern, step);
            } else if (rowIndex == 2U) {
                enabled = (step % 2U) == 0U;
            }
            (void)setSequencerPatternGate(pattern, step, enabled);
        }
        clampSequencerPattern(pattern);
        if (rowIndex == 1U && sequencerStepStore_ != nullptr &&
            !sequencerStepStore_->clearPattern(channel, slot, nowMs)) {
            return false;
        }
    } else {
        return false;
    }

    if (!sequencerPatternStore_->updatePattern(channel, slot, pattern, nowMs)) {
        return false;
    }
    engine_.updateSequencerPattern(channel, pattern, false);
    synchronizeActiveSequencerStepMetadata();
    const std::uint8_t lastPage = static_cast<std::uint8_t>((pattern.length - 1U) / 8U);
    if (navigation_.sequencerPage > lastPage) {
        navigation_.sequencerPage = lastPage;
        navigation_.sequencerCursor = 0U;
    }
    return true;
}

}  // namespace clockfw::ui
