QT       += core gui sql

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17

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

INCLUDEPATH += /widget
INCLUDEPATH += ../xamp_base/include/
INCLUDEPATH += ../xamp_metadata/include/
INCLUDEPATH += ../xamp_output_device/include/
INCLUDEPATH += ../xamp_stream/include/
INCLUDEPATH += ../xamp_player/include/
INCLUDEPATH += ../thirdparty/spdlog/include/

SOURCES += \
    widget/albumview.cpp \
    widget/appsettings.cpp \
    widget/clickablelabel.cpp \
    widget/database.cpp \
    widget/filetag.cpp \
    widget/framelesswindow.cpp \
    widget/lrcpage.cpp \
    widget/lrcparser.cpp \
    widget/lyricsshowwideget.cpp \
    widget/metadataextractadapter.cpp \
    widget/pixmapcache.cpp \
    widget/playerstateadapter.cpp \
    widget/playlistpage.cpp \
    widget/playlisttablemodel.cpp \
    widget/playlisttableproxymodel.cpp \
    widget/playlisttableview.cpp \
    widget/toast.cpp \
    widget/wheelablewidget.cpp \
    main.cpp \
    xamp.cpp

HEADERS += \
    widget/actionmap.h \
    widget/albumview.h \
    widget/appsettings.h \
    widget/clickablelabel.h \
    widget/database.h \
    widget/filetag.h \
    widget/framelesswindow.h \
    widget/image_utiltis.h \
    widget/lrcpage.h \
    widget/lrcparser.h \
    widget/lyricsshowwideget.h \
    widget/metadataextractadapter.h \
    widget/pixmapcache.h \
    widget/playerstateadapter.h \
    widget/playlistpage.h \
    widget/playlisttablemodel.h \
    widget/playlisttableproxymodel.h \
    widget/playlisttableview.h \
    widget/qdebugsink.h \
    widget/toast.h \
    widget/wheelablewidget.h \
    widget/str_utilts.h \
    widget/time_utilts.h \
    xamp.h

FORMS += \
    xamp.ui \
    widget/toast.ui

RESOURCES += \
    xamp.qrc \
    widget/toast.qrc

CONFIG(debug, debug|release) {
LIBS += -L"../thirdparty/ffmpeg/lib/x64/mac/" -lavcodec.58 \

LIBS += -L"../xamp_base/debug/" -lxamp_base \
    -L"../xamp_metadata/debug/" -lxamp_metadata \
    -L"../xamp_output_device/debug/" -lxamp_output_device \
    -L"../xamp_stream/debug/" -lxamp_stream \
    -L"../xamp_player/debug/" -lxamp_player \
} else {
LIBS += -L"../thirdparty/ffmpeg/lib/x64/mac/" -lavcodec.58 \

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


#macx {
#    QMAKE_RPATHDIR += @executable_path/../Frameworks
#    QMAKE_LFLAGS_SONAME = -Wl,-install_name,@rpath/
#}
