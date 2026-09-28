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
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "tutorial/panel_presentation.h"
#include "tutorial/png_writer.h"
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

std::string makeManifest(
    const Story& story,
    const std::string& sourceRevision,
    const StoryRunResult& run,
    const std::vector<StoryFrameRecord>& frames,
    const std::vector<SubtitleCue>& subtitles) {
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
        << "  \"subtitle_count\": " << subtitles.size() << ",\n"
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

class FrameObserver final : public StoryExecutionObserver {
public:
    FrameObserver(
        const Story& story,
        TutorialRenderer& renderer,
        PanelPresentationTimeline& presentation,
        std::filesystem::path frameDirectory)
        : story_(story), renderer_(renderer), presentation_(presentation),
          frameDirectory_(std::move(frameDirectory)) {}

    std::optional<std::uint64_t> nextPresentationSampleUs() const override {
        return frameTimestampUs(frameIndex_, story_.output.framesPerSecond);
    }

    void onPresentationSample(const StoryExecutionSample& sample) override {
        if (sample.scene == nullptr || sample.runtime == nullptr) {
            throw std::runtime_error("frame sample is missing scene/runtime state");
        }
        lastSceneIndex_ = sample.sceneIndex;
        const PhysicalPresentationState physical = presentation_.stateAt(sample.presentationUs);
        const TutorialSurface frame = renderer_.renderScene(
            story_, *sample.scene, *sample.runtime, physical, std::string(sample.activeSubtitle), 1.0);
        const std::filesystem::path relative = std::filesystem::path("frames") / frameFileName(frameIndex_);
        writeTutorialPng(frameDirectory_ / relative.filename(), frame);
        frames_.push_back({
            frameIndex_, sample.presentationUs, relative,
            formatDigest64(tutorialRgbaFnv1a64(frame))});
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
    const std::filesystem::path& outputDirectory) {
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
        FrameObserver observer(story, renderer, presentation, staging / "frames");
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
        const std::filesystem::path srtName = story.id + ".srt";
        const std::filesystem::path vttName = story.id + ".vtt";
        writeSubtitleSidecars(staging / srtName, staging / vttName, result.subtitles);
        writeTextFile(staging / "manifest.json",
                      makeManifest(story, simulatorSourceRevision_, result.run, result.frames, result.subtitles));

        if (!outputDirectory.parent_path().empty()) {
            std::filesystem::create_directories(outputDirectory.parent_path());
        }
        std::filesystem::remove_all(outputDirectory, ignored);
        std::filesystem::rename(staging, outputDirectory);
        result.srtPath = outputDirectory / srtName;
        result.vttPath = outputDirectory / vttName;
        result.manifestPath = outputDirectory / "manifest.json";
        return result;
    } catch (const std::exception& error) {
        std::filesystem::remove_all(staging, ignored);
        result.issues.push_back({story.id, renderSceneIndex, std::nullopt, 0U,
                                 std::string("frame pipeline failed: ") + error.what()});
        return result;
    }
}

}  // namespace clockfw::sim::tutorial
