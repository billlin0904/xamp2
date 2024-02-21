#[[
  MIT License

  Copyright (C) 2021-2023 by wangwenx190 (Yuhang Zhao)

  Permission is hereby granted, free of charge, to any person obtaining a copy
  of this software and associated documentation files (the "Software"), to deal
  in the Software without restriction, including without limitation the rights
  to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
  copies of the Software, and to permit persons to whom the Software is
  furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included in
  all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
  AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
  LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
  OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
  SOFTWARE.
]]

function(setup_project)
    cmake_parse_arguments(PROJ_ARGS "QT_PROJECT;ENABLE_LTO;WARNINGS_ARE_ERRORS;MAX_WARNING;NO_WARNING;RTTI;EXCEPTIONS" "QML_IMPORT_DIR" "LANGUAGES" ${ARGN})
    if(NOT PROJ_ARGS_LANGUAGES)
        message(AUTHOR_WARNING "setup_project: You need to specify at least one language for this function!")
        return()
    endif()
    if(PROJ_ARGS_MAX_WARNING AND PROJ_ARGS_NO_WARNING)
        message(AUTHOR_WARNING "setup_project: MAX_WARNING and NO_WARNING can't be enabled at the same time!")
        return()
    endif()
    if(PROJ_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "setup_project: Unrecognized arguments: ${PROJ_ARGS_UNPARSED_ARGUMENTS}")
    endif()
    # MSVC: Do not add "-Z7", "-Zi" or "-ZI" to CMAKE_<LANG>_FLAGS by default. Let developers decide.
    if(POLICY CMP0141)
        cmake_policy(SET CMP0141 NEW)
    endif()
    # Introduced by CMP0141.
    if(NOT DEFINED CMAKE_MSVC_DEBUG_INFORMATION_FORMAT)
        set(CMAKE_MSVC_DEBUG_INFORMATION_FORMAT "$<$<CONFIG:Debug,RelWithDebInfo>:ProgramDatabase>" PARENT_SCOPE)
    endif()
    # Improve language standard and extension selection.
    if(POLICY CMP0128)
        cmake_policy(SET CMP0128 NEW)
    endif()
    # LANGUAGE source file property explicitly compiles as language.
    if(POLICY CMP0119)
        cmake_policy(SET CMP0119 NEW)
    endif()
    # Do not add "-GR" to CMAKE_CXX_FLAGS by default.
    if(POLICY CMP0117)
        cmake_policy(SET CMP0117 NEW)
    endif()
    # Source file extensions must be explicit.
    if(POLICY CMP0115)
        cmake_policy(SET CMP0115 NEW)
    endif()
    # Let AUTOMOC and AUTOUIC process header files that end with a .hh extension.
    if(POLICY CMP0100)
        cmake_policy(SET CMP0100 NEW)
    endif()
    # The project() command preserves leading zeros in version components.
    if(POLICY CMP0096)
        cmake_policy(SET CMP0096 NEW)
    endif()
    # MSVC warning flags are not in CMAKE_<LANG>_FLAGS by default.
    if(POLICY CMP0092)
        cmake_policy(SET CMP0092 NEW)
    endif()
    # MSVC: Do not add "-MT(d)" or "-MD(d)" to CMAKE_<LANG>_FLAGS by default. Let developers decide.
    if(POLICY CMP0091)
        cmake_policy(SET CMP0091 NEW)
    endif()
    # Introduced by CMP0091.
    if(NOT DEFINED CMAKE_MSVC_RUNTIME_LIBRARY)
        set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>DLL" PARENT_SCOPE)
    endif()
    # Add correct link flags for PIE (Position Independent Executable).
    if(POLICY CMP0083)
        cmake_policy(SET CMP0083 NEW)
    endif()
    if(NOT DEFINED CMAKE_BUILD_TYPE AND NOT DEFINED CMAKE_CONFIGURATION_TYPES)
        set(CMAKE_BUILD_TYPE "Release" PARENT_SCOPE)
    endif()
    if(PROJ_ARGS_ENABLE_LTO)
        # MinGW has many bugs when LTO is enabled, and they are all very
        # hard to workaround, so just don't enable LTO at all for MinGW.
        if(NOT DEFINED CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE AND NOT MINGW)
            set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON PARENT_SCOPE)
        endif()
    endif()
    if(NOT DEFINED CMAKE_DEBUG_POSTFIX)
        if(WIN32)
            if(NOT MINGW) # MinGW libraries usually don't have debug postfix.
                set(CMAKE_DEBUG_POSTFIX "d" PARENT_SCOPE)
            endif()
        else()
            set(CMAKE_DEBUG_POSTFIX "_debug" PARENT_SCOPE)
            if(APPLE)
                set(CMAKE_FRAMEWORK_MULTI_CONFIG_POSTFIX_DEBUG "_debug" PARENT_SCOPE)
            endif()
        endif()
    endif()
    include(GNUInstallDirs)
    if(NOT DEFINED CMAKE_RUNTIME_OUTPUT_DIRECTORY)
        set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}" PARENT_SCOPE)
    endif()
    if(NOT DEFINED CMAKE_LIBRARY_OUTPUT_DIRECTORY)
        set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}" PARENT_SCOPE)
    endif()
    if(NOT DEFINED CMAKE_ARCHIVE_OUTPUT_DIRECTORY)
        set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/${CMAKE_INSTALL_LIBDIR}" PARENT_SCOPE)
    endif()
    set(CMAKE_INCLUDE_CURRENT_DIR ON PARENT_SCOPE)
    set(CMAKE_LINK_DEPENDS_NO_SHARED ON PARENT_SCOPE)
    set(CMAKE_POSITION_INDEPENDENT_CODE ON PARENT_SCOPE)
    include(CheckPIESupported)
    check_pie_supported() # This function must be called to ensure CMake adds -fPIE to the linker flags.
    set(CMAKE_VISIBILITY_INLINES_HIDDEN ON PARENT_SCOPE)
    set(CMAKE_INSTALL_RPATH_USE_LINK_PATH ON PARENT_SCOPE)
    if(APPLE)
        set(CMAKE_MACOSX_RPATH ON PARENT_SCOPE)
    else()
        set(CMAKE_INSTALL_RPATH "$ORIGIN/../${CMAKE_INSTALL_LIBDIR}" PARENT_SCOPE)
    endif()
    if(PROJ_ARGS_QT_PROJECT)
        set(CMAKE_AUTOUIC ON PARENT_SCOPE)
        set(CMAKE_AUTOMOC ON PARENT_SCOPE)
        set(CMAKE_AUTORCC ON PARENT_SCOPE)
    endif()
    if(PROJ_ARGS_QML_IMPORT_DIR)
        list(APPEND QML_IMPORT_PATH "${PROJ_ARGS_QML_IMPORT_DIR}")
        list(REMOVE_DUPLICATES QML_IMPORT_PATH)
        set(QML_IMPORT_PATH ${QML_IMPORT_PATH} CACHE STRING "Qt Creator extra QML import paths" FORCE)
    endif()
    foreach(__lang ${PROJ_ARGS_LANGUAGES})
        if(__lang STREQUAL "C")
            enable_language(C)
            if(NOT DEFINED CMAKE_C_STANDARD)
                set(CMAKE_C_STANDARD 11 PARENT_SCOPE)
            endif()
            set(CMAKE_C_STANDARD_REQUIRED ON PARENT_SCOPE)
            set(CMAKE_C_EXTENSIONS OFF PARENT_SCOPE)
            set(CMAKE_C_VISIBILITY_PRESET "hidden" PARENT_SCOPE)
            if(MSVC)
                if(NOT ("x${CMAKE_C_FLAGS}" STREQUAL "x"))
                    string(REGEX REPLACE "[-|/]w " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
                    string(REGEX REPLACE "[-|/]W[0|1|2|3|4|all|X] " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
                endif()
                if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
                    if(NOT ("x${CMAKE_C_FLAGS_RELEASE}" STREQUAL "x"))
                        string(REGEX REPLACE "-O[d|0|1|2|3|fast] " " " CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
                    endif()
                endif()
                if(PROJ_ARGS_NO_WARNING)
                    string(APPEND CMAKE_C_FLAGS " /w ")
                elseif(PROJ_ARGS_MAX_WARNING)
                    string(APPEND CMAKE_C_FLAGS " /W4 ") # /Wall gives me 10000+ warnings!
                else()
                    string(APPEND CMAKE_C_FLAGS " /W3 ")
                endif()
                if(PROJ_ARGS_WARNINGS_ARE_ERRORS)
                    string(APPEND CMAKE_C_FLAGS " /WX ")
                endif()
                set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} PARENT_SCOPE)
                if(MSVC_VERSION GREATER_EQUAL 1920) # Visual Studio 2019 version 16.0
                    if(NOT ("x${CMAKE_C_FLAGS_RELEASE}" STREQUAL "x"))
                        string(REGEX REPLACE "[-|/]Ob[0|1|2|3] " " " CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
                    endif()
                    if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
                        string(APPEND CMAKE_C_FLAGS_RELEASE " /Ob2 ")
                    else()
                        string(APPEND CMAKE_C_FLAGS_RELEASE " /Ob3 ")
                    endif()
                    set(CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE} PARENT_SCOPE)
                endif()
                if(CMAKE_C_COMPILER_ID STREQUAL "Clang")
                    string(APPEND CMAKE_C_FLAGS_RELEASE " /clang:-Ofast ")
                    set(CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE} PARENT_SCOPE)
                endif()
            else()
                if(NOT ("x${CMAKE_C_FLAGS}" STREQUAL "x"))
                    string(REPLACE "-w " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
                    string(REGEX REPLACE "-W[all|extra|error|pedantic] " " " CMAKE_C_FLAGS ${CMAKE_C_FLAGS})
                endif()
                if(NOT ("x${CMAKE_C_FLAGS_RELEASE}" STREQUAL "x"))
                    string(REGEX REPLACE "-O[d|0|1|2|3|fast] " " " CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE})
                endif()
                if(PROJ_ARGS_NO_WARNING)
                    string(APPEND CMAKE_C_FLAGS " -w ")
                elseif(PROJ_ARGS_MAX_WARNING)
                    string(APPEND CMAKE_C_FLAGS " -Wall -Wextra ") # -Wpedantic ?
                else()
                    string(APPEND CMAKE_C_FLAGS " -Wall ")
                endif()
                if(PROJ_ARGS_WARNINGS_ARE_ERRORS)
                    string(APPEND CMAKE_C_FLAGS " -Werror ")
                endif()
                string(APPEND CMAKE_C_FLAGS_RELEASE " -Ofast ")
                set(CMAKE_C_FLAGS ${CMAKE_C_FLAGS} PARENT_SCOPE)
                set(CMAKE_C_FLAGS_RELEASE ${CMAKE_C_FLAGS_RELEASE} PARENT_SCOPE)
            endif()
        elseif(__lang STREQUAL "CXX")
            enable_language(CXX)
            if(NOT DEFINED CMAKE_CXX_STANDARD)
                set(CMAKE_CXX_STANDARD 20 PARENT_SCOPE)
            endif()
            set(CMAKE_CXX_STANDARD_REQUIRED ON PARENT_SCOPE)
            set(CMAKE_CXX_EXTENSIONS OFF PARENT_SCOPE)
            set(CMAKE_CXX_VISIBILITY_PRESET "hidden" PARENT_SCOPE)
            if(MSVC)
                if(NOT ("x${CMAKE_CXX_FLAGS}" STREQUAL "x"))
                    string(REGEX REPLACE "[-|/]GR-? " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
                    string(REGEX REPLACE "[-|/]EH(a-?|r-?|s-?|c-?)+ " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
                    string(REGEX REPLACE "[-|/]w " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
                    string(REGEX REPLACE "[-|/]W[0|1|2|3|4|all|X] " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
                endif()
                if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                    if(NOT ("x${CMAKE_CXX_FLAGS_RELEASE}" STREQUAL "x"))
                        string(REGEX REPLACE "-O[d|0|1|2|3|fast] " " " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
                    endif()
                endif()
                if(PROJ_ARGS_NO_WARNING)
                    string(APPEND CMAKE_CXX_FLAGS " /w ")
                elseif(PROJ_ARGS_MAX_WARNING)
                    string(APPEND CMAKE_CXX_FLAGS " /W4 ") # /Wall gives me 10000+ warnings!
                else()
                    string(APPEND CMAKE_CXX_FLAGS " /W3 ")
                endif()
                if(PROJ_ARGS_WARNINGS_ARE_ERRORS)
                    string(APPEND CMAKE_CXX_FLAGS " /WX ")
                endif()
                if(PROJ_ARGS_RTTI)
                    string(APPEND CMAKE_CXX_FLAGS " /GR ")
                else()
                    string(APPEND CMAKE_CXX_FLAGS " /GR- ")
                endif()
                if(PROJ_ARGS_EXCEPTIONS)
                    string(APPEND CMAKE_CXX_FLAGS " /EHsc ")
                else()
                    string(APPEND CMAKE_CXX_FLAGS " /EHs-c- ")
                endif()
                set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
                if(MSVC_VERSION GREATER_EQUAL 1920) # Visual Studio 2019 version 16.0
                    if(NOT ("x${CMAKE_CXX_FLAGS_RELEASE}" STREQUAL "x"))
                        string(REGEX REPLACE "[-|/]Ob[0|1|2|3] " " " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
                    endif()
                    if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                        string(APPEND CMAKE_CXX_FLAGS_RELEASE " /Ob2 ")
                    else()
                        string(APPEND CMAKE_CXX_FLAGS_RELEASE " /Ob3 ")
                    endif()
                    set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} PARENT_SCOPE)
                endif()
                if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
                    string(APPEND CMAKE_CXX_FLAGS_RELEASE " /clang:-Ofast ")
                    set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} PARENT_SCOPE)
                endif()
            else()
                if(NOT ("x${CMAKE_CXX_FLAGS}" STREQUAL "x"))
                    string(REPLACE "-w " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
                    string(REGEX REPLACE "-W[all|extra|error|pedantic] " " " CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS})
                endif()
                if(NOT ("x${CMAKE_CXX_FLAGS_RELEASE}" STREQUAL "x"))
                    string(REGEX REPLACE "-O[d|0|1|2|3|fast] " " " CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE})
                endif()
                if(PROJ_ARGS_NO_WARNING)
                    string(APPEND CMAKE_CXX_FLAGS " -w ")
                elseif(PROJ_ARGS_MAX_WARNING)
                    string(APPEND CMAKE_CXX_FLAGS " -Wall -Wextra ") # -Wpedantic ?
                else()
                    string(APPEND CMAKE_CXX_FLAGS " -Wall ")
                endif()
                if(PROJ_ARGS_WARNINGS_ARE_ERRORS)
                    string(APPEND CMAKE_CXX_FLAGS " -Werror ")
                endif()
                if(PROJ_ARGS_RTTI)
                    string(APPEND CMAKE_CXX_FLAGS " -frtti ")
                else()
                    string(APPEND CMAKE_CXX_FLAGS " -fno-rtti ")
                endif()
                if(PROJ_ARGS_EXCEPTIONS)
                    string(APPEND CMAKE_CXX_FLAGS " -fexceptions ")
                else()
                    string(APPEND CMAKE_CXX_FLAGS " -fno-exceptions ")
                endif()
                string(APPEND CMAKE_CXX_FLAGS_RELEASE " -Ofast ")
                set(CMAKE_CXX_FLAGS ${CMAKE_CXX_FLAGS} PARENT_SCOPE)
                set(CMAKE_CXX_FLAGS_RELEASE ${CMAKE_CXX_FLAGS_RELEASE} PARENT_SCOPE)
            endif()
        elseif(__lang STREQUAL "RC")
            if(WIN32)
                enable_language(RC)
            endif()
            if(MSVC)
                # Clang-CL forces us use "-" instead of "/" because it always
                # regard everything begins with "/" as a file path instead of
                # a command line parameter.
                set(CMAKE_RC_FLAGS "-c65001 -DWIN32 -nologo" PARENT_SCOPE)
            endif()
        endif()
    endforeach()
endfunction()

function(get_commit_hash)
    cmake_parse_arguments(GIT_ARGS "" "RESULT" "" ${ARGN})
    if(NOT GIT_ARGS_RESULT)
        message(AUTHOR_WARNING "get_commit_hash: You need to specify a result variable for this function!")
        return()
    endif()
    if(GIT_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "get_commit_hash: Unrecognized arguments: ${GIT_ARGS_UNPARSED_ARGUMENTS}")
    endif()
    set(__hash)
    # We do not want to use git command here because we don't want to make git a build-time dependency.
    if(EXISTS "${PROJECT_SOURCE_DIR}/.git/HEAD")
        file(READ "${PROJECT_SOURCE_DIR}/.git/HEAD" __hash)
        string(STRIP "${__hash}" __hash)
        if(__hash MATCHES "^ref: (.*)")
            set(HEAD "${CMAKE_MATCH_1}")
            if(EXISTS "${PROJECT_SOURCE_DIR}/.git/${HEAD}")
                file(READ "${PROJECT_SOURCE_DIR}/.git/${HEAD}" __hash)
                string(STRIP "${__hash}" __hash)
            else()
                file(READ "${PROJECT_SOURCE_DIR}/.git/packed-refs" PACKED_REFS)
                string(REGEX REPLACE ".*\n([0-9a-f]+) ${HEAD}\n.*" "\\1" __hash "\n${PACKED_REFS}")
            endif()
        endif()
    endif()
    if(__hash)
        set(${GIT_ARGS_RESULT} "${__hash}" PARENT_SCOPE)
    endif()
endfunction()

function(setup_qt_stuff)
    cmake_parse_arguments(QT_ARGS "ALLOW_KEYWORD" "" "TARGETS" ${ARGN})
    if(NOT QT_ARGS_TARGETS)
        message(AUTHOR_WARNING "setup_qt_stuff: You need to specify at least one target for this function!")
        return()
    endif()
    if(QT_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "setup_qt_stuff: Unrecognized arguments: ${QT_ARGS_UNPARSED_ARGUMENTS}")
    endif()
    foreach(__target ${QT_ARGS_TARGETS})
        target_compile_definitions(${__target} PRIVATE
            QT_NO_CAST_TO_ASCII
            QT_NO_CAST_FROM_ASCII
            QT_NO_CAST_FROM_BYTEARRAY
            QT_NO_URL_CAST_FROM_STRING
            QT_NO_NARROWING_CONVERSIONS_IN_CONNECT
            QT_NO_FOREACH
            QT_NO_JAVA_STYLE_ITERATORS
            QT_NO_AS_CONST
            QT_NO_QEXCHANGE
            QT_EXPLICIT_QFILE_CONSTRUCTION_FROM_PATH
            #QT_TYPESAFE_FLAGS # QtQuick private headers prevent us from enabling this flag.
            QT_USE_QSTRINGBUILDER
            QT_USE_FAST_OPERATOR_PLUS
            QT_DEPRECATED_WARNINGS # Have no effect since 6.0
            QT_DEPRECATED_WARNINGS_SINCE=0x070000 # Deprecated since 6.5
            QT_WARN_DEPRECATED_UP_TO=0x070000 # Available since 6.5
            QT_DISABLE_DEPRECATED_BEFORE=0x070000 # Deprecated since 6.5
            QT_DISABLE_DEPRECATED_UP_TO=0x070000 # Available since 6.5
        )
        # On Windows enabling this flag requires us re-compile Qt with this flag enabled,
        # so only enable it on non-Windows platforms.
        if(NOT WIN32)
            target_compile_definitions(${__target} PRIVATE
                QT_STRICT_ITERATORS
            )
        endif()
        # We handle this flag specially because some Qt headers may still use the
        # traditional keywords (especially some private headers).
        if(NOT QT_ARGS_ALLOW_KEYWORD)
            target_compile_definitions(${__target} PRIVATE
                QT_NO_KEYWORDS
            )
        endif()
    endforeach()
endfunction()

function(setup_compile_params)
    cmake_parse_arguments(COM_ARGS "SPECTRE;EHCONTGUARD;PERMISSIVE;INTELCET;INTELJCC;CFGUARD" "" "TARGETS" ${ARGN})
    if(NOT COM_ARGS_TARGETS)
        message(AUTHOR_WARNING "setup_compile_params: You need to specify at least one target for this function!")
        return()
    endif()
    if(COM_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "setup_compile_params: Unrecognized arguments: ${COM_ARGS_UNPARSED_ARGUMENTS}")
    endif()
    foreach(__target ${COM_ARGS_TARGETS})
        set(__target_type "UNKNOWN")
        get_target_property(__target_type ${__target} TYPE)
        # Turn off LTCG/LTO for static libraries, because enabling it for the static libraries
        # will destroy the binary compatibility of them and some compilers will also produce
        # some way too large object files which we can't accept in most cases.
        if(__target_type STREQUAL "STATIC_LIBRARY")
            set_target_properties(${__target} PROPERTIES
                INTERPROCEDURAL_OPTIMIZATION OFF
                INTERPROCEDURAL_OPTIMIZATION_MINSIZEREL OFF
                INTERPROCEDURAL_OPTIMIZATION_RELEASE OFF
                INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO OFF
            )
        endif()
        # Needed by both MSVC and MinGW, otherwise some APIs we need will not be available.
        if(WIN32)
            set(_WIN32_WINNT_WIN10 0x0A00)
            set(NTDDI_WIN10_NI 0x0A00000C)
            # According to MS docs, both "WINVER" and "_WIN32_WINNT" should be defined
            # at the same time and they should use exactly the same value.
            target_compile_definitions(${__target} PRIVATE
                WINVER=${_WIN32_WINNT_WIN10} _WIN32_WINNT=${_WIN32_WINNT_WIN10}
                _WIN32_IE=${_WIN32_WINNT_WIN10} NTDDI_VERSION=${NTDDI_WIN10_NI}
            )
        endif()
        if(MSVC)
            target_compile_definitions(${__target} PRIVATE
                _CRT_NON_CONFORMING_SWPRINTFS _CRT_SECURE_NO_WARNINGS
                _CRT_SECURE_NO_DEPRECATE _CRT_NONSTDC_NO_WARNINGS
                _CRT_NONSTDC_NO_DEPRECATE
                _SCL_SECURE_NO_WARNINGS _SCL_SECURE_NO_DEPRECATE
                _ENABLE_EXTENDED_ALIGNED_STORAGE # STL fixed a bug which breaks binary compatibility, thus need to be enabled manually by defining this.
                _USE_MATH_DEFINES # Enable the PI constant define for the math headers.
                NOMINMAX # Avoid the Win32 macros conflict with std::min() and std::max().
                UNICODE _UNICODE # Use the -W APIs by default.
                STRICT # https://learn.microsoft.com/en-us/windows/win32/winprog/enabling-strict
                WIN32_LEAN_AND_MEAN WINRT_LEAN_AND_MEAN # Filter out some rarely used headers, to increase compilation speed.
            )
            if(NOT (CMAKE_CXX_COMPILER_ID STREQUAL "Clang"))
                target_compile_options(${__target} PRIVATE
                    /bigobj /utf-8 $<$<NOT:$<CONFIG:Debug>>:/fp:fast /GT /Gw /Gy /Zc:inline>
                )
                target_link_options(${__target} PRIVATE
                    $<$<NOT:$<CONFIG:Debug>>:/OPT:REF /OPT:ICF /OPT:LBR>
                    /DYNAMICBASE /FIXED:NO /NXCOMPAT /LARGEADDRESSAWARE /WX
                )
                if(__target_type STREQUAL "EXECUTABLE")
                    target_compile_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GA>)
                    target_link_options(${__target} PRIVATE /TSAWARE)
                endif()
                #if(CMAKE_SIZEOF_VOID_P EQUAL 4)
                #    target_link_options(${__target} PRIVATE /SAFESEH)
                #endif()
                if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                    target_link_options(${__target} PRIVATE /HIGHENTROPYVA)
                endif()
                #[[if(MSVC_VERSION GREATER_EQUAL 1910) # Visual Studio 2017 version 15.0
                    target_link_options(${__target} PRIVATE /DEPENDENTLOADFLAG:0x800)
                endif()]]
                if(MSVC_VERSION GREATER_EQUAL 1915) # Visual Studio 2017 version 15.8
                    target_compile_options(${__target} PRIVATE $<$<CONFIG:Debug,RelWithDebInfo>:/JMC>)
                endif()
                if(MSVC_VERSION GREATER_EQUAL 1920) # Visual Studio 2019 version 16.0
                    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                        target_compile_options(${__target} PRIVATE /d2FH4)
                    endif()
                endif()
                if(MSVC_VERSION GREATER_EQUAL 1925) # Visual Studio 2019 version 16.5
                    target_compile_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/QIntel-jcc-erratum>)
                endif()
                if(MSVC_VERSION GREATER_EQUAL 1929) # Visual Studio 2019 version 16.10
                    target_compile_options(${__target} PRIVATE /await:strict)
                elseif(MSVC_VERSION GREATER_EQUAL 1900) # Visual Studio 2015
                    target_compile_options(${__target} PRIVATE /await)
                endif()
                if(MSVC_VERSION GREATER_EQUAL 1930) # Visual Studio 2022 version 17.0
                    target_compile_options(${__target} PRIVATE /options:strict)
                endif()
                if(COM_ARGS_CFGUARD)
                    target_compile_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/guard:cf>)
                    target_link_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GUARD:CF>)
                endif()
                if(COM_ARGS_INTELCET)
                    if(MSVC_VERSION GREATER_EQUAL 1920) # Visual Studio 2019 version 16.0
                        target_link_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/CETCOMPAT>)
                    endif()
                endif()
                if(COM_ARGS_SPECTRE)
                    if(MSVC_VERSION GREATER_EQUAL 1925) # Visual Studio 2019 version 16.5
                        target_compile_options(${__target} PRIVATE /Qspectre-load)
                    elseif(MSVC_VERSION GREATER_EQUAL 1912) # Visual Studio 2017 version 15.5
                        target_compile_options(${__target} PRIVATE /Qspectre)
                    endif()
                endif()
                if(COM_ARGS_EHCONTGUARD)
                    if((MSVC_VERSION GREATER_EQUAL 1927) AND (CMAKE_SIZEOF_VOID_P EQUAL 8)) # Visual Studio 2019 version 16.7
                        target_compile_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/guard:ehcont>)
                        target_link_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/guard:ehcont>)
                    endif()
                endif()
                if(COM_ARGS_PERMISSIVE)
                    target_compile_options(${__target} PRIVATE
                        /Zc:auto /Zc:forScope /Zc:implicitNoexcept /Zc:noexceptTypes /Zc:referenceBinding
                        /Zc:rvalueCast /Zc:sizedDealloc /Zc:strictStrings /Zc:throwingNew /Zc:trigraphs
                        /Zc:wchar_t
                    )
                    if(MSVC_VERSION GREATER_EQUAL 1900) # Visual Studio 2015
                        target_compile_options(${__target} PRIVATE /Zc:threadSafeInit)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1910) # Visual Studio 2017 version 15.0
                        target_compile_options(${__target} PRIVATE /permissive- /Zc:ternary)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1912) # Visual Studio 2017 version 15.5
                        target_compile_options(${__target} PRIVATE /Zc:alignedNew)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1913) # Visual Studio 2017 version 15.6
                        target_compile_options(${__target} PRIVATE /Zc:externConstexpr)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1914) # Visual Studio 2017 version 15.7
                        target_compile_options(${__target} PRIVATE /Zc:__cplusplus)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1921) # Visual Studio 2019 version 16.1
                        target_compile_options(${__target} PRIVATE /Zc:char8_t)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1923) # Visual Studio 2019 version 16.3
                        target_compile_options(${__target} PRIVATE /Zc:externC)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1924) # Visual Studio 2019 version 16.4
                        target_compile_options(${__target} PRIVATE /Zc:hiddenFriend)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1925) # Visual Studio 2019 version 16.5
                        target_compile_options(${__target} PRIVATE /Zc:preprocessor /Zc:tlsGuards)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1928) # Visual Studio 2019 version 16.8 & 16.9
                        target_compile_options(${__target} PRIVATE /Zc:lambda /Zc:zeroSizeArrayNew)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1931) # Visual Studio 2022 version 17.1
                        target_compile_options(${__target} PRIVATE /Zc:static_assert)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1932) # Visual Studio 2022 version 17.2
                        target_compile_options(${__target} PRIVATE /Zc:__STDC__)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1934) # Visual Studio 2022 version 17.4
                        target_compile_options(${__target} PRIVATE /Zc:enumTypes /Zc:gotoScope /Zc:nrvo)
                    endif()
                    if(MSVC_VERSION GREATER_EQUAL 1935) # Visual Studio 2022 version 17.5
                        target_compile_options(${__target} PRIVATE /Zc:templateScope /Zc:checkGwOdr)
                    endif()
                endif()
            endif()
        else()
            if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                # Use pipes for communicating between sub-processes. Faster. Have no effect for Clang.
                target_compile_options(${__target} PRIVATE -pipe)
            endif()
            target_compile_options(${__target} PRIVATE
                $<$<NOT:$<CONFIG:Debug>>:-ffp-contract=fast -fomit-frame-pointer -ffunction-sections -fdata-sections>
            )
            if(APPLE)
                target_link_options(${__target} PRIVATE
                    -Wl,-fatal_warnings
                    $<$<NOT:$<CONFIG:Debug>>:-Wl,-dead_strip -Wl,-no_data_in_code_info -Wl,-no_function_starts>
                )
            else()
                target_link_options(${__target} PRIVATE
                    -Wl,--fatal-warnings -Wl,--build-id=sha1
                    $<$<NOT:$<CONFIG:Debug>>:-Wl,--gc-sections -Wl,-O3> # Specifically tell the linker to perform optimizations.
                )
            endif()
            if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                target_compile_options(${__target} PRIVATE
                    $<$<NOT:$<CONFIG:Debug>>:-Wa,-mbranches-within-32B-boundaries>
                )
            endif()
            if(COM_ARGS_INTELCET)
                target_compile_options(${__target} PRIVATE
                    $<$<NOT:$<CONFIG:Debug>>:-mshstk>
                )
            endif()
            if(COM_ARGS_CFGUARD)
                if(MINGW)
                    target_compile_options(${__target} PRIVATE
                        $<$<NOT:$<CONFIG:Debug>>:-mguard=cf>
                    )
                    target_link_options(${__target} PRIVATE
                        $<$<NOT:$<CONFIG:Debug>>:-Wl,-mguard=cf> # TODO: Do we need "-Wl," here?
                    )
                elseif(APPLE OR (CMAKE_CXX_COMPILER_ID STREQUAL "GNU"))
                    target_compile_options(${__target} PRIVATE
                        $<$<NOT:$<CONFIG:Debug>>:-fcf-protection=full>
                    )
                endif()
            endif()
            if(COM_ARGS_SPECTRE)
                if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
                    #[[target_compile_options(${__target} PRIVATE
                        $<$<NOT:$<CONFIG:Debug>>:
                            # These parameters are not compatible with -fcf-protection=full
                            -mindirect-branch=thunk
                            -mfunction-return=thunk
                            -mindirect-branch-register
                            -mindirect-branch-cs-prefix
                        >
                    )]]
                endif()
            endif()
        endif()
        if(CMAKE_CXX_COMPILER_ID STREQUAL "Clang")
            set(__lto_enabled)
            if(DEFINED CMAKE_BUILD_TYPE)
                set(__upper_type)
                string(TOUPPER ${CMAKE_BUILD_TYPE} __upper_type)
                get_target_property(__lto_enabled ${__target} INTERPROCEDURAL_OPTIMIZATION_${__upper_type})
            endif()
            if(NOT __lto_enabled)
                get_target_property(__lto_enabled ${__target} INTERPROCEDURAL_OPTIMIZATION)
            endif()
            if(__lto_enabled)
                target_compile_options(${__target} PRIVATE
                    $<$<NOT:$<CONFIG:Debug>>:-fsplit-lto-unit -fwhole-program-vtables>
                )
                if(MSVC)
                    target_link_options(${__target} PRIVATE
                        $<$<NOT:$<CONFIG:Debug>>:/OPT:lldltojobs=all /OPT:lldlto=3> # /lldltocachepolicy:cache_size=10%:cache_size_bytes=40g:cache_size_files=100000
                    )
                else()
                    target_link_options(${__target} PRIVATE
                        $<$<NOT:$<CONFIG:Debug>>:-fwhole-program-vtables -Wl,--thinlto-jobs=all -Wl,--lto-O3> # -Wl,--thinlto-cache-policy=cache_size=10%:cache_size_bytes=40g:cache_size_files=100000
                    )
                endif()
            endif()
            target_link_options(${__target} PRIVATE
                $<$<NOT:$<CONFIG:Debug>>:-Wl,--icf=all>
            )
            #[[target_compile_options(${__target} PRIVATE
                $<$<NOT:$<CONFIG:Debug>>:-fsanitize=shadow-call-stack -fno-stack-protector>
            )
            target_link_options(${__target} PRIVATE
                $<$<NOT:$<CONFIG:Debug>>:-fsanitize=shadow-call-stack -fno-stack-protector>
            )]]
            target_compile_options(${__target} PRIVATE
                -fcolor-diagnostics
                # Enable -fmerge-all-constants. This used to be the default in clang
                # for over a decade. It makes clang non-conforming, but is fairly safe
                # in practice and saves some binary size.
                -fmerge-all-constants
            )
            if(MSVC)
                # Required to make the 19041 SDK compatible with clang-cl.
                target_compile_definitions(${__target} PRIVATE __WRL_ENABLE_FUNCTION_STATICS__)
                target_compile_options(${__target} PRIVATE
                    /bigobj /utf-8 /FS
                    -fmsc-version=1935 # Tell clang-cl to emulate Visual Studio 2022 version 17.5
                    # This flag enforces that member pointer base types are complete.
                    # It helps prevent us from running into problems in the Microsoft C++ ABI.
                    -fcomplete-member-pointers
                    # Consistently use backslash as the path separator when expanding the
                    # __FILE__ macro when targeting Windows regardless of the build environment.
                    -ffile-reproducible
                    # Enable ANSI escape codes if something emulating them is around (cmd.exe
                    # doesn't understand ANSI escape codes by default).
                    -fansi-escape-codes
                    /Zc:dllexportInlines- # Do not export inline member functions. This is similar to "-fvisibility-inlines-hidden".
                    /Zc:char8_t /Zc:sizedDealloc /Zc:strictStrings /Zc:threadSafeInit /Zc:trigraphs /Zc:twoPhase
                    /clang:-mcx16 # Needed by _InterlockedCompareExchange128() from CPP/WinRT.
                    $<$<NOT:$<CONFIG:Debug>>:/clang:-mbranches-within-32B-boundaries /fp:fast /Gw /Gy /Zc:inline>
                )
                target_link_options(${__target} PRIVATE
                    --color-diagnostics
                    /DYNAMICBASE /FIXED:NO /NXCOMPAT /LARGEADDRESSAWARE
                    $<$<NOT:$<CONFIG:Debug>>:/OPT:REF /OPT:ICF /OPT:LBR /OPT:lldtailmerge>
                )
                if(__target_type STREQUAL "EXECUTABLE")
                    target_compile_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GA>)
                    target_link_options(${__target} PRIVATE /TSAWARE)
                endif()
                if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                    target_link_options(${__target} PRIVATE /HIGHENTROPYVA)
                endif()
                if(COM_ARGS_CFGUARD)
                    target_compile_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/guard:cf>)
                    target_link_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/GUARD:CF>)
                endif()
                if(COM_ARGS_INTELCET)
                    target_link_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/CETCOMPAT>)
                endif()
                if(COM_ARGS_EHCONTGUARD)
                    if(CMAKE_SIZEOF_VOID_P EQUAL 8)
                        target_compile_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/guard:ehcont>)
                        target_link_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:/guard:ehcont>)
                    endif()
                endif()
            else()
                target_link_options(${__target} PRIVATE -fuse-ld=lld -Wl,--color-diagnostics)
                if(APPLE)
                    # TODO: -fobjc-arc (http://clang.llvm.org/docs/AutomaticReferenceCounting.html)
                    target_compile_options(${__target} PRIVATE -fobjc-call-cxx-cdtors)
                    target_link_options(${__target} PRIVATE $<$<NOT:$<CONFIG:Debug>>:-Wl,--strict-auto-link>)
                else()
                    target_link_options(${__target} PRIVATE -Wl,-z,keep-text-section-prefix)
                endif()
                if(COM_ARGS_SPECTRE)
                    target_compile_options(${__target} PRIVATE
                        $<$<NOT:$<CONFIG:Debug>>:-mretpoline -mspeculative-load-hardening>
                    )
                    # AppleClang can't recognize "-z" parameters, why?
                    if(NOT APPLE)
                        target_link_options(${__target} PRIVATE
                            $<$<NOT:$<CONFIG:Debug>>:-Wl,-z,relro -Wl,-z,now -Wl,-z,noexecstack -Wl,-z,separate-code>
                        )
                    endif()
                endif()
                if(COM_ARGS_CFGUARD)
                    if(NOT APPLE)
                        target_compile_options(${__target} PRIVATE
                            $<$<NOT:$<CONFIG:Debug>>:-fsanitize=cfi -fsanitize-cfi-cross-dso>
                        )
                    endif()
                endif()
            endif()
        endif()
    endforeach()
