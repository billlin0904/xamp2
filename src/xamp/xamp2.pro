QT       += core gui sql network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20

DEFINES += QT_NO_CAST_FROM_ASCII

QMAKE_MACOSX_DEPLOYMENT_TARGET = 13.0

VERSION_MAJOR = 1
VERSION_MINOR = 0
VERSION_PATCH = 0

VERSION = $${VERSION_MAJOR}.$${VERSION_MINOR}.$${VERSION_PATCH}

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

SOURCES += \
    style_util.cpp \
    xapplication.cpp \
    main.cpp \
    xamp.cpp

HEADERS += \
    style_util.h \
    xapplication.h \
    xamp.h

ICON = xamp2.icns \

FORMS += \
    xamp.ui

RESOURCES += \
    xamp.qrc \
    ./Resource/Theme/dark/darkstyle.qrc \
    ./Resource/Theme/light/lightstyle.qrc

TRANSLATIONS += \
    zh_TW.ts \
    ja_JP.ts \
    en_US.ts \

# Additionally include Cocoa for OS X code

LIBS += -framework Foundation -framework Cocoa -framework Carbon

INCLUDEPATH += /System/Library/Frameworks/Foundation.framework/Versions/C/Headers \
    ../thirdparty/qcoro/qcoro/ \
    ../thirdparty/qcoro/qcoro/core/ \
    ../thirdparty/qcoro/build/Qt_6_7_2_for_macOS-Debug/qcoro/ \
    ../thirdparty/qcoro/build/Qt_6_7_2_for_macOS-Debug/qcoro/core/ \
    ../thirdparty/qcoro/qcoro/core/ \
    ../widget_shared/ \
    ../xamp_base/include \
    ../xamp_metadata/include \
    ../xamp_output_device/include \
    ../xamp_stream/include \
    ../xamp_player/include \
    ../thirdparty/spdlog/include \
    ../thirdparty/rapidxml/ \
    ../thirdparty/zlib/ \
    ../thirdparty/pybind11/include \
    ../thirdparty/QSimpleUpdater/include/ \
    /opt/anaconda3/include/python3.12/ \

mac {
    #QMAKE_INFO_PLIST += $$PWD/Info.plist
    #QMAKE_LFLAGS_SONAME  = -Wl,-install_name,@executable_path/../Frameworks/
}

CONFIG(debug, debug|release) {
CONFIG += debug
QMAKE_CXXFLAGS += -D_DEBUG
LIBS += \
    -L$$PWD/../xamp_base/debug/ -lxamp_base \
    -L$$PWD/../xamp_metadata/debug/ -lxamp_metadata \
    -L$$PWD/../xamp_output_device/debug/ -lxamp_output_device \
    -L$$PWD/../xamp_stream/debug/ -lxamp_stream \
    -L$$PWD/../xamp_player/debug/ -lxamp_player \
    -L$$PWD/../widget_shared/build/Qt_6_7_2_for_macOS-Debug/ -lwidget_shared \
    -L$$PWD/../thirdparty/QSimpleUpdater/build/Qt_6_7_2_for_macOS-Debug/ -lQSimpleUpdater \
    -L$$PWD/../thirdparty/qwindowkit/build/Qt_6_7_2_for_macOS-Debug/out-x86_64-Debug/lib/ -lQWKCore \
    -L$$PWD/../thirdparty/qwindowkit/build/Qt_6_7_2_for_macOS-Debug/out-x86_64-Debug/lib/ -lQWKWidgets \
    -L$$PWD/../thirdparty/qcoro/build/Qt_6_7_2_for_macOS-Debug/qcoro/core/ -lQCoro6Core_debug \
    -L/opt/anaconda3/lib/ -lpython3.12 \
} else {

}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target

DISTFILES +=
