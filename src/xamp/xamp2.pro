QT       += core gui sql network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20
DEFINES += QT_NO_CAST_FROM_ASCII

QMAKE_MACOSX_DEPLOYMENT_TARGET = 10.15

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
    singleinstanceapplication.cpp \
    main.cpp \    
    xamp.cpp

HEADERS += \
    singleinstanceapplication.h \
    xamp.h

ICON = xamp2.icns \

FORMS += \
    xamp.ui

RESOURCES += \
    xamp.qrc

TRANSLATIONS += \
    zh_TW.ts \
    ja_JP.ts \
    en_US.ts \

# Additionally include Cocoa for OS X code

LIBS += -framework Foundation -framework Cocoa -framework Carbon

INCLUDEPATH += /System/Library/Frameworks/Foundation.framework/Versions/C/Headers \
    ../thirdparty/qcoro/qcoro/ \
    ../thirdparty/qcoro/qcoro/core/ \
    ../thirdparty/qcoro/clang/qcoro/core/QCoro/ \
    ../thirdparty/qcoro/clang/qcoro/core/ \
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
    ../thirdparty/framelesshelper/include/ \
    ../thirdparty/QSimpleUpdater/include/ \
    /Library/Developer/CommandLineTools/Library/Frameworks/Python3.framework/Versions/3.7/Headers/

CONFIG(debug, debug|release) {
QMAKE_CXXFLAGS += -mavx2 -D_DEBUG -fcoroutines-ts
LIBS += -L"../xamp_base/debug/" -lxamp_base \
    -L"../xamp_metadata/debug/" -lxamp_metadata \
    -L"../xamp_output_device/debug/" -lxamp_output_device \
    -L"../xamp_stream/debug/" -lxamp_stream \
    -L"../xamp_player/debug/" -lxamp_player \
    -L"../build-widget_shared-Qt_6_6_1_for_macOS-Debug/" -lwidget_shared.1.0.0 \
    -L"../thirdparty/build-QSimpleUpdater-Qt_6_6_1_for_macOS-Debug" -lQSimpleUpdater.1.0.0 \
    -L"../thirdparty/build-qcoro-Qt_6_6_1_for_macOS-Debug/qcoro/core/" -lQCoro6Core_debug \
    -L"../thirdparty/build-framelesshelper-Qt_6_6_1_for_macOS-Debug/lib/" -lFramelessHelperCore_debug.2.3.6 \
    -L"../thirdparty/build-framelesshelper-Qt_6_6_1_for_macOS-Debug/lib/" -lFramelessHelperWidgets_debug.2.3.6
} else {
QMAKE_CXXFLAGS += -DNDEBUG -O3 -ftree-vectorize -march=native -mtune=native -mavx2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer -fcoroutines-ts
LIBS += -L"../xamp_base/release/" -lxamp_base \
    -L"../xamp_metadata/release/" -lxamp_metadata \
    -L"../xamp_output_device/release/" -lxamp_output_device \
    -L"../xamp_stream/release/" -lxamp_stream \
    -L"../xamp_player/release/" -lxamp_player \
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
