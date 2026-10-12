cmake_minimum_required(VERSION 3.24)
project(coreaudio_integer_tests LANGUAGES CXX)
if(NOT APPLE)
    message(FATAL_ERROR "CoreAudio format tests require the macOS SDK")
endif()
enable_testing()
add_executable(coreaudio_integer_test coreaudio_integer_test.cpp)
target_compile_features(coreaudio_integer_test PRIVATE cxx_std_20)
target_include_directories(coreaudio_integer_test PRIVATE
    ../../include ../../../xamp_base/include)
add_test(NAME coreaudio_integer_test COMMAND coreaudio_integer_test)

# Compile the production backend against a fake HAL; no physical device is opened.
get_filename_component(XAMP_ROOT "../../../.." ABSOLUTE)
set(XAMP_MACOS_BUILD "${XAMP_ROOT}/out/build/macos-xcode" CACHE PATH "Existing Debug application build")
add_executable(coreaudio_backend_test coreaudio_backend_test.cpp
    ../../src/osx/coreaudiodevice.cpp ../../src/osx/coreaudiointeger.cpp)
target_compile_features(coreaudio_backend_test PRIVATE cxx_std_23)
target_include_directories(coreaudio_backend_test PRIVATE
    ../../include ../../../xamp_base/include "${XAMP_ROOT}/out/deps/spdlog/include")
foreach(api AudioObjectGetPropertyData AudioObjectGetPropertyDataSize AudioObjectSetPropertyData
    AudioObjectHasProperty AudioObjectAddPropertyListener AudioObjectRemovePropertyListener
    AudioDeviceCreateIOProcID AudioDeviceDestroyIOProcID AudioDeviceStart AudioDeviceStop)
    target_compile_definitions(coreaudio_backend_test PRIVATE "${api}=Mock${api}")
endforeach()
target_link_libraries(coreaudio_backend_test PRIVATE
    "${XAMP_MACOS_BUILD}/src/xamp_output_device/Debug/libxamp_output_device.dylib"
    "${XAMP_MACOS_BUILD}/src/xamp_base/Debug/libxamp_base.dylib"
    "-framework CoreAudio" "-framework AudioToolbox")
add_test(NAME coreaudio_backend_test COMMAND coreaudio_backend_test)

# Read-only system enumeration regression: Hog Mode must not inherit the shared ID.
add_executable(coreaudio_device_type_test coreaudio_device_type_test.cpp)
target_compile_features(coreaudio_device_type_test PRIVATE cxx_std_23)
target_include_directories(coreaudio_device_type_test PRIVATE
    ../../include ../../../xamp_base/include "${XAMP_ROOT}/out/deps/spdlog/include")
target_link_libraries(coreaudio_device_type_test PRIVATE
    "${XAMP_MACOS_BUILD}/src/xamp_output_device/Debug/libxamp_output_device.dylib"
    "${XAMP_MACOS_BUILD}/src/xamp_base/Debug/libxamp_base.dylib")
add_test(NAME coreaudio_device_type_test COMMAND coreaudio_device_type_test)
