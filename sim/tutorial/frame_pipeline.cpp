/**
 * @file frame_pipeline.cpp
 * @brief Implements single-run deterministic frame/subtitle generation for CLOCK Storybook.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/frame_pipeline.h"

#include <fstream>
#include <iomanip>
#include <optional>
#include <set>
#include <unordered_map>
#include <array>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "tutorial/panel_presentation.h"
#include "panel_led_visual.h"
#include "tutorial/png_writer.h"
#include "tutorial/story_theme.h"
#include "tutorial/story_execution_observer.h"
#include "tutorial/tutorial_renderer.h"
#include "version.h"

namespace clockfw::sim::tutorial {
namespace {

std::uint64_t frameTimestampUs(const std::size_t frameIndex, const std::uint16_t framesPerSecond) {
    if (framesPerSecond == 0U) throw std::runtime_error("frame rate must be non-zero");
    return (static_cast<std::uint64_t>(frameIndex) * 1000000ULL) /
           static_cast<std::uint64_t>(framesPerSecond);
}

std::string frameFileName(const std::size_t index) {
    std::ostringstream out;
    out << "frame-" << std::setfill('0') << std::setw(6) << index << ".png";
    return out.str();
}

std::string jsonEscape(const std::string& text) {
    std::ostringstream out;
    for (const char raw : text) {
        const auto value = static_cast<unsigned char>(raw);
        switch (value) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\b': out << "\\b"; break;
            case '\f': out << "\\f"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default:
                if (value < 0x20U) {
                    out << "\\u" << std::hex << std::setfill('0') << std::setw(4)
                        << static_cast<unsigned>(value) << std::dec;
                } else {
                    out << static_cast<char>(value);
                }
                break;
        }
    }
    return out.str();
}

void writeTextFile(const std::filesystem::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) throw std::runtime_error("cannot create Storybook output: " + path.string());
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream) throw std::runtime_error("cannot write Storybook output: " + path.string());
}

struct NarrationCue final {
    std::string id;
    std::uint64_t startUs = 0ULL;
    std::uint64_t endUs = 0ULL;
};

std::vector<NarrationCue> narrationCuesFromTrace(const StoryRunResult& run) {
    std::vector<NarrationCue> cues;
    std::unordered_map<std::string, std::uint64_t> open;
    for (const auto& event : run.trace) {
        if (event.kind == StoryTraceKind::NarrationBegin) {
            if (!open.emplace(event.name, event.presentationUs).second) {
                throw std::runtime_error("duplicate open narration cue: " + event.name);
            }
        } else if (event.kind == StoryTraceKind::NarrationEnd) {
            const auto found = open.find(event.name);
            if (found == open.end()) throw std::runtime_error("narration cue ended without begin: " + event.name);
            cues.push_back({event.name, found->second, event.presentationUs});
            open.erase(found);
        }
    }
    if (!open.empty()) throw std::runtime_error("narration cue did not close: " + open.begin()->first);
    return cues;
}

std::string narrationManifest(const Story& story, const std::vector<NarrationCue>& cues) {
    std::ostringstream out;
    out << "{\n  \"schema\": 1,\n  \"story_id\": \"" << jsonEscape(story.id) << "\",\n  \"cues\": [\n";
    for (std::size_t index = 0U; index < cues.size(); ++index) {
        const auto& cue = cues[index];
        out << "    {\"id\": \"" << jsonEscape(cue.id) << "\", \"start_us\": " << cue.startUs
            << ", \"end_us\": " << cue.endUs << "}";
        if (index + 1U < cues.size()) out << ',';
        out << '\n';
    }
    out << "  ]\n}\n";
    return out.str();
}

std::string makeManifest(
    const Story& story,
    const std::string& sourceRevision,
    const StoryRunResult& run,
    const std::vector<StoryFrameRecord>& frames,
    const std::vector<SubtitleCue>& subtitles,
    const std::vector<NarrationCue>& narration) {
    std::ostringstream out;
    out << "{\n"
        << "  \"manifest_schema\": 1,\n"
        << "  \"story_schema\": " << story.schema << ",\n"
        << "  \"story_id\": \"" << jsonEscape(story.id) << "\",\n"
        << "  \"story_title\": \"" << jsonEscape(story.title) << "\",\n"
        << "  \"firmware_version\": \"" << CLOCK_FIRMWARE_VERSION << "\",\n"
        << "  \"simulator_source_revision\": \"" << jsonEscape(sourceRevision) << "\",\n"
        << "  \"theme\": \"" << jsonEscape(story.theme) << "\",\n"
        << "  \"interaction_profile\": \"" << interactionProfileName(story.interactionProfile) << "\",\n"
        << "  \"output\": {\"width\": " << story.output.width
        << ", \"height\": " << story.output.height
        << ", \"fps\": " << story.output.framesPerSecond << "},\n"
        << "  \"presentation_duration_us\": " << run.presentationDurationUs << ",\n"
        << "  \"frame_count\": " << frames.size() << ",\n"
        << "  \"unique_frame_file_count\": " << [&]() { std::set<std::filesystem::path> paths; for (const auto& frame : frames) paths.insert(frame.relativePath); return paths.size(); }() << ",\n"
        << "  \"subtitle_count\": " << subtitles.size() << ",\n"
        << "  \"narration_cue_count\": " << narration.size() << ",\n"
        << "  \"frame_hash_algorithm\": \"fnv1a64-rgba8\",\n"
        << "  \"frames\": [\n";
    for (std::size_t index = 0U; index < frames.size(); ++index) {
        const StoryFrameRecord& frame = frames[index];
        out << "    {\"index\": " << frame.index
            << ", \"presentation_us\": " << frame.presentationUs
            << ", \"path\": \"" << jsonEscape(frame.relativePath.generic_string())
            << "\", \"rgba_fnv1a64\": \"" << frame.rgbaFnv1a64 << "\"}";
        if (index + 1U < frames.size()) out << ',';
        out << '\n';
    }
    out << "  ]\n}\n";
    return out.str();
}

bool samePhysicalStateForFrozenFrame(
    const PhysicalPresentationState& left,
    const PhysicalPresentationState& right,
    const std::uint64_t focusLingerUs) {
    if (left.encoderDetentDelta != right.encoderDetentDelta ||
        left.encoderPressed != right.encoderPressed ||
        left.playPressed != right.playPressed ||
        left.tapPressed != right.tapPressed ||
        left.stopPressed != right.stopPressed ||
        left.recordedPowerOn != right.recordedPowerOn ||
        left.scopeMode != right.scopeMode ||
        left.explicitFocus != right.explicitFocus ||
        left.focusPlacement != right.focusPlacement ||
        left.focusX != right.focusX || left.focusY != right.focusY ||
        left.focusWidth != right.focusWidth || left.focusHeight != right.focusHeight ||
        left.focusLabel != right.focusLabel ||
        left.syncMotion != right.syncMotion || left.resetMotion != right.resetMotion ||
        left.syncInsertion != right.syncInsertion || left.resetInsertion != right.resetInsertion) {
        return false;
    }

    // Automatic focus has no fade: age matters only at the visibility cutoff. Re-render
    // once when the effective focus appears/disappears, not for every 30-fps sample while
    // an already identical focus ring remains visible or after its linger has expired.
    if (left.explicitFocus != FocusTarget::None) return true;
    const bool leftAutomaticVisible = left.automaticFocus != FocusTarget::None &&
        left.automaticFocusAgeUs <= focusLingerUs;
    const bool rightAutomaticVisible = right.automaticFocus != FocusTarget::None &&
        right.automaticFocusAgeUs <= focusLingerUs;
    if (leftAutomaticVisible != rightAutomaticVisible) return false;
    if (!leftAutomaticVisible) return true;
    return left.automaticFocus == right.automaticFocus;
}

struct VisibleRuntimeState final {
    std::array<std::uint8_t, hal::OledDisplay::kFramebufferSize> framebuffer{};
    std::array<bool, kChannelCount> leds{};
    std::int64_t encoderPosition = 0;
    bool poweredOn = false;
    bool syncConnected = false;
    bool syncHigh = false;
    bool resetConnected = false;
    bool resetHigh = false;
};

VisibleRuntimeState captureVisibleRuntimeState(const SimulatorRuntime& runtime) {
    VisibleRuntimeState state{};
    state.framebuffer = runtime.framebuffer();
    state.encoderPosition = runtime.encoderVisualPosition();
    state.poweredOn = runtime.poweredOn();
    const SyncInputTelemetry sync = runtime.syncInputTelemetry();
    state.syncConnected = sync.cableConnected;
    state.syncHigh = sync.signalHigh;
    const ResetInputTelemetry reset = runtime.resetInputTelemetry();
    state.resetConnected = reset.cableConnected;
    state.resetHigh = reset.signalHigh;
    const auto& channels = runtime.telemetry();
    const std::uint64_t nowUs = runtime.nowMicroseconds();
    for (std::size_t index = 0U; index < state.leds.size(); ++index) {
        state.leds[index] = panelLedVisuallyLit(channels[index], nowUs, 1.0);
    }
    return state;
}

bool sameVisibleRuntimeState(const VisibleRuntimeState& left, const VisibleRuntimeState& right) {
    return left.framebuffer == right.framebuffer && left.leds == right.leds &&
        left.encoderPosition == right.encoderPosition && left.poweredOn == right.poweredOn &&
        left.syncConnected == right.syncConnected && left.syncHigh == right.syncHigh &&
        left.resetConnected == right.resetConnected && left.resetHigh == right.resetHigh;
}

void materializeDuplicateFrame(
    const std::filesystem::path& source,
    const std::filesystem::path& destination) {
    std::error_code linkError;
    std::filesystem::create_hard_link(source, destination, linkError);
    if (linkError) {
        // Some filesystems do not allow hard links. Keep the required per-frame paths.
        std::filesystem::copy_file(source, destination);
    }
}

class FrameObserver final : public StoryExecutionObserver {
public:
    FrameObserver(
        const Story& story,
        TutorialRenderer& renderer,
        PanelPresentationTimeline& presentation,
        std::filesystem::path frameDirectory,
        const FrameStorageMode storageMode,
        const std::uint64_t focusLingerUs)
        : story_(story), renderer_(renderer), presentation_(presentation),
          frameDirectory_(std::move(frameDirectory)), storageMode_(storageMode),
          focusLingerUs_(focusLingerUs) {}

    std::optional<std::uint64_t> nextPresentationSampleUs() const override {
        return frameTimestampUs(frameIndex_, story_.output.framesPerSecond);
    }

    void onPresentationSample(const StoryExecutionSample& sample) override {
        if (sample.scene == nullptr || sample.runtime == nullptr) {
            throw std::runtime_error("frame sample is missing scene/runtime state");
        }
        lastSceneIndex_ = sample.sceneIndex;
        const std::filesystem::path logicalRelative =
            std::filesystem::path("frames") / frameFileName(frameIndex_);
        const std::filesystem::path destination = frameDirectory_ / logicalRelative.filename();
        const bool sparse = storageMode_ == FrameStorageMode::SparseForPublication;
        const bool staticScene = sample.scene->kind != SceneKind::Tutorial;

        if (staticScene && cachedStaticSceneIndex_ == sample.sceneIndex) {
            const std::filesystem::path relative = sparse ? cachedStaticRelativePath_ : logicalRelative;
            if (!sparse) materializeDuplicateFrame(cachedStaticFramePath_, destination);
            frames_.push_back({frameIndex_, sample.presentationUs, relative, cachedStaticDigest_});
            previousRelativePath_ = relative;
            previousDigest_ = cachedStaticDigest_;
        } else {
            const PhysicalPresentationState physical = presentation_.stateAt(sample.presentationUs);
            const std::string subtitle(sample.activeSubtitle);
            const VisibleRuntimeState visibleRuntime = captureVisibleRuntimeState(*sample.runtime);
            const bool unchangedVisualReuse = !staticScene && sparse && previousSceneIndex_ == sample.sceneIndex &&
                previousSubtitle_ == subtitle && previousPhysical_.has_value() &&
                samePhysicalStateForFrozenFrame(*previousPhysical_, physical, focusLingerUs_) &&
                previousVisibleRuntime_.has_value() &&
                physical.scopeMode != ScopeMode::VisibleChannel &&
                sameVisibleRuntimeState(*previousVisibleRuntime_, visibleRuntime) &&
                !previousRelativePath_.empty();

            if (unchangedVisualReuse) {
                frames_.push_back({frameIndex_, sample.presentationUs, previousRelativePath_, previousDigest_});
            } else {
                const TutorialSurface frame = renderer_.renderScene(
                    story_, *sample.scene, *sample.runtime, physical, subtitle, 1.0);
                const std::string digest = formatDigest64(tutorialRgbaFnv1a64(frame));
                const bool identicalToPrevious = sparse && digest == previousDigest_ && !previousRelativePath_.empty();
                const std::filesystem::path relative = identicalToPrevious ? previousRelativePath_ : logicalRelative;
                if (!identicalToPrevious) writeTutorialPng(destination, frame);
                frames_.push_back({frameIndex_, sample.presentationUs, relative, digest});
                previousRelativePath_ = relative;
                previousDigest_ = digest;

                if (staticScene) {
                    cachedStaticSceneIndex_ = sample.sceneIndex;
                    cachedStaticFramePath_ = destination;
                    cachedStaticRelativePath_ = relative;
                    cachedStaticDigest_ = digest;
                } else {
                    cachedStaticSceneIndex_.reset();
                }
            }

            previousSceneIndex_ = sample.sceneIndex;
            previousSubtitle_ = subtitle;
            previousPhysical_ = physical;
            previousVisibleRuntime_ = visibleRuntime;
        }
        ++frameIndex_;
    }

    const std::vector<StoryFrameRecord>& frames() const { return frames_; }
    std::optional<std::size_t> lastSceneIndex() const { return lastSceneIndex_; }

private:
    const Story& story_;
    TutorialRenderer& renderer_;
    PanelPresentationTimeline& presentation_;
    std::filesystem::path frameDirectory_;
    std::size_t frameIndex_ = 0U;
    std::vector<StoryFrameRecord> frames_{};
    std::optional<std::size_t> lastSceneIndex_;
    std::optional<std::size_t> cachedStaticSceneIndex_;
    std::filesystem::path cachedStaticFramePath_;
    std::string cachedStaticDigest_;
    std::filesystem::path cachedStaticRelativePath_;
    FrameStorageMode storageMode_ = FrameStorageMode::Materialized;
    std::uint64_t focusLingerUs_ = 0ULL;
    std::optional<std::size_t> previousSceneIndex_;
    std::string previousSubtitle_;
    std::optional<PhysicalPresentationState> previousPhysical_;
    std::optional<VisibleRuntimeState> previousVisibleRuntime_;
    std::filesystem::path previousRelativePath_;
    std::string previousDigest_;
};

std::filesystem::path stagingPathFor(const std::filesystem::path& outputDirectory) {
    const std::string name = outputDirectory.filename().string();
    if (name.empty()) throw std::runtime_error("Storybook output directory must have a filename component");
    return outputDirectory.parent_path() / (name + ".staging");
}

}  // namespace

StoryFramePipelineResult::operator bool() const {
    return issues.empty() && static_cast<bool>(run);
}

StoryFramePipeline::StoryFramePipeline(
    std::filesystem::path tutorialRoot,
    layout::PanelLayout panelLayout,
    std::string simulatorSourceRevision)
    : tutorialRoot_(std::move(tutorialRoot)), panelLayout_(std::move(panelLayout)),
      simulatorSourceRevision_(std::move(simulatorSourceRevision)) {
    if (simulatorSourceRevision_.empty()) {
        throw std::runtime_error("Storybook generation requires an explicit simulator source revision");
    }
}

StoryFramePipelineResult StoryFramePipeline::generate(
    const Story& story,
    StorySimulatorPort& port,
    const std::filesystem::path& outputDirectory,
    const FrameStorageMode storageMode) {
    StoryFramePipelineResult result;
    result.outputDirectory = outputDirectory;
    const std::filesystem::path staging = stagingPathFor(outputDirectory);
    std::error_code ignored;
    std::filesystem::remove_all(staging, ignored);

    std::optional<std::size_t> renderSceneIndex;
    try {
        std::filesystem::create_directories(staging / "frames");
        PanelPresentationTimeline presentation;
        TutorialRenderer renderer(tutorialRoot_, panelLayout_);
        const StoryTheme theme = loadStoryTheme(tutorialRoot_, story.theme);
        const std::uint64_t focusLingerUs = static_cast<std::uint64_t>(theme.focusLingerMs) * 1000ULL;
        FrameObserver observer(story, renderer, presentation, staging / "frames", storageMode, focusLingerUs);
        StoryRunner runner(port, tutorialRoot_, &presentation, &observer);
        result.run = runner.run(story);
        renderSceneIndex = observer.lastSceneIndex();
        result.frames = observer.frames();
        if (!result.run) {
            result.issues = result.run.issues;
            std::filesystem::remove_all(staging, ignored);
            return result;
        }
        if (result.frames.empty() && result.run.presentationDurationUs > 0ULL) {
            result.issues.push_back({story.id, std::nullopt, std::nullopt, 0U,
                                     "frame scheduler produced no output frames"});
            std::filesystem::remove_all(staging, ignored);
            return result;
        }

        result.subtitles = subtitleCuesFromTrace(result.run);
        const auto narration = narrationCuesFromTrace(result.run);
        const std::filesystem::path srtName = story.id + ".srt";
        const std::filesystem::path vttName = story.id + ".vtt";
        const std::filesystem::path narrationName = "narration.json";
        writeSubtitleSidecars(staging / srtName, staging / vttName, result.subtitles);
        writeTextFile(staging / narrationName, narrationManifest(story, narration));
        writeTextFile(staging / "manifest.json",
                      makeManifest(story, simulatorSourceRevision_, result.run, result.frames, result.subtitles, narration));

        if (!outputDirectory.parent_path().empty()) {
            std::filesystem::create_directories(outputDirectory.parent_path());
        }
        std::filesystem::remove_all(outputDirectory, ignored);
        std::filesystem::rename(staging, outputDirectory);
        result.srtPath = outputDirectory / srtName;
        result.vttPath = outputDirectory / vttName;
        result.manifestPath = outputDirectory / "manifest.json";
        result.narrationPath = outputDirectory / narrationName;
        return result;
    } catch (const std::exception& error) {
        std::filesystem::remove_all(staging, ignored);
        result.issues.push_back({story.id, renderSceneIndex, std::nullopt, 0U,
                                 std::string("frame pipeline failed: ") + error.what()});
        return result;
    }
}

}  // namespace clockfw::sim::tutorial
