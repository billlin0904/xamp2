cmake_minimum_required(VERSION 3.24)

project(xamp_app LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC OFF)
set(CMAKE_AUTORCC ON)

find_package(Qt6 6.4 REQUIRED COMPONENTS
    Core
    Gui
    Widgets
    Network
    Sql
    Multimedia
    Core5Compat
)

qt6_wrap_ui(XAMP_UI_HEADERS xamp.ui)

if(UNIX AND NOT APPLE)
    set(QSIMPLEUPDATER_DIR "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/QSimpleUpdater")
    qt6_wrap_ui(QSIMPLEUPDATER_UI_HEADERS "${QSIMPLEUPDATER_DIR}/src/Downloader.ui")

    add_library(qsimpleupdater STATIC
        "${QSIMPLEUPDATER_DIR}/src/Updater.cpp"
        "${QSIMPLEUPDATER_DIR}/src/Updater.h"
        "${QSIMPLEUPDATER_DIR}/src/Downloader.cpp"
        "${QSIMPLEUPDATER_DIR}/src/Downloader.h"
        "${QSIMPLEUPDATER_DIR}/src/QSimpleUpdater.cpp"
        "${QSIMPLEUPDATER_DIR}/include/QSimpleUpdater.h"
        "${QSIMPLEUPDATER_DIR}/etc/resources/qsimpleupdater.qrc"
        ${QSIMPLEUPDATER_UI_HEADERS}
    )

    set_target_properties(qsimpleupdater PROPERTIES
        AUTOMOC ON
        AUTORCC ON
    )

    target_compile_definitions(qsimpleupdater PRIVATE QSU_INCLUDE_MOC=0)

    target_include_directories(qsimpleupdater
        PUBLIC
            "${QSIMPLEUPDATER_DIR}/include"
        PRIVATE
            "${QSIMPLEUPDATER_DIR}"
            "${QSIMPLEUPDATER_DIR}/src"
            "${CMAKE_CURRENT_BINARY_DIR}"
    )

    target_link_libraries(qsimpleupdater
        PUBLIC
            Qt6::Core
            Qt6::Gui
            Qt6::Network
            Qt6::Widgets
    )
endif()

add_executable(xamp
    playbackcontroller.cpp
    playbackcontroller.h
    playbackpresenter.cpp
    playbackpresenter.h
    playbackqueue.h
    applicationupdater.cpp
    applicationupdater.h
    applicationservices.cpp
    applicationservices.h
    deviceselectormenu.cpp
    deviceselectormenu.h
    main.cpp
    sharedmodeplayback.cpp
    sharedmodeplayback.h
    style_util.cpp
    style_util.h
    xapplication.cpp
    xapplication.h
    xamp.cpp
    xamp.h
    ${XAMP_UI_HEADERS}
    xamp.qrc
    Resource/Theme/dark/darkstyle.qrc
    Resource/Theme/light/lightstyle.qrc
)

target_compile_features(xamp PRIVATE cxx_std_23)

target_compile_definitions(xamp
    PRIVATE
        QT_NO_CAST_FROM_ASCII
        QT_DEPRECATED_WARNINGS
)

target_include_directories(xamp
    PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}"
        "${CMAKE_CURRENT_BINARY_DIR}"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_base/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_metadata/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_output_device/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_player/include"
        "${CMAKE_CURRENT_LIST_DIR}/../xamp_stream/include"
        "${CMAKE_CURRENT_LIST_DIR}/../widget_shared"
        "${CMAKE_CURRENT_LIST_DIR}/../widget_shared/widget"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/spdlog/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/rapidjson/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/rapidxml"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/unordered_dense/include"
        "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/glaze/include"
)

set_target_properties(xamp PROPERTIES
    AUTOUIC_SEARCH_PATHS "${CMAKE_CURRENT_LIST_DIR}"
    VS_DEBUGGER_WORKING_DIRECTORY "$<TARGET_FILE_DIR:xamp>"
)

add_custom_command(TARGET xamp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/config.json"
        "$<TARGET_FILE_DIR:xamp>/config.json"
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
        "${CMAKE_CURRENT_LIST_DIR}/fonticon.json"
        "$<TARGET_FILE_DIR:xamp>/fonticon.json"
)

