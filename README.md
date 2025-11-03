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
