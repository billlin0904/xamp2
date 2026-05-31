cmake_minimum_required(VERSION 3.24)

project(xamp_player LANGUAGES CXX)

if(NOT TARGET xamp_base)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../xamp_base" "${CMAKE_BINARY_DIR}/xamp_base")
endif()

if(NOT TARGET xamp_output_device)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../xamp_output_device" "${CMAKE_BINARY_DIR}/xamp_output_device")
endif()

if(NOT TARGET xamp_stream)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../xamp_stream" "${CMAKE_BINARY_DIR}/xamp_stream")
endif()

file(GLOB_RECURSE XAMP_PLAYER_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp"
)

if(NOT WIN32)
    list(FILTER XAMP_PLAYER_SOURCES EXCLUDE REGEX "/src/mbdiscid\\.cpp$")
endif()

add_library(xamp_player SHARED
    ${XAMP_PLAYER_SOURCES}
)

target_compile_features(xamp_player PUBLIC cxx_std_23)

target_compile_definitions(xamp_player
    PRIVATE
        XAMP_PALYER_API_EXPORTS
)

if(UNIX AND NOT APPLE)
    target_compile_options(xamp_player PRIVATE -fpermissive)
endif()

target_include_directories(xamp_player
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_output_device/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_stream/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_metadata/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/bass/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/robin_hood"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/chromaprint/src"
)

target_link_libraries(xamp_player
    PUBLIC
        xamp_base
        xamp_output_device
        xamp_stream
)

if(UNIX AND NOT APPLE)
    target_link_libraries(xamp_player PRIVATE dl pthread)
endif()
