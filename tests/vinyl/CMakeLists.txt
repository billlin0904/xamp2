cmake_minimum_required(VERSION 3.24)
project(xamp_vinyl_tests LANGUAGES CXX)
set(CMAKE_AUTOMOC ON)
set(CMAKE_CXX_STANDARD 23)
find_package(Qt6 REQUIRED COMPONENTS Widgets Test)
add_executable(vinyl_widget_test
    vinyl_widget_test.cpp
    ../../src/widget_shared/widget/vinylwidget.cpp
)
target_compile_definitions(vinyl_widget_test PRIVATE BUILD_STATIC QT_NO_CAST_FROM_ASCII)
target_include_directories(vinyl_widget_test PRIVATE ../../src/widget_shared ../../src/xamp_base/include)
target_link_libraries(vinyl_widget_test PRIVATE Qt6::Widgets Qt6::Test)
enable_testing()
add_test(NAME vinyl_widget_test COMMAND vinyl_widget_test)
set_tests_properties(vinyl_widget_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")

# When included by the full application build, also exercise the actual lyrics page.
if(TARGET widget_shared AND TARGET xamp)
    add_executable(vinyl_page_test vinyl_page_test.cpp
        ../../src/xamp/xamp.qrc
        ../../src/xamp/Resource/Theme/dark/darkstyle.qrc
        ../../src/xamp/Resource/Theme/light/lightstyle.qrc)
    set_target_properties(vinyl_page_test PROPERTIES AUTORCC ON)
    target_include_directories(vinyl_page_test PRIVATE $<TARGET_PROPERTY:xamp,INCLUDE_DIRECTORIES>)
    target_link_libraries(vinyl_page_test PRIVATE widget_shared Qt6::Test)
    add_custom_command(TARGET vinyl_page_test POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
            "${CMAKE_CURRENT_SOURCE_DIR}/../../src/xamp/opencc" "$<TARGET_FILE_DIR:vinyl_page_test>/opencc"
        COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:vinyl_page_test>/fonts"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../../src/xamp/fonts/MaterialIcons-Regular.ttf"
            "$<TARGET_FILE_DIR:vinyl_page_test>/fonts/MaterialIcons-Regular.ttf"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
            "${CMAKE_CURRENT_SOURCE_DIR}/../../src/xamp/fonticon.json"
            "$<TARGET_FILE_DIR:vinyl_page_test>/fonticon.json")
    add_test(NAME vinyl_page_test COMMAND vinyl_page_test)
    set_tests_properties(vinyl_page_test PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen")
endif()
