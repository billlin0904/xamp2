cmake_minimum_required(VERSION 3.24)

project(bench
    LANGUAGES CXX)

if(NOT TARGET benchmark::benchmark)
    find_package(benchmark CONFIG QUIET)
endif()

if(NOT TARGET benchmark::benchmark)
    set(BENCHMARK_ENABLE_TESTING OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_GTEST_TESTS OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_INSTALL OFF CACHE BOOL "" FORCE)
    set(BENCHMARK_ENABLE_WERROR OFF CACHE BOOL "" FORCE)
    add_subdirectory(
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/benchmark"
        "${CMAKE_BINARY_DIR}/thirdparty/benchmark")
endif()

add_executable(bench
    bench.cpp)

target_compile_features(bench
    PRIVATE
        cxx_std_23)

target_include_directories(bench
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/utfcpp/source"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/robin_hood")

target_link_libraries(bench
    PRIVATE
        xamp_base
        benchmark::benchmark)

if(MSVC)
    target_compile_options(bench
        PRIVATE
            /utf-8)
else()
    target_compile_options(bench
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic)

    if(CMAKE_SYSTEM_PROCESSOR MATCHES "x86_64|AMD64|amd64")
        target_compile_options(bench
            PRIVATE
                -mavx2)
    endif()

    if(CMAKE_BUILD_TYPE STREQUAL "Release")
        target_compile_options(bench
            PRIVATE
                -O3
                -ftree-vectorize
                -march=native
                -mtune=native)
    endif()
endif()
