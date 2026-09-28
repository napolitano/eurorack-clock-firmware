/**
 * @file story_yaml.cpp
 * @brief Implements the strict deterministic YAML subset used by CLOCK Storybook schema files.
 * @author Axel Napolitano
 * @copyright 2026 Axel Napolitano
 * @license PolyForm-Noncommercial-1.0.0
 */
#include "tutorial/story_yaml.h"

#include <algorithm>
#include <cctype>
#include <sstream>

namespace clockfw::sim::tutorial {
namespace {

struct SourceLine {
    std::size_t number = 0U;
    std::size_t indent = 0U;
    std::string raw;
    std::string content;
};

std::string trim(const std::string& value) {
    const auto first = std::find_if_not(value.begin(), value.end(), [](const unsigned char ch) { return std::isspace(ch) != 0; });
    const auto last = std::find_if_not(value.rbegin(), value.rend(), [](const unsigned char ch) { return std::isspace(ch) != 0; }).base();
    if (first >= last) {
        return {};
    }
    return std::string(first, last);
}

std::string stripComment(const std::string& value) {
    bool single = false;
    bool dbl = false;
    bool escaped = false;
    for (std::size_t i = 0U; i < value.size(); ++i) {
        const char ch = value[i];
        if (dbl && escaped) {
            escaped = false;
            continue;
        }
        if (dbl && ch == '\\') {
            escaped = true;
            continue;
        }
        if (!dbl && ch == '\'') {
            single = !single;
            continue;
        }
        if (!single && ch == '"') {
            dbl = !dbl;
            continue;
        }
        if (!single && !dbl && ch == '#') {
            if (i == 0U || std::isspace(static_cast<unsigned char>(value[i - 1U])) != 0) {
                return value.substr(0U, i);
            }
        }
    }
    return value;
}

bool isKey(const std::string& key) {
    if (key.empty()) {
        return false;
    }
    return std::all_of(key.begin(), key.end(), [](const unsigned char ch) {
        return std::isalnum(ch) != 0 || ch == '_' || ch == '-';
    });
}

std::size_t findColon(const std::string& value) {
    bool single = false;
    bool dbl = false;
    bool escaped = false;
    for (std::size_t i = 0U; i < value.size(); ++i) {
        const char ch = value[i];
        if (dbl && escaped) {
            escaped = false;
            continue;
        }
        if (dbl && ch == '\\') {
            escaped = true;
            continue;
        }
        if (!dbl && ch == '\'') {
            single = !single;
            continue;
        }
        if (!single && ch == '"') {
            dbl = !dbl;
            continue;
        }
        if (!single && !dbl && ch == ':') {
            return i;
        }
    }
    return std::string::npos;
}

bool decodeQuoted(const std::string& value, std::string& out) {
    if (value.size() < 2U) {
        return false;
    }
    const char quote = value.front();
    if ((quote != '\'' && quote != '"') || value.back() != quote) {
        return false;
    }
    out.clear();
    for (std::size_t i = 1U; i + 1U < value.size(); ++i) {
        char ch = value[i];
        if (quote == '\'' && ch == '\'' && i + 2U < value.size() && value[i + 1U] == '\'') {
            out.push_back('\'');
            ++i;
            continue;
        }
        if (quote == '"' && ch == '\\' && i + 2U < value.size()) {
            const char next = value[++i];
            switch (next) {
                case 'n': out.push_back('\n'); break;
                case 't': out.push_back('\t'); break;
                case 'r': out.push_back('\r'); break;
                case '"': out.push_back('"'); break;
                case '\\': out.push_back('\\'); break;
                default: return false;
            }
            continue;
        }
        out.push_back(ch);
    }
    return true;
}

StoryYamlNode scalarNode(const std::string& token, const std::size_t line, std::vector<StoryYamlIssue>& issues) {
    StoryYamlNode node;
    node.line = line;
    const std::string value = trim(token);
    if (value == "{}") {
        node.type = StoryYamlNode::Type::Mapping;
        return node;
    }
    if (value == "[]") {
        node.type = StoryYamlNode::Type::Sequence;
        return node;
    }
    if (!value.empty() && (value.front() == '{' || value.front() == '[')) {
        issues.push_back({line, "non-empty flow collections are not supported by Storybook YAML"});
        return node;
    }
    if (!value.empty() && (value.front() == '&' || value.front() == '*' || value.front() == '!')) {
        issues.push_back({line, "anchors, aliases and tags are not supported by Storybook YAML"});
        return node;
    }
    node.type = StoryYamlNode::Type::Scalar;
    if (!value.empty() && (value.front() == '\'' || value.front() == '"')) {
        if (!decodeQuoted(value, node.scalar)) {
            issues.push_back({line, "invalid quoted scalar"});
        }
    } else {
        node.scalar = value;
    }
    return node;
}

class Parser final {
public:
    explicit Parser(const std::string_view source) {
        std::istringstream input{std::string(source)};
        std::string raw;
        std::size_t lineNumber = 0U;
        while (std::getline(input, raw)) {
            ++lineNumber;
            if (raw.find('\t') != std::string::npos) {
                issues_.push_back({lineNumber, "tabs are not permitted in Storybook YAML indentation"});
            }
            std::size_t indent = 0U;
            while (indent < raw.size() && raw[indent] == ' ') {
                ++indent;
            }
            SourceLine line;
            line.number = lineNumber;
            line.indent = indent;
            line.raw = raw.substr(indent);
            line.content = trim(stripComment(line.raw));
            lines_.push_back(std::move(line));
        }
    }

