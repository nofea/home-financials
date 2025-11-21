# Home Financials

A terminal-based personal financial management application for tracking family finances, managing bank accounts, and computing net worth.

## Overview

Home Financials is a C++ application that helps you manage your household finances through a terminal user interface (TUI). It allows you to:

- Organize finances by family groups and individual members
- Import bank account statements (currently supports Canara Bank, with extensible support for other banks)
- Track bank accounts and balances
- Compute net worth for individual members and entire families
- Store all data securely in a local SQLite database

## Features

- **Family Management**: Create and manage family groups with multiple members
- **Member Management**: Add, update, and delete family members
- **Bank Statement Import**: Import transaction data from bank statement CSV files
- **Net Worth Calculation**: Automatically compute net worth based on imported bank account data
- **Data Persistence**: All data stored in a local SQLite database (`homefinancials.db`)
- **Terminal UI**: Clean, interactive terminal-based interface

## Prerequisites

To build and run Home Financials, you need:

- **C++ Compiler**: Supporting C++23 standard (GCC 13+ or Clang 16+)
- **CMake**: Version 3.22 or higher
- **SQLite3**: Development libraries (`libsqlite3-dev` on Debian/Ubuntu)
- **Make**: For using the convenient Makefile wrapper
- **Git**: For cloning the repository

Optional for development:
- **Valgrind**: For memory leak detection (optional, for development)
- **GoogleTest**: Automatically downloaded by CMake for building tests

### Installing Dependencies

On Ubuntu/Debian:
```bash
sudo apt-get update
sudo apt-get install build-essential cmake libsqlite3-dev git
```

On macOS (with Homebrew):
```bash
brew install cmake sqlite3
```

## Building the Project

The project uses CMake with a convenient Makefile wrapper that simplifies common tasks.

### Quick Build

```bash
make build
```

This will:
1. Configure the project with CMake in the `build/` directory
2. Build the application with parallel compilation
3. Create the binary at `build/bin/home-financials`

### Manual Build (Direct CMake)

If you prefer to use CMake directly:

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel
```

### Available Make Targets

The Makefile provides several convenient targets:

| Target | Description |
|--------|-------------|
| `make all` | Default target; builds the project (same as `make build`) |
| `make configure` | Run CMake configuration only |
| `make build` | Configure and build the project |
| `make test` | Build and run all tests |
| `make valgrind` | Run Valgrind memory checks on test binaries |
| `make memcheck` | Alias for `make valgrind` |
| `make reconfigure` | Force a complete reconfiguration (removes build directory) |
| `make clean` | Remove all build artifacts |
| `make help` | Display available targets and their descriptions |

### Build Configuration

You can customize the build configuration using environment variables:

```bash
# Build with Debug symbols
make build CMAKE_BUILD_TYPE=Debug

# Use a different build directory
make build BUILD_DIR=mybuild

