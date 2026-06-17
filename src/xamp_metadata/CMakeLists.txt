cmake_minimum_required(VERSION 3.24)

project(xamp_metadata LANGUAGES CXX)

include("${CMAKE_SOURCE_DIR}/cmake/XampDependencyHelpers.cmake")

xamp_find_imported_library(XAMP_LIBCUE_TARGET XampLibCue
    REQUIRED
    HEADER libcue.h
    LIB_NAMES cue libcue
    PKG libcue)

if(NOT TARGET xamp_base)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../xamp_base" "${CMAKE_BINARY_DIR}/xamp_base")
endif()

if(NOT TARGET tag)
    set(BUILD_BINDINGS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2" "${CMAKE_BINARY_DIR}/taglib2")
endif()

file(GLOB_RECURSE XAMP_METADATA_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
)

file(GLOB_RECURSE XAMP_METADATA_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/include/*.h"
)

add_library(xamp_metadata SHARED
    ${XAMP_METADATA_SOURCES}
    ${XAMP_METADATA_HEADERS}
)

target_compile_features(xamp_metadata PUBLIC cxx_std_23)

target_compile_definitions(xamp_metadata
    PRIVATE
        XAMP_METADATA_LIBRARY
)

if(UNIX AND NOT APPLE)
    target_compile_options(xamp_metadata PRIVATE -fpermissive)
endif()

target_include_directories(xamp_metadata
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/unordered_dense/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/ape"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/dsdiff"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/dsf"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/flac"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/mp4"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/mpeg"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/mpeg/id3v2"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/mpeg/id3v2/frames"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/ogg"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/ogg/opus"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/riff"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/riff/wav"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib/toolkit"
        "${CMAKE_BINARY_DIR}/taglib2"
)

target_link_libraries(xamp_metadata
    PUBLIC
        xamp_base
    PRIVATE
        tag
        ${XAMP_LIBCUE_TARGET}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(xamp_metadata PRIVATE dl pthread)
endif()
