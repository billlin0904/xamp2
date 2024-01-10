# Install script for directory: D:/Source/xamp2/src/thirdparty/qwindowkit/src/widgets

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
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/out-amd64-Debug/lib/QWKWidgetsd.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/out-amd64-Release/lib/QWKWidgets.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/out-amd64-MinSizeRel/lib/QWKWidgets.lib")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/lib" TYPE STATIC_LIBRARY OPTIONAL FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/out-amd64-RelWithDebInfo/lib/QWKWidgets.lib")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  if(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Dd][Ee][Bb][Uu][Gg])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY OPTIONAL FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/out-amd64-Debug/bin/QWKWidgetsd.dll")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ee][Aa][Ss][Ee])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY OPTIONAL FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/out-amd64-Release/bin/QWKWidgets.dll")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Mm][Ii][Nn][Ss][Ii][Zz][Ee][Rr][Ee][Ll])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY OPTIONAL FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/out-amd64-MinSizeRel/bin/QWKWidgets.dll")
  elseif(CMAKE_INSTALL_CONFIG_NAME MATCHES "^([Rr][Ee][Ll][Ww][Ii][Tt][Hh][Dd][Ee][Bb][Ii][Nn][Ff][Oo])$")
    file(INSTALL DESTINATION "${CMAKE_INSTALL_PREFIX}/bin" TYPE SHARED_LIBRARY OPTIONAL FILES "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/out-amd64-RelWithDebInfo/bin/QWKWidgets.dll")
  endif()
endif()

if(CMAKE_INSTALL_COMPONENT STREQUAL "Unspecified" OR NOT CMAKE_INSTALL_COMPONENT)
  
                get_filename_component(_install_dir "include/QWindowKit/QWKWidgets" ABSOLUTE BASE_DIR ${CMAKE_INSTALL_PREFIX})
        
                execute_process(
                    COMMAND "D:/Source/xamp2/src/thirdparty/qwindowkit/msvc/_install/bin/qmcorecmd.exe" incsync -d 
                        "-s"  "D:/Source/xamp2/src/thirdparty/qwindowkit/src/widgets" ${_install_dir}
                    WORKING_DIRECTORY "D:/Source/xamp2/src/thirdparty/qwindowkit/src/widgets"
                    OUTPUT_VARIABLE _output_contents
                    OUTPUT_STRIP_TRAILING_WHITESPACE
                    COMMAND_ERROR_IS_FATAL ANY
                )
                string(REPLACE "\n" ";" _lines "${_output_contents}")

                foreach(_line ${_lines})
                    string(REGEX MATCH "from \"([^\"]*)\" to \"([^\"]*)\"" _ ${_line})
                    get_filename_component(_target_path ${CMAKE_MATCH_2} DIRECTORY)
                    file(INSTALL ${CMAKE_MATCH_1} DESTINATION ${_target_path})
                endforeach()
            
endif()

