include_guard(GLOBAL)

find_package(PkgConfig QUIET)

function(xamp_pkg_config_target out_target pkg_name required)
    if(NOT PkgConfig_FOUND)
        if(required)
            message(FATAL_ERROR "PkgConfig is required to find ${pkg_name}.")
        endif()
        set(${out_target} "" PARENT_SCOPE)
        return()
    endif()

    string(MAKE_C_IDENTIFIER "${pkg_name}" pkg_prefix)
    string(TOUPPER "${pkg_prefix}" pkg_prefix)

    if(required)
        pkg_check_modules(${pkg_prefix} REQUIRED IMPORTED_TARGET ${pkg_name})
    else()
        pkg_check_modules(${pkg_prefix} QUIET IMPORTED_TARGET ${pkg_name})
    endif()

    if(TARGET PkgConfig::${pkg_prefix})
        set(${out_target} PkgConfig::${pkg_prefix} PARENT_SCOPE)
    else()
        set(${out_target} "" PARENT_SCOPE)
    endif()
endfunction()

function(xamp_find_imported_library out_target target_name)
    set(options REQUIRED)
    set(oneValueArgs HEADER PKG)
    set(multiValueArgs LIB_NAMES INCLUDE_SUFFIXES)
    cmake_parse_arguments(ARG "${options}" "${oneValueArgs}" "${multiValueArgs}" ${ARGN})

    if(TARGET ${target_name})
        set(${out_target} ${target_name} PARENT_SCOPE)
        return()
    endif()

    find_path(${target_name}_INCLUDE_DIR
        NAMES ${ARG_HEADER}
        PATH_SUFFIXES ${ARG_INCLUDE_SUFFIXES})

    find_library(${target_name}_LIBRARY
        NAMES ${ARG_LIB_NAMES})

    if(${target_name}_INCLUDE_DIR AND ${target_name}_LIBRARY)
        add_library(${target_name} UNKNOWN IMPORTED)
        set_target_properties(${target_name} PROPERTIES
            IMPORTED_LOCATION "${${target_name}_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${${target_name}_INCLUDE_DIR}")
        set(${out_target} ${target_name} PARENT_SCOPE)
        return()
    endif()

    if(ARG_PKG)
        xamp_pkg_config_target(pkg_target ${ARG_PKG} ${ARG_REQUIRED})
        if(pkg_target)
            set(${out_target} ${pkg_target} PARENT_SCOPE)
            return()
        endif()
    endif()

    if(ARG_REQUIRED)
        message(FATAL_ERROR "Could not find dependency ${target_name}.")
    endif()

    set(${out_target} "" PARENT_SCOPE)
endfunction()
