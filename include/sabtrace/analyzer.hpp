#pragma once

#include <cstddef>
#include <filesystem>
#include <iosfwd>
#include <optional>
#include <regex>
#include <string>
#include <string_view>
#include <vector>

namespace sabtrace {

enum class LogLevel {
    trace,
    debug,
    info,
    warn,
    error,
    fatal,
    unknown,
};

[[nodiscard]] std::string_view to_string(LogLevel level) noexcept;
[[nodiscard]] std::optional<LogLevel> parse_level(std::string_view value) noexcept;
[[nodiscard]] LogLevel detect_level(std::string_view line) noexcept;

struct Filters {
    std::optional<std::string> contains;
    std::optional<std::string> regex_pattern;
    std::optional<LogLevel> level;
    bool ignore_case{false};
};

class Matcher {
public:
    explicit Matcher(Filters filters);

    [[nodiscard]] bool matches(std::string_view line, LogLevel level) const;
    [[nodiscard]] const Filters& filters() const noexcept;

private:
    Filters filters_;
    std::optional<std::regex> regex_;
    std::optional<std::string> normalized_contains_;
};

struct LogEntry {
    std::string source;
    std::size_t line_number{0};
    LogLevel level{LogLevel::unknown};
    std::string text;
};

struct LevelCounts {
    std::size_t trace{0};
    std::size_t debug{0};
    std::size_t info{0};
    std::size_t warn{0};
    std::size_t error{0};
    std::size_t fatal{0};
    std::size_t unknown{0};

    void add(LogLevel level) noexcept;
    [[nodiscard]] std::size_t get(LogLevel level) const noexcept;
};

struct AnalysisResult {
    std::size_t total_lines{0};
    std::size_t matched_lines{0};
    LevelCounts all_levels;
    LevelCounts matched_levels;
    std::vector<LogEntry> matches;
};

struct AnalysisOptions {
    Filters filters;
    std::size_t max_results{0};
    bool collect_matches{true};
};

[[nodiscard]] AnalysisResult analyze_stream(
    std::istream& input,
    std::string source,
    const AnalysisOptions& options);

[[nodiscard]] std::string escape_json(std::string_view value);
void write_json_report(
    std::ostream& output,
    const AnalysisResult& result,
    const Filters& filters);

}  // namespace sabtrace
