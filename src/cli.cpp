#include "sabtrace/cli.hpp"

#include <charconv>
#include <string_view>
#include <system_error>

namespace sabtrace {
namespace {

[[nodiscard]] bool needs_value(std::string_view argument) {
    return argument == "--contains" || argument == "--regex" || argument == "--level" ||
           argument == "--json" || argument == "--max-results";
}

[[nodiscard]] std::optional<std::size_t> parse_size(std::string_view value) {
    std::size_t parsed = 0;
    const auto* first = value.data();
    const auto* last = value.data() + value.size();
    const auto [position, error] = std::from_chars(first, last, parsed);
    if (error != std::errc{} || position != last) {
        return std::nullopt;
    }
    return parsed;
}

}  // namespace

ParseResult parse_arguments(int argc, const char* const argv[]) {
    CliOptions options;

    for (int index = 1; index < argc; ++index) {
        const std::string_view argument(argv[index]);

        if (argument == "-h" || argument == "--help") {
            options.show_help = true;
            continue;
        }
        if (argument == "-V" || argument == "--version") {
            options.show_version = true;
            continue;
        }
        if (argument == "-i" || argument == "--ignore-case") {
            options.filters.ignore_case = true;
            continue;
        }
        if (argument == "-s" || argument == "--stats") {
            options.show_stats = true;
            continue;
        }
        if (argument == "-q" || argument == "--quiet") {
            options.quiet = true;
            continue;
        }
        if (argument == "--no-color") {
            options.color = false;
            continue;
        }
        if (argument == "--") {
            for (++index; index < argc; ++index) {
                options.inputs.emplace_back(argv[index]);
            }
            break;
        }

        if (needs_value(argument)) {
            if (index + 1 >= argc) {
                return {.options = std::nullopt, .error = "missing value for " + std::string(argument)};
            }
            const std::string value(argv[++index]);

            if (argument == "--contains") {
                options.filters.contains = value;
            } else if (argument == "--regex") {
                options.filters.regex_pattern = value;
            } else if (argument == "--level") {
                const auto level = parse_level(value);
                if (!level.has_value()) {
                    return {.options = std::nullopt, .error = "invalid level '" + value + "'"};
                }
                options.filters.level = *level;
            } else if (argument == "--json") {
                options.json_output = value;
            } else if (argument == "--max-results") {
                const auto limit = parse_size(value);
                if (!limit.has_value() || *limit == 0) {
                    return {.options = std::nullopt, .error = "--max-results must be a positive integer"};
                }
                options.max_results = *limit;
            }
            continue;
        }

        if (!argument.empty() && argument.front() == '-' && argument != "-") {
            return {.options = std::nullopt, .error = "unknown option '" + std::string(argument) + "'"};
        }

        options.inputs.emplace_back(argument);
    }

    if (options.inputs.empty()) {
        options.inputs.emplace_back("-");
    }

    return {.options = std::move(options), .error = {}};
}

std::string help_text() {
    return R"(SabTrace - fast, script-friendly log filtering and statistics

Usage:
  sabtrace [OPTIONS] [FILE...]
  command | sabtrace [OPTIONS]

Inputs:
  FILE...                Read one or more log files
  -                      Read standard input (default when no file is provided)

Filters:
  --contains TEXT        Keep lines containing literal text
  --regex PATTERN        Keep lines matching an ECMAScript regular expression
  --level LEVEL          Keep TRACE, DEBUG, INFO, WARN, ERROR, FATAL, or UNKNOWN
  -i, --ignore-case      Apply case-insensitive text and regex matching
  --max-results N        Stop after N matches

Output:
  -s, --stats            Print level and match statistics
  -j, --json PATH        Write a structured JSON report
  -q, --quiet            Suppress matched lines; useful with --stats or --json
  --no-color             Disable ANSI colors

General:
  -h, --help             Show this help
  -V, --version          Show the version

Exit codes:
  0  Completed with at least one match
  1  Completed successfully but found no matches
  2  Invalid arguments, invalid regex, or an input/output error

Examples:
  sabtrace app.log --level error --stats
  sabtrace logs.txt --contains "connection refused" --ignore-case
  sabtrace app.log --regex "timeout|retry" --json report.json
  cat app.log | sabtrace --level warn --max-results 20
)";
}

}  // namespace sabtrace
