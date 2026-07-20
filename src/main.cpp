#include "sabtrace/analyzer.hpp"
#include "sabtrace/cli.hpp"

#include <array>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iostream>
#include <iterator>
#include <regex>
#include <string>
#include <string_view>

namespace {

constexpr std::string_view reset = "\x1b[0m";
constexpr std::string_view dim = "\x1b[2m";
constexpr std::string_view gray = "\x1b[90m";
constexpr std::string_view cyan = "\x1b[36m";
constexpr std::string_view green = "\x1b[32m";
constexpr std::string_view yellow = "\x1b[33m";
constexpr std::string_view red = "\x1b[31m";
constexpr std::string_view bright_red = "\x1b[91m";

[[nodiscard]] std::string_view level_color(sabtrace::LogLevel level) noexcept {
    using sabtrace::LogLevel;
    switch (level) {
        case LogLevel::trace:
            return gray;
        case LogLevel::debug:
            return cyan;
        case LogLevel::info:
            return green;
        case LogLevel::warn:
            return yellow;
        case LogLevel::error:
            return red;
        case LogLevel::fatal:
            return bright_red;
        case LogLevel::unknown:
            return dim;
    }
    return reset;
}

void merge_counts(sabtrace::LevelCounts& target, const sabtrace::LevelCounts& source) {
    target.trace += source.trace;
    target.debug += source.debug;
    target.info += source.info;
    target.warn += source.warn;
    target.error += source.error;
    target.fatal += source.fatal;
    target.unknown += source.unknown;
}

void merge_result(sabtrace::AnalysisResult& target, sabtrace::AnalysisResult source) {
    target.total_lines += source.total_lines;
    target.matched_lines += source.matched_lines;
    merge_counts(target.all_levels, source.all_levels);
    merge_counts(target.matched_levels, source.matched_levels);
    target.matches.insert(
        target.matches.end(),
        std::make_move_iterator(source.matches.begin()),
        std::make_move_iterator(source.matches.end()));
}

void print_entry(const sabtrace::LogEntry& entry, bool color) {
    if (color) {
        std::cout << dim << entry.source << ':' << entry.line_number << reset << ' '
                  << level_color(entry.level) << '[' << sabtrace::to_string(entry.level) << ']'
                  << reset << ' ' << entry.text << '\n';
        return;
    }

    std::cout << entry.source << ':' << entry.line_number << " ["
              << sabtrace::to_string(entry.level) << "] " << entry.text << '\n';
}

void print_stats(const sabtrace::AnalysisResult& result) {
    std::cout << "\nSummary\n"
              << "  lines scanned: " << result.total_lines << '\n'
              << "  lines matched: " << result.matched_lines << "\n\n"
              << "Level      all   matched\n";

    constexpr std::array<sabtrace::LogLevel, 7> levels{
        sabtrace::LogLevel::trace,
        sabtrace::LogLevel::debug,
        sabtrace::LogLevel::info,
        sabtrace::LogLevel::warn,
        sabtrace::LogLevel::error,
        sabtrace::LogLevel::fatal,
        sabtrace::LogLevel::unknown,
    };

    for (const auto level : levels) {
        std::cout << ' ' << sabtrace::to_string(level);
        const auto padding = 9U - sabtrace::to_string(level).size();
        std::cout << std::string(padding, ' ') << result.all_levels.get(level) << "     "
                  << result.matched_levels.get(level) << '\n';
    }
}

[[nodiscard]] int run(const sabtrace::CliOptions& options) {
    sabtrace::AnalysisResult combined;
    std::size_t remaining = options.max_results;

    for (const auto& input_name : options.inputs) {
        if (options.max_results > 0 && remaining == 0) {
            break;
        }

        sabtrace::AnalysisOptions analysis_options{
            .filters = options.filters,
            .max_results = options.max_results > 0 ? remaining : 0,
            .collect_matches = true,
        };

        sabtrace::AnalysisResult current;
        if (input_name == "-") {
            current = sabtrace::analyze_stream(std::cin, "<stdin>", analysis_options);
        } else {
            std::ifstream input(input_name);
            if (!input) {
                std::cerr << "SabTrace: cannot open input file '" << input_name << "'\n";
                return 2;
            }
            current = sabtrace::analyze_stream(input, input_name, analysis_options);
        }

        if (options.max_results > 0) {
            remaining -= current.matched_lines;
        }
        merge_result(combined, std::move(current));
    }

    if (!options.quiet) {
        for (const auto& entry : combined.matches) {
            print_entry(entry, options.color);
        }
    }

    if (options.show_stats) {
        print_stats(combined);
    }

    if (options.json_output.has_value()) {
        std::ofstream json(*options.json_output, std::ios::binary);
        if (!json) {
            std::cerr << "SabTrace: cannot open JSON output '"
                      << options.json_output->string() << "'\n";
            return 2;
        }
        sabtrace::write_json_report(json, combined, options.filters);
        if (!json) {
            std::cerr << "SabTrace: failed while writing JSON output\n";
            return 2;
        }
    }

    return combined.matched_lines > 0 ? 0 : 1;
}

}  // namespace

int main(int argc, const char* const argv[]) {
    const auto parsed = sabtrace::parse_arguments(argc, argv);
    if (!parsed.options.has_value()) {
        std::cerr << "SabTrace: " << parsed.error << "\n\n" << sabtrace::help_text();
        return 2;
    }

    const auto& options = *parsed.options;
    if (options.show_help) {
        std::cout << sabtrace::help_text();
        return 0;
    }
    if (options.show_version) {
        std::cout << "SabTrace " << sabtrace::version << '\n';
        return 0;
    }

    try {
        return run(options);
    } catch (const std::regex_error& error) {
        std::cerr << "SabTrace: invalid regular expression: " << error.what() << '\n';
        return 2;
    } catch (const std::exception& error) {
        std::cerr << "SabTrace: " << error.what() << '\n';
        return 2;
    }
}
