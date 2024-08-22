cmake_minimum_required(VERSION 3.16)
project(xamp2 VERSION $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_PATCH} LANGUAGES C CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOUIC ON)

find_package(QT NAMES Qt5 Qt6 REQUIRED COMPONENTS Core)
find_package(Qt${QT_VERSION_MAJOR} REQUIRED COMPONENTS Concurrent Gui Network Sql)
find_package(Qt${QT_VERSION_MAJOR} OPTIONAL_COMPONENTS Widgets)

qt_standard_project_setup()

qt_add_executable(xamp2 WIN32 MACOSX_BUNDLE
    main.cpp
    style_util.cpp style_util.h
    xamp.cpp xamp.h xamp.ui
    xapplication.cpp xapplication.h
)
target_include_directories(xamp2 PRIVATE
    ../thirdparty/QSimpleUpdater/include
    ../thirdparty/framelesshelper/include
    ../thirdparty/pybind11/include
    ../thirdparty/qcoro/build/Qt_6_7_2_for_macOS-Debug/qcoro
    ../thirdparty/qcoro/build/Qt_6_7_2_for_macOS-Debug/qcoro/core
    ../thirdparty/qcoro/qcoro
    ../thirdparty/qcoro/qcoro/core ../thirdparty/qcoro/qcoro/core
    ../thirdparty/rapidxml
    ../thirdparty/spdlog/include
    ../thirdparty/zlib
    ../widget_shared
    ../xamp_base/include
    ../xamp_metadata/include
    ../xamp_output_device/include
    ../xamp_player/include
    ../xamp_stream/include
    /System/Library/Frameworks/Foundation.framework/Versions/C/Headers
    /opt/anaconda3/include/python3.12
)

target_compile_definitions(xamp2 PRIVATE
    QT_DEPRECATED_WARNINGS
    QT_NO_CAST_FROM_ASCII
)

target_link_libraries(xamp2 PRIVATE
    "-framework Carbon"
    "-framework Cocoa"
    "-framework Foundation"
    Qt::Concurrent
    Qt::Core
    Qt::Gui
    Qt::Network
    Qt::Sql
)


# Resources:
set(xamp_resource_files
    "Resource/Black/GitHub-Mark.png"
    "Resource/Black/close-active.ico"
    "Resource/Black/close-inactive.ico"
    "Resource/Black/maximize-active.ico"
    "Resource/Black/maximize-inactive.ico"
    "Resource/Black/minimize-active.ico"
    "Resource/Black/minimize-inactive.ico"
    "Resource/Black/play_circle.png"
    "Resource/Black/restore-active.ico"
    "Resource/Black/restore-inactive.ico"
    "Resource/Black/unknown_album.png"
    "Resource/Theme/dark/style.qss"
    "Resource/Theme/light/style.qss"
    "Resource/White/GitHub-Mark.png"
    "Resource/White/close-active.ico"
    "Resource/White/close-inactive.ico"
    "Resource/White/maximize-active.ico"
    "Resource/White/maximize-inactive.ico"
    "Resource/White/minimize-active.ico"
    "Resource/White/minimize-inactive.ico"
    "Resource/White/restore-active.ico"
    "Resource/White/restore-inactive.ico"
    "Resource/White/unknown_album.png"
    "migrations/2024-01-12-093210_init/up.sql"
    "xamp.ico"
)

