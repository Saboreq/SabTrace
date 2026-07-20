<div align="center">

# SabTrace

**Fast, script-friendly log filtering and statistics in modern C++20.**

[![CI](https://github.com/Saboreq/SabTrace/actions/workflows/ci.yml/badge.svg)](https://github.com/Saboreq/SabTrace/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Saboreq/SabTrace?display_name=tag&sort=semver)](https://github.com/Saboreq/SabTrace/releases)
[![License](https://img.shields.io/badge/license-MIT-A78BFA.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C.svg?logo=cplusplus)](https://isocpp.org/)

</div>

SabTrace reads plain-text logs from files or standard input, identifies common log levels, applies literal or regular-expression filters, prints useful statistics, and exports structured JSON reports. It is designed as a small standalone binary with no runtime dependencies.

## Highlights

- Plain-text log analysis from one or more files or `stdin`
- Automatic `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, and `FATAL` detection
- Literal, regex, level, and case-insensitive filtering
- Colored terminal output with a `--no-color` option
- Per-level statistics for scanned and matched lines
- Structured JSON report export
- Predictable exit codes for scripts and CI
- Cross-platform CMake build, tests, and release automation

## Quick start

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

The binary is created at `build/release/sabtrace` on Unix-like systems or `build/release/sabtrace.exe` on Windows when using Ninja.

### Analyse a log file

```bash
sabtrace examples/sample.log --level error --stats
```

### Search without case sensitivity

```bash
sabtrace app.log --contains "connection refused" --ignore-case
```

### Use a regular expression and export JSON

```bash
sabtrace app.log --regex "timeout|retry" --json report.json
```

### Read from a pipeline

```bash
cat app.log | sabtrace --level warn --max-results 20
```

PowerShell:

```powershell
Get-Content .\app.log | .\sabtrace.exe --level warn --max-results 20
```

## CLI reference

```text
Usage:
  sabtrace [OPTIONS] [FILE...]
  command | sabtrace [OPTIONS]

Filters:
  --contains TEXT        Keep lines containing literal text
  --regex PATTERN        Keep lines matching an ECMAScript regular expression
  --level LEVEL          Keep TRACE, DEBUG, INFO, WARN, ERROR, FATAL, or UNKNOWN
  -i, --ignore-case      Apply case-insensitive text and regex matching
  --max-results N        Stop after N matches

Output:
  -s, --stats            Print level and match statistics
  -j, --json PATH        Write a structured JSON report
  -q, --quiet            Suppress matched lines
  --no-color             Disable ANSI colors
```

Run `sabtrace --help` for the complete reference and exit-code documentation.

## Example output

```text
examples/sample.log:4 [ERROR] 2026-07-20T09:01:15Z [ERROR] Database connection refused

Summary
  lines scanned: 7
  lines matched: 1

Level      all   matched
 TRACE     0     0
 DEBUG     1     0
 INFO      3     0
 WARN      1     0
 ERROR     1     1
 FATAL     1     0
 UNKNOWN   0     0
```

## JSON report

```json
{
  "tool": "SabTrace",
  "version": "0.1.0",
  "summary": {
    "total_lines": 7,
    "matched_lines": 1
  },
  "matches": [
    {
      "source": "examples/sample.log",
      "line": 4,
      "level": "ERROR",
      "text": "2026-07-20T09:01:15Z [ERROR] Database connection refused"
    }
  ]
}
```

The real report also records active filters and complete level counts.

## Build requirements

- A C++20 compiler:
  - GCC 11 or newer
  - Clang 14 or newer
  - Visual Studio 2022 / MSVC 19.3x or newer
- CMake 3.24 or newer
- Ninja for the included presets

A generator other than Ninja can be used with normal CMake commands:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Releases

The release workflow runs when a tag matching `v*` is pushed. It:

1. Builds and tests SabTrace on Windows, Linux, and macOS.
2. Installs the binary and documentation into a clean staging directory.
3. Creates platform archives.
4. Generates SHA-256 checksums.
5. Publishes the assets in a GitHub Release.

Example:

```bash
git tag v0.1.0
git push origin v0.1.0
```

## Current scope

Version `0.1.0` intentionally focuses on portable plain-text analysis. Timestamp-range filtering, recursive directory scanning, JSON/NDJSON field parsing, follow mode, and performance benchmarks are suitable next milestones.

## Project structure

```text
.github/workflows/    CI and tagged release automation
examples/             Sample log input
include/sabtrace/     Public C++ headers
src/                  CLI and analysis implementation
tests/                Dependency-free unit tests
CMakeLists.txt         Build, test, and install configuration
CMakePresets.json      Reproducible development and release presets
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the development workflow and pull-request expectations.

## License

SabTrace is available under the [MIT License](LICENSE).