# Pass additional CMake flags
make build CMAKE_FLAGS="-DCMAKE_CXX_COMPILER=clang++ -DBUILD_TESTS=OFF"
```

## Running the Application

After building, run the application from the project root:

```bash
./build/bin/home-financials
```

Or with explicit TUI mode:

```bash
./build/bin/home-financials --tui
```

To see available command-line options:

```bash
./build/bin/home-financials --help
```

### First Run

On first run, the application will:
1. Create a `homefinancials.db` SQLite database file in the project root
2. Initialize the required database tables automatically
3. Display the main menu

The database file persists between runs and stores all your family, member, and financial data.

## Using the Application

Once launched, you'll see an interactive menu with the following options:

1. **Add Family** - Create a new family group
2. **Delete Family** - Remove a family and all associated data
3. **Add Member** - Add a new member to a family
4. **Update Member** - Modify member information (name, nickname)
5. **Delete Member** - Remove a single member
6. **Delete Multiple Members** - Remove multiple members at once
7. **List Families** - Display all registered families
8. **List Members of Family** - Show all members in a specific family
9. **Import Bank Statement** - Import transaction data from a CSV file
10. **Compute Member Net Worth** - Calculate net worth for a specific member
11. **Compute Family Net Worth** - Calculate total net worth for a family
12. **Exit** - Close the application

### Bank Statement Import

Currently supported banks:
- **Canara Bank** - CSV format statements

The import feature:
- Parses CSV files with transaction data
- Associates accounts with family members
- Stores account balances and transaction history
- Updates net worth calculations automatically

## Testing

### Running Tests

To build and run all tests:

```bash
make test
```

This runs the GoogleTest test suite covering:
- Storage manager (database operations)
- Home manager (business logic)
- TUI manager (user interface)
- Bank statement readers and parsers
- Net worth calculations
- Integration tests

### Memory Leak Detection

To run tests with Valgrind memory checking:

```bash
make valgrind
```

This will:
- Build all test binaries
- Run each test through Valgrind
- Report any memory leaks or errors
- Fail if issues are detected

You can customize Valgrind flags:

```bash
make valgrind VALGRIND_FLAGS="--leak-check=full --show-leak-kinds=all --verbose"
```

**Note**: Valgrind must be installed on your system for this target to work.

## Project Structure

```
home-financials/
├── CMakeLists.txt          # Main CMake configuration
├── Makefile                # Convenient build wrapper
├── README.md               # This file
├── LICENSE                 # MIT License
├── inc/                    # Header files (.hpp)
│   ├── home_manager.hpp    # Core business logic
│   ├── storage_manager.hpp # Database operations
│   ├── tui_manager.hpp     # Terminal UI
│   ├── bank_reader.hpp     # Bank statement parsers
│   └── ...
├── src/                    # Source files (.cpp)
│   ├── main.cpp            # Application entry point
│   └── ...
├── tests/                  # Test suite
│   ├── test_home_manager.cpp
│   ├── test_storage_manager.cpp
│   └── ...
├── doc/                    # Documentation
│   └── srs.md              # Software Requirements Specification
└── build/                  # Build artifacts (generated, not in git)
    └── bin/
        └── home-financials # Main executable
```

## Database

The application uses SQLite3 for local data storage:

- **Database File**: `homefinancials.db` (created in project root)
- **Auto-initialization**: Database and tables created automatically on first run
- **Schema Management**: Handled by `StorageManager` class
- **Not Encrypted**: Currently stores data in plain SQLite format

The database file is excluded from git (via `.gitignore`) to protect your personal financial data.

## Development

### Code Style

- Modern C++ (C++23 standard)
- Allman brace style
- Descriptive variable names (no single-letter variables)
- Const-reference parameter passing for non-trivial types
- Empty lines before/after control blocks for readability

### Architecture

The application follows a layered architecture:

1. **UI Layer**: `TUIManager` - Handles user interaction
2. **Business Logic**: `HomeManager` - Implements domain rules and validation
3. **Data Layer**: `StorageManager` - Manages SQLite database operations
4. **Parsers**: `Reader` classes - Parse bank statement formats

This separation allows for:
- Testable business logic independent of UI
- Isolated database operations
- Extensible bank statement support via reader plugins

### Adding Support for New Banks

To add support for a new bank's statement format:

1. Create a new reader class inheriting from `BankReader`
2. Implement the parsing logic for the bank's CSV format
3. Register the reader using `REGISTER_BANK_READER("BankName", ReaderClass)`
4. Add tests to verify the parser works correctly

See `inc/canara_bank_reader.hpp` and `src/canara_bank_reader.cpp` for an example.

## License

This project is licensed under the MIT License. See the [LICENSE](LICENSE) file for details.

Copyright (c) 2025 Sheen

## Contributing

Contributions are welcome! Please ensure that:
- All tests pass (`make test`)
- No memory leaks are introduced (`make valgrind`)
- Code follows the project's style guidelines
- New features include appropriate tests

## Support

For issues, questions, or suggestions, please open an issue on the GitHub repository.
