TEMPLATE = lib

QT       += core gui sql network concurrent

greaterThan(QT_MAJOR_VERSION, 4): QT += widgets

CONFIG += c++20
# DEFINES += QT_NO_CAST_FROM_ASCII

QMAKE_MACOSX_DEPLOYMENT_TARGET = 13.00

# The following define makes your compiler emit warnings if you use
# any Qt feature that has been marked deprecated (the exact warnings
# depend on your compiler). Please consult the documentation of the
# deprecated API in order to know how to port your code away from it.
DEFINES += QT_DEPRECATED_WARNINGS

# You can also make your code fail to compile if it uses deprecated APIs.
# In order to do so, uncomment the following line.
# You can also select to disable deprecated APIs only up to a certain version of Qt.
#DEFINES += QT_DISABLE_DEPRECATED_BEFORE=0x060000    # disables all the APIs deprecated before Qt 6.0.0

TARGET = widget_shared
TEMPLATE = lib
CONFIG += plugin

SOURCES += \
./widget/genre_view_page.cpp \
./widget/albumartistpage.cpp \
./widget/playlisttabbar.cpp \
./widget/baseservice.cpp \
./widget/scrolllabel.cpp \
./widget/preferencepage.cpp \
./widget/tagio.cpp \
./widget/playlistentity.cpp \
./widget/xmessage.cpp \
./widget/processindicator.cpp \
./widget/localelanguage.cpp \
./widget/lrcparser.cpp \
./widget/qetag.cpp \
./widget/cdpage.cpp \
./widget/util/zib_util.cpp \
./widget/util/image_util.cpp \
./widget/util/read_until.cpp \
./widget/util/str_util.cpp \
./widget/util/mbdiscid_util.cpp \
./widget/util/log_util.cpp \
./widget/util/ui_util.cpp \
./widget/playlisttabwidget.cpp \
./widget/volumecontroldialog.cpp \
./widget/albumviewstyleddelegate.cpp \
./widget/supereqview.cpp \
./widget/spectrumwidget.cpp \
./widget/playlisttableproxymodel.cpp \
./widget/xmainwindow.cpp \
./widget/fonticon.cpp \
./widget/globalshortcut.cpp \
./widget/clickablelabel.cpp \
./widget/dao/artistdao.cpp \
./widget/dao/playlistdao.cpp \
./widget/dao/musicdao.cpp \
./widget/dao/albumdao.cpp \
./widget/appsettings.cpp \
./widget/artistview.cpp \
./widget/doubleslider.cpp \
./widget/imagecache.cpp \
./widget/wheelablewidget.cpp \
./widget/accountauthorizationpage.cpp \
./widget/volumebutton.cpp \
./widget/widget_shared_global.cpp \
./widget/appsettingnames.cpp \
./widget/playlisttableview.cpp \
./widget/database.cpp \
./widget/artistinfopage.cpp \
./widget/createplaylistview.cpp \
./widget/xtooltip.cpp \
./widget/taglistview.cpp \
./widget/lyricsshowwidget.cpp \
./widget/xmessagebox.cpp \
./widget/navbarlistview.cpp \
./widget/aboutpage.cpp \
./widget/genre_view.cpp \
./widget/uiplayerstateadapter.cpp \
./widget/filesystemwatcher.cpp \
./widget/tageditpage.cpp \
./widget/xprogressdialog.cpp \
./widget/http.cpp \
./widget/lrcpage.cpp \
./widget/playListstyledItemdelegate.cpp \
./widget/maskwidget.cpp \
./widget/databasefacade.cpp \
./widget/jsonsettings.cpp \
./widget/chatgpt/chatgptservice.cpp \
./widget/chatgpt/chatgptwidget.cpp \
./widget/albumview.cpp \
./widget/filesystemviewpage.cpp \
./widget/worker/albumcoverservice.cpp \
./widget/worker/backgroundservice.cpp \
./widget/worker/filesystemservice.cpp \
./widget/playlistpage.cpp \
./widget/win32/wintaskbar.cpp \
./widget/networkdiskcache.cpp \
./widget/xdialog.cpp \
./widget/equalizerview.cpp \
./widget/webvttparser.cpp \
./widget/playlistsqlquerytablemodel.cpp \
./widget/filesystemmodel.cpp \
./widget/youtubedl/ytmusicservice.cpp \
./widget/youtubedl/ytmusic_disckcache.cpp \
./widget/youtubedl/ytmusicoauth.cpp \
./widget/seekslider.cpp \
./version.cpp \
./thememanager.cpp \

