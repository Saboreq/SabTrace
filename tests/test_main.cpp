#include "sabtrace/analyzer.hpp"
#include "sabtrace/cli.hpp"

#include <cstdlib>
#include <iostream>
#include <regex>
#include <sstream>
#include <string>
#include <string_view>

namespace {

int failures = 0;

void expect(bool condition, std::string_view message) {
    if (!condition) {
        ++failures;
        std::cerr << "FAIL: " << message << '\n';
    }
}

void test_level_detection() {
    using sabtrace::LogLevel;
    expect(sabtrace::detect_level("[INFO] server started") == LogLevel::info, "detect INFO");
    expect(sabtrace::detect_level("2026-07-20 ERROR request failed") == LogLevel::error, "detect ERROR");
    expect(sabtrace::detect_level("critical: database unavailable") == LogLevel::fatal, "map critical to FATAL");
    expect(sabtrace::detect_level("warning retry scheduled") == LogLevel::warn, "map warning to WARN");
    expect(sabtrace::detect_level("terror value") == LogLevel::unknown, "respect token boundaries");
}

void test_matcher() {
    sabtrace::Matcher literal({.contains = "Timeout", .regex_pattern = std::nullopt, .level = std::nullopt, .ignore_case = true});
    expect(literal.matches("request TIMEOUT after 30s", sabtrace::LogLevel::error), "case-insensitive literal");
    expect(!literal.matches("request completed", sabtrace::LogLevel::info), "literal non-match");

    sabtrace::Matcher regex({.contains = std::nullopt, .regex_pattern = R"(retry\s+\d+)", .level = std::nullopt, .ignore_case = true});
    expect(regex.matches("Retry 3 scheduled", sabtrace::LogLevel::warn), "regex match");

    sabtrace::Matcher level({.contains = std::nullopt, .regex_pattern = std::nullopt, .level = sabtrace::LogLevel::warn, .ignore_case = false});
    expect(level.matches("anything", sabtrace::LogLevel::warn), "level match");
    expect(!level.matches("anything", sabtrace::LogLevel::error), "level non-match");
}

void test_analysis_and_limit() {
    std::istringstream input(
        "[INFO] started\n"
        "[ERROR] connection refused\n"
        "[WARN] retrying\n"
        "[ERROR] timeout\n");

    const auto result = sabtrace::analyze_stream(
        input,
        "sample.log",
        {.filters = {.contains = std::nullopt, .regex_pattern = std::nullopt, .level = sabtrace::LogLevel::error, .ignore_case = false}, .max_results = 1, .collect_matches = true});

    expect(result.total_lines == 2, "limit stops scanning after first match");
    expect(result.matched_lines == 1, "one match with limit");
    expect(result.matches.size() == 1, "collect one entry");
    expect(result.matches.front().line_number == 2, "preserve source line number");
}

void test_json_escape_and_report() {
    expect(sabtrace::escape_json("a\n\"b") == "a\\n\\\"b", "escape JSON control characters");

    sabtrace::AnalysisResult result;
    result.total_lines = 1;
    result.matched_lines = 1;
    result.all_levels.error = 1;
    result.matched_levels.error = 1;
    result.matches.push_back({"a.log", 7, sabtrace::LogLevel::error, "bad \"value\""});

    std::ostringstream output;
    sabtrace::write_json_report(output, result, {.contains = "bad", .regex_pattern = std::nullopt, .level = std::nullopt, .ignore_case = false});
    const auto json = output.str();
    expect(json.find("\"matched_lines\": 1") != std::string::npos, "JSON includes summary");
    expect(json.find("bad \\\"value\\\"") != std::string::npos, "JSON escapes match text");
}

void test_cli_parser() {
    const char* argv[] = {
        "sabtrace",
        "--level",
        "warning",
        "--contains",
        "retry",
        "--ignore-case",
        "--max-results",
        "10",
        "app.log",
    };
    const auto parsed = sabtrace::parse_arguments(9, argv);
    expect(parsed.options.has_value(), "parse valid CLI");
    if (!parsed.options.has_value()) return;

    expect(parsed.options->filters.level == sabtrace::LogLevel::warn, "normalize warning level");
    expect(parsed.options->filters.contains == "retry", "parse contains");
    expect(parsed.options->filters.ignore_case, "parse ignore case");
    expect(parsed.options->max_results == 10, "parse result limit");
    expect(parsed.options->inputs.size() == 1 && parsed.options->inputs.front() == "app.log", "parse input");

    const char* invalid[] = {"sabtrace", "--max-results", "zero"};
    const auto invalid_result = sabtrace::parse_arguments(3, invalid);
    expect(!invalid_result.options.has_value(), "reject invalid result limit");
}

}  // namespace

int main() {
    test_level_detection();
    test_matcher();
    test_analysis_and_limit();
    test_json_escape_and_report();
    test_cli_parser();

    if (failures == 0) {
        std::cout << "All SabTrace tests passed.\n";
        return EXIT_SUCCESS;
    }

    std::cerr << failures << " test(s) failed.\n";
    return EXIT_FAILURE;
}