qt_add_resources(xamp2 "xamp"
    PREFIX
        "/xamp"
    FILES
        ${xamp_resource_files}
)
set(darkstyle_resource_files
    "Resource/Theme/dark/rc/.keep"
    "Resource/Theme/dark/rc/arrow_down.png"
    "Resource/Theme/dark/rc/arrow_down@2x.png"
    "Resource/Theme/dark/rc/arrow_down_disabled.png"
    "Resource/Theme/dark/rc/arrow_down_disabled@2x.png"
    "Resource/Theme/dark/rc/arrow_down_focus.png"
    "Resource/Theme/dark/rc/arrow_down_focus@2x.png"
    "Resource/Theme/dark/rc/arrow_down_pressed.png"
    "Resource/Theme/dark/rc/arrow_down_pressed@2x.png"
    "Resource/Theme/dark/rc/arrow_left.png"
    "Resource/Theme/dark/rc/arrow_left@2x.png"
    "Resource/Theme/dark/rc/arrow_left_disabled.png"
    "Resource/Theme/dark/rc/arrow_left_disabled@2x.png"
    "Resource/Theme/dark/rc/arrow_left_focus.png"
    "Resource/Theme/dark/rc/arrow_left_focus@2x.png"
    "Resource/Theme/dark/rc/arrow_left_pressed.png"
    "Resource/Theme/dark/rc/arrow_left_pressed@2x.png"
    "Resource/Theme/dark/rc/arrow_right.png"
    "Resource/Theme/dark/rc/arrow_right@2x.png"
    "Resource/Theme/dark/rc/arrow_right_disabled.png"
    "Resource/Theme/dark/rc/arrow_right_disabled@2x.png"
    "Resource/Theme/dark/rc/arrow_right_focus.png"
    "Resource/Theme/dark/rc/arrow_right_focus@2x.png"
    "Resource/Theme/dark/rc/arrow_right_pressed.png"
    "Resource/Theme/dark/rc/arrow_right_pressed@2x.png"
    "Resource/Theme/dark/rc/arrow_up.png"
    "Resource/Theme/dark/rc/arrow_up@2x.png"
    "Resource/Theme/dark/rc/arrow_up_disabled.png"
    "Resource/Theme/dark/rc/arrow_up_disabled@2x.png"
    "Resource/Theme/dark/rc/arrow_up_focus.png"
    "Resource/Theme/dark/rc/arrow_up_focus@2x.png"
    "Resource/Theme/dark/rc/arrow_up_pressed.png"
    "Resource/Theme/dark/rc/arrow_up_pressed@2x.png"
    "Resource/Theme/dark/rc/base_icon.png"
    "Resource/Theme/dark/rc/base_icon@2x.png"
    "Resource/Theme/dark/rc/base_icon_disabled.png"
    "Resource/Theme/dark/rc/base_icon_disabled@2x.png"
    "Resource/Theme/dark/rc/base_icon_focus.png"
    "Resource/Theme/dark/rc/base_icon_focus@2x.png"
    "Resource/Theme/dark/rc/base_icon_pressed.png"
    "Resource/Theme/dark/rc/base_icon_pressed@2x.png"
    "Resource/Theme/dark/rc/branch_closed.png"
    "Resource/Theme/dark/rc/branch_closed@2x.png"
    "Resource/Theme/dark/rc/branch_closed_disabled.png"
    "Resource/Theme/dark/rc/branch_closed_disabled@2x.png"
    "Resource/Theme/dark/rc/branch_closed_focus.png"
    "Resource/Theme/dark/rc/branch_closed_focus@2x.png"
    "Resource/Theme/dark/rc/branch_closed_pressed.png"
    "Resource/Theme/dark/rc/branch_closed_pressed@2x.png"
    "Resource/Theme/dark/rc/branch_end.png"
    "Resource/Theme/dark/rc/branch_end@2x.png"
    "Resource/Theme/dark/rc/branch_end_disabled.png"
    "Resource/Theme/dark/rc/branch_end_disabled@2x.png"
    "Resource/Theme/dark/rc/branch_end_focus.png"
    "Resource/Theme/dark/rc/branch_end_focus@2x.png"
    "Resource/Theme/dark/rc/branch_end_pressed.png"
    "Resource/Theme/dark/rc/branch_end_pressed@2x.png"
    "Resource/Theme/dark/rc/branch_line.png"
    "Resource/Theme/dark/rc/branch_line@2x.png"
    "Resource/Theme/dark/rc/branch_line_disabled.png"
    "Resource/Theme/dark/rc/branch_line_disabled@2x.png"
    "Resource/Theme/dark/rc/branch_line_focus.png"
    "Resource/Theme/dark/rc/branch_line_focus@2x.png"
    "Resource/Theme/dark/rc/branch_line_pressed.png"
    "Resource/Theme/dark/rc/branch_line_pressed@2x.png"
    "Resource/Theme/dark/rc/branch_more.png"
    "Resource/Theme/dark/rc/branch_more@2x.png"
    "Resource/Theme/dark/rc/branch_more_disabled.png"
    "Resource/Theme/dark/rc/branch_more_disabled@2x.png"
    "Resource/Theme/dark/rc/branch_more_focus.png"
    "Resource/Theme/dark/rc/branch_more_focus@2x.png"
    "Resource/Theme/dark/rc/branch_more_pressed.png"
    "Resource/Theme/dark/rc/branch_more_pressed@2x.png"
    "Resource/Theme/dark/rc/branch_open.png"
    "Resource/Theme/dark/rc/branch_open@2x.png"
    "Resource/Theme/dark/rc/branch_open_disabled.png"
    "Resource/Theme/dark/rc/branch_open_disabled@2x.png"
    "Resource/Theme/dark/rc/branch_open_focus.png"
    "Resource/Theme/dark/rc/branch_open_focus@2x.png"
    "Resource/Theme/dark/rc/branch_open_pressed.png"
    "Resource/Theme/dark/rc/branch_open_pressed@2x.png"
    "Resource/Theme/dark/rc/checkbox_checked.png"
    "Resource/Theme/dark/rc/checkbox_checked@2x.png"
    "Resource/Theme/dark/rc/checkbox_checked_disabled.png"
    "Resource/Theme/dark/rc/checkbox_checked_disabled@2x.png"
    "Resource/Theme/dark/rc/checkbox_checked_focus.png"
    "Resource/Theme/dark/rc/checkbox_checked_focus@2x.png"
    "Resource/Theme/dark/rc/checkbox_checked_pressed.png"
    "Resource/Theme/dark/rc/checkbox_checked_pressed@2x.png"
    "Resource/Theme/dark/rc/checkbox_indeterminate.png"
    "Resource/Theme/dark/rc/checkbox_indeterminate@2x.png"
    "Resource/Theme/dark/rc/checkbox_indeterminate_disabled.png"
    "Resource/Theme/dark/rc/checkbox_indeterminate_disabled@2x.png"
    "Resource/Theme/dark/rc/checkbox_indeterminate_focus.png"
    "Resource/Theme/dark/rc/checkbox_indeterminate_focus@2x.png"
    "Resource/Theme/dark/rc/checkbox_indeterminate_pressed.png"
    "Resource/Theme/dark/rc/checkbox_indeterminate_pressed@2x.png"
    "Resource/Theme/dark/rc/checkbox_unchecked.png"
    "Resource/Theme/dark/rc/checkbox_unchecked@2x.png"
    "Resource/Theme/dark/rc/checkbox_unchecked_disabled.png"
    "Resource/Theme/dark/rc/checkbox_unchecked_disabled@2x.png"
    "Resource/Theme/dark/rc/checkbox_unchecked_focus.png"
    "Resource/Theme/dark/rc/checkbox_unchecked_focus@2x.png"
    "Resource/Theme/dark/rc/checkbox_unchecked_pressed.png"
    "Resource/Theme/dark/rc/checkbox_unchecked_pressed@2x.png"
    "Resource/Theme/dark/rc/line_horizontal.png"
    "Resource/Theme/dark/rc/line_horizontal@2x.png"
    "Resource/Theme/dark/rc/line_horizontal_disabled.png"
    "Resource/Theme/dark/rc/line_horizontal_disabled@2x.png"
    "Resource/Theme/dark/rc/line_horizontal_focus.png"
    "Resource/Theme/dark/rc/line_horizontal_focus@2x.png"
    "Resource/Theme/dark/rc/line_horizontal_pressed.png"
    "Resource/Theme/dark/rc/line_horizontal_pressed@2x.png"
    "Resource/Theme/dark/rc/line_vertical.png"
    "Resource/Theme/dark/rc/line_vertical@2x.png"
    "Resource/Theme/dark/rc/line_vertical_disabled.png"
    "Resource/Theme/dark/rc/line_vertical_disabled@2x.png"
    "Resource/Theme/dark/rc/line_vertical_focus.png"
    "Resource/Theme/dark/rc/line_vertical_focus@2x.png"
    "Resource/Theme/dark/rc/line_vertical_pressed.png"
    "Resource/Theme/dark/rc/line_vertical_pressed@2x.png"
    "Resource/Theme/dark/rc/radio_checked.png"
    "Resource/Theme/dark/rc/radio_checked@2x.png"
    "Resource/Theme/dark/rc/radio_checked_disabled.png"
    "Resource/Theme/dark/rc/radio_checked_disabled@2x.png"
    "Resource/Theme/dark/rc/radio_checked_focus.png"
    "Resource/Theme/dark/rc/radio_checked_focus@2x.png"
    "Resource/Theme/dark/rc/radio_checked_pressed.png"
    "Resource/Theme/dark/rc/radio_checked_pressed@2x.png"
    "Resource/Theme/dark/rc/radio_unchecked.png"
    "Resource/Theme/dark/rc/radio_unchecked@2x.png"
    "Resource/Theme/dark/rc/radio_unchecked_disabled.png"
    "Resource/Theme/dark/rc/radio_unchecked_disabled@2x.png"
    "Resource/Theme/dark/rc/radio_unchecked_focus.png"
    "Resource/Theme/dark/rc/radio_unchecked_focus@2x.png"
    "Resource/Theme/dark/rc/radio_unchecked_pressed.png"
    "Resource/Theme/dark/rc/radio_unchecked_pressed@2x.png"
    "Resource/Theme/dark/rc/toolbar_move_horizontal.png"
    "Resource/Theme/dark/rc/toolbar_move_horizontal@2x.png"
    "Resource/Theme/dark/rc/toolbar_move_horizontal_disabled.png"
    "Resource/Theme/dark/rc/toolbar_move_horizontal_disabled@2x.png"
    "Resource/Theme/dark/rc/toolbar_move_horizontal_focus.png"
    "Resource/Theme/dark/rc/toolbar_move_horizontal_focus@2x.png"
    "Resource/Theme/dark/rc/toolbar_move_horizontal_pressed.png"
    "Resource/Theme/dark/rc/toolbar_move_horizontal_pressed@2x.png"
    "Resource/Theme/dark/rc/toolbar_move_vertical.png"
    "Resource/Theme/dark/rc/toolbar_move_vertical@2x.png"
    "Resource/Theme/dark/rc/toolbar_move_vertical_disabled.png"
    "Resource/Theme/dark/rc/toolbar_move_vertical_disabled@2x.png"
    "Resource/Theme/dark/rc/toolbar_move_vertical_focus.png"
    "Resource/Theme/dark/rc/toolbar_move_vertical_focus@2x.png"
    "Resource/Theme/dark/rc/toolbar_move_vertical_pressed.png"
    "Resource/Theme/dark/rc/toolbar_move_vertical_pressed@2x.png"
    "Resource/Theme/dark/rc/toolbar_separator_horizontal.png"
    "Resource/Theme/dark/rc/toolbar_separator_horizontal@2x.png"
    "Resource/Theme/dark/rc/toolbar_separator_horizontal_disabled.png"
    "Resource/Theme/dark/rc/toolbar_separator_horizontal_disabled@2x.png"
    "Resource/Theme/dark/rc/toolbar_separator_horizontal_focus.png"
    "Resource/Theme/dark/rc/toolbar_separator_horizontal_focus@2x.png"
    "Resource/Theme/dark/rc/toolbar_separator_horizontal_pressed.png"
    "Resource/Theme/dark/rc/toolbar_separator_horizontal_pressed@2x.png"
    "Resource/Theme/dark/rc/toolbar_separator_vertical.png"
    "Resource/Theme/dark/rc/toolbar_separator_vertical@2x.png"
    "Resource/Theme/dark/rc/toolbar_separator_vertical_disabled.png"
    "Resource/Theme/dark/rc/toolbar_separator_vertical_disabled@2x.png"
    "Resource/Theme/dark/rc/toolbar_separator_vertical_focus.png"
    "Resource/Theme/dark/rc/toolbar_separator_vertical_focus@2x.png"
    "Resource/Theme/dark/rc/toolbar_separator_vertical_pressed.png"
    "Resource/Theme/dark/rc/toolbar_separator_vertical_pressed@2x.png"
    "Resource/Theme/dark/rc/transparent.png"
    "Resource/Theme/dark/rc/transparent@2x.png"
    "Resource/Theme/dark/rc/transparent_disabled.png"
    "Resource/Theme/dark/rc/transparent_disabled@2x.png"
    "Resource/Theme/dark/rc/transparent_focus.png"
    "Resource/Theme/dark/rc/transparent_focus@2x.png"
    "Resource/Theme/dark/rc/transparent_pressed.png"
    "Resource/Theme/dark/rc/transparent_pressed@2x.png"
    "Resource/Theme/dark/rc/window_close.png"
    "Resource/Theme/dark/rc/window_close@2x.png"
    "Resource/Theme/dark/rc/window_close_disabled.png"
    "Resource/Theme/dark/rc/window_close_disabled@2x.png"
    "Resource/Theme/dark/rc/window_close_focus.png"
    "Resource/Theme/dark/rc/window_close_focus@2x.png"
    "Resource/Theme/dark/rc/window_close_pressed.png"
    "Resource/Theme/dark/rc/window_close_pressed@2x.png"
    "Resource/Theme/dark/rc/window_grip.png"
    "Resource/Theme/dark/rc/window_grip@2x.png"
    "Resource/Theme/dark/rc/window_grip_disabled.png"
    "Resource/Theme/dark/rc/window_grip_disabled@2x.png"
    "Resource/Theme/dark/rc/window_grip_focus.png"
    "Resource/Theme/dark/rc/window_grip_focus@2x.png"
    "Resource/Theme/dark/rc/window_grip_pressed.png"
    "Resource/Theme/dark/rc/window_grip_pressed@2x.png"
    "Resource/Theme/dark/rc/window_minimize.png"
    "Resource/Theme/dark/rc/window_minimize@2x.png"
    "Resource/Theme/dark/rc/window_minimize_disabled.png"
    "Resource/Theme/dark/rc/window_minimize_disabled@2x.png"
    "Resource/Theme/dark/rc/window_minimize_focus.png"
    "Resource/Theme/dark/rc/window_minimize_focus@2x.png"
    "Resource/Theme/dark/rc/window_minimize_pressed.png"
    "Resource/Theme/dark/rc/window_minimize_pressed@2x.png"
    "Resource/Theme/dark/rc/window_undock.png"
    "Resource/Theme/dark/rc/window_undock@2x.png"
    "Resource/Theme/dark/rc/window_undock_disabled.png"
    "Resource/Theme/dark/rc/window_undock_disabled@2x.png"
    "Resource/Theme/dark/rc/window_undock_focus.png"
    "Resource/Theme/dark/rc/window_undock_focus@2x.png"
    "Resource/Theme/dark/rc/window_undock_pressed.png"
    "Resource/Theme/dark/rc/window_undock_pressed@2x.png"
)

