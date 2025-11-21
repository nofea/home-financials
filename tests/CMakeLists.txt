cmake_minimum_required(VERSION 3.22)

if (NOT BUILD_TESTS)
    return()
endif()


find_package(SQLite3 REQUIRED)

# Try to find GoogleTest; if not available, fetch it via FetchContent
include(FetchContent)
FetchContent_Declare(
    googletest
    GIT_REPOSITORY https://github.com/google/googletest.git
        GIT_TAG main
)
FetchContent_MakeAvailable(googletest)

# Determine the available gtest target
set(GTEST_LIB_TARGET "")
if (TARGET gtest_main)
    set(GTEST_LIB_TARGET gtest_main)
elseif(TARGET GTest::gtest_main)
    set(GTEST_LIB_TARGET GTest::gtest_main)
else()
    message(FATAL_ERROR "GoogleTest not available; cannot build tests")
endif()

add_executable(test_storage_manager
    test_storage_manager.cpp
)

add_executable(test_home_manager
    test_home_manager.cpp
)

add_executable(test_tui_manager
    test_tui_manager.cpp
    mock_io.cpp
)

add_executable(test_new_features
    test_bank_import.cpp
    test_commons.cpp
    test_reader.cpp
    test_net_worth_class.cpp
    test_bank_account.cpp
    test_integration_import_networth.cpp
)

target_include_directories(test_storage_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)

target_include_directories(test_home_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)

target_include_directories(test_tui_manager PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)

target_include_directories(test_new_features PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)

target_link_libraries(test_storage_manager PRIVATE
    ${GTEST_LIB_TARGET}
    home_financials_lib
    SQLite::SQLite3
)

target_link_libraries(test_home_manager PRIVATE
    ${GTEST_LIB_TARGET}
    home_financials_lib
    SQLite::SQLite3
)

target_link_libraries(test_tui_manager PRIVATE
    ${GTEST_LIB_TARGET}
    home_financials_lib
    SQLite::SQLite3
)

target_link_libraries(test_new_features PRIVATE
    ${GTEST_LIB_TARGET}
    home_financials_lib
    SQLite::SQLite3
)

include(GoogleTest)
gtest_discover_tests(test_storage_manager)
gtest_discover_tests(test_home_manager)
gtest_discover_tests(test_tui_manager)
gtest_discover_tests(test_new_features)
