# Contributing to SabTrace

## Development setup

```bash
cmake --preset dev
cmake --build --preset dev
ctest --preset dev
```

## Pull requests

Keep each change focused and explain:

- the problem being solved;
- the chosen behavior and any compatibility impact;
- tests added or updated;
- commands used for validation.

Before opening a pull request, run:

```bash
cmake --preset release
cmake --build --preset release
ctest --preset release
```

## Code style

- Use C++20 standard-library facilities where practical.
- Avoid runtime dependencies unless they provide a clear user benefit.
- Keep parsing, analysis, and terminal presentation separated.
- Treat malformed user input as a normal error path, not undefined behavior.
- Add tests for bug fixes and externally visible behavior.

The repository includes a `.clang-format` file for consistent formatting.
