cmake_minimum_required(VERSION 3.24)

project(xamp_base
    LANGUAGES CXX)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15" CACHE STRING "" FORCE)

include("${CMAKE_SOURCE_DIR}/cmake/XampDependencyHelpers.cmake")

file(GLOB_RECURSE XAMP_BASE_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

if(NOT WIN32 AND NOT APPLE AND NOT (CMAKE_SYSTEM_NAME STREQUAL "Linux"))
    list(FILTER XAMP_BASE_SOURCES EXCLUDE REGEX ".*/src/fastconditionvariable\\.cpp$")
endif()

add_library(xamp_base SHARED
    ${XAMP_BASE_SOURCES})

target_compile_features(xamp_base
    PUBLIC
        cxx_std_23)

target_compile_definitions(xamp_base
    PRIVATE
        BASE_API_EXPORTS)

target_include_directories(xamp_base
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/utfcpp/source"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/robin_hood"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/uchardet/src"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/llfio/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/llfio/include/quickcpplib/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/llfio/include/outcome/include")

find_package(Threads REQUIRED)
find_package(LibArchive REQUIRED)
find_package(simdutf CONFIG REQUIRED)

target_link_libraries(xamp_base
    PRIVATE
        Threads::Threads
        LibArchive::LibArchive
        simdutf::simdutf)

if(UNIX AND NOT APPLE)
    if(DEFINED VCPKG_TARGET_TRIPLET)
        set(XAMP_MKL_VCPKG_ROOT "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}")
    else()
        set(XAMP_MKL_VCPKG_ROOT "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux")
    endif()
    set(MKL_LINK static CACHE STRING "" FORCE)
    set(MKL_THREADING sequential CACHE STRING "" FORCE)
    set(MKL_INTERFACE lp64 CACHE STRING "" FORCE)
    find_package(MKL CONFIG REQUIRED
        PATHS "${XAMP_MKL_VCPKG_ROOT}"
        NO_DEFAULT_PATH)

    xamp_find_imported_library(XAMP_UCHARDET_TARGET XampUchardet
        REQUIRED
        HEADER uchardet.h
        LIB_NAMES uchardet
        PKG uchardet)
    xamp_find_imported_library(XAMP_OPENCC_TARGET XampOpenCC
        REQUIRED
        HEADER opencc.h
        LIB_NAMES opencc
        INCLUDE_SUFFIXES opencc
        PKG opencc)
    xamp_find_imported_library(XAMP_MECAB_TARGET XampMeCab
        REQUIRED
        HEADER mecab.h
        LIB_NAMES mecab
        INCLUDE_SUFFIXES mecab
        PKG mecab)

    target_link_libraries(xamp_base
        PRIVATE
            MKL::MKL
            ${XAMP_UCHARDET_TARGET}
            ${XAMP_OPENCC_TARGET}
            ${XAMP_MECAB_TARGET}
            dl
            uuid)
endif()

if(MSVC)
    target_compile_options(xamp_base
        PRIVATE
            /utf-8)
else()
    target_compile_options(xamp_base
        PRIVATE
            -Wall
            -Wextra
            -Wpedantic
            -fpermissive)
endif()
