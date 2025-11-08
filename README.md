# home-financials — build instructions

This project uses CMake. The top-level `CMakeLists.txt` requests C++23 for modern compilers.

Quick build (out-of-source):

```bash
mkdir -p build
cd build
cmake -DCMAKE_BUILD_TYPE=Release ..
cmake --build . --parallel
```

The produced binary will be in `build/bin/home-financials`.

Database initialization is handled natively in C++ by `StorageManager`.
When the application starts it will create `homefinancials.db` in the project
root (if missing) and ensure the template tables exist.

Memory checks (Valgrind)
------------------------

If you have Valgrind installed, the repository provides a convenient make
target that builds the tests and runs Valgrind on all test binaries found in
`build/bin/` whose names start with `test_`:

```bash
make valgrind
```

The target will fail if Valgrind reports errors or leaks. You can adjust the
Valgrind flags by setting the `VALGRIND_FLAGS` environment variable, for
example:

```bash
make valgrind VALGRIND_FLAGS="--leak-check=full --show-leak-kinds=all"
```

Note: Valgrind must be installed on your system for the target to run.
