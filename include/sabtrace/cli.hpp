#pragma once

#include "sabtrace/analyzer.hpp"

#include <cstddef>
#include <filesystem>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace sabtrace {

inline constexpr std::string_view version = "0.1.0";

struct CliOptions {
    std::vector<std::string> inputs;
    Filters filters;
    std::optional<std::filesystem::path> json_output;
    std::size_t max_results{0};
    bool show_stats{false};
    bool color{true};
    bool quiet{false};
    bool show_help{false};
    bool show_version{false};
};

struct ParseResult {
    std::optional<CliOptions> options;
    std::string error;
};

[[nodiscard]] ParseResult parse_arguments(int argc, const char* const argv[]);
[[nodiscard]] std::string help_text();

}  // namespace sabtrace
