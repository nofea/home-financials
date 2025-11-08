````instructions
## Quick orientation — Home Financials

- Language: C++ (modern C++). Build: CMake (top-level `CMakeLists.txt`) with a project-provided `Makefile` wrapper. Tests: GoogleTest (bundled). Runtime TUI binary: `build/bin/home-financials`.

## Core architecture — what to read first

- UI: `TUIManager` (`inc/tui_manager.hpp`, `src/tui_manager.cpp`) drives interactive flows; `src/main.cpp` constructs and runs it.
- Application: `HomeManager` (`inc/home_manager.hpp`, `src/home_manager.cpp`) contains business rules and validation.
- Persistence: `StorageManager` (`inc/storage_manager.hpp`, `src/storage_manager.cpp`) manages SQLite and the `homefinancials.db` file.

Design note: UI is intentionally thin. Business rules live in `HomeManager`; DB access is isolated to `StorageManager` so logic is testable and mockable.

Parser layer
- The project includes a small parser/reader subsystem for importing bank statements. Key points:
  - `Reader` is the abstract base for file parsers.
  - `BankReader` extends `Reader` with bank-specific helpers and `BankAccountInfo` extraction.
  - `ReaderFactory` provides runtime creation: `createByBankName`, `createByBankId`, and `listRegistered()`.
  - Concrete readers self-register via a small macro in `inc/reader_registration.hpp`. Example:

    REGISTER_BANK_READER("Canara", CanaraBankReader)

  - `listRegistered()` returns lowercase canonical keys; format them in the UI if you want prettier names.

## Project-specific conventions (must-follow)

- Prefer `*Ex` APIs: `*Ex` methods return `commons::Result` (and often an out-id). Avoid adding new boolean-returning DB helpers.
- Preserve user-facing strings (tests assert exact messages such as `ID:` or "added successfully").
- Use dependency injection for I/O: `TUIManager` accepts an `IOInterface`; tests inject `MockIO` (`tests/mock_io.hpp`).

Variable naming (must-follow)

- Never declare single-letter variable names such as `s`, `i`, `c`, `k`, etc. Use human-readable names that describe the variable's purpose at the point of declaration. Examples:
  - `s` -> `str` or `line` or `text`
  - `i` -> `index` or `pos` or `rowIndex`
  - `c` -> `character` or `ch`
  - `k` -> `key` or `columnKey`

- Prefer names that clarify intent (e.g. `accountNumber`, `openingBalancePaise`, `transactionDate`). This improves readability, tests, and code reviews.

- Do not rename existing identifiers in older files just to satisfy this rule unless you are updating surrounding code in the same change set — apply the rule to all new or refactored code going forward.


## Tests — patterns to follow

## Coding style (must-follow)

- Always use braces for `if`, `while`, `for`. For example prefer:

  -

    if (condition)
    {
        doSomething();
    }

- Never write control statements in a single-line form (e.g., `if (cond) doSomething();`). This reduces subtle bugs and improves diff clarity.

- Always place the opening brace on the next line (Allman style). Closing brace should be on its own line.

These rules help maintain a consistent, easily-reviewable codebase and reduce errors when modifying control-flow blocks.


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

Reader-specific recipe (adding a bank reader)
1. Implement a concrete `BankReader` in `inc/` and `src/` (e.g. `my_bank_reader.hpp` / `my_bank_reader.cpp`).
2. Register it using the convenience macro from `inc/reader_registration.hpp`:

   REGISTER_BANK_READER("MyBank", MyBankReader)

3. Add unit tests under `tests/` that exercise parsing and HomeManager import flows.
4. Run `make test` and iterate until green.

## Integration & files to inspect when debugging

- SQLite DB: `homefinancials.db` in repo root (created by `StorageManager`).
- UI: `src/tui_manager.cpp`, `inc/tui_manager.hpp`
- Business: `src/home_manager.cpp`, `inc/home_manager.hpp`
- Persistence: `src/storage_manager.cpp`, `inc/storage_manager.hpp`
- Tests/mocks: `tests/test_tui_manager.cpp`, `tests/mock_io.hpp`/`.cpp`

If you'd like CI workflow snippets (GitHub Actions), sanitizer flags, or a PR checklist added here, say which and I'll add them.

````

