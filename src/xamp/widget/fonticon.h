//=====================================================================================================================
// Copyright (c) 2018-2022 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QMap>
#include <widget/str_utilts.h>
#include <widget/widget_shared.h>

enum Glyphs {
    ICON_VOLUME_UP,
    ICON_VOLUME_OFF,
    ICON_SPEAKER,
    ICON_FOLDER,
    ICON_AUDIO,
    ICON_LOAD_FILE,
    ICON_LOAD_DIR,
    ICON_RELOAD,
    ICON_REMOVE_ALL,
    ICON_OPEN_FILE_PATH,
    ICON_READ_REPLAY_GAIN,
    ICON_EXPORT_FILE,
    ICON_COPY,
    ICON_DOWNLOAD,
    ICON_PLAYLIST,
    ICON_EQUALIZER,
    ICON_PODCAST,
    ICON_ALBUM,
    ICON_CD,
    ICON_LEFT_ARROW,
    ICON_ARTIST,
    ICON_SUBTITLE,
    ICON_PREFERENCE,
    ICON_ABOUT,
    ICON_DARK_MODE,
    ICON_LIGHT_MODE,
    ICON_SEARCH,
    ICON_THEME,
    ICON_DESKTOP,
    ICON_SHUFFLE_PLAY_ORDER,
    ICON_REPEAT_ONE_PLAY_ORDER,
    ICON_REPEAT_ONCE_PLAY_ORDER,
    ICON_MINIMIZE_WINDOW,
    ICON_MAXIMUM_WINDOW,
    ICON_CLOSE_WINDOW,
    ICON_RESTORE_WINDOW,
    ICON_SLIDER_BAR,
    ICON_PLAY,
    ICON_PAUSE,
    ICON_STOP_PLAY,
    ICON_PLAY_FORWARD,
    ICON_PLAY_BACKWARD,
    ICON_MORE,
    ICON_HIDE,
    ICON_SHOW,
    ICON_USB,
    ICON_BUILD_IN_SPEAKER,
    ICON_BLUE_TOOTH,
};

struct FontIconOption {
    const static QString animationAttr;
    const static QString rectAttr;
    const static QString scaleFactorAttr;
    const static QString fontStyleAttr;
    const static QString colorAttr;
    const static QString onColorAttr;
    const static QString activeColorAttr;
    const static QString activeOnColorAttr;
    const static QString disabledColorAttr;
    const static QString selectedColorAttr;
    const static QString flipLeftRightAttr;
    const static QString rotateAngleAttr;
    const static QString flipTopBottomAttr;

    static QColor color;
    static QColor onColor;
    static QColor activeColor;
    static QColor activeOnColor;
    static QColor disabledColor;
    static QColor selectedColor;
};

class FontIcon : public QObject {
public:
    explicit FontIcon(QObject* parent = nullptr);

    bool addFont(const QString& filename);

    QIcon animationIcon(const char32_t& code, QWidget *parent, const QString& family = QString()) const;

    QIcon icon(const char32_t& code, QVariantMap options = QVariantMap(), const QString& family = QString()) const;

    const QStringList& families() const;
protected:
    void addFamily(const QString& family);

    QStringList families_;
    static HashMap<char32_t, uint32_t> glyphs_;
};

#define qFontIcon SharedSingleton<FontIcon>::GetInstance()
#define Q_FONT_ICON(code) qFontIcon.icon(code)