//=====================================================================================================================
// Copyright (c) 2018-2024 XAMP project. All rights reserved.
// More license information, please see LICENSE file in module root folder.
//=====================================================================================================================

#pragma once

#include <QObject>
#include <QMap>

#include <widget/widget_shared.h>
#include <widget/widget_shared_global.h>

enum Glyphs {
    ICON_START,
    ICON_VOLUME_UP,
    ICON_VOLUME_OFF,
    ICON_SPEAKER,
    ICON_FOLDER,
    ICON_AUDIO_FILE,
    ICON_FILE_OPEN,
    ICON_FOLDER_OPEN,
    ICON_RELOAD,
    ICON_REMOVE_ALL,
    ICON_OPEN_FILE_PATH,
    ICON_SCAN_REPLAY_GAIN,
    ICON_EXPORT_FILE,
    ICON_COPY,
    ICON_DOWNLOAD,
    ICON_PLAYLIST,
    ICON_EQUALIZER,
    ICON_MUSIC_LIBRARY,
    ICON_CD,
    ICON_LEFT_ARROW,
    ICON_SUBTITLE,
    ICON_SETTINGS,
    ICON_ABOUT,
    ICON_SEARCH,
    ICON_DESKTOP,
    ICON_SHUFFLE_PLAY_ORDER,
    ICON_REPEAT_ONE_PLAY_ORDER,
    ICON_REPEAT_ONCE_PLAY_ORDER,
    ICON_MINIMIZE_WINDOW,
    ICON_MAXIMUM_WINDOW,
    ICON_CLOSE_WINDOW,
    ICON_RESTORE_WINDOW,
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
    ICON_MESSAGE_BOX_SUCCESS,
    ICON_HEART_PRESS,
    ICON_HEART,
    ICON_CHEVRON_RIGHT,
    ICON_CHEVRON_LEFT,
    ICON_FILE_CIRCLE_PLUS,
    ICON_EDIT,
    ICON_CIRCLE_CHECK,
    ICON_YOUTUBE,
    ICON_YOUTUBE_LIBRARY,
    ICON_LIKE,
    ICON_DISLIKE,
    ICON_END,
};

struct XAMP_WIDGET_SHARED_EXPORT FontIconOption {
    static const QString kRectAttr;
    static const QString kScaleFactorAttr; 
    static const QString kFontStyleAttr;
    static const QString kColorAttr;
    static const QString kOnColorAttr;
    static const QString kActiveColorAttr;
    static const QString kActiveOnColorAttr;
    static const QString kDisabledColorAttr;
    static const QString kSelectedColorAttr;
    static const QString kFlipLeftRightAttr;
    static const QString kRotateAngleAttr;
    static const QString kFlipTopBottomAttr;
    static const QString kOpacityAttr;
    static const QString kAnimation;

	static QColor color;
    static QColor onColor;
    static QColor activeColor;
    static QColor activeOnColor;
    static QColor disabledColor;
    static QColor selectedColor;
    static double opacity;
};

class XAMP_WIDGET_SHARED_EXPORT FontIcon : public QObject {
public:
    friend class XAMP_WIDGET_SHARED_EXPORT SharedSingleton<FontIcon>;

    explicit FontIcon(QObject* parent = nullptr);

    bool AddFont(const QString& filename);

    QIcon GetIcon(const char32_t& code, QVariantMap options = QVariantMap(), const QString& family = QString()) const;

    const QStringList& GetFamilies() const;

    void SetGlyphs(const HashMap<char32_t, uint32_t> &glyphs);
protected:
    void AddFamily(const QString& family);

    QStringList families_;
    mutable HashMap<char32_t, uint32_t> glyphs_;
};

#define qFontIcon SharedSingleton<FontIcon>::GetInstance()
#define Q_FONT_ICON(code) qFontIcon.icon(code)