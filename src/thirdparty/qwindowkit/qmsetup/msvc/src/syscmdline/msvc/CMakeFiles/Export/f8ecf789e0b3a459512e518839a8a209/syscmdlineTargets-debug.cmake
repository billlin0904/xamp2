#----------------------------------------------------------------
# Generated CMake target import file for configuration "Debug".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "syscmdline::syscmdline" for configuration "Debug"
set_property(TARGET syscmdline::syscmdline APPEND PROPERTY IMPORTED_CONFIGURATIONS DEBUG)
set_target_properties(syscmdline::syscmdline PROPERTIES
  IMPORTED_LINK_INTERFACE_LANGUAGES_DEBUG "CXX;RC"
  IMPORTED_LOCATION_DEBUG "${_IMPORT_PREFIX}/lib/syscmdlined.lib"
  )

list(APPEND _cmake_import_check_targets syscmdline::syscmdline )
list(APPEND _cmake_import_check_files_for_syscmdline::syscmdline "${_IMPORT_PREFIX}/lib/syscmdlined.lib" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
