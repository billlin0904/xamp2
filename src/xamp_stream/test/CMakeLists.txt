add_executable(UnitTests
    stream_unitest.cpp)

include_directories("../include")

find_library (
    LIB_XAMP_STREAM
    NAMES xamp_stream libxamp_stream
    HINTS "${CMAKE_CURRENT_LIST_DIR}/../debug/"
    NO_DEFAULT_PATH
)

target_link_libraries(UnitTests gtest_main ${LIB_XAMP_STREAM})

add_test(
  NAME
    unit
  COMMAND
    ${CMAKE_BINARY_DIR}/${CMAKE_INSTALL_BINDIR}/unit_tests
  )
