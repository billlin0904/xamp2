cmake_minimum_required(VERSION 3.24)

project(xamp_output_device LANGUAGES CXX)

if(NOT TARGET xamp_base)
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/../xamp_base" "${CMAKE_BINARY_DIR}/xamp_base")
endif()

set(XAMP_OUTPUT_DEVICE_SOURCES
    "${CMAKE_CURRENT_LIST_DIR}/src/api.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/audiodevicemanager.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/nulloutputdevice.cpp"
    "${CMAKE_CURRENT_LIST_DIR}/src/nulloutputdevicetype.cpp"
)

if(WIN32)
    file(GLOB_RECURSE XAMP_OUTPUT_DEVICE_PLATFORM_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/src/win32/*.cpp"
    )
elseif(APPLE)
    file(GLOB_RECURSE XAMP_OUTPUT_DEVICE_PLATFORM_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/src/osx/*.cpp"
    )
elseif(UNIX)
    file(GLOB_RECURSE XAMP_OUTPUT_DEVICE_PLATFORM_SOURCES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/src/posix/*.cpp"
    )
else()
    set(XAMP_OUTPUT_DEVICE_PLATFORM_SOURCES)
endif()

add_library(xamp_output_device SHARED
    ${XAMP_OUTPUT_DEVICE_SOURCES}
    ${XAMP_OUTPUT_DEVICE_PLATFORM_SOURCES}
)

target_compile_features(xamp_output_device PUBLIC cxx_std_23)

target_compile_definitions(xamp_output_device
    PRIVATE
        XAMP_OUTPUT_DEVICE_API_EXPORTS
)

target_include_directories(xamp_output_device
    PUBLIC
        "${CMAKE_CURRENT_LIST_DIR}/include"
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/robin_hood"
)

target_link_libraries(xamp_output_device
    PUBLIC
        xamp_base
)

if(APPLE)
    find_library(LIB_COREAUDIO CoreAudio REQUIRED)
    find_library(LIB_COREFOUNDATION CoreFoundation REQUIRED)
    find_library(IOKIT_LIB IOKit REQUIRED)
    find_library(SECURITY_LIB Security REQUIRED)
    target_link_libraries(xamp_output_device
        PRIVATE
            ${LIB_COREAUDIO}
            ${LIB_COREFOUNDATION}
            ${IOKIT_LIB}
            ${SECURITY_LIB})
elseif(UNIX)
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(PULSE REQUIRED IMPORTED_TARGET libpulse)
    pkg_check_modules(ALSA REQUIRED IMPORTED_TARGET alsa)
    pkg_check_modules(PIPEWIRE REQUIRED IMPORTED_TARGET libpipewire-0.3)
    target_link_libraries(xamp_output_device
        PRIVATE
            pthread
            PkgConfig::PULSE
            PkgConfig::ALSA
            PkgConfig::PIPEWIRE)
endif()