qt_add_resources(xamp2 "darkstyle"
    PREFIX
        "/qss_icons/dark"
    BASE
        "Resource/Theme/dark"
    FILES
        ${darkstyle_resource_files}
)
set(darkstyle1_resource_files
    "Resource/Theme/dark/style.qss"
)

qt_add_resources(xamp2 "darkstyle1"
    PREFIX
        "/qdarkstyle/dark"
    BASE
        "Resource/Theme/dark"
    FILES
        ${darkstyle1_resource_files}
)
set(lightstyle_resource_files
    "Resource/Theme/light/rc/.keep"
    "Resource/Theme/light/rc/arrow_down.png"
    "Resource/Theme/light/rc/arrow_down@2x.png"
    "Resource/Theme/light/rc/arrow_down_disabled.png"
    "Resource/Theme/light/rc/arrow_down_disabled@2x.png"
    "Resource/Theme/light/rc/arrow_down_focus.png"
    "Resource/Theme/light/rc/arrow_down_focus@2x.png"
    "Resource/Theme/light/rc/arrow_down_pressed.png"
    "Resource/Theme/light/rc/arrow_down_pressed@2x.png"
    "Resource/Theme/light/rc/arrow_left.png"
    "Resource/Theme/light/rc/arrow_left@2x.png"
    "Resource/Theme/light/rc/arrow_left_disabled.png"
    "Resource/Theme/light/rc/arrow_left_disabled@2x.png"
    "Resource/Theme/light/rc/arrow_left_focus.png"
    "Resource/Theme/light/rc/arrow_left_focus@2x.png"
    "Resource/Theme/light/rc/arrow_left_pressed.png"
    "Resource/Theme/light/rc/arrow_left_pressed@2x.png"
    "Resource/Theme/light/rc/arrow_right.png"
    "Resource/Theme/light/rc/arrow_right@2x.png"
    "Resource/Theme/light/rc/arrow_right_disabled.png"
    "Resource/Theme/light/rc/arrow_right_disabled@2x.png"
    "Resource/Theme/light/rc/arrow_right_focus.png"
    "Resource/Theme/light/rc/arrow_right_focus@2x.png"
    "Resource/Theme/light/rc/arrow_right_pressed.png"
    "Resource/Theme/light/rc/arrow_right_pressed@2x.png"
    "Resource/Theme/light/rc/arrow_up.png"
    "Resource/Theme/light/rc/arrow_up@2x.png"
    "Resource/Theme/light/rc/arrow_up_disabled.png"
    "Resource/Theme/light/rc/arrow_up_disabled@2x.png"
    "Resource/Theme/light/rc/arrow_up_focus.png"
    "Resource/Theme/light/rc/arrow_up_focus@2x.png"
    "Resource/Theme/light/rc/arrow_up_pressed.png"
    "Resource/Theme/light/rc/arrow_up_pressed@2x.png"
    "Resource/Theme/light/rc/base_icon.png"
    "Resource/Theme/light/rc/base_icon@2x.png"
    "Resource/Theme/light/rc/base_icon_disabled.png"
    "Resource/Theme/light/rc/base_icon_disabled@2x.png"
    "Resource/Theme/light/rc/base_icon_focus.png"
    "Resource/Theme/light/rc/base_icon_focus@2x.png"
    "Resource/Theme/light/rc/base_icon_pressed.png"
    "Resource/Theme/light/rc/base_icon_pressed@2x.png"
    "Resource/Theme/light/rc/branch_closed.png"
    "Resource/Theme/light/rc/branch_closed@2x.png"
    "Resource/Theme/light/rc/branch_closed_disabled.png"
    "Resource/Theme/light/rc/branch_closed_disabled@2x.png"
    "Resource/Theme/light/rc/branch_closed_focus.png"
    "Resource/Theme/light/rc/branch_closed_focus@2x.png"
    "Resource/Theme/light/rc/branch_closed_pressed.png"
    "Resource/Theme/light/rc/branch_closed_pressed@2x.png"
    "Resource/Theme/light/rc/branch_end.png"
    "Resource/Theme/light/rc/branch_end@2x.png"
    "Resource/Theme/light/rc/branch_end_disabled.png"
    "Resource/Theme/light/rc/branch_end_disabled@2x.png"
    "Resource/Theme/light/rc/branch_end_focus.png"
    "Resource/Theme/light/rc/branch_end_focus@2x.png"
    "Resource/Theme/light/rc/branch_end_pressed.png"
    "Resource/Theme/light/rc/branch_end_pressed@2x.png"
    "Resource/Theme/light/rc/branch_line.png"
    "Resource/Theme/light/rc/branch_line@2x.png"
    "Resource/Theme/light/rc/branch_line_disabled.png"
    "Resource/Theme/light/rc/branch_line_disabled@2x.png"
    "Resource/Theme/light/rc/branch_line_focus.png"
    "Resource/Theme/light/rc/branch_line_focus@2x.png"
    "Resource/Theme/light/rc/branch_line_pressed.png"
    "Resource/Theme/light/rc/branch_line_pressed@2x.png"
    "Resource/Theme/light/rc/branch_more.png"
    "Resource/Theme/light/rc/branch_more@2x.png"
    "Resource/Theme/light/rc/branch_more_disabled.png"
    "Resource/Theme/light/rc/branch_more_disabled@2x.png"
    "Resource/Theme/light/rc/branch_more_focus.png"
    "Resource/Theme/light/rc/branch_more_focus@2x.png"
    "Resource/Theme/light/rc/branch_more_pressed.png"
    "Resource/Theme/light/rc/branch_more_pressed@2x.png"
    "Resource/Theme/light/rc/branch_open.png"
    "Resource/Theme/light/rc/branch_open@2x.png"
    "Resource/Theme/light/rc/branch_open_disabled.png"
    "Resource/Theme/light/rc/branch_open_disabled@2x.png"
    "Resource/Theme/light/rc/branch_open_focus.png"
    "Resource/Theme/light/rc/branch_open_focus@2x.png"
    "Resource/Theme/light/rc/branch_open_pressed.png"
    "Resource/Theme/light/rc/branch_open_pressed@2x.png"
    "Resource/Theme/light/rc/checkbox_checked.png"
    "Resource/Theme/light/rc/checkbox_checked@2x.png"
    "Resource/Theme/light/rc/checkbox_checked_disabled.png"
    "Resource/Theme/light/rc/checkbox_checked_disabled@2x.png"
    "Resource/Theme/light/rc/checkbox_checked_focus.png"
    "Resource/Theme/light/rc/checkbox_checked_focus@2x.png"
    "Resource/Theme/light/rc/checkbox_checked_pressed.png"
    "Resource/Theme/light/rc/checkbox_checked_pressed@2x.png"
    "Resource/Theme/light/rc/checkbox_indeterminate.png"
    "Resource/Theme/light/rc/checkbox_indeterminate@2x.png"
    "Resource/Theme/light/rc/checkbox_indeterminate_disabled.png"
    "Resource/Theme/light/rc/checkbox_indeterminate_disabled@2x.png"
    "Resource/Theme/light/rc/checkbox_indeterminate_focus.png"
    "Resource/Theme/light/rc/checkbox_indeterminate_focus@2x.png"
    "Resource/Theme/light/rc/checkbox_indeterminate_pressed.png"
    "Resource/Theme/light/rc/checkbox_indeterminate_pressed@2x.png"
    "Resource/Theme/light/rc/checkbox_unchecked.png"
    "Resource/Theme/light/rc/checkbox_unchecked@2x.png"
    "Resource/Theme/light/rc/checkbox_unchecked_disabled.png"
    "Resource/Theme/light/rc/checkbox_unchecked_disabled@2x.png"
    "Resource/Theme/light/rc/checkbox_unchecked_focus.png"
    "Resource/Theme/light/rc/checkbox_unchecked_focus@2x.png"
    "Resource/Theme/light/rc/checkbox_unchecked_pressed.png"
    "Resource/Theme/light/rc/checkbox_unchecked_pressed@2x.png"
    "Resource/Theme/light/rc/line_horizontal.png"
    "Resource/Theme/light/rc/line_horizontal@2x.png"
    "Resource/Theme/light/rc/line_horizontal_disabled.png"
    "Resource/Theme/light/rc/line_horizontal_disabled@2x.png"
    "Resource/Theme/light/rc/line_horizontal_focus.png"
    "Resource/Theme/light/rc/line_horizontal_focus@2x.png"
    "Resource/Theme/light/rc/line_horizontal_pressed.png"
    "Resource/Theme/light/rc/line_horizontal_pressed@2x.png"
    "Resource/Theme/light/rc/line_vertical.png"
    "Resource/Theme/light/rc/line_vertical@2x.png"
    "Resource/Theme/light/rc/line_vertical_disabled.png"
    "Resource/Theme/light/rc/line_vertical_disabled@2x.png"
    "Resource/Theme/light/rc/line_vertical_focus.png"
    "Resource/Theme/light/rc/line_vertical_focus@2x.png"
    "Resource/Theme/light/rc/line_vertical_pressed.png"
    "Resource/Theme/light/rc/line_vertical_pressed@2x.png"
    "Resource/Theme/light/rc/radio_checked.png"
    "Resource/Theme/light/rc/radio_checked@2x.png"
    "Resource/Theme/light/rc/radio_checked_disabled.png"
    "Resource/Theme/light/rc/radio_checked_disabled@2x.png"
    "Resource/Theme/light/rc/radio_checked_focus.png"
    "Resource/Theme/light/rc/radio_checked_focus@2x.png"
    "Resource/Theme/light/rc/radio_checked_pressed.png"
    "Resource/Theme/light/rc/radio_checked_pressed@2x.png"
    "Resource/Theme/light/rc/radio_unchecked.png"
    "Resource/Theme/light/rc/radio_unchecked@2x.png"
    "Resource/Theme/light/rc/radio_unchecked_disabled.png"
    "Resource/Theme/light/rc/radio_unchecked_disabled@2x.png"
    "Resource/Theme/light/rc/radio_unchecked_focus.png"
    "Resource/Theme/light/rc/radio_unchecked_focus@2x.png"
    "Resource/Theme/light/rc/radio_unchecked_pressed.png"
    "Resource/Theme/light/rc/radio_unchecked_pressed@2x.png"
    "Resource/Theme/light/rc/toolbar_move_horizontal.png"
    "Resource/Theme/light/rc/toolbar_move_horizontal@2x.png"
    "Resource/Theme/light/rc/toolbar_move_horizontal_disabled.png"
    "Resource/Theme/light/rc/toolbar_move_horizontal_disabled@2x.png"
    "Resource/Theme/light/rc/toolbar_move_horizontal_focus.png"
    "Resource/Theme/light/rc/toolbar_move_horizontal_focus@2x.png"
    "Resource/Theme/light/rc/toolbar_move_horizontal_pressed.png"
    "Resource/Theme/light/rc/toolbar_move_horizontal_pressed@2x.png"
    "Resource/Theme/light/rc/toolbar_move_vertical.png"
    "Resource/Theme/light/rc/toolbar_move_vertical@2x.png"
    "Resource/Theme/light/rc/toolbar_move_vertical_disabled.png"
    "Resource/Theme/light/rc/toolbar_move_vertical_disabled@2x.png"
    "Resource/Theme/light/rc/toolbar_move_vertical_focus.png"
    "Resource/Theme/light/rc/toolbar_move_vertical_focus@2x.png"
    "Resource/Theme/light/rc/toolbar_move_vertical_pressed.png"
    "Resource/Theme/light/rc/toolbar_move_vertical_pressed@2x.png"
    "Resource/Theme/light/rc/toolbar_separator_horizontal.png"
    "Resource/Theme/light/rc/toolbar_separator_horizontal@2x.png"
    "Resource/Theme/light/rc/toolbar_separator_horizontal_disabled.png"
    "Resource/Theme/light/rc/toolbar_separator_horizontal_disabled@2x.png"
    "Resource/Theme/light/rc/toolbar_separator_horizontal_focus.png"
    "Resource/Theme/light/rc/toolbar_separator_horizontal_focus@2x.png"
    "Resource/Theme/light/rc/toolbar_separator_horizontal_pressed.png"
    "Resource/Theme/light/rc/toolbar_separator_horizontal_pressed@2x.png"
    "Resource/Theme/light/rc/toolbar_separator_vertical.png"
    "Resource/Theme/light/rc/toolbar_separator_vertical@2x.png"
    "Resource/Theme/light/rc/toolbar_separator_vertical_disabled.png"
    "Resource/Theme/light/rc/toolbar_separator_vertical_disabled@2x.png"
    "Resource/Theme/light/rc/toolbar_separator_vertical_focus.png"
    "Resource/Theme/light/rc/toolbar_separator_vertical_focus@2x.png"
    "Resource/Theme/light/rc/toolbar_separator_vertical_pressed.png"
    "Resource/Theme/light/rc/toolbar_separator_vertical_pressed@2x.png"
    "Resource/Theme/light/rc/transparent.png"
    "Resource/Theme/light/rc/transparent@2x.png"
    "Resource/Theme/light/rc/transparent_disabled.png"
    "Resource/Theme/light/rc/transparent_disabled@2x.png"
    "Resource/Theme/light/rc/transparent_focus.png"
    "Resource/Theme/light/rc/transparent_focus@2x.png"
    "Resource/Theme/light/rc/transparent_pressed.png"
    "Resource/Theme/light/rc/transparent_pressed@2x.png"
    "Resource/Theme/light/rc/window_close.png"
    "Resource/Theme/light/rc/window_close@2x.png"
    "Resource/Theme/light/rc/window_close_disabled.png"
    "Resource/Theme/light/rc/window_close_disabled@2x.png"
    "Resource/Theme/light/rc/window_close_focus.png"
    "Resource/Theme/light/rc/window_close_focus@2x.png"
    "Resource/Theme/light/rc/window_close_pressed.png"
    "Resource/Theme/light/rc/window_close_pressed@2x.png"
    "Resource/Theme/light/rc/window_grip.png"
    "Resource/Theme/light/rc/window_grip@2x.png"
    "Resource/Theme/light/rc/window_grip_disabled.png"
    "Resource/Theme/light/rc/window_grip_disabled@2x.png"
    "Resource/Theme/light/rc/window_grip_focus.png"
    "Resource/Theme/light/rc/window_grip_focus@2x.png"
    "Resource/Theme/light/rc/window_grip_pressed.png"
    "Resource/Theme/light/rc/window_grip_pressed@2x.png"
    "Resource/Theme/light/rc/window_minimize.png"
    "Resource/Theme/light/rc/window_minimize@2x.png"
    "Resource/Theme/light/rc/window_minimize_disabled.png"
    "Resource/Theme/light/rc/window_minimize_disabled@2x.png"
    "Resource/Theme/light/rc/window_minimize_focus.png"
    "Resource/Theme/light/rc/window_minimize_focus@2x.png"
    "Resource/Theme/light/rc/window_minimize_pressed.png"
    "Resource/Theme/light/rc/window_minimize_pressed@2x.png"
    "Resource/Theme/light/rc/window_undock.png"
    "Resource/Theme/light/rc/window_undock@2x.png"
    "Resource/Theme/light/rc/window_undock_disabled.png"
    "Resource/Theme/light/rc/window_undock_disabled@2x.png"
    "Resource/Theme/light/rc/window_undock_focus.png"
    "Resource/Theme/light/rc/window_undock_focus@2x.png"
    "Resource/Theme/light/rc/window_undock_pressed.png"
    "Resource/Theme/light/rc/window_undock_pressed@2x.png"
)

