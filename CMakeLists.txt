cmake_minimum_required(VERSION 3.0)
project(libzstd-seek-tests)

set(CMAKE_C_STANDARD 99)

add_subdirectory(libzstd-seek)

add_subdirectory(googletest)
include_directories(${gtest_SOURCE_DIR}/include ${gtest_SOURCE_DIR})

add_executable(tests tests.cpp)

target_link_libraries(tests zstd-seek)
target_link_libraries(tests gtest gtest_main)
