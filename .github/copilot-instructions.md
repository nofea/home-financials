````instructions
## Quick orientation — Home Financials

- Language: Modern C++ (C++17+). Build uses CMake with a project-provided `Makefile` wrapper. Tests: GoogleTest (bundled).
- Runtime TUI binary: `build/bin/home-financials`.

## High-level architecture (what to read first)

- UI: `TUIManager` — thin interactive layer (`inc/tui_manager.hpp`, `src/tui_manager.cpp`). `src/main.cpp` boots the TUI.
- Application domain: `HomeManager` — business rules and validation (`inc/home_manager.hpp`, `src/home_manager.cpp`).
- Persistence: `StorageManager` — SQLite access and DB file management (`inc/storage_manager.hpp`, `src/storage_manager.cpp`).

Design intent: keep UI thin and push logic into `HomeManager`; isolate DB into `StorageManager` so core logic is testable.

## Parser / reader subsystem

- `Reader` is the abstract base; `BankReader` adds bank-specific helpers. See `inc/reader.hpp`, `src/reader.cpp`, `inc/bank_reader.hpp`.
- Factory: `ReaderFactory` provides `createByBankName`, `createByBankId`, `listRegistered()` (`src/reader_factory.cpp`).
- Registration: concrete readers self-register using `inc/reader_registration.hpp` macro. Example to add a reader:

  REGISTER_BANK_READER("MyBank", MyBankReader)

  Note: `listRegistered()` returns lowercase canonical keys — format them in the UI if you need prettier names.

## Project-specific conventions (short, copyable rules)

- API style: prefer `*Ex` methods for DB work — they return `commons::Result` and often an out-id. Do not add boolean-returning DB helpers.
- Tests depend on exact user-facing strings (e.g., lines beginning with `ID:`). Keep those formats stable.
- I/O is injected: `TUIManager` takes an `IOInterface`; tests use `MockIO` (`tests/mock_io.hpp/.cpp`). Use `MockIO::queueInput()` and assert with `mock->getOutput()` / `getErrors()`.
- Naming: avoid single-letter variables (`s`, `i`, `c`, `k`). Use descriptive names like `accountNumber`, `transactionDate`, `openingBalancePaise`.
- Style: Allman braces (opening brace on the next line). Always use braces for `if/for/while`.

## Code style (must-follow)

- Single-character variable names are prohibited. Always choose names that reflect usage and intent (e.g. `accountNumber`, `transactionDate`, `openingBalancePaise`). Do not introduce `s`, `i`, `c`, `k` as new names in new or refactored code.
- Do not implement `if`, `while`, `for`, or `switch` statements on a single line. The following is forbidden:

  if (cond) doSomething();

  Always expand control statements across multiple lines.
- Always use braces for control blocks and follow Allman style: place the opening brace on the next line and the closing brace on its own line. For example:

  if (condition)
  {
      doSomething();
  }

- Require an empty line before and after top-level control blocks (`if`, `while`, `for`, `switch`) to improve readability. Example:

  // some setup

  if (condition)
  {
      doSomething();
  }

  // follow-up code

## Build & test (essential commands)

- Preferred workflow (project Makefile wrapper):

  cd /home/sheen/Documents/provinggrounds/home-financials
  make build      # configures + builds via CMake
  make test       # builds tests and runs them (prefers test binaries with GTest XML; falls back to ctest)

- Helpful targets: `make configure` (cmake -S . -B build), `make clean` (removes build dir). The Makefile forwards `CMAKE_FLAGS` and defaults to `Release`.

## Common change recipes (examples you will use)

- Add a menu action: add enum to `inc/tui_manager.hpp` (`TUIManager::MenuOption`), update `TUIManager::run()` to gather inputs and call a `HomeManager` method, add method to `inc/home_manager.hpp` / `src/home_manager.cpp`, add/extend `StorageManager` `*Ex` APIs if DB is needed, and add a `tests/` case using `MockIO`.

- Add a bank reader: create `inc/my_bank_reader.hpp` + `src/my_bank_reader.cpp`, implement `BankReader` helpers, register with `REGISTER_BANK_READER("MyBank", MyBankReader)`, add unit tests that exercise `ReaderFactory` and `HomeManager` import flows.

## Files to inspect when debugging or extending

- UI: `src/tui_manager.cpp`, `inc/tui_manager.hpp`
- Business: `src/home_manager.cpp`, `inc/home_manager.hpp`
- Persistence: `src/storage_manager.cpp`, `inc/storage_manager.hpp`; DB file: `homefinancials.db` at repo root
- Reader subsystem: `inc/reader.hpp`, `inc/bank_reader.hpp`, `inc/reader_registration.hpp`, `src/reader_factory.cpp`
- Tests & mocks: `tests/mock_io.hpp/.cpp`, `tests/test_tui_manager.cpp`, other tests in `tests/`.

If you want CI snippets, sanitizer flags, or an automated PR checklist added, tell me which and I will add a short recipe.
cd /home/sheen/Documents/provinggrounds/home-financials
