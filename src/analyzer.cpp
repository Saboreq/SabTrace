#include "sabtrace/analyzer.hpp"

#include <algorithm>
#include <array>
#include <cctype>
#include <iomanip>
#include <istream>
#include <ostream>
#include <sstream>
#include <stdexcept>
#include <utility>

namespace sabtrace {
namespace {

[[nodiscard]] char ascii_lower(char value) noexcept {
    const auto byte = static_cast<unsigned char>(value);
    return static_cast<char>(std::tolower(byte));
}

[[nodiscard]] std::string lowercase(std::string_view value) {
    std::string result(value);
    std::transform(result.begin(), result.end(), result.begin(), ascii_lower);
    return result;
}

[[nodiscard]] bool is_token_character(char value) noexcept {
    const auto byte = static_cast<unsigned char>(value);
    return std::isalnum(byte) != 0 || value == '_';
}

[[nodiscard]] bool contains_token_case_insensitive(
    std::string_view line,
    std::string_view token) noexcept {
    if (token.empty() || line.size() < token.size()) {
        return false;
    }

    for (std::size_t index = 0; index + token.size() <= line.size(); ++index) {
        bool equal = true;
        for (std::size_t offset = 0; offset < token.size(); ++offset) {
            if (ascii_lower(line[index + offset]) != ascii_lower(token[offset])) {
                equal = false;
                break;
            }
        }

        if (!equal) {
            continue;
        }

        const bool left_boundary = index == 0 || !is_token_character(line[index - 1]);
        const std::size_t end = index + token.size();
        const bool right_boundary = end == line.size() || !is_token_character(line[end]);
        if (left_boundary && right_boundary) {
            return true;
        }
    }

    return false;
}

[[nodiscard]] std::string optional_json_string(const std::optional<std::string>& value) {
    if (!value.has_value()) {
        return "null";
    }
    return '"' + escape_json(*value) + '"';
}

void write_level_counts(std::ostream& output, const LevelCounts& counts, int indent) {
    const std::string padding(static_cast<std::size_t>(indent), ' ');
    output << "{\n"
           << padding << "  \"trace\": " << counts.trace << ",\n"
           << padding << "  \"debug\": " << counts.debug << ",\n"
           << padding << "  \"info\": " << counts.info << ",\n"
           << padding << "  \"warn\": " << counts.warn << ",\n"
           << padding << "  \"error\": " << counts.error << ",\n"
           << padding << "  \"fatal\": " << counts.fatal << ",\n"
           << padding << "  \"unknown\": " << counts.unknown << '\n'
           << padding << '}';
}

}  // namespace

std::string_view to_string(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::trace:
            return "TRACE";
        case LogLevel::debug:
            return "DEBUG";
        case LogLevel::info:
            return "INFO";
        case LogLevel::warn:
            return "WARN";
        case LogLevel::error:
            return "ERROR";
        case LogLevel::fatal:
            return "FATAL";
        case LogLevel::unknown:
            return "UNKNOWN";
    }
    return "UNKNOWN";
}

std::optional<LogLevel> parse_level(std::string_view value) noexcept {
    const auto normalized = lowercase(value);
    if (normalized == "trace") return LogLevel::trace;
    if (normalized == "debug") return LogLevel::debug;
    if (normalized == "info") return LogLevel::info;
    if (normalized == "warn" || normalized == "warning") return LogLevel::warn;
    if (normalized == "error" || normalized == "err") return LogLevel::error;
    if (normalized == "fatal" || normalized == "critical") return LogLevel::fatal;
    if (normalized == "unknown") return LogLevel::unknown;
    return std::nullopt;
}

LogLevel detect_level(std::string_view line) noexcept {
    static constexpr std::array<std::pair<std::string_view, LogLevel>, 9> levels{{
        {"fatal", LogLevel::fatal},
        {"critical", LogLevel::fatal},
        {"error", LogLevel::error},
        {"err", LogLevel::error},
        {"warning", LogLevel::warn},
        {"warn", LogLevel::warn},
        {"info", LogLevel::info},
        {"debug", LogLevel::debug},
        {"trace", LogLevel::trace},
    }};

    for (const auto& [token, level] : levels) {
        if (contains_token_case_insensitive(line, token)) {
            return level;
        }
    }
    return LogLevel::unknown;
}

Matcher::Matcher(Filters filters) : filters_(std::move(filters)) {
    if (filters_.contains.has_value()) {
        normalized_contains_ = filters_.ignore_case
            ? lowercase(*filters_.contains)
            : *filters_.contains;
    }

    if (filters_.regex_pattern.has_value()) {
        auto flags = std::regex_constants::ECMAScript;
        if (filters_.ignore_case) {
            flags |= std::regex_constants::icase;
        }
        regex_.emplace(*filters_.regex_pattern, flags);
    }
}

