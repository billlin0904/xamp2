#----------------------------------------------------------------
# Generated CMake target import file for configuration "MinSizeRel".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "syscmdline::syscmdline" for configuration "MinSizeRel"
set_property(TARGET syscmdline::syscmdline APPEND PROPERTY IMPORTED_CONFIGURATIONS MINSIZEREL)
set_target_properties(syscmdline::syscmdline PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_MINSIZEREL "CXX;RC"
  IMPORTED_LOCATION_MINSIZEREL "${_IMPORT_PREFIX}/lib/syscmdline.lib"
  )

list(APPEND _cmake_import_check_targets syscmdline::syscmdline )
list(APPEND _cmake_import_check_files_for_syscmdline::syscmdline "${_IMPORT_PREFIX}/lib/syscmdline.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
