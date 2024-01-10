# Install script for directory: D:/Source/xamp2/src/thirdparty/qwindowkit/src

# Set the install prefix
if(NOT DEFINED CMAKE_INSTALL_PREFIX)
  set(CMAKE_INSTALL_PREFIX "C:/Program Files (x86)/QWindowKit")
endif()
string(REGEX REPLACE "/$" "" CMAKE_INSTALL_PREFIX "${CMAKE_INSTALL_PREFIX}")

# Set the install configuration name.
if(NOT DEFINED CMAKE_INSTALL_CONFIG_NAME)
  if(BUILD_TYPE)
    string(REGEX REPLACE "^[^A-Za-z0-9_]+" ""
           CMAKE_INSTALL_CONFIG_NAME "${BUILD_TYPE}")
  else()
    set(CMAKE_INSTALL_CONFIG_NAME "Release")
  endif()
  message(STATUS "Install configuration: \"${CMAKE_INSTALL_CONFIG_NAME}\"")
endif()

# Set the component getting installed.
if(NOT CMAKE_INSTALL_COMPONENT)
  if(COMPONENT)
    message(STATUS "Install component: \"${COMPONENT}\"")
    set(CMAKE_INSTALL_COMPONENT "${COMPONENT}")
  else()
    set(CMAKE_INSTALL_COMPONENT)
  endif()
endif()

# Is this installation the result of a crosscompile?
if(NOT DEFINED CMAKE_CROSSCOMPILING)
  set(CMAKE_CROSSCOMPILING "FALSE")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/include/QWindowKit/QWKCore" TYPE FILE FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/../etc/include/QWKCore/qwkconfig.h")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/core/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/widgets/cmake_install.cmake")
endif()

if(NOT CMAKE_INSTALL_LOCAL_ONLY)
  # Include the install script for the subdirectory.
  include("D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/quick/cmake_install.cmake")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit" TYPE FILE FILES
    "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/QWindowKitConfig.cmake"
    "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/QWindowKitConfigVersion.cmake"
    )
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(EXISTS "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit/QWindowKitTargets.cmake")
    file(DIFFERENT _cmake_export_file_changed FILES
         "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit/QWindowKitTargets.cmake"
         "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/CMakeFiles/Export/fedb68827e47645b781ae9e32c483318/QWindowKitTargets.cmake")
    if(_cmake_export_file_changed)
      file(GLOB _cmake_old_config_files "$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit/QWindowKitTargets-*.cmake")
      if(_cmake_old_config_files)
        string(REPLACE ";" ", " _cmake_old_config_files_text "${_cmake_old_config_files}")
        message(STATUS "Old export file \"$ENV{DESTDIR}${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit/QWindowKitTargets.cmake\" will be replaced.  Removing files [${_cmake_old_config_files_text}].")
        unset(_cmake_old_config_files_text)
        file(REMOVE ${_cmake_old_config_files})
      endif()
      unset(_cmake_old_config_files)
    endif()
    unset(_cmake_export_file_changed)
  endif()
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit" TYPE FILE FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/CMakeFiles/Export/fedb68827e47645b781ae9e32c483318/QWindowKitTargets.cmake")
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit" TYPE FILE FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/CMakeFiles/Export/fedb68827e47645b781ae9e32c483318/QWindowKitTargets-debug.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit" TYPE FILE FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/CMakeFiles/Export/fedb68827e47645b781ae9e32c483318/QWindowKitTargets-minsizerel.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit" TYPE FILE FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/CMakeFiles/Export/fedb68827e47645b781ae9e32c483318/QWindowKitTargets-relwithdebinfo.cmake")
  endif()
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib/cmake/QWindowKit" TYPE FILE FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/CMakeFiles/Export/fedb68827e47645b781ae9e32c483318/QWindowKitTargets-release.cmake")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/QWindowKit/qmake" TYPE DIRECTORY FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/../etc/share/qmake/")
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/share/QWindowKit/msbuild" TYPE DIRECTORY FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/src/../etc/share/msbuild/")
endif()