qt_add_resources(xamp2 "lightstyle"
    PREFIX
        "/qss_icons/light"
    BASE
        "Resource/Theme/light"
    FILES
        ${lightstyle_resource_files}
)
set(lightstyle1_resource_files
    "Resource/Theme/light/style.qss"
)

qt_add_resources(xamp2 "lightstyle1"
    PREFIX
        "/qdarkstyle/light"
    BASE
        "Resource/Theme/light"
    FILES
        ${lightstyle1_resource_files}
)

if((QT_VERSION_MAJOR GREATER 4))
    target_link_libraries(xamp2 PRIVATE
        Qt::Widgets
    )
endif()

if(CMAKE_BUILD_TYPE STREQUAL Debug)
    target_compile_definitions(xamp2 PRIVATE
        _DEBUG
    )

    target_link_libraries(xamp2 PRIVATE
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/QSimpleUpdater/build/Qt_6_7_2_for_macOS-Debug/
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/framelesshelper/build/Qt_6_7_2_for_macOS-Debug/lib/
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../thirdparty/qcoro/build/Qt_6_7_2_for_macOS-Debug/qcoro/core/
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../widget_shared/build/Qt_6_7_2_for_macOS-Debug/
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../xamp_base/debug/
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../xamp_metadata/debug/
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../xamp_output_device/debug/
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../xamp_player/debug/
        # Remove: L${CMAKE_CURRENT_SOURCE_DIR}/../xamp_stream/debug/
        # Remove: L/opt/anaconda3/lib/
        FramelessHelperCore64_debug
        FramelessHelperWidgets64_debug
        QCoro6Core_debug
        QSimpleUpdater
        python3.12
        widget_shared
        xamp_base
        xamp_metadata
        xamp_output_device
        xamp_player
        xamp_stream
    )
endif()

install(TARGETS xamp2
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

qt_generate_deploy_app_script(
    TARGET xamp2
    FILENAME_VARIABLE deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
)
install(SCRIPT ${deploy_script})
