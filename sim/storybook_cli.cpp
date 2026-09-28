/**
 * @file storybook_cli.cpp
 * @brief Command-line entry point for validating, rendering, and publishing CLOCK Storybook tutorials.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include <cstdlib>
#include <filesystem>
#include <iostream>
#include <optional>
#include <stdexcept>
#include <string>
#include <vector>

#include "panel_layout.h"
#include "simulator_runtime.h"
#include "tutorial/external_process.h"
#include "tutorial/frame_pipeline.h"
#include "tutorial/publication_pipeline.h"
#include "tutorial/story_parser.h"
#include "tutorial/story_simulator_port.h"
#include "tutorial/story_validator.h"

namespace {
using namespace clockfw::sim;
using namespace clockfw::sim::tutorial;

struct CliOptions final {
    std::string command;
    std::filesystem::path storyPath;
    std::optional<std::filesystem::path> outputDirectory;
    std::string sourceRevision = "working-tree";
    std::optional<std::filesystem::path> ffmpeg;
    std::optional<std::filesystem::path> ffprobe;
    std::vector<PublicationFormat> formats{PublicationFormat::Mp4H264};
    bool keepFrames = false;
};

void printUsage() {
    std::cout
        << "CLOCK Storybook\n\n"
        << "Usage:\n"
        << "  clock-storybook validate <story.yaml>\n"
        << "  clock-storybook frames <story.yaml> --output <directory> [--source-revision <text>]\n"
        << "  clock-storybook video <story.yaml> --output <directory> [--format mp4|webm|both]\n"
        << "                        [--keep-frames] [--ffmpeg <path>] [--ffprobe <path>]\n"
        << "                        [--source-revision <text>]\n\n"
        << "The video command runs the deterministic frame pipeline first, then the publication pipeline.\n"
        << "Optional intro/outro clips are taken from the Story YAML publication block.\n";
}

[[noreturn]] void usageError(const std::string& message) {
    throw std::runtime_error(message);
}

CliOptions parseOptions(const int argc, char** argv) {
    if (argc < 3) usageError("missing command or story path");
    CliOptions options;
    options.command = argv[1];
    options.storyPath = argv[2];
    if (options.command != "validate" && options.command != "frames" && options.command != "video") {
        usageError("unknown command: " + options.command);
    }

    for (int index = 3; index < argc; ++index) {
        const std::string argument = argv[index];
        auto requireValue = [&](const std::string& name) -> std::string {
            if (index + 1 >= argc) usageError("missing value for " + name);
            ++index;
            return argv[index];
        };

        if (argument == "--output") {
            options.outputDirectory = std::filesystem::path(requireValue(argument));
        } else if (argument == "--source-revision") {
            options.sourceRevision = requireValue(argument);
        } else if (argument == "--ffmpeg") {
            options.ffmpeg = std::filesystem::path(requireValue(argument));
        } else if (argument == "--ffprobe") {
            options.ffprobe = std::filesystem::path(requireValue(argument));
        } else if (argument == "--format") {
            const std::string format = requireValue(argument);
            if (format == "mp4") {
                options.formats = {PublicationFormat::Mp4H264};
            } else if (format == "webm") {
                options.formats = {PublicationFormat::WebM};
            } else if (format == "both") {
                options.formats = {PublicationFormat::Mp4H264, PublicationFormat::WebM};
            } else {
                usageError("unsupported format: " + format);
            }
        } else if (argument == "--keep-frames") {
            options.keepFrames = true;
        } else {
            usageError("unknown option: " + argument);
        }
    }

    if ((options.command == "frames" || options.command == "video") && !options.outputDirectory) {
        usageError("--output is required for " + options.command);
    }
    return options;
}

void printIssue(const StoryIssue& issue) {
    std::cerr << (issue.storyId.empty() ? "story" : issue.storyId);
    if (issue.sourceLine > 0U) std::cerr << ':' << issue.sourceLine;
    if (issue.sceneIndex) std::cerr << " scene=" << (*issue.sceneIndex + 1U);
    if (issue.actionIndex) std::cerr << " action=" << (*issue.actionIndex + 1U);
    std::cerr << ": " << issue.reason << '\n';
}

std::optional<Story> loadValidatedStory(
    const std::filesystem::path& storyPath,
    const std::filesystem::path& tutorialRoot) {
    const StoryParseResult parsed = parseStoryFile(storyPath);
    if (!parsed) {
        for (const auto& issue : parsed.issues) printIssue(issue);
        return std::nullopt;
    }
    const auto issues = validateStory(*parsed.story, tutorialRoot);
    if (!issues.empty()) {
        for (const auto& issue : issues) printIssue(issue);
        return std::nullopt;
    }
    return parsed.story;
}

std::filesystem::path resolveExecutable(
    const std::optional<std::filesystem::path>& configured,
    const std::string& name) {
    if (configured) {
        const auto resolved = findHostExecutable(configured->string());
        if (resolved) return *resolved;
        throw std::runtime_error("cannot locate " + name + ": " + configured->string());
    }
    const auto resolved = findHostExecutable(name);
    if (!resolved) throw std::runtime_error(name + " not found on PATH");
    return *resolved;
}

std::filesystem::path siblingFramesDirectory(const std::filesystem::path& output) {
    const auto parent = output.parent_path();
    const auto name = output.filename().string() + ".frames";
    return parent.empty() ? std::filesystem::path(name) : parent / name;
}

int execute(const CliOptions& options) {
    const std::filesystem::path sourceRoot = CLOCK_SOURCE_ROOT;
    const std::filesystem::path tutorialRoot = sourceRoot / "docs" / "tutorials";
    const std::filesystem::path storyPath = std::filesystem::absolute(options.storyPath);
    const auto story = loadValidatedStory(storyPath, tutorialRoot);
    if (!story) return EXIT_FAILURE;

    if (options.command == "validate") {
        std::cout << "VALID " << story->id << " — " << story->title << '\n';
        return EXIT_SUCCESS;
    }

    const std::filesystem::path output = std::filesystem::absolute(*options.outputDirectory);
    const std::filesystem::path framesDirectory =
        options.command == "frames" ? output : siblingFramesDirectory(output);
    const std::filesystem::path statePath = framesDirectory.string() + ".state.bin";
    std::error_code ignored;
    std::filesystem::remove(statePath, ignored);

    SimulatorRuntime runtime(statePath);
    runtime.begin();
    StorySimulatorPort port(runtime);
    const auto panelLayout = layout::loadPanelLayout(sourceRoot / "sim" / "panel_layout.ini");
    StoryFramePipeline framePipeline(tutorialRoot, panelLayout, options.sourceRevision);
    std::cout << "Rendering Storybook frames for " << story->id << "..." << std::endl;
    const StoryFramePipelineResult generated = framePipeline.generate(*story, port, framesDirectory);
    runtime.flushPersistence();
    std::filesystem::remove(statePath, ignored);

    if (!generated) {
        for (const auto& issue : generated.issues) printIssue(issue);
        return EXIT_FAILURE;
    }

    if (options.command == "frames") {
        std::cout << "Generated " << generated.frames.size() << " frames in "
                  << generated.outputDirectory.string() << '\n';
        std::cout << "Subtitles: " << generated.srtPath.string() << " and "
                  << generated.vttPath.string() << '\n';
        return EXIT_SUCCESS;
    }

    std::cout << "Generated " << generated.frames.size() << " frames. Starting publication..." << std::endl;
    const auto ffmpeg = resolveExecutable(options.ffmpeg, "ffmpeg");
    const auto ffprobe = resolveExecutable(options.ffprobe, "ffprobe");
    std::cout << "FFmpeg: " << ffmpeg.string() << std::endl;
    std::cout << "FFprobe: " << ffprobe.string() << std::endl;
    StoryPublicationPipeline publication(ffmpeg, ffprobe);
    const StoryPublicationResult published = publication.publish(
        *story, storyPath, generated, output, options.formats);
    if (!published) {
        for (const auto& issue : published.issues) printIssue(issue);
        return EXIT_FAILURE;
    }

    if (!options.keepFrames) std::filesystem::remove_all(framesDirectory, ignored);

    if (published.mp4Path) std::cout << "MP4: " << published.mp4Path->string() << '\n';
    if (published.webmPath) std::cout << "WebM: " << published.webmPath->string() << '\n';
    std::cout << "SRT: " << published.srtPath.string() << '\n';
    std::cout << "WebVTT: " << published.vttPath.string() << '\n';
    std::cout << "Manifest: " << published.manifestPath.string() << '\n';
    return EXIT_SUCCESS;
}

}  // namespace

int main(const int argc, char** argv) {
    if (argc == 2 && std::string(argv[1]) == "--help") {
        printUsage();
        return EXIT_SUCCESS;
    }
    try {
        return execute(parseOptions(argc, argv));
    } catch (const std::exception& exception) {
        std::cerr << "clock-storybook: " << exception.what() << '\n';
        printUsage();
        return EXIT_FAILURE;
    }
}
