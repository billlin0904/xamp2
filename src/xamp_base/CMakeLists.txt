project(xamp_base)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_OSX_DEPLOYMENT_TARGET "10.15")

if (${CMAKE_BUILD_TYPE} STREQUAL Debug)
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/debug)
    message(STATUS "Build debug mode")
    set(BUILD_PATH debug)
else()
    set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${PROJECT_SOURCE_DIR}/release)
    set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -DNDEBUG -O3 -fomit-frame-pointer -flto")
    message(STATUS "Build release mode")
    set(BUILD_PATH release)
endif(${CMAKE_BUILD_TYPE} STREQUAL Debug)


include_directories(${CMAKE_CURRENT_LIST_DIR}/include)
include_directories("../thirdparty/spdlog/include/")

file(GLOB_RECURSE SOURCES "${CMAKE_CURRENT_LIST_DIR}/src/*.cpp")

add_library(${PROJECT_NAME} SHARED ${SOURCES})
