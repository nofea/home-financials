# Gemini Code Assist Instructions - Home Financials

This document serves as a guide for AI-assisted development on the Home Financials project. It outlines the architecture, coding standards, and build processes to ensure high-quality, consistent code.

## 1. Folder Structure

- **`/inc`**: Header files (`.hpp`). Contains class definitions and interfaces.
- **`/src`**: Implementation files (`.cpp`). Contains the core logic.
- **`/tests`**: GoogleTest suite for unit and integration testing.
- **`/doc`**: Project documentation, including the Software Requirements Specification (SRS).
- **`/build`**: (Generated) Build artifacts and binaries.
- **`.google/`**: AI-specific instructions and context.

## 2. File Overview

### Core Logic
- `home_manager.hpp/cpp`: The primary facade/orchestrator. It interfaces between the UI and the data layer.
- `storage_manager.hpp/cpp`: Manages the SQLite3 database, schema initialization, and CRUD operations.
- `commons.hpp`: Defines shared types, specifically the `commons::Result` enum used for error handling.

### Domain Models
- `family.hpp/cpp`: Represents a family group.
- `member.hpp/cpp`: Represents an individual family member.
- `bank_account.hpp/cpp`: Data model for persisted bank account information.

### Financial & Parsing
- `bank_reader.hpp`: Abstract base class for bank statement parsers.
- `canara_bank_reader.hpp/cpp`: Concrete implementation for Canara Bank CSVs.
- `reader_factory.hpp/cpp`: Factory and registry for creating `BankReader` instances by name or ID.
- `net_worth.hpp/cpp`: Logic for calculating financial totals.

### UI
- `tui_manager.hpp/cpp`: Terminal User Interface implementation.
- `ui_manager.hpp/cpp`: Base UI helpers and error message translation.

## 3. Build and Compile

The project uses **CMake** with a **Makefile** wrapper for convenience.

### Common Commands:
- **Build Project**: `make build` (Produces binary at `build/bin/home-financials`)
- **Run Tests**: `make test`
- **Memory Check**: `make valgrind`
- **Clean**: `make clean`

### Dependencies:
- C++23 compatible compiler (GCC 11+)
- SQLite3 development libraries (`libsqlite3-dev`)
- GoogleTest (auto-downloaded by CMake)

## 4. Code Style Guidelines

To maintain the existing codebase quality, adhere to these rules:

- **Standard**: Modern C++ (C++23).
- **Brace Style**: **Allman style** (braces on new lines).
  ```cpp
  if (condition)
  {
      // code
  }
  ```
- **Naming**: 
  - Classes: `PascalCase`.
  - Functions/Variables: `camelCase` or `snake_case` (consistent with the specific file's existing style).
  - **No single-letter variables** (except in very short loops).
- **Parameters**: Pass non-trivial types by `const std::string&` or `const Type&`.
- **Readability**: Use empty lines before and after control blocks (if, for, while).
- **Error Handling**: Prefer returning `commons::Result` instead of raw booleans for complex operations to provide better context (e.g., `NotFound`, `DbError`).

## 5. Architectural Patterns

### Layered Architecture
1. **UI Layer** (`TUIManager`) -> 2. **Business Layer** (`HomeManager`) -> 3. **Data Layer** (`StorageManager`).
Logic should rarely skip layers.

### Data Persistence
- SQLite is the engine. 
- `StorageManager` uses "Extended" APIs (suffixed with `Ex`) that return `commons::Result` and often provide an `out_id` pointer for the inserted row's primary key.
- Always enable Foreign Keys: `PRAGMA foreign_keys = ON;`.

### Financial Precision
- Currency is handled in **Paise** (integer) using `long long` to avoid floating-point errors.

### Extensibility
- Adding a new bank requires inheriting from `BankReader` and registering it with `ReaderFactory`.

## 6. Implementation Checklist for Gemini

1. **Documentation**: Add Doxygen-style comments to new public methods.
2. **Resource Management**: Use `std::unique_ptr` and `std::shared_ptr` to manage memory.
3. **Validation**: Validate inputs at the `HomeManager` level before passing them to `StorageManager`.
4. **Testing**: When adding a feature, suggest corresponding test cases in `tests/`.
5. **SRS Compliance**: Ensure changes align with requirements in `doc/srs.md` (e.g., REQ-3: Max 255 members).

---
*This file is internal for AI guidance. Do not modify the structure without updating this document.*