add_executable(UnitTests
    player_unitest.cpp)

include_directories("../include")

find_library (
    LIB_XAMP_OUTPUT_DEVICE
    NAMES xamp_output_device libxamp_output_device
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../xamp_output_device/debug/"
    NO_DEFAULT_PATH
)

find_library (
    LIB_XAMP_PLAYER
    NAMES xamp_player libxamp_player
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../debug/"
    NO_DEFAULT_PATH
)

target_link_libraries(UnitTests gtest_main ${LIB_XAMP_PLAYER} ${LIB_XAMP_OUTPUT_DEVICE})

add_test(
  NAME
    unit
  COMMAND
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/unit_tests
  )