endfunction()

function(setup_gui_app)
    # TODO: macOS bundle icon
    cmake_parse_arguments(GUI_ARGS "" "BUNDLE_ID;BUNDLE_VERSION;BUNDLE_VERSION_SHORT" "TARGETS" ${ARGN})
    if(NOT GUI_ARGS_TARGETS)
        message(AUTHOR_WARNING "setup_gui_app: You need to specify at least one target for this function!")
        return()
    endif()
    if(GUI_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "setup_gui_app: Unrecognized arguments: ${GUI_ARGS_UNPARSED_ARGUMENTS}")
    endif()
    foreach(__target ${GUI_ARGS_TARGETS})
        set_target_properties(${__target} PROPERTIES
            WIN32_EXECUTABLE TRUE
            MACOSX_BUNDLE TRUE
        )
        if(GUI_ARGS_BUNDLE_ID)
            set_target_properties(${__target} PROPERTIES
                MACOSX_BUNDLE_GUI_IDENTIFIER ${GUI_ARGS_BUNDLE_ID}
            )
        endif()
        if(GUI_ARGS_BUNDLE_VERSION)
            set_target_properties(${__target} PROPERTIES
                MACOSX_BUNDLE_BUNDLE_VERSION ${GUI_ARGS_BUNDLE_VERSION}
            )
        endif()
        if(GUI_ARGS_BUNDLE_VERSION_SHORT)
            set_target_properties(${__target} PROPERTIES
                MACOSX_BUNDLE_SHORT_VERSION_STRING ${GUI_ARGS_BUNDLE_VERSION_SHORT}
            )
        endif()
    endforeach()
