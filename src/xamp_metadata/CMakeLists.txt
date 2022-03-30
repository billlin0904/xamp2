cmake_minimum_required(VERSION 3.15)

project(xamp_metadata)

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
include_directories("../thirdparty/spdlog/include/")
include_directories("../thirdparty/taglib2/taglib/")
include_directories("../thirdparty/taglib2/taglib/flac/")
include_directories("../thirdparty/taglib2/taglib/mp4/")
include_directories("../thirdparty/taglib2/taglib/mpeg/")
include_directories("../thirdparty/taglib2/taglib/riff/")
include_directories("../thirdparty/taglib2/taglib/mpeg/id3v2/")
include_directories("../thirdparty/taglib2/taglib/toolkit/")
include_directories("../xamp_base/include")
include_directories("../thirdparty/robin_hood/")

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

find_library (
    LIB_XAMP_BASE
    NAMES xamp_base libxamp_base
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/${BUILD_PATH}/"
    NO_DEFAULT_PATH
)

message(STATUS "LIB_XAMP_BASE: [${LIB_XAMP_BASE}]")

find_library (
    LIB_TAGLIB
    NAMES libtag.a
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/osx_${BUILD_PATH}"
    NO_DEFAULT_PATH
)

message(STATUS "LIB_TAGLIB: [${LIB_TAGLIB}]")

add_library(${PROJECT_NAME} SHARED ${SOURCES})

target_link_libraries(xamp_metadata stdc++ ${LIB_XAMP_BASE} ${LIB_TAGLIB})
