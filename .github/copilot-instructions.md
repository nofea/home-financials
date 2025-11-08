## Quick orientation — Home Financials

- Language: C++ (modern C++). Build: CMake (top-level `CMakeLists.txt`) with a project-provided `Makefile` wrapper. Tests: GoogleTest (bundled). Runtime TUI binary: `build/bin/home-financials`.

## Core architecture — what to read first

- UI: `TUIManager` (`inc/tui_manager.hpp`, `src/tui_manager.cpp`) drives interactive flows; `src/main.cpp` constructs and runs it.
- Application: `HomeManager` (`inc/home_manager.hpp`, `src/home_manager.cpp`) contains business rules and validation.
- Persistence: `StorageManager` (`inc/storage_manager.hpp`, `src/storage_manager.cpp`) manages SQLite and the `homefinancials.db` file.

Design note: UI is intentionally thin. Business rules live in `HomeManager`; DB access is isolated to `StorageManager` so logic is testable and mockable.

## Project-specific conventions (must-follow)

- Prefer `*Ex` APIs: `*Ex` methods return `commons::Result` (and often an out-id). Avoid adding new boolean-returning DB helpers.
- Preserve user-facing strings (tests assert exact messages such as `ID:` or "added successfully").
- Use dependency injection for I/O: `TUIManager` accepts an `IOInterface`; tests inject `MockIO` (`tests/mock_io.hpp`).

## Tests — patterns to follow

- Tests queue inputs via `MockIO::queueInput()` and call `tui->run()`. They assert outputs with `mock->getOutput()` / `getErrors()`.
- When adding features that print IDs, keep the `ID:` format so tests can parse created IDs consistently.

## Build / test (use the project's Makefile)

The repo provides a `Makefile` wrapper that calls CMake for you. Prefer these targets to using raw cmake calls:

- Configure & build:
  make build

- Run tests (builds first if needed):
  make test

- Other helpers:
  make configure   # runs cmake -S . -B build
  make clean       # removes the build directory

Notes:
- The Makefile sets `CMAKE_BUILD_TYPE=Release` by default and forwards flags via `CMAKE_FLAGS`.
- The `test` target prefers running the `test_storage_manager` binary with GTest XML output when present; otherwise it falls back to `ctest --output-on-failure`.

Example reproduce steps (copy/paste):

```bash
cd /home/sheen/Documents/provinggrounds/home-financials
make build
make test
```

## Common change recipe (add a menu action)

1. Add enum value to `inc/tui_manager.hpp` (`TUIManager::MenuOption`).
2. Update `TUIManager::run()` to gather inputs and call a `HomeManager` method.
3. Add the method to `inc/home_manager.hpp` and implement in `src/home_manager.cpp`.
4. If DB work is required, add/extend a `StorageManager` `*Ex` API in `inc/storage_manager.hpp` / `src/storage_manager.cpp`.
5. Add a test in `tests/` using `MockIO` following `test_tui_manager.cpp`.

## Integration & files to inspect when debugging

- SQLite DB: `homefinancials.db` in repo root (created by `StorageManager`).
- UI: `src/tui_manager.cpp`, `inc/tui_manager.hpp`
- Business: `src/home_manager.cpp`, `inc/home_manager.hpp`
- Persistence: `src/storage_manager.cpp`, `inc/storage_manager.hpp`
- Tests/mocks: `tests/test_tui_manager.cpp`, `tests/mock_io.hpp`/`.cpp`

If you'd like CI workflow snippets (GitHub Actions), sanitizer flags, or a PR checklist added here, say which and I'll add them.
