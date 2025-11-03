cmake_minimum_required(VERSION 3.22)

project(home_financials VERSION 0.1 LANGUAGES CXX)

# Use the latest stable C++ standard available - request C++23
set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

option(BUILD_TESTS "Build tests" ON)

# Enable testing if BUILD_TESTS is ON
if(BUILD_TESTS)
    enable_testing()
endif()

# Put build outputs into a directory under the build tree
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

add_subdirectory(src)
if (BUILD_TESTS)
        add_subdirectory(tests)
endif()

install(TARGETS home-financials
        RUNTIME DESTINATION bin)
