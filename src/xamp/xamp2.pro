QT       += core gui sql network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++17
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

INCLUDEPATH += /widget
INCLUDEPATH += ../xamp_base/include/
INCLUDEPATH += ../xamp_metadata/include/
INCLUDEPATH += ../xamp_output_device/include/
INCLUDEPATH += ../xamp_stream/include/
INCLUDEPATH += ../xamp_player/include/
INCLUDEPATH += ../thirdparty/spdlog/include/
INCLUDEPATH += ../thirdparty/rapidxml/
INCLUDEPATH += ../thirdparty/rapidjson/include/
INCLUDEPATH += ../thirdparty/robin_hood/

OBJECTIVE_SOURCES += \
    widget/osx/osx.mm

SOURCES += \
    widget/lyricsshowwidget.cpp \
    widget/playbackhistorypage.cpp \
    widget/musicbrainzclient.cpp \
    widget/discogsclient.cpp \
    widget/artistview.cpp \
    widget/http.cpp \
    widget/playlistsqlquerytablemodel.cpp \
    widget/seekslider.cpp \
    widget/albumartistpage.cpp \
    widget/albumview.cpp \
    widget/appsettings.cpp \
    widget/clickablelabel.cpp \
    widget/database.cpp \
    widget/filetag.cpp \
    widget/framelesswindow.cpp \
    widget/lrcpage.cpp \
    widget/lrcparser.cpp \
    widget/metadataextractadapter.cpp \
    widget/pixmapcache.cpp \
    widget/playlistpage.cpp \
    widget/playlisttableproxymodel.cpp \
    widget/playlisttableview.cpp \
    widget/tablistview.cpp \
    widget/toast.cpp \
    widget/uiplayerstateadapter.cpp \
    widget/vinylwidget.cpp \
    widget/wheelablewidget.cpp \
    widget/selectcolorwidget.cpp \
    widget/artistinfopage.cpp \
    widget/starrating.cpp \
    widget/stareditor.cpp \
    widget/stardelegate.cpp \
    widget/localelanguage.cpp \
    widget/jsonsettings.cpp \
    widget/scrolllabel.cpp \
    widget/xampdialog.cpp \
    widget/image_utiltis.cpp \
    widget/spectrograph.cpp \
    widget/read_helper.cpp \
    widget/win32/directorywatcher.cpp \
    widget/readlufsworker.cpp \
    widget/musixmatchclient.cpp \
    DarkStyle.cpp \
    singleinstanceapplication.cpp \
    thememanager.cpp \
    aboutdialog.cpp \
    preferencedialog.cpp \
    main.cpp \
    xamp.cpp

HEADERS += \
    widget/lyricsshowwidget.h \
    widget/osx/osx.h \
    widget/playbackhistorypage.h \
    widget/musicbrainzclient.h \
    widget/discogsclient.h \
    widget/artistview.h \
    widget/http.h \
    widget/playerorder.h \
    widget/playlistsqlquerytablemodel.h \
    widget/seekslider.h \
    widget/actionmap.h \
    widget/albumview.h \
    widget/appsettings.h \
    widget/clickablelabel.h \
    widget/database.h \
    widget/filetag.h \
    widget/albumartistpage.h \
    widget/framelesswindow.h \
    widget/image_utiltis.h \
    widget/lrcpage.h \
    widget/lrcparser.h \
    widget/metadataextractadapter.h \
    widget/pixmapcache.h \
    widget/playlistpage.h \
    widget/playlisttablemodel.h \
    widget/playlisttableproxymodel.h \
    widget/playlisttableview.h \
    widget/qdebugsink.h \
    widget/settingnames.h \
    widget/tablistview.h \
    widget/toast.h \
    widget/uiplayerstateadapter.h \
    widget/vinylwidget.h \
    widget/wheelablewidget.h \
    widget/str_utilts.h \
    widget/time_utilts.h \
    widget/selectcolorwidget.h \
    widget/artistinfopage.h \
    widget/starrating.h \
    widget/stareditor.h \
    widget/stardelegate.h \
    widget/localelanguage.h \
    widget/jsonsettings.h \
    widget/scrolllabel.h \
    widget/spectrograph.h \
    widget/xampdialog.h \
    widget/read_helper.h \
    widget/directorywatcher.h \
    widget/readlufsworker.h \
    widget/musixmatchclient.h \
    singleinstanceapplication.h \
    DarkStyle.h \
    thememanager.h \
    aboutdialog.h \
    preferencedialog.h \
    xamp.h

FORMS += \
    widget/toast.ui \
    widget/compressordialog.ui \
    aboutdialog.ui \
    preferencedialog.ui \
    xamp.ui \

RESOURCES += \
    xamp.qrc \
    darkstyle.qrc \
    widget/toast.qrc

TRANSLATIONS += \
    zh_TW.ts \
    ja_JP.ts \

# Additionally include Cocoa for OS X code

LIBS += -framework Foundation -framework Cocoa
INCLUDEPATH += /System/Library/Frameworks/Foundation.framework/Versions/C/Headers

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

QMAKE_CFLAGS_LTCG = -flto=thin -fwhole-program-vtables
QMAKE_CFLAGS_RELEASE += -O3
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
