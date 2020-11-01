cmake_minimum_required(VERSION 3.12)
project(linoodle)
enable_language(ASM)
find_package(Threads REQUIRED)

# Build PE parser library
option(BUILD_SHARED_LIBS "" OFF)
add_subdirectory(pe-parse)
set(BUILD_SHARED_LIBS ON)

# Build linoodle
add_library(linoodle linoodle.cpp windows_api.cpp windows_library.cpp)
target_link_libraries(linoodle PRIVATE pe-parse Threads::Threads)

# Build tester
add_executable(testlinoodle test/test.cpp)
target_link_libraries(testlinoodle PRIVATE Threads::Threads ${CMAKE_DL_LIBS})
