add_executable(UnitTests
    open_device_unitest.cpp)

include_directories("../include")

find_library (
    LIB_XAMP_OUTPUT_DEVICE
    NAMES xamp_output_device libxamp_output_device
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../debug/"
    NO_DEFAULT_PATH
)

target_link_libraries(UnitTests gtest_main ${LIB_XAMP_OUTPUT_DEVICE})

add_test(
  NAME
    unit
  COMMAND
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/unit_tests
  )
