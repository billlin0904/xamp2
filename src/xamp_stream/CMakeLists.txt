cmake_minimum_required(VERSION 3.15)

option(LIBRARY_TESTS "Build unit tests" OFF)

project(xamp_stream)

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
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3 -ftree-vectorize -march=native -mtune=native -mavx2 -flto=thin -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer")
    message(STATUS "Build release mode")
    set(BUILD_PATH release)
endif(${CMAKE_BUILD_TYPE} STREQUAL Debug)

set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/${BUILD_PATH})

include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories("../xamp_base/include")
include_directories("../thirdparty/bass/include/")
include_directories("../thirdparty/soxr/src/")
include_directories("../thirdparty/spdlog/include/")
include_directories("../thirdparty/robin_hood/")
include_directories("../thirdparty/r8brain-free-src/")
include_directories("../thirdparty/intel_mkl/include/")
include_directories("../thirdparty/ffmpeg/include")
include_directories("../thirdparty/fftw/")
include_directories("../thirdparty/supereq/")

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

find_library (
    LIB_XAMP_BASE
    NAMES xamp_base libxamp_base
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/${BUILD_PATH}/"
    NO_DEFAULT_PATH
)

add_library(${PROJECT_NAME} SHARED ${SOURCES})

find_library(ACCELERATE_LIB Accelerate)
find_library(AUDIOTOOL_BOX_LIB AudioToolbox)

target_link_libraries(xamp_stream ${LIB_XAMP_BASE} ${ACCELERATE_LIB} ${AUDIOTOOL_BOX_LIB} ${FFMPEG_LIBRARIES})
