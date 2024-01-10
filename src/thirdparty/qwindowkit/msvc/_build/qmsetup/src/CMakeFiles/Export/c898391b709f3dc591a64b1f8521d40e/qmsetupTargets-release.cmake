#----------------------------------------------------------------
# Generated CMake target import file for configuration "Release".
#----------------------------------------------------------------

# Commands may need to know the format version.
set(CMAKE_IMPORT_FILE_VERSION 1)

# Import target "qmsetup::corecmd" for configuration "Release"
set_property(TARGET qmsetup::corecmd APPEND PROPERTY IMPORTED_CONFIGURATIONS RELEASE)
set_target_properties(qmsetup::corecmd PROPERTIES
  IMPORTED_LOCATION_RELEASE "${_IMPORT_PREFIX}/bin/qmcorecmd.exe"
  )

list(APPEND _cmake_import_check_targets qmsetup::corecmd )
list(APPEND _cmake_import_check_files_for_qmsetup::corecmd "${_IMPORT_PREFIX}/bin/qmcorecmd.exe" )

# Commands beyond this point should not need to know the version.
set(CMAKE_IMPORT_FILE_VERSION)
