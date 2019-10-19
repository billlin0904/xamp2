cmake_minimum_required(VERSION 3.15)

project(xamp LANGUAGES CXX)

set(QT_QMAKE_EXECUTABLE /User/bill/Qt5.12.5/5.12.5/clang_64/bin/qmake)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug)

include_directories(${CMAKE_CURRENT_LIST_DIR})
include_directories("../thirdparty/spdlog/include/")
include_directories("../xamp_base/")
include_directories("../xamp_base/include")
include_directories("../xamp_output_device/include")
include_directories("../xamp_stream/include")
include_directories("../xamp_player/include")
include_directories("../xamp_metadata/include")

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/*.cpp" "${CMAKE_CURRENT_LIST_DIR}/widget/*.cpp")

find_library (
    LIB_XAMP_BASE
    NAMES xamp_base libxamp_base
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/debug/"
    NO_DEFAULT_PATH
    )
message(STATUS "LIB_XAMP_BASE: [${LIB_XAMP_BASE}]")

find_library (
    LIB_XAMP_OUTPUT_DEVICE
    NAMES xamp_output_device libxamp_output_device
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_output_device/debug/"
    NO_DEFAULT_PATH
    )
message(STATUS "LIB_XAMP_OUTPUT_DEVICE: [${LIB_XAMP_OUTPUT_DEVICE}]")

find_library (
    LIB_XAMP_STREAM
    NAMES xamp_stream libxamp_stream
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_stream/debug/"
    NO_DEFAULT_PATH
    )
message(STATUS "LIB_XAMP_STREAM: [${LIB_XAMP_STREAM}]")

find_library (
    LIB_XAMP_PLAYER
    NAMES xamp_player libxamp_player
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_player/debug/"
    NO_DEFAULT_PATH
    )
message(STATUS "LIB_XAMP_PLAYER: [${LIB_XAMP_PLAYER}]")

find_library (
    LIB_XAMP_METADATA
    NAMES xamp_metadata libxamp_metadata
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_metadata/debug/"
    NO_DEFAULT_PATH
    )
message(STATUS "LIB_XAMP_METADATA: [${LIB_XAMP_METADATA}]")

find_package(Qt5 COMPONENTS Core Gui Widgets Sql REQUIRED)

add_executable(xamp xamp.ui main.cpp xamp.qrc ${SOURCES})

set(Qt5_LIBRARIES Qt5::Core Qt5::Gui Qt5::Widgets Qt5::Sql)

set_target_properties(${Qt5_LIBRARIES} PROPERTIES MAP_IMPORTED_CONFIG_COVERAGE "DEBUG")

target_link_libraries(xamp
    PUBLIC
    ${LIB_XAMP_BASE}
    ${LIB_XAMP_OUTPUT_DEVICE}
    ${LIB_XAMP_STREAM}
    ${LIB_XAMP_PLAYER}
    ${LIB_XAMP_METADATA}
    ${Qt5_LIBRARIES}
    ${Qt5_QSQLiteDriverPlugin}
    )
