//=====================================================================================================================
// Copyright (c) 2018-2023 xamp project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QMap>

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
    ICON_PLAY_LIST_PLAY,
    ICON_PLAY_LIST_PAUSE,
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
    ICON_MESSAGE_BOX_WARNING,
    ICON_MESSAGE_BOX_INFORMATION,
    ICON_MESSAGE_BOX_ERROR,
    ICON_MESSAGE_BOX_QUESTION,
};

struct FontIconOption {
    static const QString animationAttr;
    static const QString rectAttr;
    static const QString scaleFactorAttr;
    static const QString fontStyleAttr;
    static const QString colorAttr;
    static const QString onColorAttr;
    static const QString activeColorAttr;
    static const QString activeOnColorAttr;
    static const QString disabledColorAttr;
    static const QString selectedColorAttr;
    static const QString flipLeftRightAttr;
    static const QString rotateAngleAttr;
    static const QString flipTopBottomAttr;
    static const QString opacityAttr;

	static QColor color;
    static QColor onColor;
    static QColor activeColor;
    static QColor activeOnColor;
    static QColor disabledColor;
    static QColor selectedColor;
    static double opacity;
};

class FontIcon : public QObject {
public:
    explicit FontIcon(QObject* parent = nullptr);

    bool addFont(const QString& filename);

    QIcon animationIcon(const char32_t& code, QWidget *parent, const QString& family = QString()) const;

    QIcon icon(const char32_t& code, QVariantMap options = QVariantMap(), const QString& family = QString()) const;

    const QStringList& families() const;

    static void setGlyphs(const HashMap<char32_t, uint32_t> &glyphs);
protected:
    void addFamily(const QString& family);

    QStringList families_;
    static HashMap<char32_t, uint32_t> glyphs_;
};

#define qFontIcon SharedSingleton<FontIcon>::GetInstance()
#define Q_FONT_ICON(code) qFontIcon.icon(code)