    StoryYamlResult parse() {
        StoryYamlResult result;
        std::size_t index = nextSignificant(0U);
        if (index >= lines_.size()) {
            issues_.push_back({0U, "empty YAML document"});
            result.issues = std::move(issues_);
            return result;
        }
        result.root = parseBlock(index, lines_[index].indent);
        const std::size_t trailing = nextSignificant(index);
        if (trailing < lines_.size()) {
            issues_.push_back({lines_[trailing].number, "unexpected trailing YAML content"});
        }
        result.issues = std::move(issues_);
        if (!result.issues.empty()) {
            result.root.reset();
        }
        return result;
    }

private:
    std::size_t nextSignificant(std::size_t index) const {
        while (index < lines_.size() && lines_[index].content.empty()) {
            ++index;
        }
        return index;
    }

    StoryYamlNode parseBlock(std::size_t& index, const std::size_t indent) {
        index = nextSignificant(index);
        if (index >= lines_.size()) {
            return {};
        }
        if (lines_[index].indent != indent) {
            issues_.push_back({lines_[index].number, "inconsistent indentation"});
            return {};
        }
        if (lines_[index].content.rfind("-", 0U) == 0U) {
            return parseSequence(index, indent);
        }
        return parseMapping(index, indent);
    }

    StoryYamlNode parseMapping(std::size_t& index, const std::size_t indent) {
        StoryYamlNode node;
        node.type = StoryYamlNode::Type::Mapping;
        node.line = lines_[index].number;
        while (true) {
            index = nextSignificant(index);
            if (index >= lines_.size() || lines_[index].indent < indent) {
                break;
            }
            if (lines_[index].indent > indent) {
                issues_.push_back({lines_[index].number, "unexpected indentation in mapping"});
                ++index;
                continue;
            }
            if (lines_[index].content.rfind("-", 0U) == 0U) {
                break;
            }
            parseMappingEntry(node, index, indent);
        }
        return node;
    }

    void parseMappingEntry(StoryYamlNode& parent, std::size_t& index, const std::size_t indent) {
        const SourceLine& line = lines_[index];
        const std::size_t colon = findColon(line.content);
        if (colon == std::string::npos) {
            issues_.push_back({line.number, "mapping entry requires ':'"});
            ++index;
            return;
        }
        const std::string key = trim(line.content.substr(0U, colon));
        if (!isKey(key)) {
            issues_.push_back({line.number, "invalid mapping key"});
            ++index;
            return;
        }
        if (std::any_of(parent.mapping.begin(), parent.mapping.end(), [&](const auto& entry) { return entry.first == key; })) {
            issues_.push_back({line.number, "duplicate mapping key '" + key + "'"});
        }
        const std::string rest = trim(line.content.substr(colon + 1U));
        ++index;
        StoryYamlNode value;
        value.line = line.number;
        if (rest == "|") {
            value.type = StoryYamlNode::Type::Scalar;
            value.scalar = parseLiteral(index, indent);
        } else if (!rest.empty()) {
            value = scalarNode(rest, line.number, issues_);
        } else {
            const std::size_t nested = nextSignificant(index);
            if (nested < lines_.size() && lines_[nested].indent > indent) {
                index = nested;
                value = parseBlock(index, lines_[nested].indent);
            }
        }
        parent.mapping.emplace_back(key, std::move(value));
    }

