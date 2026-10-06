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

if(APPLE)
    set(XAMP_TAGLIB_DIR "${CMAKE_SOURCE_DIR}/out/deps/taglib")
else()
    set(XAMP_TAGLIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2")
endif()

if(NOT TARGET tag)
    set(BUILD_BINDINGS OFF CACHE BOOL "" FORCE)
    set(BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON)
    add_subdirectory("${XAMP_TAGLIB_DIR}" "${CMAKE_BINARY_DIR}/taglib2")
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
        "${XAMP_TAGLIB_DIR}/taglib"
        "${XAMP_TAGLIB_DIR}/taglib/ape"
        "${XAMP_TAGLIB_DIR}/taglib/dsdiff"
        "${XAMP_TAGLIB_DIR}/taglib/dsf"
        "${XAMP_TAGLIB_DIR}/taglib/flac"
        "${XAMP_TAGLIB_DIR}/taglib/mp4"
        "${XAMP_TAGLIB_DIR}/taglib/mpeg"
        "${XAMP_TAGLIB_DIR}/taglib/mpeg/id3v2"
        "${XAMP_TAGLIB_DIR}/taglib/mpeg/id3v2/frames"
        "${XAMP_TAGLIB_DIR}/taglib/ogg"
        "${XAMP_TAGLIB_DIR}/taglib/ogg/opus"
        "${XAMP_TAGLIB_DIR}/taglib/riff"
        "${XAMP_TAGLIB_DIR}/taglib/riff/wav"
        "${XAMP_TAGLIB_DIR}/taglib/toolkit"
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
