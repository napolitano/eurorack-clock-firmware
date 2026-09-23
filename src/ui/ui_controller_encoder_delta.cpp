/**
 * @file ui_controller_encoder_delta.cpp
 * @brief Encoder-turn routing including Custom Groove editing gestures.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "ui/ui_controller_encoder_delta.h"

#include "services/template_service.h"
#include "ui/menu_model.h"
#include "ui/mode_functions.h"

namespace clockfw::ui {

void UiController::handleEncoderDelta(
    const std::int8_t delta,
    const bool tapPressed,
    const bool transportPressed) {
    if (navigation_.screen == Screen::GrooveEditor) {
        if (transportPressed) {
            transportPressConsumedByGrooveZoom_ = true;
            adjustGrooveZoom(delta);
        } else if (tapPressed) {
            adjustGrooveOffset(delta, true);
        } else {
            adjustGrooveOffset(delta, false);
        }
        return;
    }

    if (navigation_.screen == Screen::GrooveRecorder) {
        if (transportPressed) {
            transportPressConsumedByGrooveZoom_ = true;
            adjustGrooveZoom(delta);
        }
        return;
    }

    if (navigation_.screen == Screen::GrooveSlots) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta, 0,
            static_cast<int>(kCustomGrooveSlotCount - 1U)));
        if (navigation_.cursor < navigation_.scrollOffset) navigation_.scrollOffset = navigation_.cursor;
        if (navigation_.cursor >= navigation_.scrollOffset + 5U) {
            navigation_.scrollOffset = static_cast<std::uint8_t>(navigation_.cursor - 4U);
        }
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::GrooveNameEntry) {
        adjustGrooveNameCharacter(delta);
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::GrooveOverwriteConfirm ||
        navigation_.screen == Screen::GrooveDeleteConfirm ||
        navigation_.screen == Screen::GrooveDiscardConfirm) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta, 0, 1));
        invalidate();
        return;
    }
    if (navigation_.screen == Screen::ChannelQuickSelect && tapPressed) {
        if (!modeTapTurnActive_) {
            if (state_.operatingMode == OperatingMode::Independent) {
                navigation_.selectedChannel = navigation_.cursor;
            }
            openModeSelect(Screen::ChannelQuickSelect);
            modeTapTurnActive_ = true;
        }
        const int optionCount = static_cast<int>(kModeFunctions.size());
        navigation_.cursor = static_cast<std::uint8_t>(
            (static_cast<int>(navigation_.cursor) + optionCount + delta) % optionCount);
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::ChannelQuickSelect) {
        if (state_.operatingMode == OperatingMode::Independent) {
            navigation_.cursor = static_cast<std::uint8_t>(
                (static_cast<int>(navigation_.cursor) + static_cast<int>(kChannelCount) + delta) %
                static_cast<int>(kChannelCount));
            invalidate();
        }
        return;
    }

    if (navigation_.screen == Screen::Performance) {
        settingsEditor_.changeMasterTempo(delta);
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::ModeSelect) {
        const int optionCount = static_cast<int>(kModeFunctions.size());
        navigation_.cursor = static_cast<std::uint8_t>(
            (static_cast<int>(navigation_.cursor) + optionCount + delta) % optionCount);
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::Templates) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta,
            0,
            static_cast<int>(services::TemplateService::kTemplateCount - 1U)));
        if (navigation_.cursor < navigation_.scrollOffset) {
            navigation_.scrollOffset = navigation_.cursor;
        }
        if (navigation_.cursor >= navigation_.scrollOffset + 5U) {
            navigation_.scrollOffset = static_cast<std::uint8_t>(navigation_.cursor - 4U);
        }
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::PresetSlots) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta,
            0,
            static_cast<int>(services::PersistentStateService::kUserPresetSlotCount - 1U)));
        if (navigation_.cursor < navigation_.scrollOffset) {
            navigation_.scrollOffset = navigation_.cursor;
        }
        if (navigation_.cursor >= navigation_.scrollOffset + 5U) {
            navigation_.scrollOffset = static_cast<std::uint8_t>(navigation_.cursor - 4U);
        }
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::NameEntry) {
        adjustPresetNameCharacter(delta);
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::OverwriteConfirm) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta, 0, 1));
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::ModeChangeConfirm || navigation_.screen == Screen::HighScoreClearConfirm ||
        navigation_.screen == Screen::FactoryResetConfirm) {
        navigation_.cursor = static_cast<std::uint8_t>(clampInt(
            static_cast<int>(navigation_.cursor) + delta, 0, 1));
        invalidate();
        return;
    }

    if (navigation_.screen == Screen::Settings) {
        if (navigation_.editing && navigation_.settingsPage == SettingsPage::GrooveEditorMenu) {
            if (navigation_.cursor == 2U) {
                adjustGrooveZoom(delta);
            } else if (navigation_.cursor == 3U) {
                const int nextLength = clampInt(
                    static_cast<int>(navigation_.grooveDraft.length) + delta, 1,
                    static_cast<int>(kCustomGrooveMaximumSteps));
                navigation_.grooveDraft.length = static_cast<std::uint8_t>(nextLength);
                if (navigation_.grooveCursor >= navigation_.grooveDraft.length) {
                    navigation_.grooveCursor = static_cast<std::uint8_t>(navigation_.grooveDraft.length - 1U);
                }
                grooveEditorDirty_ = true;
                updateGroovePreview();
                invalidate();
            }
        } else if (navigation_.editing && navigation_.settingsPage == SettingsPage::GrooveRecordMenu) {
            if (navigation_.cursor == 0U) {
                navigation_.grooveRecordMode = navigation_.grooveRecordMode == GrooveRecordMode::OneShot
                    ? GrooveRecordMode::Endless
                    : GrooveRecordMode::OneShot;
                grooveRecorder_.setMode(
                    navigation_.grooveRecordMode == GrooveRecordMode::OneShot
                        ? services::GrooveRecordMode::OneShot
                        : services::GrooveRecordMode::Endless);
                invalidate();
            } else if (navigation_.cursor == 1U) {
                navigation_.grooveRecordCountInBeats = static_cast<std::uint8_t>(clampInt(
                    static_cast<int>(navigation_.grooveRecordCountInBeats) + delta, 0, 64));
                grooveRecorder_.setCountInBeats(navigation_.grooveRecordCountInBeats);
                invalidate();
            } else if (navigation_.cursor == 2U) {
                const std::uint8_t previousLength = navigation_.grooveDraft.length;
                const int nextLength = clampInt(
                    static_cast<int>(navigation_.grooveDraft.length) + delta, 1,
                    static_cast<int>(kCustomGrooveMaximumSteps));
                navigation_.grooveDraft.length = static_cast<std::uint8_t>(nextLength);
                if (navigation_.grooveDraft.length < previousLength) {
                    for (std::size_t step = navigation_.grooveDraft.length;
                         step < navigation_.grooveDraft.offsets256.size(); ++step) {
                        navigation_.grooveDraft.offsets256[step] = 0;
                    }
                }
                navigation_.grooveRecordCapturedMask &= navigation_.grooveDraft.length >= 64U
                    ? UINT64_MAX
                    : ((1ULL << navigation_.grooveDraft.length) - 1ULL);
                // The recorder is stopped while its menu is open. Re-seed its
                // capture bookkeeping after a length edit so a later pass cannot
                // resurrect captured bits that are now outside the pattern.
                grooveRecorder_.reset(
                    navigation_.grooveRecordMode == GrooveRecordMode::OneShot
                        ? services::GrooveRecordMode::OneShot
                        : services::GrooveRecordMode::Endless,
                    navigation_.grooveRecordCountInBeats,
                    navigation_.grooveRecordCapturedMask);
                grooveEditorDirty_ = true;
                updateGroovePreview();
                invalidate();
            } else if (navigation_.cursor == 3U) {
                adjustGrooveZoom(delta);
            }
        } else if (navigation_.editing) {
            settingsEditor_.adjust(
                navigation_.settingsPage,
                navigation_.cursor,
                navigation_.selectedChannel,
                delta);
            invalidate();
        } else {
            navigation_.cursor = static_cast<std::uint8_t>(clampInt(
                static_cast<int>(navigation_.cursor) + delta,
                0,
                static_cast<int>(settingsPageItemCount(
                    navigation_.settingsPage,
                    state_.channels[navigation_.selectedChannel].common.mode,
                    navigation_.highScoreResetAvailable)) - 1));
            normalizeScrollOffset();
            invalidate();
        }
        return;
    }

    if (navigation_.screen == Screen::SequencerEditor) {
        const SequencerSettings& sequencer = state_.channels[navigation_.selectedChannel].sequencer;
        int absoluteStep = navigation_.sequencerPage * 16 + navigation_.sequencerCursor + delta;
        absoluteStep = clampInt(absoluteStep, 0, static_cast<int>(sequencer.length) - 1);
        navigation_.sequencerPage = static_cast<std::uint8_t>(absoluteStep / 16);
        navigation_.sequencerCursor = static_cast<std::uint8_t>(absoluteStep % 16);
        invalidate();
    }
}


}  // namespace clockfw::ui
