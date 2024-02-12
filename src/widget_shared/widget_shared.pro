TEMPLATE = lib

QT       += core gui sql network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20
# DEFINES += QT_NO_CAST_FROM_ASCII

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
    thememanager.cpp \
    version.cpp \
    widget/aboutpage.cpp \
    widget/accountauthorizationpage.cpp \
    widget/acrylic/acrylic.cpp \
    widget/albumartistpage.cpp \
    widget/albumview.cpp \
    widget/appsettingnames.cpp \
    widget/appsettings.cpp \
    widget/artistinfopage.cpp \
    widget/artistview.cpp \
    widget/cdpage.cpp \
    widget/clickablelabel.cpp \
    widget/createplaylistview.cpp \
    widget/database.cpp \
    widget/databasefacade.cpp \
    widget/doubleslider.cpp \
    widget/equalizerview.cpp \
    widget/file_system_watcher.cpp \
    widget/filesystemmodel.cpp \
    widget/filesystemviewpage.cpp \
    widget/flowlayout.cpp \
    widget/fonticon.cpp \
    widget/genre_view.cpp \
    widget/genre_view_page.cpp \
    widget/globalshortcut.cpp \
    widget/http.cpp \
    widget/imagecache.cpp \
    widget/jsonsettings.cpp \
    widget/localelanguage.cpp \
    widget/lrcpage.cpp \
    widget/lrcparser.cpp \
    widget/lyricsshowwidget.cpp \
    widget/maskwidget.cpp \
    widget/networkdiskcache.cpp \
    widget/playlistentity.cpp \
    widget/playlistpage.cpp \
    widget/playlistsqlquerytablemodel.cpp \
    widget/playlisttabbar.cpp \
    widget/playlisttableproxymodel.cpp \
    widget/playlisttableview.cpp \
    widget/playlisttabwidget.cpp \
    widget/preferencepage.cpp \
    widget/processindicator.cpp \
    widget/qetag.cpp \
    widget/scrolllabel.cpp \
    widget/seekslider.cpp \
    widget/spectrumwidget.cpp \
    widget/supereqview.cpp \
    widget/tablistview.cpp \
    widget/tageditpage.cpp \
    widget/tagio.cpp \
    widget/taglistview.cpp \
    widget/uiplayerstateadapter.cpp \
    widget/util/image_utiltis.cpp \
    widget/util/log_util.cpp \
    widget/util/mbdiscid_uiltis.cpp \
    widget/util/read_until.cpp \
    widget/util/str_utilts.cpp \
    widget/util/ui_utilts.cpp \
    widget/util/zib_utiltis.cpp \
    widget/volumebutton.cpp \
    widget/volumecontroldialog.cpp \
    widget/wheelablewidget.cpp \
    widget/widget_shared_global.cpp \
    widget/worker/backgroundworker.cpp \
    widget/worker/filesystemworker.cpp \
    widget/worker/findalbumcoverworker.cpp \
    widget/xdialog.cpp \
    widget/xmainwindow.cpp \
    widget/xmessage.cpp \
    widget/xmessagebox.cpp \
    widget/xprogressdialog.cpp \
    widget/youtubedl/ytmusic.cpp \
    widget/youtubedl/ytmusicoauth.cpp


