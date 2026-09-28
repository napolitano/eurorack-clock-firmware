/**
 * @file publication_pipeline.cpp
 * @brief Implements FFmpeg/ffprobe-backed Storybook publication without feeding media concerns back into CLOCK.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/publication_pipeline.h"

#include <algorithm>
#include <cmath>
#include <fstream>
#include <iomanip>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <utility>

#include "tutorial/external_process.h"
#include "tutorial/subtitle_writer.h"

namespace clockfw::sim::tutorial {
namespace {

void writeText(const std::filesystem::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) throw std::runtime_error("cannot create publication file: " + path.string());
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream) throw std::runtime_error("cannot write publication file: " + path.string());
}

std::string readText(const std::filesystem::path& path) {
    std::ifstream stream(path, std::ios::binary);
    if (!stream) throw std::runtime_error("cannot read publication probe output: " + path.string());
    return std::string(std::istreambuf_iterator<char>(stream), std::istreambuf_iterator<char>());
}

std::string trim(std::string text) {
    const auto first = text.find_first_not_of(" \t\r\n");
    if (first == std::string::npos) return {};
    const auto last = text.find_last_not_of(" \t\r\n");
    return text.substr(first, last - first + 1U);
}

std::string jsonEscape(const std::string& text) {
    std::ostringstream out;
    for (const char raw : text) {
        switch (raw) {
            case '"': out << "\\\""; break;
            case '\\': out << "\\\\"; break;
            case '\n': out << "\\n"; break;
            case '\r': out << "\\r"; break;
            case '\t': out << "\\t"; break;
            default: out << raw; break;
        }
    }
    return out.str();
}

std::filesystem::path stagingPathFor(const std::filesystem::path& outputDirectory) {
    const std::string name = outputDirectory.filename().string();
    if (name.empty()) throw std::runtime_error("publication output directory must have a filename component");
    return outputDirectory.parent_path() / (name + ".staging");
}

std::filesystem::path previousPathFor(const std::filesystem::path& outputDirectory) {
    const std::string name = outputDirectory.filename().string();
    if (name.empty()) throw std::runtime_error("publication output directory must have a filename component");
    return outputDirectory.parent_path() / (name + ".previous");
}

void publishStagingDirectory(
    const std::filesystem::path& staging,
    const std::filesystem::path& outputDirectory) {
    const auto previous = previousPathFor(outputDirectory);
    std::error_code error;
    std::filesystem::remove_all(previous, error);
    if (error) throw std::runtime_error("cannot clear stale publication backup: " + error.message());

    const bool hadPrevious = std::filesystem::exists(outputDirectory);
    if (hadPrevious) {
        std::filesystem::rename(outputDirectory, previous, error);
        if (error) throw std::runtime_error("cannot preserve previous publication output: " + error.message());
    }

    error.clear();
    std::filesystem::rename(staging, outputDirectory, error);
    if (error) {
        const std::string publishError = error.message();
        if (hadPrevious) {
            std::error_code restoreError;
            std::filesystem::rename(previous, outputDirectory, restoreError);
            if (restoreError) {
                throw std::runtime_error("cannot publish staged output (" + publishError +
                                         ") and cannot restore previous output (" + restoreError.message() + ")");
            }
        }
        throw std::runtime_error("cannot publish staged output: " + publishError);
    }

    if (hadPrevious) {
        error.clear();
        std::filesystem::remove_all(previous, error);
        if (error) throw std::runtime_error("publication succeeded but previous-output cleanup failed: " + error.message());
    }
}

void runChecked(const std::vector<std::string>& arguments, const std::string& operation) {
    const int exitCode = runHostProcess(arguments);
    if (exitCode != 0) {
        throw std::runtime_error(operation + " failed with exit code " + std::to_string(exitCode));
    }
}

std::filesystem::path resolveAsset(
    const std::optional<std::string>& reference,
    const std::filesystem::path& storySourcePath) {
    if (!reference.has_value()) return {};
    std::filesystem::path asset = std::filesystem::u8path(*reference);
    if (!asset.is_absolute()) asset = storySourcePath.parent_path() / asset;
    std::error_code error;
    const auto absolute = std::filesystem::absolute(asset, error);
    if (!error) asset = absolute;
    if (!std::filesystem::is_regular_file(asset)) {
        throw std::runtime_error("referenced publication media does not exist: " + asset.string());
    }
    return asset;
}

StoryMediaProbe probeMedia(
    const std::filesystem::path& ffprobe,
    const std::filesystem::path& media,
    const std::filesystem::path& scratch,
    const std::string& stem) {
    const auto durationFile = scratch / (stem + "-duration.txt");
    const auto videoFile = scratch / (stem + "-video.txt");
    const auto audioFile = scratch / (stem + "-audio.txt");

    runChecked({ffprobe.string(), "-v", "error", "-show_entries", "format=duration", "-of",
                "default=noprint_wrappers=1:nokey=1", "-o", durationFile.string(), media.string()},
               "ffprobe duration");
    runChecked({ffprobe.string(), "-v", "error", "-select_streams", "v:0", "-show_entries",
                "stream=width,height", "-of", "default=noprint_wrappers=1", "-o", videoFile.string(), media.string()},
               "ffprobe video stream");
    runChecked({ffprobe.string(), "-v", "error", "-select_streams", "a:0", "-show_entries",
                "stream=codec_type", "-of", "default=noprint_wrappers=1:nokey=1", "-o", audioFile.string(), media.string()},
               "ffprobe audio stream");

    const std::string durationText = trim(readText(durationFile));
    const std::string videoText = readText(videoFile);
    const std::string audioText = trim(readText(audioFile));
    if (durationText.empty() || videoText.find("width=") == std::string::npos ||
        videoText.find("height=") == std::string::npos) {
        throw std::runtime_error("publication asset has no usable video stream: " + media.string());
    }

    const double seconds = std::stod(durationText);
    if (!(seconds > 0.0) || !std::isfinite(seconds)) {
        throw std::runtime_error("publication asset has invalid duration: " + media.string());
    }
    StoryMediaProbe probe;
    probe.durationUs = static_cast<std::uint64_t>(std::llround(seconds * 1000000.0));
    probe.hasAudio = !audioText.empty();

    std::istringstream video(videoText);
    std::string line;
    while (std::getline(video, line)) {
        if (line.rfind("width=", 0U) == 0U) probe.width = static_cast<std::uint32_t>(std::stoul(line.substr(6U)));
        if (line.rfind("height=", 0U) == 0U) probe.height = static_cast<std::uint32_t>(std::stoul(line.substr(7U)));
    }
    if (probe.width == 0U || probe.height == 0U) {
        throw std::runtime_error("publication asset has invalid video geometry: " + media.string());
    }
    return probe;
}

std::string videoFilter(const Story& story) {
    return "scale=" + std::to_string(story.output.width) + ":" + std::to_string(story.output.height) +
           ":force_original_aspect_ratio=decrease,pad=" + std::to_string(story.output.width) + ":" +
           std::to_string(story.output.height) + ":(ow-iw)/2:(oh-ih)/2,fps=" +
           std::to_string(story.output.framesPerSecond) + ",format=yuv420p";
}

std::string durationSeconds(const std::uint64_t durationUs) {
    std::ostringstream out;
    out << std::fixed << std::setprecision(6) << (static_cast<double>(durationUs) / 1000000.0);
    return out.str();
}

std::vector<std::string> normalizedClipCommand(
    const std::filesystem::path& ffmpeg,
    const std::filesystem::path& input,
    const std::filesystem::path& output,
    const Story& story,
    const std::uint64_t durationUs,
    const bool inputHasAudio,
    const bool requireAudio) {
    std::vector<std::string> args{ffmpeg.string(), "-y", "-v", "error", "-i", input.string()};
    if (requireAudio && !inputHasAudio) {
        args.insert(args.end(), {"-f", "lavfi", "-i", "anullsrc=channel_layout=stereo:sample_rate=48000"});
    }
    args.insert(args.end(), {"-map", "0:v:0", "-vf", videoFilter(story), "-c:v", "libx264", "-preset", "medium",
                             "-crf", "18", "-pix_fmt", "yuv420p"});
    if (requireAudio) {
        if (inputHasAudio) args.insert(args.end(), {"-map", "0:a:0", "-af", "apad"});
        else args.insert(args.end(), {"-map", "1:a:0"});
        args.insert(args.end(), {"-c:a", "aac", "-ar", "48000", "-ac", "2", "-shortest"});
    } else {
        args.push_back("-an");
    }
    args.insert(args.end(), {"-t", durationSeconds(durationUs)});
    args.push_back(output.string());
    return args;
}

std::vector<std::string> tutorialClipCommand(
    const std::filesystem::path& ffmpeg,
    const Story& story,
    const StoryFramePipelineResult& generated,
    const std::filesystem::path& output,
    const bool requireAudio) {
    const auto pattern = generated.outputDirectory / "frames" / "frame-%06d.png";
    std::vector<std::string> args{ffmpeg.string(), "-y", "-v", "error", "-framerate",
                                  std::to_string(story.output.framesPerSecond), "-start_number", "0", "-i", pattern.string()};
    if (requireAudio) {
        args.insert(args.end(), {"-f", "lavfi", "-i", "anullsrc=channel_layout=stereo:sample_rate=48000"});
    }
    args.insert(args.end(), {"-map", "0:v:0", "-vf", "format=yuv420p", "-c:v", "libx264", "-preset", "medium",
                             "-crf", "18", "-pix_fmt", "yuv420p"});
    if (requireAudio) {
        args.insert(args.end(), {"-map", "1:a:0", "-c:a", "aac", "-ar", "48000", "-ac", "2", "-shortest"});
    } else {
        args.push_back("-an");
    }
    args.push_back(output.string());
    return args;
}

std::vector<std::string> concatCommand(
    const std::filesystem::path& ffmpeg,
    const std::vector<std::filesystem::path>& inputs,
    const std::filesystem::path& output,
    const bool withAudio) {
    std::vector<std::string> args{ffmpeg.string(), "-y", "-v", "error"};
    for (const auto& input : inputs) args.insert(args.end(), {"-i", input.string()});
    std::ostringstream filter;
    for (std::size_t index = 0U; index < inputs.size(); ++index) {
        filter << '[' << index << ":v:0]";
        if (withAudio) filter << '[' << index << ":a:0]";
    }
    filter << "concat=n=" << inputs.size() << ":v=1:a=" << (withAudio ? 1 : 0) << "[v]";
    if (withAudio) filter << "[a]";
    args.insert(args.end(), {"-filter_complex", filter.str(), "-map", "[v]"});
    if (withAudio) args.insert(args.end(), {"-map", "[a]"});
    args.insert(args.end(), {"-c:v", "libx264", "-preset", "medium", "-crf", "18", "-pix_fmt", "yuv420p"});
    if (withAudio) args.insert(args.end(), {"-c:a", "aac", "-ar", "48000", "-ac", "2"});
    else args.push_back("-an");
    args.insert(args.end(), {"-movflags", "+faststart", output.string()});
    return args;
}

std::vector<SubtitleCue> offsetCues(const std::vector<SubtitleCue>& cues, const std::uint64_t offsetUs) {
    std::vector<SubtitleCue> shifted;
    shifted.reserve(cues.size());
    for (const auto& cue : cues) shifted.push_back({cue.startUs + offsetUs, cue.endUs + offsetUs, cue.text});
    return shifted;
}

std::string publicationManifest(
    const Story& story,
    const std::uint64_t introDurationUs,
    const std::uint64_t tutorialDurationUs,
    const std::uint64_t outroDurationUs,
    const bool withAudio,
    const std::set<PublicationFormat>& formats) {
    std::ostringstream out;
    out << "{\n"
        << "  \"publication_schema\": 1,\n"
        << "  \"story_id\": \"" << jsonEscape(story.id) << "\",\n"
        << "  \"intro_duration_us\": " << introDurationUs << ",\n"
        << "  \"tutorial_duration_us\": " << tutorialDurationUs << ",\n"
        << "  \"outro_duration_us\": " << outroDurationUs << ",\n"
        << "  \"audio_present\": " << (withAudio ? "true" : "false") << ",\n"
        << "  \"formats\": [";
    bool first = true;
    if (formats.count(PublicationFormat::Mp4H264) != 0U) { out << "\"mp4-h264\""; first = false; }
    if (formats.count(PublicationFormat::WebM) != 0U) { if (!first) out << ", "; out << "\"webm-vp9\""; }
    out << "]\n}\n";
    return out.str();
}

}  // namespace

StoryPublicationPipeline::StoryPublicationPipeline(std::filesystem::path ffmpeg, std::filesystem::path ffprobe)
    : ffmpeg_(std::move(ffmpeg)), ffprobe_(std::move(ffprobe)) {
    if (!std::filesystem::is_regular_file(ffmpeg_) || !std::filesystem::is_regular_file(ffprobe_)) {
        throw std::runtime_error("Storybook publication requires valid ffmpeg and ffprobe executables");
    }
}

StoryPublicationResult StoryPublicationPipeline::publish(
    const Story& story,
    const std::filesystem::path& storySourcePath,
    const StoryFramePipelineResult& generated,
    const std::filesystem::path& outputDirectory,
    const std::vector<PublicationFormat>& formats) const {
    StoryPublicationResult result;
    result.outputDirectory = outputDirectory;
    if (!generated) {
        result.issues.push_back({story.id, std::nullopt, std::nullopt, 0U,
                                 "publication requires a successful frame-pipeline result"});
        return result;
    }
    if (formats.empty()) {
        result.issues.push_back({story.id, std::nullopt, std::nullopt, 0U,
                                 "publication requires at least one output format"});
        return result;
    }

    const std::set<PublicationFormat> requested(formats.begin(), formats.end());
    const auto staging = stagingPathFor(outputDirectory);
    std::error_code ignored;
    std::filesystem::remove_all(staging, ignored);
    try {
        std::filesystem::create_directories(staging / ".work");
        const auto work = staging / ".work";
        const auto introAsset = resolveAsset(story.publication.introVideo, storySourcePath);
        const auto outroAsset = resolveAsset(story.publication.outroVideo, storySourcePath);
        std::optional<StoryMediaProbe> introProbe;
        std::optional<StoryMediaProbe> outroProbe;
        if (!introAsset.empty()) introProbe = probeMedia(ffprobe_, introAsset, work, "intro-source");
        if (!outroAsset.empty()) outroProbe = probeMedia(ffprobe_, outroAsset, work, "outro-source");
        const bool withAudio = (introProbe && introProbe->hasAudio) || (outroProbe && outroProbe->hasAudio);

        std::vector<std::filesystem::path> segments;
        std::filesystem::path normalizedIntro;
        if (introProbe) {
            normalizedIntro = work / "intro-normalized.mp4";
            runChecked(normalizedClipCommand(ffmpeg_, introAsset, normalizedIntro, story, introProbe->durationUs, introProbe->hasAudio, withAudio),
                       "normalizing intro media");
            const auto normalized = probeMedia(ffprobe_, normalizedIntro, work, "intro-normalized");
            result.introDurationUs = normalized.durationUs;
            segments.push_back(normalizedIntro);
        }

        const auto tutorialSegment = work / "tutorial-normalized.mp4";
        runChecked(tutorialClipCommand(ffmpeg_, story, generated, tutorialSegment, withAudio),
                   "encoding generated tutorial frames");
        const auto tutorialProbe = probeMedia(ffprobe_, tutorialSegment, work, "tutorial-normalized");
        segments.push_back(tutorialSegment);

        std::filesystem::path normalizedOutro;
        if (outroProbe) {
            normalizedOutro = work / "outro-normalized.mp4";
            runChecked(normalizedClipCommand(ffmpeg_, outroAsset, normalizedOutro, story, outroProbe->durationUs, outroProbe->hasAudio, withAudio),
                       "normalizing outro media");
            const auto normalized = probeMedia(ffprobe_, normalizedOutro, work, "outro-normalized");
            result.outroDurationUs = normalized.durationUs;
            segments.push_back(normalizedOutro);
        }

        const auto intermediateMp4 = work / "composed.mp4";
        runChecked(concatCommand(ffmpeg_, segments, intermediateMp4, withAudio), "composing publication segments");

        if (requested.count(PublicationFormat::Mp4H264) != 0U) {
            const auto target = staging / (story.id + ".mp4");
            std::filesystem::copy_file(intermediateMp4, target, std::filesystem::copy_options::overwrite_existing);
            result.mp4Path = outputDirectory / target.filename();
        }
        if (requested.count(PublicationFormat::WebM) != 0U) {
            const auto target = staging / (story.id + ".webm");
            std::vector<std::string> args{ffmpeg_.string(), "-y", "-v", "error", "-i", intermediateMp4.string(),
                                          "-c:v", "libvpx-vp9", "-crf", "30", "-b:v", "0", "-pix_fmt", "yuv420p"};
            if (withAudio) args.insert(args.end(), {"-c:a", "libopus", "-ar", "48000", "-ac", "2"});
            else args.push_back("-an");
            args.push_back(target.string());
            runChecked(args, "encoding WebM publication");
            result.webmPath = outputDirectory / target.filename();
        }

        const auto shifted = offsetCues(generated.subtitles, result.introDurationUs);
        const auto srtName = story.id + ".srt";
        const auto vttName = story.id + ".vtt";
        writeSubtitleSidecars(staging / srtName, staging / vttName, shifted);
        writeText(staging / "publication-manifest.json",
                  publicationManifest(story, result.introDurationUs, tutorialProbe.durationUs,
                                      result.outroDurationUs, withAudio, requested));
        std::filesystem::remove_all(work, ignored);

        if (!outputDirectory.parent_path().empty()) std::filesystem::create_directories(outputDirectory.parent_path());
        publishStagingDirectory(staging, outputDirectory);
        result.srtPath = outputDirectory / srtName;
        result.vttPath = outputDirectory / vttName;
        result.manifestPath = outputDirectory / "publication-manifest.json";
        return result;
    } catch (const std::exception& error) {
        std::filesystem::remove_all(staging, ignored);
        result.issues.push_back({story.id, std::nullopt, std::nullopt, 0U,
                                 std::string("publication pipeline failed: ") + error.what()});
        return result;
    }
}

}  // namespace clockfw::sim::tutorial
