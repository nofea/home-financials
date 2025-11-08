cmake_minimum_required(VERSION 3.22)

# Source files
set(LIB_SRC
    family.cpp
    member.cpp
    storage_manager.cpp
    home_manager.cpp
    reader.cpp
    bank_reader.cpp
    canara_bank_reader.cpp
    reader_factory.cpp
    ui_manager.cpp
    tui_manager.cpp
    terminal_io.cpp
)

add_library(home_financials_lib STATIC ${LIB_SRC})

target_include_directories(home_financials_lib PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)

add_executable(home-financials main.cpp)
target_link_libraries(home-financials PRIVATE home_financials_lib)

target_include_directories(home-financials PRIVATE
    ${CMAKE_SOURCE_DIR}/inc
)

# Default compile options (add warnings)
if (MSVC)
    target_compile_options(home-financials PRIVATE /W4 /permissive-)
else()
    target_compile_options(home-financials PRIVATE -Wall -Wextra -Wpedantic)
endif()

# Find and link SQLite3
find_package(SQLite3 REQUIRED)
if (TARGET SQLite::SQLite3)
    target_link_libraries(home-financials PRIVATE SQLite::SQLite3)
else()
    target_link_libraries(home-financials PRIVATE ${SQLite3_LIBRARIES})
    target_include_directories(home-financials PRIVATE ${SQLite3_INCLUDE_DIRS})
endif()
