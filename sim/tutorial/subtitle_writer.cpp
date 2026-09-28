/**
 * @file subtitle_writer.cpp
 * @brief Implements subtitle interval extraction and deterministic sidecar serialization.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/subtitle_writer.h"

#include <fstream>
#include <iomanip>
#include <optional>
#include <sstream>
#include <stdexcept>

namespace clockfw::sim::tutorial {
namespace {

std::string timestamp(const std::uint64_t us, const char millisecondSeparator) {
    const std::uint64_t totalMs = us / 1000ULL;
    const std::uint64_t hours = totalMs / 3600000ULL;
    const std::uint64_t minutes = (totalMs / 60000ULL) % 60ULL;
    const std::uint64_t seconds = (totalMs / 1000ULL) % 60ULL;
    const std::uint64_t milliseconds = totalMs % 1000ULL;
    std::ostringstream out;
    out << std::setfill('0') << std::setw(2) << hours << ':'
        << std::setw(2) << minutes << ':' << std::setw(2) << seconds
        << millisecondSeparator << std::setw(3) << milliseconds;
    return out.str();
}

void writeText(const std::filesystem::path& path, const std::string& text) {
    std::ofstream stream(path, std::ios::binary | std::ios::trunc);
    if (!stream) throw std::runtime_error("cannot create subtitle sidecar: " + path.string());
    stream.write(text.data(), static_cast<std::streamsize>(text.size()));
    if (!stream) throw std::runtime_error("cannot write subtitle sidecar: " + path.string());
}

}  // namespace

std::vector<SubtitleCue> subtitleCuesFromTrace(const StoryRunResult& run) {
    struct OpenCue {
        std::size_t sceneIndex = 0U;
        std::uint64_t startUs = 0ULL;
        std::string text;
    };
    std::optional<OpenCue> open;
    std::vector<SubtitleCue> cues;

    auto closeAt = [&](const std::uint64_t endUs) {
        if (!open.has_value()) return;
        if (endUs > open->startUs && !open->text.empty()) {
            cues.push_back({open->startUs, endUs, open->text});
        }
        open.reset();
    };

    for (const StoryTraceEvent& event : run.trace) {
        if (event.kind == StoryTraceKind::Subtitle && event.sceneIndex.has_value()) {
            closeAt(event.presentationUs);
            if (!event.value.empty()) {
                open = OpenCue{*event.sceneIndex, event.presentationUs, event.value};
            }
            continue;
        }
        if (event.kind == StoryTraceKind::SceneEnd && event.sceneIndex.has_value() &&
            open.has_value() && open->sceneIndex == *event.sceneIndex) {
            closeAt(event.presentationUs);
        }
    }
    closeAt(run.presentationDurationUs);
    return cues;
}

std::string renderSrt(const std::vector<SubtitleCue>& cues) {
    std::ostringstream out;
    for (std::size_t index = 0U; index < cues.size(); ++index) {
        out << (index + 1U) << '\n'
            << timestamp(cues[index].startUs, ',') << " --> " << timestamp(cues[index].endUs, ',') << '\n'
            << cues[index].text << "\n\n";
    }
    return out.str();
}

std::string renderWebVtt(const std::vector<SubtitleCue>& cues) {
    std::ostringstream out;
    out << "WEBVTT\n\n";
    for (const SubtitleCue& cue : cues) {
        out << timestamp(cue.startUs, '.') << " --> " << timestamp(cue.endUs, '.') << '\n'
            << cue.text << "\n\n";
    }
    return out.str();
}

void writeSubtitleSidecars(
    const std::filesystem::path& srtPath,
    const std::filesystem::path& vttPath,
    const std::vector<SubtitleCue>& cues) {
    writeText(srtPath, renderSrt(cues));
    writeText(vttPath, renderWebVtt(cues));
}

}  // namespace clockfw::sim::tutorial