endfunction()

function(prepare_package_export)
    cmake_parse_arguments(PKG_ARGS "NO_INSTALL" "PACKAGE_NAME;PACKAGE_VERSION" "" ${ARGN})
    if(NOT PKG_ARGS_PACKAGE_NAME)
        message(AUTHOR_WARNING "prepare_package_export: You need to specify the package name for this function!")
        return()
    endif()
    if(NOT PKG_ARGS_PACKAGE_VERSION)
        message(AUTHOR_WARNING "prepare_package_export: You need to specify the package version for this function!")
        return()
    endif()
    if(PKG_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "prepare_package_export: Unrecognized arguments: ${PKG_ARGS_UNPARSED_ARGUMENTS}")
    endif()
    include(CMakePackageConfigHelpers)
    include(GNUInstallDirs)
    write_basic_package_version_file(
        "${CMAKE_CURRENT_BINARY_DIR}/${PKG_ARGS_PACKAGE_NAME}ConfigVersion.cmake"
        VERSION ${PKG_ARGS_PACKAGE_VERSION}
        COMPATIBILITY AnyNewerVersion
    )
    configure_package_config_file("${CMAKE_CURRENT_SOURCE_DIR}/${PKG_ARGS_PACKAGE_NAME}Config.cmake.in"
        "${CMAKE_CURRENT_BINARY_DIR}/${PKG_ARGS_PACKAGE_NAME}Config.cmake"
        INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PKG_ARGS_PACKAGE_NAME}"
        NO_CHECK_REQUIRED_COMPONENTS_MACRO
    )
    if(NOT PKG_ARGS_NO_INSTALL)
        install(FILES
            "${CMAKE_CURRENT_BINARY_DIR}/${PKG_ARGS_PACKAGE_NAME}Config.cmake"
            "${CMAKE_CURRENT_BINARY_DIR}/${PKG_ARGS_PACKAGE_NAME}ConfigVersion.cmake"
            DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PKG_ARGS_PACKAGE_NAME}"
        )
    endif()
