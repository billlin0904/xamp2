cmake_minimum_required(VERSION 3.24)

project(xamp_stream LANGUAGES CXX)

if(NOT TARGET xamp_base)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../xamp_base" "${CMAKE_BINARY_DIR}/xamp_base")
endif()

include("${CMAKE_SOURCE_DIR}/cmake/XampDependencyHelpers.cmake")

find_package(FFMPEG QUIET)
if(FFMPEG_FOUND)
    add_library(XampFFmpeg INTERFACE)
    target_include_directories(XampFFmpeg INTERFACE ${FFMPEG_INCLUDE_DIRS})
    target_link_directories(XampFFmpeg INTERFACE ${FFMPEG_LIBRARY_DIRS})
    target_link_libraries(XampFFmpeg INTERFACE ${FFMPEG_LIBRARIES})
    set(XAMP_FFMPEG_TARGET XampFFmpeg)
else()
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(FFMPEG REQUIRED IMPORTED_TARGET
        libavcodec
        libavformat
        libavutil
        libswresample
        libswscale
    )
    set(XAMP_FFMPEG_TARGET PkgConfig::FFMPEG)
endif()

xamp_find_imported_library(XAMP_SOXR_TARGET XampSoxr
    REQUIRED
    HEADER soxr.h
    LIB_NAMES soxr
    PKG soxr)
xamp_find_imported_library(XAMP_SAMPLERATE_TARGET XampSampleRate
    REQUIRED
    HEADER samplerate.h
    LIB_NAMES samplerate
    PKG samplerate)

find_package(FLAC CONFIG QUIET)
if(TARGET FLAC::FLAC)
    set(XAMP_FLAC_TARGET FLAC::FLAC)
else()
    xamp_find_imported_library(XAMP_FLAC_TARGET XampFlac
        REQUIRED
        HEADER FLAC/stream_decoder.h
        LIB_NAMES FLAC flac
        PKG flac)
endif()

file(GLOB_RECURSE XAMP_STREAM_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
)

if(NOT WIN32)
    set(XAMP_R8BRAIN_SOURCES
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/r8brain-free-src/DLL/r8bsrc.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/r8brain-free-src/r8bbase.cpp"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/r8brain-free-src/pffft.cpp"
    )
    set_source_files_properties(
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/r8brain-free-src/DLL/r8bsrc.cpp"
        PROPERTIES COMPILE_DEFINITIONS "R8BSRC_DECL="
    )
    list(APPEND XAMP_STREAM_SOURCES
        ${XAMP_R8BRAIN_SOURCES}
    )
endif()

if(NOT WIN32)
    list(FILTER XAMP_STREAM_SOURCES EXCLUDE REGEX "/src/basscddevice\\.cpp$")
endif()

add_library(xamp_stream SHARED
    ${XAMP_STREAM_SOURCES}
)

target_compile_features(xamp_stream PUBLIC cxx_std_23)

target_compile_definitions(xamp_stream
    PRIVATE
        XAMP_STREAM_API_EXPORTS
)

if(UNIX AND NOT APPLE)
    target_compile_options(xamp_stream PRIVATE -fpermissive)
endif()

target_include_directories(xamp_stream
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/bass/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/robin_hood"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/r8brain-free-src"
)

target_link_libraries(xamp_stream
    PUBLIC
        xamp_base
    PRIVATE
        ${XAMP_FFMPEG_TARGET}
        ${XAMP_SOXR_TARGET}
        ${XAMP_SAMPLERATE_TARGET}
        ${XAMP_FLAC_TARGET}
)

if(UNIX AND NOT APPLE)
    target_link_libraries(xamp_stream PRIVATE dl pthread)
endif()
