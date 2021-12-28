cmake_minimum_required(VERSION 3.15)

option(LIBRARY_TESTS "Build unit tests" OFF)

project(xamp_player)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS ON)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")

set(CLANG_FLAGS_ENABLE_AVX2 "-mavx2")

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug)
    message(STATUS "Build debug mode")
    set(BUILD_PATH debug)
else()
    set(CMAKE_VERBOSE_MAKEFILE ON)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/release)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3 -ftree-vectorize -march=native -mtune=native -mavx2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
    message(STATUS "Build release mode")
    set(BUILD_PATH release)
endif(${CMAKE_BUILD_TYPE} STREQUAL Debug)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/${BUILD_PATH})

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories("../xamp_base/include")
include_directories("../xamp_output_device/include")
include_directories("../xamp_stream/include")
include_directories("../xamp_metadata/include")
include_directories("../thirdparty/bass/include/")
include_directories("../thirdparty/spdlog/include/")
include_directories("../thirdparty/soxr/src/")
include_directories("../thirdparty/robin_hood/")
include_directories("../thirdparty/chromaprint/src")

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/src/libebur128/*.c")

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
    LIB_XAMP_METADATA
    NAMES xamp_metadata libxamp_metadata
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_metadata/${BUILD_PATH}/"
    NO_DEFAULT_PATH
)

message(STATUS "LIB_XAMP_METADATA: [${LIB_XAMP_METADATA}]")

find_library(ACCELERATE_LIB Accelerate)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(xamp_player ${LIB_XAMP_BASE} ${LIB_XAMP_OUTPUT_DEVICE} ${LIB_XAMP_STREAM} ${LIB_XAMP_METADATA} ${ACCELERATE_LIB})