endfunction()

function(setup_package_export)
    cmake_parse_arguments(PKG_ARGS ""
        "TARGET;BIN_PATH;LIB_PATH;INCLUDE_PATH;NAMESPACE;PACKAGE_NAME"
        "PUBLIC_HEADERS;PRIVATE_HEADERS;ALIAS_HEADERS" ${ARGN})
    if(NOT PKG_ARGS_TARGET)
        message(AUTHOR_WARNING "setup_package_export: You need to specify a target for this function!")
        return()
    endif()
    if(NOT PKG_ARGS_NAMESPACE)
        message(AUTHOR_WARNING "setup_package_export: You need to specify an export namespace for this function!")
        return()
    endif()
    if(NOT PKG_ARGS_PACKAGE_NAME)
        message(AUTHOR_WARNING "setup_package_export: You need to specify a package name for this function!")
        return()
    endif()
    if(PKG_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "setup_package_export: Unrecognized arguments: ${PKG_ARGS_UNPARSED_ARGUMENTS}")
    endif()
    include(GNUInstallDirs)
    set(__bin_dir "${CMAKE_INSTALL_BINDIR}")
    if(PKG_ARGS_BIN_PATH)
        set(__bin_dir "${__bin_dir}/${PKG_ARGS_BIN_PATH}")
    endif()
    set(__lib_dir "${CMAKE_INSTALL_LIBDIR}")
    if(PKG_ARGS_LIB_PATH)
        set(__lib_dir "${__lib_dir}/${PKG_ARGS_LIB_PATH}")
    endif()
    set(__inc_dir "${CMAKE_INSTALL_INCLUDEDIR}")
    if(PKG_ARGS_INCLUDE_PATH)
        set(__inc_dir "${__inc_dir}/${PKG_ARGS_INCLUDE_PATH}")
    endif()
    install(TARGETS ${PKG_ARGS_TARGET}
        EXPORT ${PKG_ARGS_TARGET}Targets
        RUNTIME  DESTINATION "${__bin_dir}"
        LIBRARY  DESTINATION "${__lib_dir}"
        ARCHIVE  DESTINATION "${__lib_dir}"
        INCLUDES DESTINATION "${__inc_dir}"
    )
    export(EXPORT ${PKG_ARGS_TARGET}Targets
        FILE "${CMAKE_CURRENT_BINARY_DIR}/cmake/${PKG_ARGS_TARGET}Targets.cmake"
        NAMESPACE ${PKG_ARGS_NAMESPACE}::
    )
    if(PKG_ARGS_PUBLIC_HEADERS)
        install(FILES ${PKG_ARGS_PUBLIC_HEADERS} DESTINATION "${__inc_dir}")
    endif()
    if(PKG_ARGS_PRIVATE_HEADERS)
        install(FILES ${PKG_ARGS_PRIVATE_HEADERS} DESTINATION "${__inc_dir}/private")
    endif()
    if(PKG_ARGS_ALIAS_HEADERS)
        install(FILES ${PKG_ARGS_ALIAS_HEADERS} DESTINATION "${__inc_dir}")
    endif()
    install(EXPORT ${PKG_ARGS_TARGET}Targets
        FILE ${PKG_ARGS_TARGET}Targets.cmake
        NAMESPACE ${PKG_ARGS_NAMESPACE}::
        DESTINATION "${__lib_dir}/cmake/${PKG_ARGS_PACKAGE_NAME}"
    )
