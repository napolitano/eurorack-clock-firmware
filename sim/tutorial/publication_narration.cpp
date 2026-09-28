/**
 * @file publication_narration.cpp
 * @brief Implements publication-timeline narration cue extraction and JSON serialization.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/publication_narration.h"

#include <sstream>
#include <stdexcept>
#include <unordered_map>

namespace clockfw::sim::tutorial {
namespace {

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

}  // namespace

std::vector<PublicationNarrationCue> publicationNarrationCues(
    const StoryRunResult& run,
    const std::uint64_t introOffsetUs) {
    std::vector<PublicationNarrationCue> cues;
    std::unordered_map<std::string, std::uint64_t> open;
    for (const auto& event : run.trace) {
        if (event.kind == StoryTraceKind::NarrationBegin) {
            if (!open.emplace(event.name, event.presentationUs).second) {
                throw std::runtime_error("duplicate open publication narration cue: " + event.name);
            }
        } else if (event.kind == StoryTraceKind::NarrationEnd) {
            const auto found = open.find(event.name);
            if (found == open.end()) {
                throw std::runtime_error("publication narration cue ended without begin: " + event.name);
            }
            cues.push_back({event.name, found->second + introOffsetUs, event.presentationUs + introOffsetUs});
            open.erase(found);
        }
    }
    if (!open.empty()) {
        throw std::runtime_error("publication narration cue did not close: " + open.begin()->first);
    }
    return cues;
}

std::string publicationNarrationManifest(
    const Story& story,
    const std::vector<PublicationNarrationCue>& cues) {
    std::ostringstream out;
    out << "{\n  \"schema\": 1,\n  \"timeline\": \"publication\",\n  \"story_id\": \""
        << jsonEscape(story.id) << "\",\n  \"cues\": [\n";
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

}  // namespace clockfw::sim::tutorial
