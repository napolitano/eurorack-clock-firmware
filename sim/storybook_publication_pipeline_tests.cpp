/**
 * @file storybook_publication_pipeline_tests.cpp
 * @brief Integration tests for FFmpeg Storybook publication, intro/outro composition and shifted subtitle sidecars.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <filesystem>
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include "tutorial/external_process.h"
#include "tutorial/png_writer.h"
#include "tutorial/publication_pipeline.h"
#include "tutorial/subtitle_writer.h"
#include "tutorial/tutorial_surface.h"

namespace {
using namespace clockfw::sim::tutorial;

bool require(const bool condition, const char* message) {
    if (!condition) std::cerr << "Storybook publication failure: " << message << '\n';
    return condition;
}

std::string readText(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

void writeFrames(const std::filesystem::path& generatedDir) {
    std::filesystem::create_directories(generatedDir / "frames");
    for (std::size_t index = 0U; index < 3U; ++index) {
        TutorialSurface frame(64U, 64U, {static_cast<std::uint8_t>(40U + index * 50U), 30U, 90U, 255U});
        frame.fillRect({static_cast<int>(8U + index * 8U), 12, 20, 20}, {230U, 230U, 230U, 255U});
        char name[32];
        std::snprintf(name, sizeof(name), "frame-%06zu.png", index);
        writeTutorialPng(generatedDir / "frames" / name, frame);
    }
}

StoryFramePipelineResult generatedResult(const std::filesystem::path& directory) {
    StoryFramePipelineResult generated;
    generated.outputDirectory = directory;
    generated.run.presentationDurationUs = 300000ULL;
    generated.subtitles = {{50000ULL, 200000ULL, "Tutorial subtitle"}};
    generated.srtPath = directory / "demo.srt";
    generated.vttPath = directory / "demo.vtt";
    generated.manifestPath = directory / "manifest.json";
    return generated;
}

void makeIntro(const std::filesystem::path& ffmpeg, const std::filesystem::path& output) {
    runHostProcess({ffmpeg.string(), "-y", "-v", "error", "-f", "lavfi", "-i",
                    "color=c=blue:s=80x60:r=25:d=0.4", "-f", "lavfi", "-i",
                    "sine=frequency=440:sample_rate=48000:duration=0.4", "-shortest",
                    "-c:v", "libx264", "-pix_fmt", "yuv420p", "-c:a", "aac", output.string()});
}

void makeOutro(const std::filesystem::path& ffmpeg, const std::filesystem::path& output) {
    runHostProcess({ffmpeg.string(), "-y", "-v", "error", "-f", "lavfi", "-i",
                    "color=c=red:s=96x72:r=24:d=0.3", "-c:v", "libx264", "-pix_fmt", "yuv420p",
                    "-an", output.string()});
}

bool probeHasStreams(const std::filesystem::path& ffprobe, const std::filesystem::path& media,
                     const std::filesystem::path& probeFile, const bool expectAudio) {
    const int result = runHostProcess({ffprobe.string(), "-v", "error", "-show_entries",
                                      "stream=codec_type,width,height", "-of", "default=noprint_wrappers=1",
                                      "-o", probeFile.string(), media.string()});
    if (result != 0) return false;
    const std::string text = readText(probeFile);
    const bool video = text.find("codec_type=video") != std::string::npos &&
                       text.find("width=64") != std::string::npos && text.find("height=64") != std::string::npos;
    const bool audio = text.find("codec_type=audio") != std::string::npos;
    return video && audio == expectAudio;
}

}  // namespace

int main() {
    const auto ffmpeg = findHostExecutable("ffmpeg");
    const auto ffprobe = findHostExecutable("ffprobe");
    if (!ffmpeg || !ffprobe) {
        std::cout << "Storybook publication tests skipped: ffmpeg/ffprobe unavailable\n";
        return 77;
    }

    const auto root = std::filesystem::temp_directory_path() / "clock-storybook-publication-tests";
    std::error_code ignored;
    std::filesystem::remove_all(root, ignored);
    std::filesystem::create_directories(root / "story" / "assets");
    const auto generatedDir = root / "generated";
    writeFrames(generatedDir);
    StoryFramePipelineResult generated = generatedResult(generatedDir);

    const auto intro = root / "story" / "assets" / "intro.mp4";
    const auto outro = root / "story" / "assets" / "outro.mp4";
    makeIntro(*ffmpeg, intro);
    makeOutro(*ffmpeg, outro);
    if (!require(std::filesystem::file_size(intro) > 0U && std::filesystem::file_size(outro) > 0U,
                 "test media generation failed")) return 1;

    Story story;
    story.schema = 1U;
    story.id = "publication-demo";
    story.title = "Publication demo";
    story.output = {64U, 64U, 10U};
    story.publication.introVideo = "assets/intro.mp4";
    story.publication.outroVideo = "assets/outro.mp4";
    const auto storyPath = root / "story" / "publication-demo.yaml";
    std::ofstream(storyPath) << "schema: 1\n";

    StoryPublicationPipeline pipeline(*ffmpeg, *ffprobe);
    const auto output = root / "published";
    const StoryPublicationResult result = pipeline.publish(
        story, storyPath, generated, output, {PublicationFormat::Mp4H264, PublicationFormat::WebM});
    if (!require(static_cast<bool>(result), "intro/outro publication failed")) {
        for (const auto& issue : result.issues) std::cerr << issue.reason << '\n';
        return 1;
    }
    if (!require(result.mp4Path && std::filesystem::file_size(*result.mp4Path) > 0U, "MP4 missing")) return 1;
    if (!require(result.webmPath && std::filesystem::file_size(*result.webmPath) > 0U, "WebM missing")) return 1;
    if (!require(result.introDurationUs >= 350000ULL && result.introDurationUs <= 450000ULL,
                 "normalized intro duration not measured")) return 1;
    if (!require(result.outroDurationUs >= 250000ULL && result.outroDurationUs <= 350000ULL,
                 "normalized outro duration not measured")) return 1;
    if (!require(probeHasStreams(*ffprobe, *result.mp4Path, root / "mp4-probe.txt", true),
                 "MP4 stream profile invalid")) return 1;
    if (!require(probeHasStreams(*ffprobe, *result.webmPath, root / "webm-probe.txt", true),
                 "WebM stream profile invalid")) return 1;

    const std::vector<SubtitleCue> expected{{50000ULL + result.introDurationUs,
                                              200000ULL + result.introDurationUs,
                                              "Tutorial subtitle"}};
    if (!require(readText(result.srtPath) == renderSrt(expected), "SRT intro offset mismatch")) return 1;
    if (!require(readText(result.vttPath) == renderWebVtt(expected), "WebVTT intro offset mismatch")) return 1;
    const std::string manifest = readText(result.manifestPath);
    if (!require(manifest.find("\"audio_present\": true") != std::string::npos,
                 "publication manifest lost audio state")) return 1;

    Story introOnly = story;
    introOnly.id = "publication-intro-only";
    introOnly.publication.outroVideo.reset();
    const StoryPublicationResult introOnlyResult = pipeline.publish(
        introOnly, storyPath, generated, root / "published-intro-only", {PublicationFormat::Mp4H264});
    if (!require(static_cast<bool>(introOnlyResult) && introOnlyResult.introDurationUs > 0ULL &&
                 introOnlyResult.outroDurationUs == 0ULL, "intro-only publication failed")) return 1;

    Story outroOnly = story;
    outroOnly.id = "publication-outro-only";
    outroOnly.publication.introVideo.reset();
    const StoryPublicationResult outroOnlyResult = pipeline.publish(
        outroOnly, storyPath, generated, root / "published-outro-only", {PublicationFormat::Mp4H264});
    if (!require(static_cast<bool>(outroOnlyResult) && outroOnlyResult.introDurationUs == 0ULL &&
                 outroOnlyResult.outroDurationUs > 0ULL, "outro-only publication failed")) return 1;
    if (!require(readText(outroOnlyResult.srtPath) == renderSrt(generated.subtitles),
                 "outro-only subtitles must not be offset")) return 1;

    Story noClips = story;
    noClips.id = "publication-no-clips";
    noClips.publication = {};
    const auto noClipsOutput = root / "published-no-clips";
    std::filesystem::create_directories(noClipsOutput);
    std::ofstream(noClipsOutput / "old-success.txt") << "previous";
    const StoryPublicationResult noClipsResult = pipeline.publish(
        noClips, storyPath, generated, noClipsOutput, {PublicationFormat::Mp4H264});
    if (!require(static_cast<bool>(noClipsResult), "publication without intro/outro failed")) return 1;
    if (!require(noClipsResult.introDurationUs == 0ULL && noClipsResult.outroDurationUs == 0ULL,
                 "no-clip publication gained phantom durations")) return 1;
    if (!require(probeHasStreams(*ffprobe, *noClipsResult.mp4Path, root / "silent-probe.txt", false),
                 "tutorial-only publication should remain audio-free")) return 1;
    if (!require(readText(noClipsResult.srtPath) == renderSrt(generated.subtitles),
                 "tutorial-only subtitles should remain unshifted")) return 1;
    if (!require(!std::filesystem::exists(noClipsOutput / "old-success.txt"),
                 "successful publication did not replace previous output")) return 1;
    if (!require(!std::filesystem::exists(root / "published-no-clips.previous"),
                 "successful publication left previous-output backup")) return 1;

    Story missing = noClips;
    missing.id = "publication-missing-intro";
    missing.publication.introVideo = "assets/missing.mp4";
    const auto failureOutput = root / "failure-output";
    std::filesystem::create_directories(failureOutput);
    std::ofstream(failureOutput / "keep.txt") << "preserve";
    const StoryPublicationResult failed = pipeline.publish(
        missing, storyPath, generated, failureOutput, {PublicationFormat::Mp4H264});
    if (!require(!static_cast<bool>(failed), "missing referenced media must fail publication")) return 1;
    if (!require(readText(failureOutput / "keep.txt") == "preserve", "failed publication replaced prior output")) return 1;
    if (!require(!std::filesystem::exists(root / "failure-output.staging"), "failed publication left staging output")) return 1;

    std::filesystem::remove_all(root, ignored);
    std::cout << "Storybook publication pipeline: PASS\n";
    return 0;
}