endfunction()

function(deploy_qt_runtime)
    cmake_parse_arguments(DEPLOY_ARGS "NO_INSTALL" "TARGET;QML_SOURCE_DIR;QML_IMPORT_DIR" "" ${ARGN})
    if(NOT DEPLOY_ARGS_TARGET)
        message(AUTHOR_WARNING "deploy_qt_runtime: You need to specify a target for this function!")
        return()
    endif()
    if(DEPLOY_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "deploy_qt_runtime: Unrecognized arguments: ${DEPLOY_ARGS_UNPARSED_ARGUMENTS}")
    endif()
    find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Core)
    if(NOT (Qt5_FOUND OR Qt6_FOUND))
        message(AUTHOR_WARNING "deploy_qt_runtime: You need to install the QtCore module first to be able to deploy the Qt libraries.")
        return()
    endif()
    find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Core)
    # "QT_QMAKE_EXECUTABLE" is usually defined by QtCreator.
    if(NOT DEFINED QT_QMAKE_EXECUTABLE)
        get_target_property(QT_QMAKE_EXECUTABLE Qt::qmake IMPORTED_LOCATION)
    endif()
    if(NOT EXISTS "${QT_QMAKE_EXECUTABLE}")
        message(WARNING "deploy_qt_runtime: Can't locate the QMake executable.")
        return()
    endif()
    cmake_path(GET QT_QMAKE_EXECUTABLE PARENT_PATH __qt_bin_dir)
    find_program(__deploy_tool NAMES windeployqt macdeployqt HINTS "${__qt_bin_dir}")
    if(NOT EXISTS "${__deploy_tool}")
        message(WARNING "deploy_qt_runtime: Can't locate the deployqt tool.")
        return()
    endif()
    set(__is_quick_app FALSE)
    if(WIN32)
        set(__old_deploy_params)
        if(QT_VERSION_MAJOR LESS 6)
            set(__old_deploy_params
                --no-webkit2
                #--no-angle
            )
        endif()
        set(__quick_deploy_params)
        if(DEPLOY_ARGS_QML_SOURCE_DIR)
            set(__is_quick_app TRUE)
            set(__quick_deploy_params
                --qmldir "${DEPLOY_ARGS_QML_SOURCE_DIR}"
            )
            if(QT_VERSION VERSION_GREATER_EQUAL "6.6") # FIXME
                set(__quick_deploy_params
                    ${__quick_deploy_params}
                    --qml-deploy-dir "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>/../qml"
                )
            else()
                set(__quick_deploy_params
                    ${__quick_deploy_params}
                    --dir "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>/../qml"
                )
            endif()
        endif()
        if(DEPLOY_ARGS_QML_IMPORT_DIR)
            set(__is_quick_app TRUE)
            set(__quick_deploy_params
                ${__quick_deploy_params}
                --qmlimport "${DEPLOY_ARGS_QML_IMPORT_DIR}"
            )
        endif()
        set(__extra_deploy_params)
        if(QT_VERSION VERSION_GREATER_EQUAL "6.6") # FIXME
            set(__extra_deploy_params
                --translationdir "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>/../translations"
            )
        endif()
        add_custom_command(TARGET ${DEPLOY_ARGS_TARGET} POST_BUILD COMMAND
            "${__deploy_tool}"
            $<$<CONFIG:Debug>:--debug>
            $<$<CONFIG:MinSizeRel,Release,RelWithDebInfo>:--release>
            --libdir "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>"
            --plugindir "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>/../plugins"
            #--no-translations
            #--no-system-d3d-compiler
            --no-virtualkeyboard
            --no-compiler-runtime
            #--no-opengl-sw
            --force
            #--verbose 0
            ${__quick_deploy_params}
            ${__old_deploy_params}
            ${__extra_deploy_params}
            "$<TARGET_FILE:${DEPLOY_ARGS_TARGET}>"
        )
    elseif(APPLE)
        set(__quick_deploy_params)
        if(DEPLOY_ARGS_QML_SOURCE_DIR)
            set(__is_quick_app TRUE)
            set(__quick_deploy_params
                -qmldir="${DEPLOY_ARGS_QML_SOURCE_DIR}"
            )
        endif()
        if(DEPLOY_ARGS_QML_IMPORT_DIR)
            set(__is_quick_app TRUE)
            set(__quick_deploy_params
                ${__quick_deploy_params}
                -qmlimport="${DEPLOY_ARGS_QML_IMPORT_DIR}"
            )
        endif()
        add_custom_command(TARGET ${DEPLOY_ARGS_TARGET} POST_BUILD COMMAND
            "${__deploy_tool}"
            "$<TARGET_BUNDLE_DIR:${DEPLOY_ARGS_TARGET}>"
            #-verbose=0
            ${__quick_deploy_params}
        )
    elseif(UNIX)
        # TODO
    endif()
    #[[add_custom_command(TARGET ${DEPLOY_ARGS_TARGET} POST_BUILD COMMAND
        "${CMAKE_COMMAND}"
        -E copy
        "${CMAKE_CURRENT_LIST_DIR}/qt.conf" # FIXME
        "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>"
    )]]
    if(MSVC)
        add_custom_command(TARGET ${DEPLOY_ARGS_TARGET} POST_BUILD COMMAND
            "${CMAKE_COMMAND}"
            -E rm -f
            "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>/$<TARGET_FILE_BASE_NAME:${DEPLOY_ARGS_TARGET}>.manifest"
        )
    endif()
    add_custom_command(TARGET ${DEPLOY_ARGS_TARGET} POST_BUILD COMMAND
        "${CMAKE_COMMAND}"
        -E rm -rf
        "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>/translations"
        "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>/qml/translations"
        "$<TARGET_FILE_DIR:${DEPLOY_ARGS_TARGET}>/../qml/translations"
    )
    if(NOT DEPLOY_ARGS_NO_INSTALL)
        include(GNUInstallDirs)
        install(TARGETS ${DEPLOY_ARGS_TARGET}
            BUNDLE  DESTINATION .
            RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
        )
        if(QT_VERSION VERSION_GREATER_EQUAL "6.3")
            set(__deploy_script)
            if(${__is_quick_app})
                qt_generate_deploy_qml_app_script(
                    TARGET ${DEPLOY_ARGS_TARGET}
                    OUTPUT_SCRIPT __deploy_script
                    #MACOS_BUNDLE_POST_BUILD
                    NO_UNSUPPORTED_PLATFORM_ERROR
                    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
                )
            else()
                qt_generate_deploy_app_script(
                    TARGET ${DEPLOY_ARGS_TARGET}
                    OUTPUT_SCRIPT __deploy_script
                    NO_UNSUPPORTED_PLATFORM_ERROR
                )
            endif()
            install(SCRIPT "${__deploy_script}")
        endif()
    endif()