bool Matcher::matches(std::string_view line, LogLevel level) const {
    if (filters_.level.has_value() && level != *filters_.level) {
        return false;
    }

    if (normalized_contains_.has_value()) {
        if (filters_.ignore_case) {
            if (lowercase(line).find(*normalized_contains_) == std::string::npos) {
                return false;
            }
        } else if (line.find(*normalized_contains_) == std::string_view::npos) {
            return false;
        }
    }

    if (regex_.has_value()) {
        const std::string owned_line(line);
        if (!std::regex_search(owned_line, *regex_)) {
            return false;
        }
    }

    return true;
}

const Filters& Matcher::filters() const noexcept {
    return filters_;
}

void LevelCounts::add(LogLevel level) noexcept {
    switch (level) {
        case LogLevel::trace:
            ++trace;
            break;
        case LogLevel::debug:
            ++debug;
            break;
        case LogLevel::info:
            ++info;
            break;
        case LogLevel::warn:
            ++warn;
            break;
        case LogLevel::error:
            ++error;
            break;
        case LogLevel::fatal:
            ++fatal;
            break;
        case LogLevel::unknown:
            ++unknown;
            break;
    }
}

std::size_t LevelCounts::get(LogLevel level) const noexcept {
    switch (level) {
        case LogLevel::trace:
            return trace;
        case LogLevel::debug:
            return debug;
        case LogLevel::info:
            return info;
        case LogLevel::warn:
            return warn;
        case LogLevel::error:
            return error;
        case LogLevel::fatal:
            return fatal;
        case LogLevel::unknown:
            return unknown;
    }
    return 0;
}

AnalysisResult analyze_stream(
    std::istream& input,
    std::string source,
    const AnalysisOptions& options) {
    const Matcher matcher(options.filters);
    AnalysisResult result;
    std::string line;
    std::size_t line_number = 0;

    while (std::getline(input, line)) {
        ++line_number;
        ++result.total_lines;

        const LogLevel level = detect_level(line);
        result.all_levels.add(level);

        if (!matcher.matches(line, level)) {
            continue;
        }

        ++result.matched_lines;
        result.matched_levels.add(level);

        if (options.collect_matches) {
            result.matches.push_back(LogEntry{
                .source = source,
                .line_number = line_number,
                .level = level,
                .text = line,
            });
        }

        if (options.max_results > 0 && result.matched_lines >= options.max_results) {
            break;
        }
    }

    return result;
}

std::string escape_json(std::string_view value) {
    std::ostringstream output;
    output << std::hex << std::setfill('0');

    for (const unsigned char character : value) {
        switch (character) {
            case '"':
                output << "\\\"";
                break;
            case '\\':
                output << "\\\\";
                break;
            case '\b':
                output << "\\b";
                break;
            case '\f':
                output << "\\f";
                break;
            case '\n':
                output << "\\n";
                break;
            case '\r':
                output << "\\r";
                break;
            case '\t':
                output << "\\t";
                break;
            default:
                if (character < 0x20U) {
                    output << "\\u" << std::setw(4) << static_cast<unsigned int>(character);
                } else {
                    output << static_cast<char>(character);
                }
                break;
        }
    }

    return output.str();
}

void write_json_report(
    std::ostream& output,
    const AnalysisResult& result,
    const Filters& filters) {
    output << "{\n"
           << "  \"tool\": \"SabTrace\",\n"
           << "  \"version\": \"0.1.0\",\n"
           << "  \"filters\": {\n"
           << "    \"contains\": " << optional_json_string(filters.contains) << ",\n"
           << "    \"regex\": " << optional_json_string(filters.regex_pattern) << ",\n"
           << "    \"level\": ";

    if (filters.level.has_value()) {
        output << '"' << to_string(*filters.level) << '"';
    } else {
        output << "null";
    }

    output << ",\n"
           << "    \"ignore_case\": " << (filters.ignore_case ? "true" : "false") << "\n"
           << "  },\n"
           << "  \"summary\": {\n"
           << "    \"total_lines\": " << result.total_lines << ",\n"
           << "    \"matched_lines\": " << result.matched_lines << ",\n"
           << "    \"all_levels\": ";
    write_level_counts(output, result.all_levels, 4);
    output << ",\n    \"matched_levels\": ";
    write_level_counts(output, result.matched_levels, 4);
    output << "\n  },\n"
           << "  \"matches\": [";

    if (!result.matches.empty()) {
        output << '\n';
    }

    for (std::size_t index = 0; index < result.matches.size(); ++index) {
        const auto& entry = result.matches[index];
        output << "    {\n"
               << "      \"source\": \"" << escape_json(entry.source) << "\",\n"
               << "      \"line\": " << entry.line_number << ",\n"
               << "      \"level\": \"" << to_string(entry.level) << "\",\n"
               << "      \"text\": \"" << escape_json(entry.text) << "\"\n"
               << "    }";
        if (index + 1 < result.matches.size()) {
            output << ',';
        }
        output << '\n';
    }

    output << "  ]\n}\n";
}

}  // namespace sabtrace
