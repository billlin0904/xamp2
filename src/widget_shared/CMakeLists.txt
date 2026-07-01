cmake_minimum_required(VERSION 3.24)

project(widget_shared LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    Concurrent
    Multimedia
    Core5Compat
)

include("${CMAKE_SOURCE_DIR}/cmake/XampDependencyHelpers.cmake")

find_package(libdeflate CONFIG QUIET)
find_package(simdjson CONFIG REQUIRED)
if(TARGET libdeflate::libdeflate)
    set(XAMP_LIBDEFLATE_TARGET libdeflate::libdeflate)
elseif(TARGET libdeflate::libdeflate_shared)
    set(XAMP_LIBDEFLATE_TARGET libdeflate::libdeflate_shared)
elseif(TARGET unofficial::libdeflate::libdeflate)
    set(XAMP_LIBDEFLATE_TARGET unofficial::libdeflate::libdeflate)
else()
    xamp_find_imported_library(XAMP_LIBDEFLATE_TARGET XampLibdeflate
        REQUIRED
        HEADER libdeflate.h
        LIB_NAMES deflate libdeflate
        PKG libdeflate)
endif()

if(NOT TARGET xamp_base)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../xamp_base" "${CMAKE_BINARY_DIR}/xamp_base")
endif()

if(NOT TARGET QCoro6::Core OR NOT TARGET QCoro6::Network)
    set(QCORO_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(QCORO_BUILD_TESTING OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QTDBUS OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QTWEBSOCKETS OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QTQUICK OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QML OFF CACHE BOOL "" FORCE)
    set(QCORO_WITH_QTTEST OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../thirdparty/qcoro" "${CMAKE_BINARY_DIR}/qcoro")
endif()

if(NOT TARGET QWKWidgets)
    set(QWINDOWKIT_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
    set(QWINDOWKIT_BUILD_DOCUMENTATIONS OFF CACHE BOOL "" FORCE)
    set(QWINDOWKIT_BUILD_QUICK OFF CACHE BOOL "" FORCE)
    set(QWINDOWKIT_INSTALL OFF CACHE BOOL "" FORCE)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../thirdparty/qwindowkit" "${CMAKE_BINARY_DIR}/qwindowkit")
endif()

file(GLOB WIDGET_SHARED_ROOT_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.cpp"
)

file(GLOB_RECURSE WIDGET_SHARED_WIDGET_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/widget/*.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/widgetframe/*.cpp"
)

file(GLOB_RECURSE WIDGET_SHARED_HEADERS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.h"
)

file(GLOB WIDGET_SHARED_FORMS CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/*.ui"
)

qt6_wrap_ui(WIDGET_SHARED_UI_HEADERS ${WIDGET_SHARED_FORMS})

list(FILTER WIDGET_SHARED_WIDGET_SOURCES EXCLUDE REGEX "/widget/win32/")
list(FILTER WIDGET_SHARED_WIDGET_SOURCES EXCLUDE REGEX "/widget/playliststyleditem_delegate\\.cpp$")
list(FILTER WIDGET_SHARED_HEADERS EXCLUDE REGEX "/widget/win32/")
list(FILTER WIDGET_SHARED_HEADERS EXCLUDE REGEX "/widget/playliststyleditem_delegate\\.h$")

add_library(widget_shared SHARED
    ${WIDGET_SHARED_ROOT_SOURCES}
    ${WIDGET_SHARED_WIDGET_SOURCES}
    ${WIDGET_SHARED_HEADERS}
    ${WIDGET_SHARED_UI_HEADERS}
)

target_compile_features(widget_shared PUBLIC cxx_std_23)

target_compile_definitions(widget_shared
    PRIVATE
        WIDGET_SHARED_LIB
        QT_NO_CAST_FROM_ASCII
        QT_DEPRECATED_WARNINGS
)

target_include_directories(widget_shared
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/widget"
        "${CMAKE_CURRENT_LIST_DIR}/widgetframe"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_metadata/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_output_device/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_stream/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_player/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/rapidjson/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/rapidxml"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/zlib"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/robin_hood"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2/taglib"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/taglib2"
)

set_target_properties(widget_shared PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_LIST_DIR}"
)

target_link_libraries(widget_shared
    PUBLIC
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
        Qt6::Concurrent
        Qt6::Multimedia
        Qt6::Core5Compat
        xamp_base
        ${XAMP_LIBDEFLATE_TARGET}
        simdjson::simdjson
        QCoro6::Core
        QCoro6::Network
        QWKWidgets
)

if(UNIX AND NOT APPLE)
    target_link_libraries(widget_shared PRIVATE dl pthread)
endif()
