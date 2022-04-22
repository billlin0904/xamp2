project(bench)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")

include_directories("../thirdparty/benchmark/include/")
include_directories("../thirdparty/spdlog/include/")
include_directories("../thirdparty/utfcpp/source/")
include_directories("../thirdparty/robin_hood/")

include_directories("../xamp_base/include/")
include_directories("../xamp_output_device/include/")
include_directories("../xamp_stream/include/")
include_directories("../xamp_player/include/")

set(CLANG_FLAGS_ENABLE_AVX2 "-mavx2")
add_executable(bench bench.cpp)

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set(BUILD_PATH debug)
    set_property(TARGET bench PROPERTY COMPILE_FLAGS "-mavx2")
else()
    set(BUILD_PATH release)
    set_property(TARGET bench PROPERTY COMPILE_FLAGS "-DNDEBUG -O3 -ftree-vectorize -march=native -mtune=native -mavx2")
endif(${CMAKE_BUILD_TYPE} STREQUAL Debug)

find_library (
    LIB_XAMP_BASE
    NAMES xamp_base libxamp_base
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/${BUILD_PATH}/"
    NO_DEFAULT_PATH
)

message(STATUS "LIB_XAMP_BASE: [${LIB_XAMP_BASE}]")

find_library (
    LIB_XAMP_OUTPUT_DEVICE
    NAMES xamp_output_device libxamp_output_device
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_output_device/${BUILD_PATH}/"
    NO_DEFAULT_PATH
)

message(STATUS "LIB_XAMP_OUTPUT_DEVICE: [${LIB_XAMP_OUTPUT_DEVICE}]")

find_library (
    LIB_XAMP_STREAM
    NAMES xamp_stream libxamp_stream
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_stream/${BUILD_PATH}/"
    NO_DEFAULT_PATH
)

message(STATUS "LIB_XAMP_STREAM: [${LIB_XAMP_STREAM}]")


find_library (
    LIB_XAMP_PLAYER
    NAMES xamp_player libxamp_player
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_player/${BUILD_PATH}/"
    NO_DEFAULT_PATH
)

message(STATUS "LIB_XAMP_PLAYER: [${LIB_XAMP_PLAYER}]")

find_library (
    LIB_BENCHMARK
    NAMES benchmark libbenchmark
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/benchmark/${BUILD_PATH}/"
    NO_DEFAULT_PATH
)

message(STATUS "LIB_BENCHMARK: [${LIB_BENCHMARK}]")

find_library (
    LIB_BENCHMARK_MAIN
    NAMES benchmark_main libbenchmark_main
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/benchmark/${BUILD_PATH}/"
    NO_DEFAULT_PATH
)

message(STATUS "LIB_BENCHMARK_MAIN: [$LIB_BENCHMARK_MAIN]")

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp")
target_link_libraries(bench ${LIB_XAMP_BASE} ${LIB_XAMP_PLAYER} ${LIB_XAMP_OUTPUT_DEVICE} ${LIB_XAMP_STREAM} ${LIB_BENCHMARK})