HEADERS += \
    thememanager.h \
    version.h \
    widget/aboutpage.h \
    widget/accountauthorizationpage.h \
    widget/acrylic/acrylic.h \
    widget/actionmap.h \
    widget/albumartistpage.h \
    widget/albumview.h \
    widget/appsettingnames.h \
    widget/appsettings.h \
    widget/artistinfopage.h \
    widget/artistview.h \
    widget/cdpage.h \
    widget/clickablelabel.h \
    widget/createplaylistview.h \
    widget/database.h \
    widget/databasecoverid.h \
    widget/databasefacade.h \
    widget/doubleslider.h \
    widget/driveinfo.h \
    widget/equalizerview.h \
    widget/file_system_watcher.h \
    widget/filesystemmodel.h \
    widget/filesystemviewpage.h \
    widget/flowlayout.h \
    widget/fluentthemecolor.h \
    widget/fonticon.h \
    widget/genre_view.h \
    widget/genre_view_page.h \
    widget/globalshortcut.h \
    widget/http.h \
    widget/iconsizestyle.h \
    widget/imagecache.h \
    widget/jsonsettings.h \
    widget/lazyloadingmodel.h \
    widget/localelanguage.h \
    widget/lrcpage.h \
    widget/lrcparser.h \
    widget/lyricsshowwidget.h \
    widget/maskwidget.h \
    widget/networkdiskcache.h \
    widget/playerorder.h \
    widget/playlistentity.h \
    widget/playlistpage.h \
    widget/playlistsqlquerytablemodel.h \
    widget/playlisttabbar.h \
    widget/playlisttablemodel.h \
    widget/playlisttableproxymodel.h \
    widget/playlisttableview.h \
    widget/playlisttabwidget.h \
    widget/preferencepage.h \
    widget/processindicator.h \
    widget/qdebugsink.h \
    widget/qetag.h \
    widget/scrolllabel.h \
    widget/seekslider.h \
    widget/spectrumwidget.h \
    widget/supereqview.h \
    widget/tablistview.h \
    widget/tageditpage.h \
    widget/tagio.h \
    widget/taglistview.h \
    widget/themecolor.h \
    widget/uiplayerstateadapter.h \
    widget/util/image_utiltis.h \
    widget/util/log_util.h \
    widget/util/mbdiscid_uiltis.h \
    widget/util/read_until.h \
    widget/util/str_utilts.h \
    widget/util/ui_utilts.h \
    widget/util/zib_utiltis.h \
    widget/volumebutton.h \
    widget/volumecontroldialog.h \
    widget/wheelablewidget.h \
    widget/widget_shared.h \
    widget/widget_shared_global.h \
    widget/worker/backgroundworker.h \
    widget/worker/filesystemworker.h \
    widget/worker/findalbumcoverworker.h \
    widget/xdialog.h \
    widget/xmainwindow.h \
    widget/xmessage.h \
    widget/xmessagebox.h \
    widget/xprogressdialog.h \
    widget/youtubedl/ytmusic.h \
    widget/youtubedl/ytmusicoauth.h \
    xampplayer.h

TRANSLATIONS += \
    zh_TW.ts \
    ja_JP.ts \
    en_US.ts \

# Additionally include Cocoa for OS X code

LIBS += -framework Foundation -framework Cocoa -framework Carbon
INCLUDEPATH += /System/Library/Frameworks/Foundation.framework/Versions/C/Headers \
    ../xamp_base/include \
    ../xamp_metadata/include \
    ../xamp_output_device/include \
    ../xamp_stream/include \
    ../xamp_player/include \
    ../thirdparty/spdlog/include \
    ../thirdparty/rapidxml/ \
    ../thirdparty/zlib/ \
    ../thirdparty/pybind11/include \
    /Library/Developer/CommandLineTools/Library/Frameworks/Python3.framework/Versions/3.7/Headers/ \
    ../thirdparty/framelesshelper/include \

CONFIG(debug, debug|release) {
QMAKE_CXXFLAGS += -mavx2 -D_DEBUG
LIBS += -L"../xamp_base/debug/" -lxamp_base \
    -L"../xamp_metadata/debug/" -lxamp_metadata \
    -L"../xamp_output_device/debug/" -lxamp_output_device \
    -L"../xamp_stream/debug/" -lxamp_stream \
    -L"../xamp_player/debug/" -lxamp_player \
    -L"../thirdparty/framelesshelper/lib/debug/" -lFramelessHelperCore_debug.2.3.6 \
    -L"../thirdparty/framelesshelper/lib/debug/" -lFramelessHelperWidgets_debug.2.3.6 \
    -L"/Library/Developer/CommandLineTools/Library/Frameworks/Python3.framework/Versions/3.7/lib/" -lpython3.7 \
} else {
QMAKE_CXXFLAGS += -DNDEBUG -O3 -ftree-vectorize -march=native -mtune=native -mavx2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer
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

FORMS += \
    aboutdialog.ui \
    accountauthorizationpage.ui \
    cdpage.ui \
    createplaylistdialog.ui \
    equalizerdialog.ui \
    filesystemviewpage.ui \
    preferencedialog.ui \
    supereqdialog.ui \
    tageditpage.ui \
    volumecontroldialog.ui
