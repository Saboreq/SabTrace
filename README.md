<div align="center">

# SabTrace

**Fast, script-friendly log analysis in modern C++20.**

[![CI](https://github.com/Saboreq/SabTrace/actions/workflows/ci.yml/badge.svg)](https://github.com/Saboreq/SabTrace/actions/workflows/ci.yml)
[![Release](https://img.shields.io/github/v/release/Saboreq/SabTrace?display_name=tag&sort=semver)](https://github.com/Saboreq/SabTrace/releases)
[![License](https://img.shields.io/badge/license-MIT-A78BFA.svg)](LICENSE)
[![C++](https://img.shields.io/badge/C%2B%2B-20-00599C.svg?logo=cplusplus)](https://isocpp.org/)

</div>

SabTrace is a lightweight command-line tool for searching plain-text logs, isolating important events, reviewing severity statistics, and exporting structured JSON reports. It reads one or more files or standard input and ships as a standalone executable with no third-party runtime dependencies.

## Highlights

- Analyse one or more log files or read directly from `stdin`
- Detect `TRACE`, `DEBUG`, `INFO`, `WARN`, `ERROR`, `FATAL`, and unknown lines
- Filter by severity, literal text, ECMAScript regular expression, or result limit
- Use case-insensitive matching when required
- Print readable terminal output and per-level statistics
- Export structured JSON reports
- Use predictable exit codes in scripts and automated workflows
- Run on Windows, Linux, and macOS

## Download

Prebuilt archives are available on the [Releases page](https://github.com/Saboreq/SabTrace/releases/latest) for Windows, Linux, and macOS. Each release includes SHA-256 checksums so downloaded archives can be verified before use.

After extracting the archive, run:

```bash
./bin/sabtrace --version
```

On Windows:

```powershell
.\bin\sabtrace.exe --version
```

## Usage

### Filter by severity

```bash
sabtrace application.log --level error --stats
```

### Search for a phrase

```bash
sabtrace application.log --contains "connection refused" --ignore-case
```

### Use a regular expression and export JSON

```bash
sabtrace application.log --regex "timeout|retry" --json report.json
```

### Read from a pipeline

```bash
cat application.log | sabtrace --level warn --max-results 20
```

PowerShell:

```powershell
Get-Content .\application.log | .\sabtrace.exe --level warn --max-results 20
```

## Command reference

```text
Usage:
  sabtrace [OPTIONS] [FILE...]
  command | sabtrace [OPTIONS]

Inputs:
  FILE...                Read one or more log files
  -                      Read standard input

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
  --no-color             Disable ANSI colours

General:
  -h, --help             Show help
  -V, --version          Show the version
```

Exit codes:

- `0` — completed with one or more matches
- `1` — completed successfully with no matches
- `2` — invalid arguments, invalid regex, or input/output failure

## Example output

```text
application.log:4 [ERROR] 2026-07-20T09:01:15Z [ERROR] Database connection refused

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

## JSON reports

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
      "source": "application.log",
      "line": 4,
      "level": "ERROR",
      "text": "2026-07-20T09:01:15Z [ERROR] Database connection refused"
    }
  ]
}
```

The complete report also records active filters and per-level counts.

## Build from source

Requirements:

- CMake 3.24 or newer
- A C++20 compiler
- Ninja for the included presets, or another supported CMake generator

Using the provided presets:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

Using a different generator:

```bash
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build --config Release --parallel
ctest --test-dir build -C Release --output-on-failure
```

## Release process

Pushing a semantic version tag such as `v0.2.0` starts the release workflow. It builds and tests the project on each supported platform, packages the executable and documentation, generates SHA-256 checksums, and publishes the assets in a GitHub Release.

```bash
git tag v0.2.0
git push origin v0.2.0
```

## Current scope

Version `0.1.0` focuses on portable plain-text analysis. Potential future additions include timestamp-range filtering, recursive directory scanning, structured JSON/NDJSON fields, follow mode, and performance benchmarks.

## Project structure

```text
.github/workflows/    Continuous integration and release packaging
examples/             Sample log input
include/sabtrace/     Public C++ headers
src/                  CLI and analysis implementation
tests/                Dependency-free tests
CMakeLists.txt         Build, test, and install configuration
CMakePresets.json      Reproducible development and release presets
```

## Contributing

See [CONTRIBUTING.md](CONTRIBUTING.md) for the development workflow and pull-request expectations.

## License

SabTrace is available under the [MIT License](LICENSE).

Developed and maintained by [Saboreq](https://saboreq.xyz), a software development company.
