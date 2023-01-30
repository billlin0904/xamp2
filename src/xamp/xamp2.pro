QT       += core gui sql network concurrent webenginewidgets

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
    widget/processindicator.cpp \
    widget/albumentity.cpp \
    widget/discordnotify.cpp \
    widget/doubleslider.cpp \
    widget/equalizerdialog.cpp \
    widget/lyricsshowwidget.cpp \
    widget/http.cpp \
    widget/playlistsqlquerytablemodel.cpp \
    widget/backgroundworker.cpp \
    widget/seekslider.cpp \
    widget/albumartistpage.cpp \
    widget/albumview.cpp \
    widget/appsettings.cpp \
    widget/clickablelabel.cpp \
    widget/database.cpp \
    widget/qetag.cpp \
    widget/lrcpage.cpp \
    widget/lrcparser.cpp \
    widget/databasefacade.cpp \
    widget/pixmapcache.cpp \
    widget/playlistpage.cpp \
    widget/appsettingnames.cpp \
    widget/playlisttableproxymodel.cpp \
    widget/playlisttableview.cpp \
    widget/switchbutton.cpp \
    widget/tablistview.cpp \
    widget/uiplayerstateadapter.cpp \
    widget/wheelablewidget.cpp \
    widget/artistinfopage.cpp \
    widget/starrating.cpp \
    widget/stareditor.cpp \
    widget/stardelegate.cpp \
    widget/localelanguage.cpp \
    widget/jsonsettings.cpp \
    widget/scrolllabel.cpp \
    widget/xdialog.cpp \
    widget/xframe.cpp \
    widget/image_utiltis.cpp \
    widget/read_utiltis.cpp \
    widget/str_utilts.cpp \
    widget/ui_utilts.cpp \
    widget/podcast_uiltis.cpp \
    widget/spectrumwidget.cpp \
    widget/filesystemviewpage.cpp \
    widget/filesystemmodel.cpp \
    widget/fonticon.cpp \
    widget/fonticonanimation.cpp \
    widget/xmessagebox.cpp \
    widget/globalshortcut.cpp \
    widget/xprogressdialog.cpp \
    widget/volumecontroldialog.cpp \
    widget/spotify_utilis.cpp \
    widget/xmainwindow.cpp \
    singleinstanceapplication.cpp \
    widget/zib_utiltis.cpp \
    version.cpp \
    cdpage.cpp \
    thememanager.cpp \
    aboutpage.cpp \
    preferencepage.cpp \
    main.cpp \    
    xamp.cpp

HEADERS += \
    widget/processindicator.h \
    widget/albumentity.h \
    widget/discordnotify.h \
    widget/doubleslider.h \
    widget/equalizerdialog.h \
    widget/lyricsshowwidget.h \
    widget/osx/osx.h \
    widget/http.h \
    widget/playerorder.h \
    widget/playlistsqlquerytablemodel.h \
    widget/seekslider.h \
    widget/actionmap.h \
    widget/albumview.h \
    widget/appsettings.h \
    widget/appsettingnames.h \
    widget/clickablelabel.h \
    widget/database.h \
    widget/qetag.h \
    widget/albumartistpage.h \
    widget/image_utiltis.h \
    widget/lrcpage.h \
    widget/lrcparser.h \
    widget/databasefacade.h \
    widget/pixmapcache.h \
    widget/playlistpage.h \
    widget/playlisttablemodel.h \
    widget/playlisttableproxymodel.h \
    widget/playlisttableview.h \
    widget/qdebugsink.h \
    widget/switchbutton.h \
    widget/tablistview.h \
    widget/uiplayerstateadapter.h \
    widget/wheelablewidget.h \
    widget/str_utilts.h \
    widget/artistinfopage.h \
    widget/starrating.h \
    widget/stareditor.h \
    widget/stardelegate.h \
    widget/localelanguage.h \
    widget/jsonsettings.h \
    widget/scrolllabel.h \
    widget/xdialog.h \
    widget/xframe.h \
    widget/read_utiltis.h \
    widget/backgroundworker.h \
    widget/spectrumwidget.h \
    widget/smoothcurvegenerator2.h \
    widget/filesystemviewpage.h \
    widget/filesystemmodel.h \
    widget/fonticon.h \
    widget/iconsizestyle.h \
    widget/fonticonanimation.h \
    widget/xmessagebox.h \
    widget/globalshortcut.h \
    widget/spotify_utilis.h \
    widget/zib_utiltis.h \
    widget/xprogressdialog.h \
    widget/xmainwindow.h \
    widget/volumecontroldialog.h \
    version.h \
    singleinstanceapplication.h \
    cdpage.h \
    thememanager.h \
    aboutpage.h \
    preferencepage.h \    
    xamp.h \
    xampplayer.h

ICON = xamp2.icns \

FORMS += \
    widget/equalizerdialog.ui \
    volumecontroldialog.ui \
    filesystemviewpage.ui \
    aboutdialog.ui \
    preferencedialog.ui \
    cdpage.ui \
    xamp.ui \

RESOURCES += \
    xamp.qrc \
    widget/toast.qrc \
    Resource/theme/dark/darkstyle.qrc \
    Resource/theme/light/lightstyle.qrc


TRANSLATIONS += \
    zh_TW.ts \
    ja_JP.ts \
    en_US.ts \

# Additionally include Cocoa for OS X code

LIBS += -framework Foundation -framework Cocoa -framework Carbon
INCLUDEPATH += /System/Library/Frameworks/Foundation.framework/Versions/C/Headers
INCLUDEPATH += "../thirdparty/discord-rpc/include"
INCLUDEPATH += "../thirdparty/zopfli/src/zopflipng"

CONFIG(debug, debug|release) {
QMAKE_CXXFLAGS += -mavx2 -D_DEBUG
LIBS += -L"../xamp_base/debug/" -lxamp_base \
    -L"../xamp_metadata/debug/" -lxamp_metadata \
    -L"../xamp_output_device/debug/" -lxamp_output_device \
    -L"../xamp_stream/debug/" -lxamp_stream \
    -L"../xamp_player/debug/" -lxamp_player \
    -L"../thirdparty/zopfli/mac/Debug" -lzopfli\
    -L"../thirdparty/zopfli/mac/Debug" -lzopflipng\
} else {
QMAKE_CXXFLAGS += -DNDEBUG -O3 -ftree-vectorize -march=native -mtune=native -mavx2 -fno-omit-frame-pointer -mno-omit-leaf-frame-pointer
LIBS += -L"../xamp_base/release/" -lxamp_base \
    -L"../xamp_metadata/release/" -lxamp_metadata \
    -L"../xamp_output_device/release/" -lxamp_output_device \
    -L"../xamp_stream/release/" -lxamp_stream \
    -L"../xamp_player/release/" -lxamp_player \
    -L"../thirdparty/zopfli/mac/Release" -lzopfli\
    -L"../thirdparty/zopfli/mac/Release" -lzopflipng\
}

# Default rules for deployment.
qnx: target.path = /tmp/$${TARGET}/bin
else: unix:!android: target.path = /opt/$${TARGET}/bin
!isEmpty(target.path): INSTALLS += target