endfunction()

function(setup_translations)
    cmake_parse_arguments(TRANSLATION_ARGS "NO_INSTALL" "TARGET;TS_DIR;QM_DIR;INSTALL_DIR" "LOCALES" ${ARGN})
    if(NOT TRANSLATION_ARGS_TARGET)
        message(AUTHOR_WARNING "setup_translations: You need to specify a target for this function!")
        return()
    endif()
    if(NOT TRANSLATION_ARGS_LOCALES)
        message(AUTHOR_WARNING "setup_translations: You need to specify at least one locale for this function!")
        return()
    endif()
    if(TRANSLATION_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "setup_translations: Unrecognized arguments: ${TRANSLATION_ARGS_UNPARSED_ARGUMENTS}")
    endif()
    # Qt5's CMake functions to create translations lack many features
    # we need and what's worse, they also have a severe bug which will
    # wipe out our .ts files' contents every time we call them, so we
    # really can't use them until Qt6 (the functions have been completely
    # re-written in Qt6 and according to my experiments they work reliably
    # now finally).
    find_package(Qt6 QUIET COMPONENTS LinguistTools)
    if(NOT Qt6LinguistTools_FOUND)
        message(AUTHOR_WARNING "setup_translations: You need to install the Qt Linguist Tools first to be able to create translations.")
        return()
    endif()
    set(__ts_dir translations)
    if(TRANSLATION_ARGS_TS_DIR)
        set(__ts_dir "${TRANSLATION_ARGS_TS_DIR}")
    endif()
    set(__qm_dir "${PROJECT_BINARY_DIR}/translations")
    if(TRANSLATION_ARGS_QM_DIR)
        set(__qm_dir "${TRANSLATION_ARGS_QM_DIR}")
    endif()
    set(__ts_files)
    foreach(__locale ${TRANSLATION_ARGS_LOCALES})
        list(APPEND __ts_files "${__ts_dir}/${TRANSLATION_ARGS_TARGET}_${__locale}.ts")
    endforeach()
    set_source_files_properties(${__ts_files} PROPERTIES
        OUTPUT_LOCATION "${__qm_dir}"
    )
    set(__qm_files)
    qt_add_translations(${TRANSLATION_ARGS_TARGET}
        TS_FILES ${__ts_files}
        QM_FILES_OUTPUT_VARIABLE __qm_files
        LUPDATE_OPTIONS
            -no-obsolete # Don't keep vanished translation contexts.
        LRELEASE_OPTIONS
            -compress # Compress the QM file if the file size can be decreased siginificantly.
            -nounfinished # Don't include unfinished translations (to save file size).
            -removeidentical # Don't include translations that are the same with their original texts (to save file size).
    )
    if(NOT TRANSLATION_ARGS_NO_INSTALL)
        set(__inst_dir translations)
        if(TRANSLATION_ARGS_INSTALL_DIR)
            set(__inst_dir "${TRANSLATION_ARGS_INSTALL_DIR}")
        endif()
        install(FILES ${__qm_files} DESTINATION "${__inst_dir}")
    endif()