foreach(XAMP_RUNTIME_DATA_DIR fonts migrations opencc eqpresets)
    if(EXISTS "${CMAKE_CURRENT_LIST_DIR}/${XAMP_RUNTIME_DATA_DIR}")
        add_custom_command(TARGET xamp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${CMAKE_CURRENT_LIST_DIR}/${XAMP_RUNTIME_DATA_DIR}"
                "$<TARGET_FILE_DIR:xamp>/${XAMP_RUNTIME_DATA_DIR}"
        )
    endif()
endforeach()

if(UNIX AND NOT APPLE)
    set(XAMP_PRELOAD_COMPONENT_LIB_NAMES
        avcodec
        avformat
        avutil
        cue
        mecab
        opencc
        samplerate
        soxr
        swresample
        swscale
        uchardet
    )

    set(XAMP_BASS_LINUX_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/bass/bin/linux/x86_64")
    if(EXISTS "${XAMP_BASS_LINUX_LIB_DIR}")
        add_custom_command(TARGET xamp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:xamp>/components"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${XAMP_BASS_LINUX_LIB_DIR}"
                "$<TARGET_FILE_DIR:xamp>/components"
        )
    endif()

    set(XAMP_FLAC_LINUX_LIB_DIR "${CMAKE_CURRENT_LIST_DIR}/../thirdparty/libflac/bin/linux/x86_64")
    if(EXISTS "${XAMP_FLAC_LINUX_LIB_DIR}")
        add_custom_command(TARGET xamp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:xamp>/components"
            COMMAND ${CMAKE_COMMAND} -E copy_directory
                "${XAMP_FLAC_LINUX_LIB_DIR}"
                "$<TARGET_FILE_DIR:xamp>/components"
        )
    endif()

    if(DEFINED VCPKG_TARGET_TRIPLET AND VCPKG_TARGET_TRIPLET MATCHES "dynamic")
        set(XAMP_VCPKG_COMPONENT_LIB_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib")
        if(CMAKE_BUILD_TYPE STREQUAL "Debug" AND EXISTS "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/lib")
            set(XAMP_VCPKG_COMPONENT_LIB_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/debug/lib")
        endif()

        file(GLOB XAMP_VCPKG_COMPONENT_LIBS CONFIGURE_DEPENDS
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libavcodec.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libavformat.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libavutil.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libcue.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libmecab.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libopencc.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libsamplerate.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libsoxr.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libswresample.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libswscale.so*"
            "${XAMP_VCPKG_COMPONENT_LIB_DIR}/libuchardet.so*"
        )

        if(XAMP_VCPKG_COMPONENT_LIBS)
            add_custom_command(TARGET xamp POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:xamp>/components"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${XAMP_VCPKG_COMPONENT_LIBS}
                    "$<TARGET_FILE_DIR:xamp>/components"
            )
        endif()
    endif()

    if(DEFINED VCPKG_TARGET_TRIPLET)
        set(XAMP_VCPKG_MKL_LIB_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/${VCPKG_TARGET_TRIPLET}/lib/intel64")
    else()
        set(XAMP_VCPKG_MKL_LIB_DIR "${CMAKE_BINARY_DIR}/vcpkg_installed/x64-linux/lib/intel64")
    endif()

    file(GLOB XAMP_VCPKG_MKL_RUNTIME_LIBS CONFIGURE_DEPENDS
        "${XAMP_VCPKG_MKL_LIB_DIR}/libiomp*.so*"
        "${XAMP_VCPKG_MKL_LIB_DIR}/libmkl*.so*"
    )

    if(XAMP_VCPKG_MKL_RUNTIME_LIBS)
        add_custom_command(TARGET xamp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:xamp>/components"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                ${XAMP_VCPKG_MKL_RUNTIME_LIBS}
                "$<TARGET_FILE_DIR:xamp>/components"
        )
    endif()

    foreach(XAMP_PRELOAD_COMPONENT_LIB_NAME IN LISTS XAMP_PRELOAD_COMPONENT_LIB_NAMES)
        set(XAMP_SYSTEM_COMPONENT_LIBS "")
        find_library(XAMP_SYSTEM_${XAMP_PRELOAD_COMPONENT_LIB_NAME}_LIBRARY
            NAMES ${XAMP_PRELOAD_COMPONENT_LIB_NAME})
        if(XAMP_SYSTEM_${XAMP_PRELOAD_COMPONENT_LIB_NAME}_LIBRARY)
            get_filename_component(XAMP_SYSTEM_COMPONENT_LIB_DIR
                "${XAMP_SYSTEM_${XAMP_PRELOAD_COMPONENT_LIB_NAME}_LIBRARY}"
                DIRECTORY)
            file(GLOB XAMP_SYSTEM_COMPONENT_LIBS CONFIGURE_DEPENDS
                "${XAMP_SYSTEM_COMPONENT_LIB_DIR}/lib${XAMP_PRELOAD_COMPONENT_LIB_NAME}.so*"
            )
        endif()

        if(XAMP_SYSTEM_COMPONENT_LIBS)
            add_custom_command(TARGET xamp POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:xamp>/components"
                COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${XAMP_SYSTEM_COMPONENT_LIBS}
                    "$<TARGET_FILE_DIR:xamp>/components"
            )
        endif()
    endforeach()

    find_file(XAMP_SYSTEM_MECABRC
        NAMES mecabrc
        PATHS /etc
        NO_DEFAULT_PATH)
    if(XAMP_SYSTEM_MECABRC)
        add_custom_command(TARGET xamp POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:xamp>/mecab"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                "${XAMP_SYSTEM_MECABRC}"
                "$<TARGET_FILE_DIR:xamp>/mecab/mecabrc"
        )

        file(STRINGS "${XAMP_SYSTEM_MECABRC}" XAMP_MECAB_DICDIR_LINE
            REGEX "^[ \t]*dicdir[ \t]*=")
        if(XAMP_MECAB_DICDIR_LINE)
            list(GET XAMP_MECAB_DICDIR_LINE 0 XAMP_MECAB_DICDIR_LINE)
            string(REGEX REPLACE "^[ \t]*dicdir[ \t]*=[ \t]*([^ \t\r\n]+).*$" "\\1"
                XAMP_SYSTEM_MECAB_DIC_DIR
                "${XAMP_MECAB_DICDIR_LINE}")
            if(EXISTS "${XAMP_SYSTEM_MECAB_DIC_DIR}")
                add_custom_command(TARGET xamp POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                        "${XAMP_SYSTEM_MECAB_DIC_DIR}"
                        "$<TARGET_FILE_DIR:xamp>/mecab/dic"
                )
            endif()
        endif()
    endif()
endif()

target_link_libraries(xamp
    PRIVATE
        Qt6::Core
        Qt6::Gui
        Qt6::Widgets
        Qt6::Network
        Qt6::Sql
        Qt6::Multimedia
        Qt6::Core5Compat
        xamp_base
        xamp_metadata
        xamp_output_device
        xamp_player
        xamp_stream
        widget_shared
)

if(UNIX AND NOT APPLE)
    target_link_libraries(xamp PRIVATE dl pthread qsimpleupdater)
endif()

if(UNIX AND NOT APPLE AND CMAKE_BUILD_TYPE STREQUAL "Release")
    add_custom_target(deploy_linux_zip ALL
        COMMAND ${CMAKE_COMMAND} -E env
            XAMP_DEPLOY_SKIP_BUILD=1
            XAMP_DEPLOY_BUILD_DIR=${CMAKE_BINARY_DIR}
            bash "${CMAKE_SOURCE_DIR}/tools/deploy_linux.sh" "wsl-linux-release" "zip"
        DEPENDS xamp
        COMMENT "Deploying Linux Release zip package"
        VERBATIM
    )
endif()

find_package(Qt6 REQUIRED COMPONENTS LinguistTools)
add_custom_command(TARGET xamp POST_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:xamp>/langs"
    COMMAND Qt6::lrelease "${CMAKE_CURRENT_SOURCE_DIR}/zh_TW.ts" -qm "$<TARGET_FILE_DIR:xamp>/langs/zh_TW.qm"
    COMMAND Qt6::lrelease "${CMAKE_CURRENT_SOURCE_DIR}/../widget_shared/widget_shared_zh_TW.ts" -qm "$<TARGET_FILE_DIR:xamp>/langs/widget_shared_zh_TW.qm"
    COMMAND Qt6::lrelease "${CMAKE_CURRENT_SOURCE_DIR}/qt_zh_TW.ts" -qm "$<TARGET_FILE_DIR:xamp>/langs/qt_zh_TW.qm"
    VERBATIM)

# Qt's standard widget/dialog translations accompany the application catalogs.
get_target_property(XAMP_QMAKE_EXECUTABLE Qt6::qmake IMPORTED_LOCATION)
execute_process(COMMAND "${XAMP_QMAKE_EXECUTABLE}" -query QT_INSTALL_TRANSLATIONS
    OUTPUT_VARIABLE XAMP_QT_TRANSLATIONS_DIR OUTPUT_STRIP_TRAILING_WHITESPACE
    COMMAND_ERROR_IS_FATAL ANY)
foreach(locale IN ITEMS ja_JP ko_KR)
    string(SUBSTRING "${locale}" 0 2 qt_language)
    add_custom_command(TARGET xamp POST_BUILD
        COMMAND Qt6::lrelease "${CMAKE_CURRENT_SOURCE_DIR}/${locale}.ts" -qm "$<TARGET_FILE_DIR:xamp>/langs/${locale}.qm"
        COMMAND Qt6::lrelease "${CMAKE_CURRENT_SOURCE_DIR}/../widget_shared/widget_shared_${locale}.ts" -qm "$<TARGET_FILE_DIR:xamp>/langs/widget_shared_${locale}.qm"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "${XAMP_QT_TRANSLATIONS_DIR}/qtbase_${qt_language}.qm" "$<TARGET_FILE_DIR:xamp>/langs/qt_${locale}.qm"
        VERBATIM)
endforeach()

option(XAMP_BUILD_ARCHITECTURE_TESTS "Build playback queue/state regression tests" OFF)
if(XAMP_BUILD_ARCHITECTURE_TESTS)
    enable_testing()
    add_executable(playbackarchitecture_test tests/playbackarchitecture_test.cpp)
    target_compile_features(playbackarchitecture_test PRIVATE cxx_std_23)
    target_include_directories(playbackarchitecture_test PRIVATE $<TARGET_PROPERTY:xamp,INCLUDE_DIRECTORIES>)
    target_link_libraries(playbackarchitecture_test PRIVATE widget_shared Qt6::Widgets)
    add_test(NAME playbackarchitecture COMMAND playbackarchitecture_test -platform offscreen)
endif()

if(XAMP_BUILD_ARCHITECTURE_TESTS)
    add_executable(applicationupdater_test tests/applicationupdater_test.cpp applicationupdater.cpp applicationupdater.h)
    set_target_properties(applicationupdater_test PROPERTIES AUTOMOC ON)
    target_compile_features(applicationupdater_test PRIVATE cxx_std_23)
    target_include_directories(applicationupdater_test PRIVATE $<TARGET_PROPERTY:xamp,INCLUDE_DIRECTORIES>)
    target_link_libraries(applicationupdater_test PRIVATE widget_shared xamp_base Qt6::Widgets Qt6::Network)
    add_test(NAME applicationupdater COMMAND applicationupdater_test -platform offscreen)
endif()

if(XAMP_BUILD_ARCHITECTURE_TESTS)
    add_executable(playlistdeletion_test tests/playlistdeletion_test.cpp)
    target_compile_features(playlistdeletion_test PRIVATE cxx_std_23)
    target_include_directories(playlistdeletion_test PRIVATE $<TARGET_PROPERTY:xamp,INCLUDE_DIRECTORIES>)
    target_link_libraries(playlistdeletion_test PRIVATE widget_shared Qt6::Sql)
    add_test(NAME playlistdeletion COMMAND playlistdeletion_test)
endif()