    StoryYamlNode parseSequence(std::size_t& index, const std::size_t indent) {
        StoryYamlNode node;
        node.type = StoryYamlNode::Type::Sequence;
        node.line = lines_[index].number;
        while (true) {
            index = nextSignificant(index);
            if (index >= lines_.size() || lines_[index].indent < indent) {
                break;
            }
            if (lines_[index].indent != indent || lines_[index].content.rfind("-", 0U) != 0U) {
                issues_.push_back({lines_[index].number, "sequence item has invalid indentation"});
                ++index;
                continue;
            }
            const SourceLine line = lines_[index];
            std::string tail = trim(line.content.substr(1U));
            ++index;
            if (tail.empty()) {
                const std::size_t nested = nextSignificant(index);
                if (nested < lines_.size() && lines_[nested].indent > indent) {
                    index = nested;
                    node.sequence.push_back(parseBlock(index, lines_[nested].indent));
                } else {
                    node.sequence.push_back({});
                }
                continue;
            }
            const std::size_t colon = findColon(tail);
            if (colon != std::string::npos) {
                StoryYamlNode item;
                item.type = StoryYamlNode::Type::Mapping;
                item.line = line.number;
                const std::string key = trim(tail.substr(0U, colon));
                const std::string rest = trim(tail.substr(colon + 1U));
                if (!isKey(key)) {
                    issues_.push_back({line.number, "invalid sequence mapping key"});
                }
                StoryYamlNode value;
                value.line = line.number;
                if (!rest.empty()) {
                    value = scalarNode(rest, line.number, issues_);
                } else {
                    const std::size_t nested = nextSignificant(index);
                    if (nested < lines_.size() && lines_[nested].indent > indent) {
                        index = nested;
                        value = parseBlock(index, lines_[nested].indent);
                    }
                }
                item.mapping.emplace_back(key, std::move(value));
                const std::size_t sibling = nextSignificant(index);
                if (sibling < lines_.size() && lines_[sibling].indent > indent &&
                    lines_[sibling].content.rfind("-", 0U) != 0U) {
                    index = sibling;
                    StoryYamlNode extra = parseMapping(index, lines_[sibling].indent);
                    for (auto& entry : extra.mapping) {
                        const bool duplicate = std::any_of(
                            item.mapping.begin(), item.mapping.end(),
                            [&](const auto& existing) { return existing.first == entry.first; });
                        if (duplicate) {
                            issues_.push_back({entry.second.line, "duplicate mapping key '" + entry.first + "'"});
                        } else {
                            item.mapping.push_back(std::move(entry));
                        }
                    }
                }
                node.sequence.push_back(std::move(item));
            } else {
                node.sequence.push_back(scalarNode(tail, line.number, issues_));
            }
        }
        return node;
    }

    std::string parseLiteral(std::size_t& index, const std::size_t parentIndent) {
        std::string result;
        bool baseSet = false;
        std::size_t baseIndent = 0U;
        while (index < lines_.size()) {
            const SourceLine& line = lines_[index];
            if (!line.content.empty() && line.indent <= parentIndent) {
                break;
            }
            if (!baseSet && !trim(line.raw).empty()) {
                baseIndent = line.indent;
                baseSet = true;
            }
            if (baseSet) {
                if (line.indent < baseIndent && !trim(line.raw).empty()) {
                    break;
                }
                if (!result.empty()) {
                    result.push_back('\n');
                }
                if (line.indent >= baseIndent) {
                    result.append(line.indent - baseIndent, ' ');
                    result += line.raw;
                }
            }
            ++index;
        }
        return result;
    }

    std::vector<SourceLine> lines_;
    std::vector<StoryYamlIssue> issues_;
};

}  // namespace

const StoryYamlNode* StoryYamlNode::find(const std::string_view key) const {
    if (type != Type::Mapping) {
        return nullptr;
    }
    const auto it = std::find_if(mapping.begin(), mapping.end(), [&](const auto& entry) { return entry.first == key; });
    return it == mapping.end() ? nullptr : &it->second;
}

StoryYamlResult::operator bool() const {
    return root.has_value() && issues.empty();
}

StoryYamlResult parseStoryYaml(const std::string_view source) {
    Parser parser(source);
    return parser.parse();
}

}  // namespace clockfw::sim::tutorial