endfunction()

function(generate_win32_rc_file)
    cmake_parse_arguments(RC_ARGS "LIBRARY" "PATH;COMMENTS;COMPANY;DESCRIPTION;VERSION;INTERNAL_NAME;COPYRIGHT;TRADEMARK;ORIGINAL_FILENAME;PRODUCT" "ICONS" ${ARGN})
    if(NOT RC_ARGS_PATH)
        message(AUTHOR_WARNING "generate_win32_rc_file: You need to specify where to put the generated rc file for this function!")
        return()
    endif()
    if(RC_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "generate_win32_rc_file: Unrecognized arguments: ${RC_ARGS_UNPARSED_ARGUMENTS}")
        return()
    endif()
    set(__file_type)
    if(RC_ARGS_LIBRARY)
        set(__file_type "VFT_DLL")
    else()
        set(__file_type "VFT_APP")
    endif()
    set(__icons)
    if(RC_ARGS_ICONS)
        set(__index 1)
        foreach(__icon IN LISTS RC_ARGS_ICONS)
            string(APPEND __icons "IDI_ICON${__index}    ICON    \"${__icon}\"\n")
            math(EXPR __index "${__index} +1")
        endforeach()
    endif()
    set(__comments)
    if(RC_ARGS_COMMENTS)
        set(__comments "${RC_ARGS_COMMENTS}")
    endif()
    set(__company)
    if(RC_ARGS_COMPANY)
        set(__company "${RC_ARGS_COMPANY}")
    endif()
    set(__description)
    if(RC_ARGS_DESCRIPTION)
        set(__description "${RC_ARGS_DESCRIPTION}")
    endif()
    set(__version)
    if(RC_ARGS_VERSION)
        if(RC_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
            set(__version "${RC_ARGS_VERSION}")
        elseif(RC_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
            set(__version "${RC_ARGS_VERSION}.0")
        elseif(RC_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+")
            set(__version "${RC_ARGS_VERSION}.0.0")
        elseif(RC_ARGS_VERSION MATCHES "[0-9]+")
            set(__version "${RC_ARGS_VERSION}.0.0.0")
        else()
            message(FATAL_ERROR "generate_win32_rc_file: Invalid version format: '${RC_ARGS_VERSION}'")
        endif()
    else()
        set(__version "0.0.0.0")
    endif()
    set(__version_comma)
    string(REPLACE "." "," __version_comma ${__version})
    set(__internal_name)
    if(RC_ARGS_INTERNAL_NAME)
        set(__internal_name "${RC_ARGS_INTERNAL_NAME}")
    endif()
    set(__copyright)
    if(RC_ARGS_COPYRIGHT)
        set(__copyright "${RC_ARGS_COPYRIGHT}")
    endif()
    set(__trademark)
    if(RC_ARGS_TRADEMARK)
        set(__trademark "${RC_ARGS_TRADEMARK}")
    endif()
    set(__original_filename)
    if(RC_ARGS_ORIGINAL_FILENAME)
        set(__original_filename "${RC_ARGS_ORIGINAL_FILENAME}")
    endif()
    set(__product)
    if(RC_ARGS_PRODUCT)
        set(__product "${RC_ARGS_PRODUCT}")
    endif()
    set(__contents "// This file is auto-generated by CMake. DO NOT EDIT! ALL MODIFICATIONS WILL BE LOST!

#include <windows.h> // Use lower-cased file names to be compatible with MinGW.

${__icons}

VS_VERSION_INFO VERSIONINFO
FILEVERSION     ${__version_comma}
PRODUCTVERSION  ${__version_comma}
FILEFLAGSMASK   0x3fL
#ifdef _DEBUG
    FILEFLAGS   VS_FF_DEBUG
#else // !_DEBUG
    FILEFLAGS   0x0L
#endif // _DEBUG
FILEOS          VOS_NT_WINDOWS32
FILETYPE        ${__file_type}
FILESUBTYPE     VFT2_UNKNOWN
BEGIN
    BLOCK \"StringFileInfo\"
    BEGIN
        BLOCK \"040904b0\"
        BEGIN
            VALUE \"CompanyName\",      \"${__company}\\0\"
            VALUE \"FileDescription\",  \"${__description}\\0\"
            VALUE \"FileVersion\",      \"${__version}\\0\"
            VALUE \"LegalCopyright\",   \"${__copyright}\\0\"
            VALUE \"OriginalFilename\", \"${__original_filename}\\0\"
            VALUE \"ProductName\",      \"${__product}\\0\"
            VALUE \"ProductVersion\",   \"${__version}\\0\"
            VALUE \"Comments\",         \"${__comments}\\0\"
            VALUE \"LegalTrademarks\",  \"${__trademark}\\0\"
            VALUE \"InternalName\",     \"${__internal_name}\\0\"
        END
    END
    BLOCK \"VarFileInfo\"
    BEGIN
        VALUE \"Translation\", 0x0409, 1200
    END
END
")
    file(WRITE "${RC_ARGS_PATH}" ${__contents})
endfunction()

function(generate_win32_manifest_file)
    cmake_parse_arguments(MF_ARGS "UTF8_CODEPAGE;VISTA_COMPAT;WIN7_COMPAT;WIN8_COMPAT;WIN8_1_COMPAT;WIN10_COMPAT;WIN11_COMPAT;XAML_ISLANDS_COMPAT;REQUIRE_ADMIN" "PATH;ID;VERSION;DESCRIPTION" "" ${ARGN})
    if(NOT MF_ARGS_PATH)
        message(AUTHOR_WARNING "generate_win32_manifest_file: You need to specify where to put the generated rc file for this function!")
        return()
    endif()
    if(NOT MF_ARGS_ID)
        message(AUTHOR_WARNING "generate_win32_manifest_file: You need to specify your application identifier for this function!")
        return()
    endif()
    if(MF_ARGS_UNPARSED_ARGUMENTS)
        message(AUTHOR_WARNING "generate_win32_manifest_file: Unrecognized arguments: ${MF_ARGS_UNPARSED_ARGUMENTS}")
        return()
    endif()
    set(__id "${MF_ARGS_ID}")
    set(__version)
    if(MF_ARGS_VERSION)
        if(MF_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+\\.[0-9]+")
            set(__version "${MF_ARGS_VERSION}")
        elseif(MF_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+\\.[0-9]+")
            set(__version "${MF_ARGS_VERSION}.0")
        elseif(MF_ARGS_VERSION MATCHES "[0-9]+\\.[0-9]+")
            set(__version "${MF_ARGS_VERSION}.0.0")
        elseif(MF_ARGS_VERSION MATCHES "[0-9]+")
            set(__version "${MF_ARGS_VERSION}.0.0.0")
        else()
            message(FATAL_ERROR "generate_win32_manifest_file: Invalid version format: '${MF_ARGS_VERSION}'")
        endif()
    else()
        set(__version "0.0.0.0")
    endif()
    set(__description)
    if(MF_ARGS_DESCRIPTION)
        set(__description "<description>${MF_ARGS_DESCRIPTION}</description>")
    endif()
    set(__execution_level)
    if(MF_ARGS_REQUIRE_ADMIN)
        set(__execution_level "requireAdministrator")
    else()
        set(__execution_level "asInvoker")
    endif()
    set(__vista_compat)
    if(MF_ARGS_VISTA_COMPAT)
        set(__vista_compat "<!-- Windows Vista and Windows Server 2008 -->
      <supportedOS Id=\"{e2011457-1546-43c5-a5fe-008deee3d3f0}\"/>")
    endif()
    set(__win7_compat)
    if(MF_ARGS_WIN7_COMPAT)
        set(__win7_compat "<!-- Windows 7 and Windows Server 2008 R2 -->
      <supportedOS Id=\"{35138b9a-5d96-4fbd-8e2d-a2440225f93a}\"/>")
    endif()
    set(__win8_compat)
    if(MF_ARGS_WIN8_COMPAT)
        set(__win8_compat "<!-- Windows 8 and Windows Server 2012 -->
      <supportedOS Id=\"{4a2f28e3-53b9-4441-ba9c-d69d4a4a6e38}\"/>")
    endif()
    set(__win8_1_compat)
    if(MF_ARGS_WIN8_1_COMPAT)
        set(__win8_1_compat "<!-- Windows 8.1 and Windows Server 2012 R2 -->
      <supportedOS Id=\"{1f676c76-80e1-4239-95bb-83d0f6d0da78}\"/>")
    endif()
    set(__win10_11_compat)
    if(MF_ARGS_WIN10_COMPAT OR MF_ARGS_WIN11_COMPAT)
        set(__win10_11_compat "<!-- Windows 10 and Windows 11 -->
      <supportedOS Id=\"{8e0f7a12-bfb3-4fe8-b9a5-48fd50a15a9a}\"/>")
    endif()
    set(__xaml_islands_compat "<!-- Windows 10 Version 1809 (October 2018 Update) -->
      <maxversiontested Id=\"10.0.17763.0\"/>
      <!-- Windows 10 Version 1903 (May 2019 Update) -->
      <maxversiontested Id=\"10.0.18362.0\"/>
      <!-- Windows 10 Version 1909 (November 2019 Update) -->
      <maxversiontested Id=\"10.0.18363.0\"/>
      <!-- Windows 10 Version 2004 (May 2020 Update) -->
      <maxversiontested Id=\"10.0.19041.0\"/>
      <!-- Windows 10 Version 20H2 (October 2020 Update) -->
      <maxversiontested Id=\"10.0.19042.0\"/>
      <!-- Windows 10 Version 21H1 (May 2021 Update) -->
      <maxversiontested Id=\"10.0.19043.0\"/>
      <!-- Windows 10 Version 21H2 (November 2021 Update) -->
      <maxversiontested Id=\"10.0.19044.0\"/>
      <!-- Windows 10 Version 22H2 (October 2022 Update) -->
      <maxversiontested Id=\"10.0.19045.0\"/>
      <!-- Windows 11 Version 21H2 -->
      <maxversiontested Id=\"10.0.22000.0\"/>
      <!-- Windows 11 Version 22H2 (October 2022 Update) -->
      <maxversiontested Id=\"10.0.22621.0\"/>")
    set(__utf8_codepage)
    if(MF_ARGS_UTF8_CODEPAGE)
        set(__utf8_codepage "<activeCodePage xmlns=\"http://schemas.microsoft.com/SMI/2019/WindowsSettings\">UTF-8</activeCodePage>")
    endif()
    set(__contents "<?xml version=\"1.0\" encoding=\"UTF-8\" standalone=\"yes\"?>

<!-- This file is auto-generated by CMake. DO NOT EDIT! ALL MODIFICATIONS WILL BE LOST! -->

<assembly xmlns=\"urn:schemas-microsoft-com:asm.v1\" manifestVersion=\"1.0\">
  <assemblyIdentity type=\"win32\" name=\"${__id}\" version=\"${__version}\"/>
  ${__description}
  <dependency>
    <dependentAssembly>
      <assemblyIdentity type=\"win32\" name=\"Microsoft.Windows.Common-Controls\" version=\"6.0.0.0\" processorArchitecture=\"*\" publicKeyToken=\"6595b64144ccf1df\" language=\"*\"/>
    </dependentAssembly>
  </dependency>
  <trustInfo xmlns=\"urn:schemas-microsoft-com:asm.v3\">
    <security>
      <requestedPrivileges>
        <requestedExecutionLevel level=\"${__execution_level}\" uiAccess=\"false\"/>
      </requestedPrivileges>
    </security>
  </trustInfo>
  <compatibility xmlns=\"urn:schemas-microsoft-com:compatibility.v1\">
    <application>
      ${__xaml_islands_compat}
      ${__vista_compat}
      ${__win7_compat}
      ${__win8_compat}
      ${__win8_1_compat}
      ${__win10_11_compat}
    </application>
  </compatibility>
  <application xmlns=\"urn:schemas-microsoft-com:asm.v3\">
    <windowsSettings>
      <dpiAware xmlns=\"http://schemas.microsoft.com/SMI/2005/WindowsSettings\">True/PM</dpiAware>
      <dpiAwareness xmlns=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">PerMonitorV2, PerMonitor</dpiAwareness>
      <printerDriverIsolation xmlns=\"http://schemas.microsoft.com/SMI/2011/WindowsSettings\">True</printerDriverIsolation>
      <longPathAware xmlns=\"http://schemas.microsoft.com/SMI/2016/WindowsSettings\">True</longPathAware>
      <heapType xmlns=\"http://schemas.microsoft.com/SMI/2020/WindowsSettings\">SegmentHeap</heapType>
      ${__utf8_codepage}
    </windowsSettings>
  </application>
</assembly>
")
    file(WRITE "${MF_ARGS_PATH}" ${__contents})
endfunction()

function(query_qt_paths)
    cmake_parse_arguments(QT_ARGS "" "SDK_DIR;BIN_DIR;DOC_DIR;INCLUDE_DIR;LIB_DIR;PLUGINS_DIR;QML_DIR;TRANSLATIONS_DIR" "" ${ARGN})
    find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Core)
    find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Core)
    if(NOT (Qt6_FOUND OR Qt5_FOUND))
        message(AUTHOR_WARNING "You need to install the QtCore module first to be able to query Qt installation paths.")
        return()
    endif()
    # /whatever/Qt/6.5.0/gcc_64/lib/cmake/Qt6
    set(__qt_inst_dir "${Qt${QT_VERSION_MAJOR}_DIR}")
    cmake_path(GET __qt_inst_dir PARENT_PATH __qt_inst_dir)
    cmake_path(GET __qt_inst_dir PARENT_PATH __qt_inst_dir)
    cmake_path(GET __qt_inst_dir PARENT_PATH __qt_inst_dir)
    if(QT_ARGS_SDK_DIR)
        set(${QT_ARGS_SDK_DIR} "${__qt_inst_dir}" PARENT_SCOPE)
    endif()
    if(QT_ARGS_BIN_DIR)
        set(${QT_ARGS_BIN_DIR} "${__qt_inst_dir}/bin" PARENT_SCOPE)
    endif()
    if(QT_ARGS_DOC_DIR)
        set(${QT_ARGS_DOC_DIR} "${__qt_inst_dir}/doc" PARENT_SCOPE)
    endif()
    if(QT_ARGS_INCLUDE_DIR)
        set(${QT_ARGS_INCLUDE_DIR} "${__qt_inst_dir}/include" PARENT_SCOPE)
    endif()
    if(QT_ARGS_LIB_DIR)
        set(${QT_ARGS_LIB_DIR} "${__qt_inst_dir}/lib" PARENT_SCOPE)
    endif()
    if(QT_ARGS_PLUGINS_DIR)
        set(${QT_ARGS_PLUGINS_DIR} "${__qt_inst_dir}/plugins" PARENT_SCOPE)
    endif()
    if(QT_ARGS_QML_DIR)
        set(${QT_ARGS_QML_DIR} "${__qt_inst_dir}/qml" PARENT_SCOPE)
    endif()
    if(QT_ARGS_TRANSLATIONS_DIR)
        set(${QT_ARGS_TRANSLATIONS_DIR} "${__qt_inst_dir}/translations" PARENT_SCOPE)
    endif()
endfunction()

function(query_qt_library_info)
    cmake_parse_arguments(QT_ARGS "" "STATIC;SHARED;VERSION" "" ${ARGN})
    find_package(QT NAMES Qt6 Qt5 QUIET COMPONENTS Core)
    find_package(Qt${QT_VERSION_MAJOR} QUIET COMPONENTS Core)
    if(NOT (Qt6_FOUND OR Qt5_FOUND))
        message(AUTHOR_WARNING "You need to install the QtCore module first to be able to query Qt library information.")
        return()
    endif()
    if(QT_ARGS_STATIC OR QT_ARGS_SHARED)
        get_target_property(__lib_type Qt${QT_VERSION_MAJOR}::Core TYPE)
        if(QT_ARGS_STATIC)
            if(__lib_type STREQUAL "STATIC_LIBRARY")
                set(${QT_ARGS_STATIC} ON PARENT_SCOPE)
            elseif(__lib_type STREQUAL "SHARED_LIBRARY")
                set(${QT_ARGS_STATIC} OFF PARENT_SCOPE)
            endif()
        endif()
        if(QT_ARGS_SHARED)
            if(__lib_type STREQUAL "STATIC_LIBRARY")
                set(${QT_ARGS_SHARED} OFF PARENT_SCOPE)
            elseif(__lib_type STREQUAL "SHARED_LIBRARY")
                set(${QT_ARGS_SHARED} ON PARENT_SCOPE)
            endif()
        endif()
    endif()
    if(QT_ARGS_VERSION)
        set(${QT_ARGS_VERSION} "${QT_VERSION}" PARENT_SCOPE)
    endif()
endfunction()

function(dump_target_info)
    cmake_parse_arguments(arg "" "" "TARGETS" ${ARGN})
    if(NOT arg_TARGETS)
        message(AUTHOR_WARNING "dump_target_info: you have to specify at least one target!")
        return()
    endif()
    foreach(__target ${arg_TARGETS})
        if(NOT TARGET ${__target})
            message(AUTHOR_WARNING "${__target} is not a valid CMake target!")
            continue()
        endif()
        set(__compile_options)
        set(__compile_definitions)
        set(__link_options)
        get_target_property(__compile_options ${__target} COMPILE_OPTIONS)
        get_target_property(__compile_definitions ${__target} COMPILE_DEFINITIONS)
        get_target_property(__link_options ${__target} LINK_OPTIONS)
        message("${__target}'s compile options: ${__compile_options}")
        message("${__target}'s compile definitions: ${__compile_definitions}")
        message("${__target}'s link options: ${__link_options}")
    endforeach()
endfunction()