HEADERS += \
./version.h \
./widget/albumviewstyleddelegate.h \
./widget/iconsizestyle.h \
./widget/xmessagebox.h \
./widget/databasecoverid.h \
./widget/xtooltip.h \
./widget/scrolllabel.h \
./widget/xprogressdialog.h \
./widget/taglistview.h \
./widget/appsettings.h \
./widget/lyricsshowwidget.h \
./widget/aboutpage.h \
./widget/driveinfo.h \
./widget/http.h \
./widget/xdialog.h \
./widget/tageditpage.h \
./widget/xmainwindow.h \
./widget/util/ui_util.h \
./widget/util/read_until.h \
./widget/util/str_util.h \
./widget/util/json_util.h \
./widget/util/log_util.h \
./widget/util/zib_util.h \
./widget/util/mbdiscid_util.h \
./widget/util/image_util.h \
./widget/localelanguage.h \
./widget/databasefacade.h \
./widget/cdpage.h \
./widget/filesystemmodel.h \
./widget/albumview.h \
./widget/createplaylistview.h \
./widget/playerorder.h \
./widget/ilrrcparser.h \
./widget/uiplayerstateadapter.h \
./widget/playlisttableproxymodel.h \
./widget/filesystemviewpage.h \
./widget/dao/artistdao.h \
./widget/dao/playlistdao.h \
./widget/dao/musicdao.h \
./widget/dao/albumdao.h \
./widget/volumecontroldialog.h \
./widget/networkdiskcache.h \
./widget/artistview.h \
./widget/volumebutton.h \
./widget/playlistentity.h \
./widget/widget_shared.h \
./widget/webvttparser.h \
./widget/navbarlistview.h \
./widget/playlisttabbar.h \
./widget/playlisttablemodel.h \
./widget/themecolor.h \
./widget/maskwidget.h \
./widget/supereqview.h \
./widget/qetag.h \
./widget/playlisttabwidget.h \
./widget/accountauthorizationpage.h \
./widget/albumartistpage.h \
./widget/genre_view_page.h \
./widget/lrcpage.h \
./widget/globalshortcut.h \
./widget/widget_shared_global.h \
./widget/clickablelabel.h \
./widget/preferencepage.h \
./widget/spectrumwidget.h \
./widget/database.h \
./widget/artistinfopage.h \
./widget/qdebugsink.h \
./widget/tagio.h \
./widget/baseservice.h \
./widget/xmessage.h \
./widget/playlistsqlquerytablemodel.h \
./widget/jsonsettings.h \
./widget/wheelablewidget.h \
./widget/doubleslider.h \
./widget/filesystemwatcher.h \
./widget/chatgpt/chatgptwidget.h \
./widget/chatgpt/chatgptservice.h \
./widget/worker/backgroundservice.h \
./widget/worker/filesystemservice.h \
./widget/worker/albumcoverservice.h \
./widget/win32/wintaskbar.h \
./widget/playListstyledItemdelegate.h \
./widget/imagecache.h \
./widget/seekslider.h \
./widget/playlistpage.h \
./widget/playlisttableview.h \
./widget/lrcparser.h \
./widget/processindicator.h \
./widget/equalizerview.h \
./widget/genre_view.h \
./widget/actionmap.h \
./widget/youtubedl/ytmusic_disckcache.h \
./widget/youtubedl/ytmusicservice.h \
./widget/youtubedl/ytmusicoauth.h \
./widget/fonticon.h \
./widget/appsettingnames.h \
./xampplayer.h \
./thememanager.h \

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
    ../thirdparty/expected/include \
    /Library/Developer/CommandLineTools/Library/Frameworks/Python3.framework/Versions/3.7/Headers/ \
    ../thirdparty/framelesshelper/include \

CONFIG(debug, debug|release) {
CONFIG += debug
QMAKE_CXXFLAGS += -D_DEBUG
LIBS += -L$$PWD/../xamp_base/debug/ -lxamp_base \
    -L$$PWD/../xamp_metadata/debug/ -lxamp_metadata \
    -L$$PWD/../xamp_output_device/debug/ -lxamp_output_device \
    -L$$PWD/../xamp_stream/debug/ -lxamp_stream \
    -L$$PWD/../xamp_player/debug/ -lxamp_player \
    -L$$PWD/../thirdparty/framelesshelper/build/Qt_6_5_3_for_macOS-Debug/lib/ -lFramelessHelperCore64_debug \
    -L$$PWD/../thirdparty/framelesshelper/build/Qt_6_5_3_for_macOS-Debug/lib/ -lFramelessHelperWidgets64_debug \
    -L/Library/Developer/CommandLineTools/Library/Frameworks/Python3.framework/Versions/3.7/lib/ -lpython3.7 \
} else {

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
